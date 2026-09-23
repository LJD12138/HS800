#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""体检（可选只修 A 类）：按 .agents/rules/c-coding-style.md 第 2.1 节检查排版/控制结构规范。

用法：python style_check.py <项目根> <目录列表,逗号分隔> [--apply]
  - 不带 --apply：只读报告，并打印 A1/A2 的预计可修数量（干跑）。
  - 带 --apply  ：先只修 A1、A2（B/C1/C2/C3/C4 一律不动），再打印修复结果。
检查项：
  A1  代码块内（花括号深度>0）的条件编译顶格书写
  A2  代码块内的 #endif 无行尾条件注释
  B   单语句分支使用了大括号（if / else if / else）
  C1  case/default 主体未用 {} 包裹
  C2  { 与 case X: 写在同一行
  C3  break; 写在 case 主体大括号内部
  C4  case 主体大括号之后疑似缺 break（可能故意 fallthrough，单独归类）
      —— 体末语句本身是 break/return/continue/goto（按语句层级判断），
         或 `}` 之后紧跟 break/return/continue/goto 时不报
不带 --apply 时不写盘。"""
import bisect
import os
import re
import sys

SKIP_DIRS = {'.git', '.cmsis', '.pack', 'RTE', 'build', 'DebugConfig', 'Objects', 'Listings',
             'eez_ui', 'eez_project', 'lvgl', 'FreeRTOS', 'EasyFlash', 'EasyLogger',
             'CmBacktrace', 'lwrb', 'LightweightRingBuffer', 'SeggerRtt', 'MultiFuncKey',
             'Firmware', '.agents', '.trae', '.vscode'}
SKIP_LOWER = {x.lower() for x in SKIP_DIRS}   # 目录名大小写不敏感
EXEMPT = {'gd32f50x_it.c', 'gd32f50x_it.h', 'gd32f50x_libopt.h',
          'FreeRTOSConfig.h', 'board_config.h', 'board_config.c'}

DIRECTIVE_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef|else|elif|endif)\b')
EXTERN_C_RE = re.compile(r'^\s*extern\s+"C"\s*\{')      # 头文件 extern "C" { 模板，不计入代码块深度
IF_RE = re.compile(r'(?<![#\w.])if\s*\(')
ELSE_RE = re.compile(r'(?<![#\w.])else(?!\s*if\b)')
SWITCH_RE = re.compile(r'(?<![#\w.])switch\s*\(')
BREAK_RE = re.compile(r'(?<![#\w.])break\s*;')
END_RE = re.compile(r'\s*(break|return|continue|goto)\b')
CASE_RE = re.compile(r'(?<![#\w.])case\b')
DEFAULT_RE = re.compile(r'(?<![#\w.])default\b')

CAT_NAME = {'A1': 'A1 代码块内条件编译顶格（应随层级缩进）',
            'A2': 'A2 代码块内 #endif 缺行尾条件注释',
            'B': 'B 单语句分支使用了大括号（if/else if/else）',
            'C1': 'C1 case/default 主体未用 {} 包裹',
            'C2': 'C2 { 与 case X: 写在同一行（应换行独立）',
            'C3': 'C3 break; 写在 case 主体大括号内部（应在外独立成行）',
            'C4': 'C4 疑似缺 break（可能故意 fallthrough，单独归类）'}
ORDER = ['A1', 'A2', 'B', 'C1', 'C2', 'C3', 'C4']


def load(path):
    """读 UTF-8 文件为行列表；非 UTF-8 返回 None"""
    try:
        with open(path, 'rb') as f:
            raw = f.read()
        text = raw.decode('utf-8-sig')
    except Exception:
        return None
    lines = text.replace('\r\n', '\n').replace('\r', '\n').split('\n')
    if lines and lines[-1] == '':
        lines.pop()
    return lines


def mask_lines(lines):
    """把注释、字符串/字符字面量内容替换为空格（长度保持不变），便于按列做正则"""
    masked = []
    in_block = False
    for ln in lines:
        out = []
        i = 0
        n = len(ln)
        in_str = ''
        while i < n:
            c = ln[i]
            if in_block:
                if c == '*' and i + 1 < n and ln[i + 1] == '/':
                    out.append('  ')
                    i += 2
                    in_block = False
                else:
                    out.append(' ')
                    i += 1
                continue
            if in_str:
                if c == '\\' and i + 1 < n:
                    out.append('  ')
                    i += 2
                    continue
                out.append(' ')
                i += 1
                if c == in_str:
                    in_str = ''
                continue
            if c == '/' and i + 1 < n and ln[i + 1] == '*':
                out.append('  ')
                i += 2
                in_block = True
                continue
            if c == '/' and i + 1 < n and ln[i + 1] == '/':
                out.append(' ' * (n - i))
                i = n
                continue
            if c in '"\'':
                out.append(' ')
                in_str = c
                i += 1
                continue
            out.append(c)
            i += 1
        masked.append(''.join(out))
    return masked


def pp_flags(masked):
    """标记预处理指令行（含反斜杠续行），这些行不参与花括号深度与分支/switch 判定"""
    flags = []
    cont = False
    for ml in masked:
        s = ml.strip()
        is_pp = cont or s.startswith('#')
        flags.append(is_pp)
        cont = is_pp and ml.rstrip().endswith('\\')
    return flags


def cond_depths(lines, masked, pp):
    """每行判定条件编译缩进用的花括号深度（跳过注释/字符串；预处理行不改变深度）。

    `#else`/`#elif`/`#endif` 取与其配对的 `#if` 处深度：`#if/#else` 各分支里出现的
    初始化大括号（如 `= {` / `};`）不会把条件编译行本身误算成"处于代码块内部"。
    `extern "C" {` 是头文件模板固定写法，不计入代码块。"""
    cdepth = []
    stack = []
    depth = 0
    for i, ml in enumerate(masked):
        cd = depth
        if pp[i]:
            m = DIRECTIVE_RE.match(ml)
            if m:
                kind = m.group(1)
                if kind in ('if', 'ifdef', 'ifndef'):
                    stack.append(depth)
                elif kind in ('else', 'elif'):
                    if stack:
                        depth = stack[-1]
                        cd = depth
                elif kind == 'endif' and stack:
                    snap = stack.pop()
                    cd = snap
                    depth = max(depth, snap)     # 分支内开启的块可能在 #endif 之后才闭合
        else:
            if EXTERN_C_RE.match(lines[i]):      # extern "C" { 不算代码块
                cdepth.append(cd)
                continue
            d = 0
            for c in ml:
                if c == '{':
                    d += 1
                elif c == '}':
                    d -= 1
            depth += d
            if depth < 0:
                depth = 0
        cdepth.append(cd)
    return cdepth


def match_delim(text, i, oc, cc):
    """text[i] 为 oc，返回与其配对的 cc 下标；找不到返回 -1"""
    depth = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == oc:
            depth += 1
        elif c == cc:
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return -1


def next_sig(text, i):
    """下一个非空白字符下标"""
    n = len(text)
    while i < n and text[i] in ' \t\n':
        i += 1
    return i


def count_stmts(body):
    """统计一块 {} 内部顶层语句条数（用于判断分支体是否只有一条语句）
    口径：顶层 `;` 计一条；顶层嵌套块（if/else/for/while/do/裸块）闭合时计一条。"""
    count = 0
    depth = 0
    pending = False
    stack = []
    for c in body:
        if c == '{':
            stack.append(pending)
            depth += 1
            pending = False
        elif c == '}':
            if depth > 0:
                stack.pop()
                depth -= 1
                if depth == 0:
                    count += 1
                    pending = False
        elif c == ';':
            if depth == 0 and pending:
                count += 1
                pending = False
        elif c not in ' \t\n':
            if depth == 0:
                pending = True
    return count


def last_stmt_head(body):
    """取 body 内最后一条顶层语句的首个标识符 token（供 C4 判定体末是否跳转语句）。

    切分口径与 count_stmts 一致：顶层 `;` 收尾一条语句；顶层嵌套块（if/else/for/while/do/
    裸块）闭合时收尾一条语句。因此 `if (x) return -1;` 这类「唯一语句是 if」的写法，
    末语句首 token 是 `if`（而非 `return`），不会被末尾出现的 return 骗过。
    body 内无有效语句时返回 ''。"""
    depth = 0
    start = None
    last = None
    for i, c in enumerate(body):
        if c in ' \t\n':
            continue
        if c == '{':
            if depth == 0 and start is None:
                start = i
            depth += 1
        elif c == '}':
            if depth > 0:
                depth -= 1
                if depth == 0:
                    last = start if start is not None else i
                    start = None
        elif c == ';':
            if depth == 0:
                last = start if start is not None else i
                start = None
        elif depth == 0 and start is None:
            start = i
    if start is not None:                        # 末尾语句未以 `;`/`}` 收尾（防御）
        last = start
    if last is None:
        return ''
    m = re.match(r'[A-Za-z_]\w*', body[last:])
    return m.group(0) if m else ''


def case_labels(body):
    """找出 switch 主体内顶层（相对深度 0）的 case/default 标签，返回 [(offset, kind, colon)]"""
    labels = []
    depth = 0
    i = 0
    n = len(body)
    while i < n:
        c = body[i]
        if c == '{':
            depth += 1
            i += 1
            continue
        if c == '}':
            depth -= 1
            i += 1
            continue
        if depth == 0:
            m = CASE_RE.match(body, i) or DEFAULT_RE.match(body, i)
            if m:
                colon = body.find(':', m.end())
                if colon < 0:
                    break
                labels.append((i, m.group(0), colon))
                i = colon + 1
                continue
        i += 1
    return labels


def blank_pp(text):
    """把文本里的预处理指令行整体替换为等长空格（保持列/行结构不变）"""
    out = []
    for ln in text.split('\n'):
        out.append(' ' * len(ln) if ln.strip().startswith('#') else ln)
    return '\n'.join(out)


def find_break_inside(inner):
    """在 case 主体 {} 内顶层（相对深度 0）找 break;，返回相对下标或 -1"""
    depth = 0
    i = 0
    n = len(inner)
    while i < n:
        c = inner[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
        elif depth == 0 and c == 'b':
            m = BREAK_RE.match(inner, i)
            if m:
                return i
        i += 1
    return -1


def is_ctrl_body(inner, i):
    """判断 inner[i] 处的 break 是否正是某个无大括号分支（if/else/for/while）的语句体。
    这类 break 是"条件提前跳出 switch"，不属于 case 的收尾 break，不判 C3。"""
    k = i - 1
    while k >= 0 and inner[k] in ' \t\n':
        k -= 1
    if k < 0:
        return False
    if inner[k] == ')':
        d = 0
        while k >= 0:
            if inner[k] == ')':
                d += 1
            elif inner[k] == '(':
                d -= 1
                if d == 0:
                    break
            k -= 1
        j = k - 1
        while j >= 0 and inner[j] in ' \t\n':
            j -= 1
        e = j + 1
        while j >= 0 and (inner[j].isalnum() or inner[j] == '_'):
            j -= 1
        return inner[j + 1:e] in ('if', 'for', 'while')
    if inner[k] == 'e':
        return inner[:k + 1].endswith('else')
    return False


def check_file(lines):
    """返回 [(检查项, 行号, 该行代码)]"""
    out = []
    masked = mask_lines(lines)
    pp = pp_flags(masked)
    cdepth = cond_depths(lines, masked, pp)
    text = '\n'.join(masked)
    starts = []
    pos = 0
    for ml in masked:
        starts.append(pos)
        pos += len(ml) + 1

    def ln_of(o):
        return bisect.bisect_right(starts, o) - 1

    def hit(code, ln):
        out.append((code, ln + 1, lines[ln] if 0 <= ln < len(lines) else ''))

    # ---- 检查 A：条件编译缩进 / #endif 注释 ----
    for i, ml in enumerate(masked):
        if not pp[i]:
            continue
        m = DIRECTIVE_RE.match(ml)
        if not m or cdepth[i] <= 0:
            continue
        if not lines[i].startswith((' ', '\t')):
            hit('A1', i)
        if m.group(1) == 'endif' and not re.search(r'//|/\*', lines[i][m.end():]):
            hit('A2', i)

    # ---- 检查 B：单语句分支大括号 ----
    for m in IF_RE.finditer(text):
        ln = ln_of(m.start())
        if pp[ln]:
            continue
        pclose = match_delim(text, m.end() - 1, '(', ')')
        if pclose < 0:
            continue
        j = next_sig(text, pclose + 1)
        if j >= len(text) or text[j] != '{':
            continue
        jclose = match_delim(text, j, '{', '}')
        if jclose < 0:
            continue
        if count_stmts(text[j + 1:jclose]) == 1:
            hit('B', ln)
    for m in ELSE_RE.finditer(text):
        ln = ln_of(m.start())
        if pp[ln]:
            continue
        j = next_sig(text, m.end())
        if j >= len(text) or text[j] != '{':
            continue
        jclose = match_delim(text, j, '{', '}')
        if jclose < 0:
            continue
        if count_stmts(text[j + 1:jclose]) == 1:
            hit('B', ln)

    # ---- 检查 C：switch-case 分支块格式 ----
    for m in SWITCH_RE.finditer(text):
        if pp[ln_of(m.start())]:
            continue
        pclose = match_delim(text, m.end() - 1, '(', ')')
        if pclose < 0:
            continue
        j = next_sig(text, pclose + 1)
        if j >= len(text) or text[j] != '{':
            continue
        jclose = match_delim(text, j, '{', '}')
        if jclose < 0:
            continue
        body, base = text[j + 1:jclose], j + 1
        labels = case_labels(body)
        for k, (off, _kind, colon) in enumerate(labels):
            seg_end = labels[k + 1][0] if k + 1 < len(labels) else len(body)
            seg = body[colon + 1:seg_end]
            s = next_sig(seg, 0)
            if s >= len(seg):
                continue                       # 末尾标签后无内容
            if CASE_RE.match(seg, s) or DEFAULT_RE.match(seg, s):
                continue                       # 分组标签：主体由后续标签承接，非违规
            if seg[s] != '{':
                hit('C1', ln_of(base + off))
                continue
            brace_line = ln_of(base + colon + 1 + s)
            if brace_line == ln_of(base + colon):
                hit('C2', brace_line)
            bclose = match_delim(seg, s, '{', '}')
            if bclose < 0:
                continue
            inner = blank_pp(seg[s + 1:bclose])
            bi = find_break_inside(inner)
            if bi >= 0 and not is_ctrl_body(inner, bi):
                hit('C3', ln_of(base + colon + 1 + s + 1 + bi))
            # C4：`}` 之后紧跟终结语句，或体末语句本身是跳转语句（break/return/continue/goto）
            #     → 该 case 不会 fallthrough，不报 C4。体末语句按语句层级取（见 last_stmt_head），
            #     故 `if (x) return -1;` 这种末语句是 if 的写法仍报 C4。
            if (not END_RE.match(blank_pp(seg[bclose + 1:]))
                    and not END_RE.match(last_stmt_head(inner))):
                hit('C4', ln_of(base + colon + 1 + bclose))
    return out


# ---- A1/A2 修复（--apply；B/C1/C2/C3/C4 一律不动）--------------------------------
IF_OPEN_RE = re.compile(r'^\s*#\s*(if|ifdef|ifndef)\b\s*(.*)$')
IDENT_RE = re.compile(r'[A-Za-z_]\w*')
DEFINED_PAREN_RE = re.compile(r'\bdefined\s*\([^()]*\)')
DEFINED_RE = re.compile(r'\bdefined\b')


def lead_ws(s):
    """行首空白字符串（原样复制，不换算 Tab/空格）"""
    return s[:len(s) - len(s.lstrip(' \t'))]


def strip_comments(text):
    """按字符状态剥离 /* */ 与 // 注释（跳过字符串/字符字面量），与 tmpl_verify.py 同口径"""
    out, i, n = [], 0, len(text)
    while i < n:
        c = text[i]
        if c in '"\'':
            q = c
            out.append(c)
            i += 1
            while i < n:
                if text[i] == '\\' and i + 1 < n:
                    out.append(text[i:i + 2])
                    i += 2
                    continue
                out.append(text[i])
                if text[i] == q:
                    i += 1
                    break
                i += 1
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            out.append(' ')
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
            out.append(' ')
            continue
        out.append(c)
        i += 1
    return ''.join(out)


def fingerprint(lines):
    """代码特征串：剥离注释后再压缩空白（用于修复前后的完整性断言）"""
    return '\n'.join(re.sub(r'\s+', ' ', strip_comments(l)).strip() for l in lines)


def owner_map(masked, pp):
    """预处理条件配对：owner[i] 为第 i 行所属（#if 行为自身）的 #if/#ifdef/#ifndef 行号"""
    owner = [None] * len(masked)
    stack = []
    for i, ml in enumerate(masked):
        if not pp[i]:
            continue
        m = DIRECTIVE_RE.match(ml)
        if not m:
            continue
        kind = m.group(1)
        if kind in ('if', 'ifdef', 'ifndef'):
            stack.append(i)
            owner[i] = i
        elif stack:
            if kind in ('else', 'elif'):
                owner[i] = stack[-1]
            else:                                            # endif
                owner[i] = stack.pop()
    return owner


def ref_forward_indent(lines, masked, pp, owner, i):
    """#if 的缩进参考：本分支内紧随其后的第一个非空、非预处理行的行首空白"""
    for j in range(i + 1, len(lines)):
        if pp[j]:
            m = DIRECTIVE_RE.match(masked[j])
            if m and m.group(1) in ('else', 'elif', 'endif') and owner[j] == i:
                return None                                  # 本分支已结束，无参考行
            continue
        if lines[j].strip() == '':
            continue
        return lead_ws(lines[j])
    return None


def ref_backward_indent(lines, pp, cdepth, i):
    """回退参考：同层级上前面最近一条非空、非预处理行的行首空白"""
    for j in range(i - 1, -1, -1):
        if pp[j] or lines[j].strip() == '' or cdepth[j] != cdepth[i]:
            continue
        return lead_ws(lines[j])
    return None


def cond_name(raw):
    """从配对的 #if 行取 #endif 注释用的条件名；取不出（含 #ifdef __cplusplus）返回 None"""
    m = IF_OPEN_RE.match(strip_comments(raw).strip())
    if not m:
        return None
    kind, rest = m.group(1), m.group(2).strip()
    if kind in ('ifdef', 'ifndef'):
        mm = IDENT_RE.match(rest)
        if not mm or mm.group(0) == '__cplusplus':
            return None                                      # 模板 extern "C" 的 #endif 不带注释
        return mm.group(0)
    s = rest
    if s.startswith('(') and match_delim(s, 0, '(', ')') == len(s) - 1:
        s = s[1:-1].strip()                                  # 去掉最外层括号
    if s in ('0', '1'):
        return s                                             # 裸 1/0
    s = DEFINED_PAREN_RE.sub(' ', s)                          # 删除 defined(...)
    s = DEFINED_RE.sub(' ', s).replace('!', ' ')
    ids = IDENT_RE.findall(s)                                # 全同取该标识符，多标识符取第一个
    return ids[0] if ids else None


def plan_fix(lines):
    """只修 A1（块内顶格条件编译行缩进）与 A2（块内 #endif 补条件注释）。
    返回 (new_lines, n_a1, n_a2)；无改动时 new_lines 与原列表等值。"""
    masked = mask_lines(lines)
    pp = pp_flags(masked)
    cdepth = cond_depths(lines, masked, pp)
    owner = owner_map(masked, pp)
    new = list(lines)
    n_a1 = n_a2 = 0

    # ---- A1：块内顶格的条件编译行缩进 ----
    for i, ml in enumerate(masked):
        if not pp[i] or cdepth[i] <= 0 or lines[i].startswith((' ', '\t')):
            continue
        m = DIRECTIVE_RE.match(ml)
        if not m:
            continue
        if m.group(1) in ('if', 'ifdef', 'ifndef'):
            ind = ref_forward_indent(lines, masked, pp, owner, i)
            if ind is None:
                ind = ref_backward_indent(lines, pp, cdepth, i)
        else:
            o = owner[i]
            ind = lead_ws(new[o]) if o is not None else None  # #else/#elif/#endif 随配对 #if
        if not ind:                                           # 找不到参考行 → 宁可不改
            continue
        new[i] = ind + lines[i]
        n_a1 += 1

    # ---- A2：块内 #endif 补条件注释 ----
    for i, ml in enumerate(masked):
        if not pp[i] or cdepth[i] <= 0:
            continue
        m = DIRECTIVE_RE.match(ml)
        if not m or m.group(1) != 'endif':
            continue
        if re.search(r'//|/\*', lines[i][m.end():]):
            continue
        o = owner[i]
        name = cond_name(lines[o]) if o is not None else None
        if not name:
            continue
        new[i] = new[i] + '  /* %s */' % name
        n_a2 += 1

    return new, n_a1, n_a2


def load_meta(path):
    """取原文件的换行风格/BOM/末尾换行信息；非 UTF-8 返回 None"""
    try:
        with open(path, 'rb') as f:
            raw = f.read()
        bom = raw.startswith(b'\xef\xbb\xbf')
        text = raw.decode('utf-8-sig')
    except Exception:
        return None
    nl = '\r\n' if '\r\n' in text else ('\r' if '\r' in text else '\n')
    return nl, bom, text.endswith(('\n', '\r'))


def save_file(path, lines, meta):
    """按原换行风格/BOM 回写，仅行内容按 lines 更新（其余字节原样保留）"""
    nl, bom, final_nl = meta
    data = (nl.join(lines) + (nl if final_nl else '')).encode('utf-8')
    if bom:
        data = b'\xef\xbb\xbf' + data
    with open(path, 'wb') as f:
        f.write(data)


def report_fix(proj, plan, apply_mode):
    """打印 A1/A2 修复结果；apply_mode 为真时写盘"""
    rows = []
    n_a1 = n_a2 = 0
    for rel, p, lines, new, a1, a2 in plan:
        if apply_mode:
            if fingerprint(lines) != fingerprint(new):
                print('!! 跳过 %s：修复前后代码特征串不一致' % rel)
                continue
            meta = load_meta(p)
            if meta is None:
                print('!! 跳过 %s：无法保留原换行/BOM' % rel)
                continue
            save_file(p, new, meta)
        rows.append('%s  A1=%d A2=%d' % (rel, a1, a2))
        n_a1 += a1
        n_a2 += a2
    print('\n--- %s ---' % ('A1/A2 修复（已写盘）' if apply_mode else '预计可修（A1/A2，干跑，未写盘）'))
    for s in rows:
        print(s)
    print('%s：修复 A1=%d A2=%d，涉及 %d 个文件%s'
          % (proj, n_a1, n_a2, len(rows), '' if apply_mode else '（干跑，未写盘）'))


def main():
    argv = sys.argv[1:]
    apply_mode = '--apply' in argv
    args = [a for a in argv if a != '--apply']
    if len(args) < 2:
        print('用法: python style_check.py <项目根> <目录列表,逗号分隔> [--apply]', file=sys.stderr)
        return 1
    root, dirs = args[0], args[1].split(',')
    proj = os.path.basename(root.rstrip('\\/'))
    files = []
    for d in dirs:
        d = d.strip()
        if not d:
            continue
        top = os.path.join(root, d)
        if not os.path.isdir(top):
            print('SKIP (no such dir) %s' % top, file=sys.stderr)
            continue
        for dp, dns, fns in os.walk(top):
            dns[:] = [x for x in dns if x.lower() not in SKIP_LOWER]
            for fn in sorted(fns):
                if fn.lower().endswith(('.c', '.h')):
                    files.append(os.path.join(dp, fn))

    hits = {}          # code -> [(rel, line, snippet)]
    bad_files = set()
    plan = []          # [(rel, 路径, 原行, 修复后行, n_a1, n_a2)]
    n_file = 0
    n_exempt = 0
    for p in files:
        if os.path.basename(p) in EXEMPT:
            n_exempt += 1
            continue
        lines = load(p)
        if lines is None:
            print('SKIP (not utf-8) %s' % p, file=sys.stderr)
            continue
        n_file += 1
        rel = os.path.relpath(p, root)
        new, n_a1, n_a2 = plan_fix(lines)
        if n_a1 or n_a2:
            plan.append((rel, p, lines, new, n_a1, n_a2))
        for code, ln, raw in check_file(lines):
            hits.setdefault(code, []).append((rel, ln, raw.strip()[:60]))
            bad_files.add(rel)

    print('SKIP (exempt) %d 个生成/配置向导文件' % n_exempt, file=sys.stderr)

    print('=== %s：%d 个文件，%d 个文件有违规 ===' % (proj, n_file, len(bad_files)))
    for code in ORDER:
        items = hits.get(code, [])
        nf = len({x[0] for x in items})
        print('\n--- [%d 处 / %d 个文件] %s' % (len(items), nf, CAT_NAME[code]))
        for rel, ln, snip in items[:10]:
            print('    %s:%d  %s' % (rel, ln, snip))
        if len(items) > 10:
            print('    … 另 %d 处' % (len(items) - 10))
    print('\nSUMMARY %s %s' % (proj, ' '.join('%s=%d' % (c, len(hits.get(c, []))) for c in ORDER)))
    report_fix(proj, plan, apply_mode)
    return 0


if __name__ == '__main__':
    sys.exit(main())
