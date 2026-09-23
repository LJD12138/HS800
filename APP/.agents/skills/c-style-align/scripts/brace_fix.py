#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""B 类去大括号修复：按 .agents/rules/c-coding-style.md 第 2.1 节
「if / else if / else 单行语句严禁添加 {}，直接换行缩进书写」删掉单语句分支的大括号。

用法：python brace_fix.py <项目根> <目录列表,逗号分隔> [--apply]
  - 不带 --apply：干跑，只报告「可修 / 跳过」清单与统计，不写盘。
  - 带 --apply ：逐文件做写盘前断言，任一不过则整文件跳过（打印 `!! 跳过 <文件> <原因>`）。

支持的三种括号书写形态（K 行 = 分支关键字所在行，分支体恰好 1 条语句）：
  形态 A  `{` 独占一行、`}` 独占一行            -> 删掉这两行，其余行一字不动
  形态 B  `{` 与 K 行同行、`}` 独占一行          -> K 行去掉行尾 `{`，删掉 `}` 行
  形态 C  整体一行 `if (x) { stmt; }`            -> 拆成 K 行 + 缩进后的 stmt 行
  形态 C2 `{ stmt; }` 独占一行（K 行在上一行）   -> 该行改写为「缩进 + stmt」

跳过（一条都不漏）：dangling-else 风险、体内是声明、体内含条件编译、体内含跳转标签、
括号行带注释、空体、K 行条件跨多行、链式/未支持书写形态，以及 EXEMPT / SKIP_DIRS /
非 UTF-8 文件。

复用同目录 style_check.py 的工具函数（load/mask_lines/pp_flags/match_delim/next_sig/
count_stmts/lead_ws/strip_comments/load_meta/save_file 与 SKIP_LOWER/EXEMPT/IF_RE/
ELSE_RE/CASE_RE/DEFAULT_RE）。style_check.py 已用 `if __name__ == '__main__'` 保护，
import 不会执行 main；本脚本不修改 style_check.py。
"""
import bisect
import math
import os
import re
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from style_check import (CASE_RE, DEFAULT_RE, ELSE_RE, EXEMPT, IF_RE, SKIP_LOWER,
                         count_stmts, lead_ws, load, load_meta, mask_lines,
                         match_delim, next_sig, pp_flags, save_file, strip_comments)

# ---- 判定用常量 ------------------------------------------------------------------
STMT_KEYWORDS = {'return', 'break', 'continue', 'goto', 'if', 'else', 'for', 'while',
                 'do', 'switch', 'case', 'default', 'sizeof'}

TYPE_KEYWORDS = {
    'int', 'char', 'short', 'long', 'float', 'double', 'unsigned', 'signed', 'void',
    'bool', 'size_t', 'ssize_t', 'typedef', 'struct', 'union', 'enum', 'static',
    'const', 'volatile', 'register', 'extern', 'inline', 'auto', '_Bool', 'uchar',
    'ushort', 'uint', 'ulong', 'u8', 'u16', 'u32', 'u64', 's8', 's16', 's32', 's64',
    'vu8', 'vu16', 'vu32', 'vu64', 'vs8', 'vs16', 'vs32', 'vs64',
    'uint8_t', 'uint16_t', 'uint32_t', 'uint64_t', 'int8_t', 'int16_t', 'int32_t',
    'int64_t', 'u_int8_t', 'u_int16_t', 'u_int32_t', 'u_int64_t',
    'vuint8_t', 'vuint16_t', 'vuint32_t', 'vuint64_t', 'vint8_t', 'vint16_t',
    'vint32_t', 'vint64_t', 'intptr_t', 'uintptr_t', 'ptrdiff_t', 'uintmax_t',
}

# 「两个标识符挨着」= 典型声明（Type var / Type *p / Type a[ , ;）——group1 后必须词边界，
# 否则 `s_inc = x;` 会被拆成 `s_in` + `c` 而误判为声明
DECL_RE = re.compile(r'^(?:(?:const|volatile|static|register|extern|inline|unsigned|'
                     r'signed|struct|union|enum|long|short)\s+)*([A-Za-z_]\w*)\b\s*'
                     r'(\*+\s*)?([A-Za-z_]\w*)\s*(\[|=|;|,)')
GOTO_RE = re.compile(r'(?<![#\w.])goto\b')
LABEL_RE = re.compile(r'^[A-Za-z_]\w*\s*:')
BODY_IF_RE = re.compile(r'^if\b')

# 自定义类型名收集（工程级预处理；行内匹配，禁止跨行，否则 `}` 换行后的 `return;` 会被误当类型名）
TD_CLOSE_RE = re.compile(r'\}[ \t]*([A-Za-z_]\w*)[ \t]*;')
TD_FUNC_PTR_RE = re.compile(r'typedef\s+[^;{]*\(\s*\*\s*([A-Za-z_]\w*)\s*\)')
TD_SIMPLE_RE = re.compile(r'typedef\s+(?!struct\b|union\b|enum\b)([^;{}()]*?)'
                          r'\b([A-Za-z_]\w*)[ \t]*;')
TAG_RE = re.compile(r'\b(?:struct|union|enum)\s+([A-Za-z_]\w*)')

FORM_DESC = {'A': '形态A（{ 与 } 各独占一行）', 'B': '形态B（{ 与 K 行同行）',
             'C': '形态C（整体一行）', 'C2': '形态C2（{ stmt; } 独占一行）'}
FORM_ORDER = ['A', 'B', 'C', 'C2']

SKIP_REASONS = [
    '分支关键字前有其它 token（同行链式写法）',
    'if 条件跨多行（K 行未闭合）',
    '体内含条件编译（#if/#elif/#else/#endif…）',
    '分支关键字与 { 之间有其它 token',
    '} 后同行还有其它 token（链式写法）',
    '体内含大括号（复合语句/嵌套块）',
    '体内唯一语句以 if 开头（dangling-else 风险）',
    '体内含 case/default/goto/标签',
    '体内唯一语句是声明（去括号改作用域）',
    '括号行带注释',
    '未支持形态（括号书写形式）',
    '与其它分支编辑区间冲突',
    '括号行含其它 token',
]


# ---- 通用小工具 ------------------------------------------------------------------
def tokenize(text):
    """词法切分，返回 [(token, offset)]；注释剔除，字符串/字符字面量归一为 STR/CHR"""
    toks = []
    i, n = 0, len(text)
    while i < n:
        c = text[i]
        if c in ' \t\n\r':
            i += 1
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            i = n if j < 0 else j + 2
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = text.find('\n', i)
            i = n if j < 0 else j
            continue
        if c in '"\'':
            q = c
            k = i + 1
            while k < n:
                if text[k] == '\\' and k + 1 < n:
                    k += 2
                    continue
                if text[k] == q:
                    k += 1
                    break
                k += 1
            toks.append(('STR' if q == '"' else 'CHR', i))
            i = k
            continue
        if c.isalpha() or c == '_':
            k = i + 1
            while k < n and (text[k].isalnum() or text[k] == '_'):
                k += 1
            toks.append((text[i:k], i))
            i = k
            continue
        if c.isdigit():
            k = i + 1
            while k < n and (text[k].isalnum() or text[k] in '._'):
                k += 1
            toks.append((text[i:k], i))
            i = k
            continue
        toks.append((c, i))
        i += 1
    return toks


def apply_edits(lines, edits):
    """按 edits（行号 -> ('del',) / ('set', [行…])）生成新行列表"""
    out = []
    for i, ln in enumerate(lines):
        op = edits.get(i)
        if op is None:
            out.append(ln)
        elif op[0] == 'del':
            continue
        else:
            out.extend(op[1])
    return out


def walk_files(root, dirs):
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
    return files


def collect_types(paths):
    """收集工程内出现过的自定义类型名（typedef 名 / struct·union·enum 标签）"""
    names = set()
    for p in paths:
        lines = load(p)
        if lines is None:
            continue
        text = '\n'.join(mask_lines(lines))
        names.update(TD_CLOSE_RE.findall(text))
        names.update(TD_FUNC_PTR_RE.findall(text))
        names.update(g for _a, g in TD_SIMPLE_RE.findall(text))
        names.update(TAG_RE.findall(text))
    return names - STMT_KEYWORDS - TYPE_KEYWORDS


def infer_indent_unit(lines, pp):
    """该文件最常见的一级缩进增量：Tab 优先；纯空格的取各行缩进长度的最大公约数"""
    tabs = 0
    space_lines = 0
    g = 0
    for i, ln in enumerate(lines):
        if pp[i]:
            continue
        s = lead_ws(ln)
        if not s:
            continue
        if s[0] == '\t':
            tabs += 1
        else:
            space_lines += 1
            g = math.gcd(g, len(s))
    if space_lines == 0 or tabs >= space_lines:
        return '\t'
    return ' ' * (g if g in (2, 4, 8) else 4)


def is_declaration(body_mask, types):
    """分支体内唯一语句是否为声明/定义（去括号会改变变量作用域）"""
    s = body_mask.strip()
    if not s:
        return True
    m = DECL_RE.match(s)
    if m:
        first = m.group(1)
        if first in TYPE_KEYWORDS or first in types:
            return True
        if first not in STMT_KEYWORDS:
            return True                      # 两个标识符挨着（自定义类型未被收集到）
    head = re.match(r'[A-Za-z_]\w*', s)
    if head:
        name = head.group(0)
        rest = s[head.end():].lstrip()
        # 类型名 + 成员访问/赋值/比较不是声明（如 `tSysInfo.usX = 1;`）
        if name in TYPE_KEYWORDS or (name in types and not rest.startswith(('.', '=', '-'))):
            return True
    return False


# ---- 单文件分析 ------------------------------------------------------------------
def analyze_file(lines, types, unit):
    """扫描一个文件，返回 {'edits','fixes','skips','ncand'}（不写盘）"""
    masked = mask_lines(lines)
    pp = pp_flags(masked)
    text = '\n'.join(masked)
    raw = '\n'.join(lines)
    starts = []
    pos = 0
    for ml in masked:
        starts.append(pos)
        pos += len(ml) + 1

    def lof(off):
        return bisect.bisect_right(starts, off) - 1

    cands = []
    for m in IF_RE.finditer(text):
        cands.append((m.start(), m.end(), False))
    for m in ELSE_RE.finditer(text):
        cands.append((m.start(), m.end(), True))
    cands.sort()

    cand, fixes, skips = 0, [], []
    for off0, off1, is_else in cands:
        lk = lof(off0)
        if pp[lk]:
            continue
        if is_else:                                           # else：无括号条件
            pend = off1
        else:
            pclose = match_delim(text, off1 - 1, '(', ')')
            if pclose < 0:
                continue
            pend = pclose + 1
        j = next_sig(text, pend)
        if j >= len(text) or text[j] != '{':
            continue
        jclose = match_delim(text, j, '{', '}')
        if jclose < 0 or count_stmts(text[j + 1:jclose]) != 1:
            continue
        cand += 1
        ls, le = lof(j), lof(jclose)
        body_mask = text[j + 1:jclose]
        body_raw = raw[j + 1:jclose].strip()

        # ---- 跳过判定（自上而下，命中即停） ----
        reason = None
        prefix = strip_comments(raw[starts[lk]:off0]).strip()
        if is_else:
            if prefix:
                reason = '分支关键字前有其它 token（同行链式写法）'
        elif prefix not in ('', 'else'):
            reason = '分支关键字前有其它 token（同行链式写法）'
        if reason is None and not is_else and lof(pclose) != lk:
            reason = 'if 条件跨多行（K 行未闭合）'
        if reason is None and any(pp[i] for i in range(lk, le + 1)):
            reason = '体内含条件编译（#if/#elif/#else/#endif…）'
        if reason is None and strip_comments(raw[pend:j]).strip():
            reason = '分支关键字与 { 之间有其它 token'
        if reason is None and raw[jclose + 1:starts[le] + len(lines[le])].strip():
            reason = '} 后同行还有其它 token（链式写法）'
        if reason is None and ('{' in body_mask or '}' in body_mask):
            reason = '体内含大括号（复合语句/嵌套块）'
        if reason is None and BODY_IF_RE.match(body_mask.lstrip()):
            reason = '体内唯一语句以 if 开头（dangling-else 风险）'
        if reason is None and (CASE_RE.search(body_mask) or DEFAULT_RE.search(body_mask)
                               or GOTO_RE.search(body_mask)
                               or LABEL_RE.match(body_mask.strip())):
            reason = '体内含 case/default/goto/标签'
        if reason is None and is_declaration(body_mask, types):
            reason = '体内唯一语句是声明（去括号改作用域）'

        # ---- 形态判定 ----
        form, deleted, check = None, None, None
        if reason is None:
            lsl, lse = starts[ls], starts[ls] + len(lines[ls])
            lel, lee = starts[le], starts[le] + len(lines[le])
            pre_brace_ws = raw[lsl:j].strip() == ''
            post_brace_ws = raw[j + 1:lse].strip() == ''
            pre_close_ws = raw[lel:jclose].strip() == ''
            post_close_ws = raw[jclose + 1:lee].strip() == ''
            # B/C 会截断 K 行（只保留到 `)`），故 K 行此后不允许有注释/其它 token
            mid_clean = raw[pend:j].strip() == ''
            if ls == lk and le == ls and mid_clean:
                form = 'C'
                deleted = [(j, j + 1), (jclose, jclose + 1)]
            elif (ls == lk and le > ls and mid_clean and post_brace_ws
                  and pre_close_ws and post_close_ws):
                form = 'B'
                deleted = [(j, lse + 1), (lel, lee + 1)]
            elif ls > lk and le == ls and pre_brace_ws and post_close_ws:
                form = 'C2'
                deleted = [(lsl, j), (j, j + 1), (jclose, jclose + 1),
                           (jclose + 1, lee)]
            elif (ls > lk and le > ls and pre_brace_ws and post_brace_ws
                  and pre_close_ws and post_close_ws):
                form = 'A'
                deleted = [(lsl, lse + 1), (lel, lee + 1)]
            if form is None:
                reason = '未支持形态（括号书写形式）'
        if reason is None:
            # 被删/被改写文本只允许是空白与这对括号，绝不丢注释或其它 token
            check = ([(j, j + 1), (jclose, jclose + 1)] if form in ('C', 'C2') else deleted)
            for a, b in check:
                seg = strip_comments(raw[a:b]).strip()
                if seg not in ('{', '}'):
                    if '//' in raw[a:b] or '/*' in raw[a:b]:
                        reason = '括号行带注释'
                    else:
                        reason = '括号行含其它 token'
                    break

        if reason is not None:
            skips.append((reason, lk, lines[lk].strip()[:70]))
            continue

        indent = lead_ws(lines[lk]) + unit
        if form == 'A':
            own = {ls: ('del',), le: ('del',)}
        elif form == 'B':
            own = {lk: ('set', [raw[starts[lk]:pend].rstrip()]), le: ('del',)}
        elif form == 'C':
            own = {lk: ('set', [raw[starts[lk]:pend].rstrip(), indent + body_raw])}
        else:                                                 # C2
            own = {ls: ('set', [indent + body_raw])}
        fixes.append({'lk': lk, 'le': le, 'j': j, 'jclose': jclose, 'form': form,
                      'own_edits': own, 'off_base': starts[lk], 'text': lines[lk]})

    # ---- 合并编辑；同区间冲突则放弃该分支 ----
    file_edits, kept = {}, []
    for fx in fixes:
        keys = list(fx['own_edits'])
        if any(k in file_edits for k in keys):
            skips.append(('与其它分支编辑区间冲突', fx['lk'], lines[fx['lk']].strip()[:70]))
            continue
        for k in keys:
            file_edits[k] = fx['own_edits'][k]
        kept.append(fx)
    return {'edits': file_edits, 'fixes': kept, 'skips': skips, 'ncand': cand}


# ---- 写盘前断言 ------------------------------------------------------------------
def verify_file(lines, plan):
    """四项断言；返回 (ok, msg)"""
    edits, fixes = plan['edits'], plan['fixes']
    new = apply_edits(lines, edits)

    # 断言 1：区间外行逐字节不变
    origin = []
    for i, ln in enumerate(lines):
        op = edits.get(i)
        if op is None:
            origin.append((i, ln))
        elif op[0] == 'del':
            continue
        else:
            for x in op[1]:
                origin.append((i, x))
    if len(origin) != len(new):
        return False, '行映射长度不一致'
    for k, (o, val) in enumerate(origin):
        if o not in edits and val != lines[o]:
            return False, '区间外字节不一致（行 %d）' % (o + 1)

    # 断言 2：每个分支区间「恰好只少这一对括号」
    for fx in fixes:
        lk, le, base = fx['lk'], fx['le'], fx['off_base']
        old_text = '\n'.join(lines[lk:le + 1])
        local = {k - lk: v for k, v in fx['own_edits'].items()}
        new_text = '\n'.join(apply_edits(lines[lk:le + 1], local))
        ot, nt = tokenize(old_text), tokenize(new_text)
        ro, rc = fx['j'] - base, fx['jclose'] - base
        io = [k for k, (t, o) in enumerate(ot) if o == ro]
        ic = [k for k, (t, o) in enumerate(ot) if o == rc]
        if not io or not ic or ot[io[0]][0] != '{' or ot[ic[0]][0] != '}':
            return False, '无法定位被删括号 token'
        a, b = io[0], ic[0]
        if a >= b:
            return False, '被删括号顺序异常'
        if [t for t, _ in ot[:a] + ot[a + 1:b] + ot[b + 1:]] != [t for t, _ in nt]:
            return False, '分支区间 token 序列不等于「仅少一对括号」'

    # 断言 2'（全文件）：除这些括号外，全文件 token 序列完全一致
    ot_all = tokenize('\n'.join(lines))
    nt_all = tokenize('\n'.join(new))
    del_off = set()
    for fx in fixes:
        del_off.add(fx['j'])
        del_off.add(fx['jclose'])
    removed = [(t, o) for t, o in ot_all if o in del_off]
    if len(removed) != 2 * len(fixes) or any(t not in '{}' for t, _ in removed):
        return False, '被删 token 不是恰好 %d 对括号' % len(fixes)
    if [t for t, o in ot_all if o not in del_off] != [t for t, _ in nt_all]:
        return False, '全文件 token 序列（除被删括号外）不一致'

    # 断言 3：全文件 { 与 } 各自恰好减少「分支数」
    mo, mn = mask_lines(lines), mask_lines(new)
    for ch in '{}':
        d = sum(l.count(ch) for l in mo) - sum(l.count(ch) for l in mn)
        if d != len(fixes):
            return False, '%s 总数减少了 %d（应为 %d）' % (ch, d, len(fixes))
    return True, ''


# ---- 报告 ------------------------------------------------------------------------
def run(root, dirs, apply_mode):
    proj = os.path.basename(root.rstrip('\\/'))
    files = walk_files(root, dirs)
    todo = [p for p in files if os.path.basename(p) not in EXEMPT]
    types = collect_types(todo)

    n_exempt = len(files) - len(todo)
    n_file = n_utf8_bad = 0
    n_cand = n_fix = 0
    form_cnt = Counter()
    skip_cnt = Counter()
    skip_samples = {}
    unsupported = []
    file_rows = []
    applied = []
    assert_skip = []

    for p in files:
        if os.path.basename(p) in EXEMPT:
            continue
        lines = load(p)
        if lines is None:
            n_utf8_bad += 1
            print('SKIP (not utf-8) %s' % p, file=sys.stderr)
            continue
        n_file += 1
        rel = os.path.relpath(p, root)
        pp = pp_flags(mask_lines(lines))
        unit = infer_indent_unit(lines, pp)
        plan = analyze_file(lines, types, unit)
        n_cand += plan['ncand']
        for reason, ln, snip in plan['skips']:
            skip_cnt[reason] += 1
            skip_samples.setdefault(reason, []).append((rel, ln + 1, snip))
            if reason in ('未支持形态（括号书写形式）', '与其它分支编辑区间冲突',
                          '括号行带注释', '括号行含其它 token',
                          '分支关键字前有其它 token（同行链式写法）',
                          '} 后同行还有其它 token（链式写法）'):
                unsupported.append((rel, ln + 1, snip))
        if not plan['fixes']:
            continue
        ok, msg = verify_file(lines, plan)
        if not ok:
            assert_skip.append((rel, msg))
            print('!! 跳过 %s %s' % (rel, msg))
            continue
        if apply_mode:
            meta = load_meta(p)
            if meta is None:
                assert_skip.append((rel, '无法保留原换行/BOM'))
                print('!! 跳过 %s 无法保留原换行/BOM' % rel)
                continue
            save_file(p, apply_edits(lines, plan['edits']), meta)
            applied.append((rel, len(plan['fixes'])))
        n_fix += len(plan['fixes'])
        cnt = Counter(fx['form'] for fx in plan['fixes'])
        form_cnt.update(cnt)
        file_rows.append((rel, len(plan['fixes']), cnt))

    print('=== %s：扫描 %d 个文件（EXEMPT 跳过 %d 个，非 UTF-8 跳过 %d 个）'
          % (proj, n_file, n_exempt, n_utf8_bad))
    print('B 类候选：%d 处（与 style_check.py 的 B 口径一致）' % n_cand)
    head = '--- %s %d 处 / %d 个文件' % ('已写盘修复' if apply_mode else '可修',
                                         n_fix, len(file_rows))
    print('%s：%s' % (head, ' '.join('%s=%d' % (f, form_cnt[f]) for f in FORM_ORDER)))
    for f in FORM_ORDER:
        print('    %s %d 处' % (FORM_DESC[f], form_cnt[f]))
    for rel, cnt, c in file_rows:
        print('    %s  可修=%d（%s）'
              % (rel, cnt, ' '.join('%s=%d' % (f, c[f]) for f in FORM_ORDER if c[f])))
    print('--- 跳过 %d 处（可修 + 跳过 = %d）' % (sum(skip_cnt.values()), n_fix + sum(skip_cnt.values())))
    for reason in SKIP_REASONS:
        if not skip_cnt[reason]:
            continue
        print('    [%d 处] %s' % (skip_cnt[reason], reason))
        for rel, ln, snip in skip_samples.get(reason, [])[:4]:
            print('        %s:%d  %s' % (rel, ln, snip))
    print('未支持形态清单（最多 20 条）：')
    for rel, ln, snip in unsupported[:20]:
        print('    %s:%d  %s' % (rel, ln, snip))
    if len(unsupported) > 20:
        print('    … 另 %d 条' % (len(unsupported) - 20))
    if assert_skip:
        print('断言未过而未写盘的文件：%d 个' % len(assert_skip))
        for rel, msg in assert_skip[:20]:
            print('    %s  %s' % (rel, msg))
    print('SUMMARY %s %s%d 处 跳过=%d 候选=%d %s'
          % (proj, '已修复=' if apply_mode else '可修=', n_fix, sum(skip_cnt.values()),
             n_cand, ' '.join('%s=%d' % (f, form_cnt[f]) for f in FORM_ORDER)))
    return 0


def main():
    argv = sys.argv[1:]
    apply_mode = '--apply' in argv
    args = [a for a in argv if a != '--apply']
    if len(args) < 2:
        print('用法: python brace_fix.py <项目根> <目录列表,逗号分隔> [--apply]',
              file=sys.stderr)
        return 1
    return run(args[0], args[1].split(','), apply_mode)


if __name__ == '__main__':
    sys.exit(main())
