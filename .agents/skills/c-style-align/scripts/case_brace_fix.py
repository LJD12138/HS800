#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""C1/C3 类修复：按 .agents/rules/c-coding-style.md 第 2.1 节
「每个 case 分支语句块必须用大括号 {} 包裹其执行主体，break; 放置于大括号外独立一行」
  - C1：给 case/default 分支体补上 {}，并把 break; 移出大括号外。
  - SL：单行 case 展开——标签与整个分支体（含终结语句）写在同一行时，拆成
        「标签独占一行 / { / 体逐条独立成行 / } / break;」，与 C1 目标形态一致。
  - C3：把已经用 {} 包裹的 case 主体里、位于大括号内（相对深度 0、且不是无括号控制
        语句体）的收尾 break; 删掉，改在 `}` 之后新增一行（缩进取 `}` 行的行首空白）。

用法：python case_brace_fix.py <项目根> <目录列表,逗号分隔> [--apply]
  - 不带 --apply：干跑，只报告「可修 / 跳过」清单与统计，不写盘。
  - 带 --apply ：依次做 C1 补括号、SL 单行 case 展开、C3 的 break; 外移；逐文件先做
                 断言 + style_check 不变式校验，任一不过则整文件跳过
                 （打印 `!! 跳过 <文件> <原因>`）。

SL（单行 case 展开）覆盖的形态（缩进 = 标签行行首空白 + 该文件的一级缩进单位）：
  形态1 `case X:   stmt;   break;`      -> 标签行 / { / stmt; / } / break;
  形态2 `case X:   return 6;   /* c */` -> 标签行 / { / return 6;   /* c */ / }
        （体以 return/continue/goto 结尾不需要 break，故不补 break;）
  形态3 `case A: case B: stmt; break;`  -> case A: / case B: / { / stmt; / } / break;
  形态4 `case X: break;`                -> 标签行 / { / } / break;
  形态5 `case X: a = 1; b = 2; break;`  -> 标签行 / { / a = 1; / b = 2; / } / break;
  拆行只改动空白，token 序列不变；唯一新增 token 就是这一对 {}。

SL 跳过（一条都不漏）：
  1 体内含变量/类型声明（补括号会收紧作用域，沿用 C1 的 is_declaration 判定）
  2 体内含 case/default/goto 标签
  3 体内含条件编译（#if/#ifdef/#else/#elif/#endif…），或标签行/体内出现 `#`
  4 注释无法安全归属（注释在 break; 之后、夹在两条语句之间、或注释本身跨行）
  5 标签行有多个 case/default 与其它 token 混杂；本组标签未全部与体同行（未支持形态）
  6 体跨多行（非单行 case，交由 C1 处理）；体内无终结语句（疑似故意 fallthrough）
  7 与其它单行 case 编辑区间冲突；断言/写盘未过（整文件跳过）

目标形态（标签行原样不动，{ 与 } 各独占一行且与标签同缩进，break; 与标签同缩进）：
    case X:                 case A:                 case X:
    {                       case B:                 {
        stmt1;              {                           if (a)
        stmt2;                  stmt;                   { stmt; }
    }                       }                       }
    break;                  break;                  break;

支持：分组标签（连续多个 case/default 共用一个体，只在最后一个标签后插一对 {}）、
      空体（标签后直接 break;）、体内含嵌套 {}（if/for/while 复合语句一律允许）。

跳过（一条都不漏）：
  1 体内含变量/类型声明（补括号会收紧作用域 → 后面 case 引用即编译失败）——最重要
  2 体内含 case/default/goto 标签
  3 体内含条件编译（#if/#ifdef/#ifndef/#else/#elif/#endif）
  4 标签行带注释
  5 标签行不是独占一行（`case X: stmt;`，改由 SL 单行 case 展开处理）或 break; 与其它语句同行
  6 体内无终结语句（疑似故意 fallthrough）
  7 以 return/continue/goto 结尾（补括号会让 `}` 后不再紧跟终结语句 → style_check C4
    会新增，与「C4 逐项不变」自检要求冲突，故一律跳过并单独计数）
  8 已有 {} 包裹（C1 合规写法）→ C1 不动，改由 C3 处理体内的收尾 break;
  9 EXEMPT 文件 / SKIP_DIRS（第三方、生成、.agents/.trae/.vscode…）/ 非 UTF-8 → 不动

C3 额外跳过（不擅自删除任何 break;）：
  a 括号外已经有 break;（括号内那一条属「多余 break」，只报不动，建议人工确认后删除）
  b break; 行带注释 / 与其它语句同行；体内顶层 break; 多于 1 条
  c break; 位于体内条件编译分支内（移动会改变条件编译语义）
  d 嵌套 for/while/if 内部的 break 不动（相对深度 != 0，本就不在候选里）

写盘前断言（每文件，任一不过 → 整文件跳过）：
  1 区间外行逐字节不变
  2 每个被修改 case：新区间 token 序列删掉新插入的那 1 个 { 与 1 个 } token 后，
    与旧区间 token 序列完全一致（break; 的缩进变化是空白，不影响 token）
     C3：去掉被删/新增的 `break ;` 两 token 后，全文件 token 序列完全一致
     （即只有 break; 的位置变了，内容与前后语句顺序不变）
  3 全文件 { 与 } 各自总数恰好增加「本次 C1 修复的 case 数」（C3 不变）
  4 换行风格/BOM/末尾换行保持原样
  5 额外安全闸：修复后 style_check 的其它检查项计数不变、C1 恰好减少「C1 修复数」、
    C3 恰好减少「C3 修复数」（逐候选贪心校验，任一候选会破坏该不变式就丢弃该候选）

复用同目录 style_check.py / brace_fix.py（两者均无副作用 main，import 不执行 main）。
不修改这两个脚本。
"""
import bisect
import os
import re
import sys
from collections import Counter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from brace_fix import (collect_types, infer_indent_unit, is_declaration,
                       tokenize, walk_files)
from style_check import (BREAK_RE, DIRECTIVE_RE, EXEMPT, SWITCH_RE, blank_pp,
                         case_labels, check_file, find_break_inside, is_ctrl_body,
                         lead_ws, load, load_meta, mask_lines, match_delim, next_sig,
                         pp_flags, save_file, strip_comments)

BREAK_ONLY_RE = re.compile(r'^break\s*;$')
OP_RE = re.compile(r'^(return|continue|goto)\b')
IDENT_RE = re.compile(r'^[A-Za-z_]\w*$')
CMT_RE = re.compile(r'/\*.*?\*/|//[^\n]*')

CATS = ['A1', 'A2', 'B', 'C1', 'C2', 'C3', 'C4']

SKIP_REASONS = [
    '体内含变量/类型声明（补括号会收紧作用域）',
    '体内含 case/default/goto 标签',
    '体内含条件编译（#if/#elif/#else/#endif…）',
    '标签行带注释',
    'break 行带注释',
    '标签行与体语句同行（标签非独占一行）',
    'break 行还有其它语句 / break 语句跨多行',
    '体内无终结语句（疑似故意 fallthrough）',
    '以 return/continue/goto 结尾（补括号会新增 style_check C4）',
    'break 缩进既非标签缩进也非标签缩进+1级',
    '标签后无任何语句（switch 体尾）',
    '补括号会改变 style_check 其它检查项（C4/C3/B…）',
    '与其它修复编辑区间冲突',
    '断言/写盘未过（整文件跳过）',
]

C3_SKIP_REASONS = [
    '括号外已有 break;（括号内多余 break，只报不动，建议人工确认后删除）',
    'break; 行带注释或与其它语句同行（未动）',
    '体内顶层 break; 多于 1 条（仅处理唯一收尾 break）',
    'break; 位于体内条件编译分支（移动会改变条件编译语义）',
    '移动后会改变 style_check 其它检查项（C4/C1/B…）',
    '与其它修复编辑区间冲突',
    '断言/写盘未过（整文件跳过）',
]

# SL（单行 case 展开）跳过原因 / 形态
SL_SKIP_REASONS = [
    '体内含变量/类型声明（补括号会收紧作用域）',
    '体内含 case/default/goto 标签',
    '体内含条件编译（#if/#elif/#else/#endif…）',
    '标签行或体内出现 #（预处理/宏，未支持）',
    '注释无法安全归属（不在收尾语句之后）',
    'break; 之后带注释（注释无法安全归属）',
    '注释跨行（无法安全归属）',
    '标签行有多个 case/default 与其它 token 混杂',
    '分组标签未全部与体语句同行（未支持形态）',
    '体跨多行（非单行 case，交由 C1 处理）',
    '体内无终结语句（疑似故意 fallthrough）',
    '展开会改变 style_check 其它检查项（C3/C4/B/C2…）',
    '与其它单行 case 编辑区间冲突',
    '断言/写盘未过（整文件跳过）',
]

SL_FORM_DESC = {'形态1（单语句 + break）': '形态1 单语句 + break',
                '形态2（return/continue/goto 结尾）': '形态2 return/continue/goto 结尾',
                '形态3（分组标签同行）': '形态3 分组标签同行',
                '形态4（空体）': '形态4 空体',
                '形态5（多条语句同行）': '形态5 多条语句同行'}
SL_FORM_ORDER = ['形态1（单语句 + break）', '形态2（return/continue/goto 结尾）',
                 '形态3（分组标签同行）', '形态4（空体）', '形态5（多条语句同行）']


# ---- 小工具 ----------------------------------------------------------------------
def has_comment(s):
    """该行（去注释后是否变短）是否含注释 token"""
    return strip_comments(s) != s


def top_stmts(seg):
    """seg（掩码文本）内相对深度 0 的顶层语句区间 [(start,end)]，含 break;/return; 等"""
    out = []
    depth = 0
    start = None
    i, n = 0, len(seg)
    while i < n:
        c = seg[i]
        if c in ' \t\n':
            i += 1
            continue
        if c == '{':
            if depth == 0 and start is None:
                start = i
            depth += 1
            i += 1
            continue
        if c == '}':
            depth -= 1
            if depth <= 0:
                out.append((start if start is not None else i, i + 1))
                start = None
            i += 1
            continue
        if c == ';':
            if depth == 0:
                out.append((start if start is not None else i, i + 1))
                start = None
            i += 1
            continue
        if depth == 0 and start is None:
            start = i
        i += 1
    return out


def top_labels(seg):
    """seg 内相对深度 0 的 goto 标签（`LoopOn:`）偏移列表；三目 `a ? b : c` 不算"""
    toks = tokenize(seg)
    depth = 0
    out = []
    for k, (t, off) in enumerate(toks):
        if t == '{':
            depth += 1
        elif t == '}':
            depth -= 1
        elif depth == 0 and t == ':' and k >= 1:
            name = toks[k - 1][0]
            if IDENT_RE.match(name) and name not in ('case', 'default'):
                prev = toks[k - 2][0] if k >= 2 else ''
                if prev != '?':
                    out.append(toks[k - 1][1])
    return out


def split_top_stmts(seg):
    """seg（掩码文本）内相对深度 0 的顶层语句区间 [(start,end)]。
    与 style_check.count_stmts 口径一致，另计圆括号/方括号深度，
    避免 `for (i = 0; i < n; i++)` 被括号内的 `;` 误切。"""
    out = []
    depth = pdepth = 0
    start = None
    i, n = 0, len(seg)
    while i < n:
        c = seg[i]
        if c in ' \t\n':
            i += 1
            continue
        if c in '([{':
            if depth == 0 and pdepth == 0 and start is None:
                start = i
            if c == '{':
                depth += 1
            else:
                pdepth += 1
            i += 1
            continue
        if c in ')]}':
            if c == '}':
                if depth > 0:
                    depth -= 1
                if depth == 0 and pdepth == 0:
                    out.append((start if start is not None else i, i + 1))
                    start = None
            else:
                if pdepth > 0:
                    pdepth -= 1
            i += 1
            continue
        if c == ';':
            if depth == 0 and pdepth == 0:
                out.append((start if start is not None else i, i + 1))
                start = None
            i += 1
            continue
        if depth == 0 and pdepth == 0 and start is None:
            start = i
        i += 1
    return out


# ---- SL：单行 case 展开 ------------------------------------------------------------
def build_single_case(lines, text, rawtext, starts, base, run, col, lk, seg,
                      types, unit, pp, lof):
    """单行 case 展开：标签与整个分支体（含终结语句）同行 → 拆多行 + 补一对 {}。
    返回 (cand, reason)；cand 为 None 时 reason 为跳过原因。"""
    line_raw = lines[lk]
    line_off = starts[lk]
    lws = lead_ws(line_raw)
    p = col - line_off                                     # 冒号在该行内的下标

    # 形态3：本组标签必须全部与体同行，且行首即第一个标签、标签之间只允许空白
    if any(lof(base + x[0]) != lk for x in run):
        return None, '分组标签未全部与体语句同行（未支持形态）'
    if base + run[0][0] != line_off + len(lws):
        return None, '标签行有多个 case/default 与其它 token 混杂'
    for a, b in zip(run, run[1:]):
        if rawtext[base + a[2] + 1:base + b[0]].strip():
            return None, '标签行有多个 case/default 与其它 token 混杂'
    # 标签行或体内出现 '#'
    if '#' in line_raw:
        return None, '标签行或体内出现 #（预处理/宏，未支持）'

    stmts = split_top_stmts(seg)
    if not stmts:
        return None, '体内无终结语句（疑似故意 fallthrough）'
    b0 = col + 1 + stmts[0][0]
    b1 = col + 1 + stmts[-1][1]
    if lof(b0) != lk or lof(b1 - 1) != lk or b1 > line_off + len(line_raw):
        return None, '体跨多行（非单行 case，交由 C1 处理）'

    last_raw = text[col + 1 + stmts[-1][0]:b1].strip()
    if BREAK_ONLY_RE.match(last_raw):
        kind = 'break'
    elif OP_RE.match(last_raw):
        kind = 'op'
    else:
        return None, '体内无终结语句（疑似故意 fallthrough）'

    # 体内条件编译（本行至本组体末）
    last_ch = len(seg) - 1
    while last_ch >= 0 and seg[last_ch] in ' \t\n':
        last_ch -= 1
    endl = lof(col + 1 + last_ch) if last_ch >= 0 else lk
    if any(pp[i] for i in range(lk, endl + 1)):
        return None, '体内含条件编译（#if/#elif/#else/#endif…）'
    # 体内声明（作用域风险）
    for a, b in stmts:
        if is_declaration(text[col + 1 + a:col + 1 + b], types):
            return None, '体内含变量/类型声明（补括号会收紧作用域）'
    # 体内顶层 case/default/goto/标签
    if top_labels(seg):
        return None, '体内含 case/default/goto 标签'
    dep = 0
    for t, _o in tokenize(seg):
        if t == '{':
            dep += 1
        elif t == '}':
            dep -= 1
        elif dep == 0 and t in ('case', 'default', 'goto'):
            return None, '体内含 case/default/goto 标签'

    # 注释归属：只允许「收尾语句之后的行尾注释」
    rel = [(col + 1 + a - line_off, col + 1 + b - line_off)
           for a, b in stmts]
    if has_comment(line_raw[p + 1:rel[0][0]]):
        return None, '注释无法安全归属（不在收尾语句之后）'
    for (_a, e1), (s2, _b) in zip(rel, rel[1:]):
        if has_comment(line_raw[e1:s2]):
            return None, '注释无法安全归属（不在收尾语句之后）'
    tail = line_raw[rel[-1][1]:]
    tail_cmt = has_comment(tail)
    if tail_cmt:
        if kind != 'op':
            return None, 'break; 之后带注释（注释无法安全归属）'
        if tail.count('/*') != tail.count('*/'):
            return None, '注释跨行（无法安全归属）'

    # ---- 构造新行（拆行只动空白；break; 放在 } 之外，与 rule 2.1 目标形态一致）----
    new_lines = [lws + rawtext[base + x[0]:base + x[2] + 1] for x in run]
    inner = rel[:-1] if kind == 'break' else rel
    body = [lws + unit + line_raw[a:b].strip() for a, b in inner]
    if kind == 'op' and tail_cmt and body:
        body[-1] += tail.rstrip()
    open_idx = len(new_lines)
    new_lines.append(lws + '{')
    new_lines.extend(body)
    close_idx = len(new_lines)
    new_lines.append(lws + '}')
    if kind == 'break':
        new_lines.append(lws + 'break;')
    if len(run) > 1:
        form = '形态3（分组标签同行）'
    elif kind == 'op':
        form = '形态2（return/continue/goto 结尾）'
    elif not inner:
        form = '形态4（空体）'
    elif len(inner) > 1:
        form = '形态5（多条语句同行）'
    else:
        form = '形态1（单语句 + break）'
    return {'lbl_line': lk, 'new_lines': new_lines, 'form': form,
            'open_idx': open_idx, 'close_idx': close_idx,
            'n_labels': len(run), 'empty': not inner, 'line': lk}, None


def counts(lines):
    c = dict.fromkeys(CATS, 0)
    for code, _ln, _raw in check_file(lines):
        c[code] += 1
    return c


# ---- 编辑算子 --------------------------------------------------------------------
def build_ops(cands):
    """[(opens, closes, sets, conflict)]：opens 插在行尾，closes 插在行首，sets 替换行"""
    opens, closes, sets = {}, {}, {}
    for c in cands:
        if c['lbl_line'] in opens or c['brk_line'] in closes:
            return opens, closes, sets, True
        opens[c['lbl_line']] = c['lbl_ws'] + '{'
        closes[c['brk_line']] = c['lbl_ws'] + '}'
        if c['deindent']:
            sets[c['brk_line']] = c['lbl_ws'] + c['brk_raw'][len(c['brk_ws']):]
    return opens, closes, sets, False


def apply_ops(lines, opens, closes, sets):
    out = []
    for i, ln in enumerate(lines):
        if i in closes:
            out.append(closes[i])
        out.append(sets.get(i, ln))
        if i in opens:
            out.append(opens[i])
    return out


def region_pairs(lines, opens, closes, sets, a, b):
    """区间 [a,b] 的行序列，含新增行标记 [(文本, 是否新插入)]"""
    pairs = []
    for i in range(a, b + 1):
        if i in closes:
            pairs.append((closes[i], True))
        pairs.append((sets.get(i, lines[i]), False))
        if i in opens:
            pairs.append((opens[i], True))
    return pairs


# ---- 单文件分析 ------------------------------------------------------------------
def analyze_file(lines, types, unit):
    """返回 {'cands','sl_cands','skips','n_braced','hits_braced','hits_cand'}（不写盘）"""
    masked = mask_lines(lines)
    pp = pp_flags(masked)
    text = '\n'.join(masked)
    rawtext = '\n'.join(lines)
    starts = []
    pos = 0
    for ml in masked:
        starts.append(pos)
        pos += len(ml) + 1

    def lof(off):
        return bisect.bisect_right(starts, off) - 1

    cands, sl_cands, skips, sl_skips = [], [], [], []
    n_braced = hits_braced = hits_cand = 0

    for m in SWITCH_RE.finditer(text):
        if pp[lof(m.start())]:
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
        if not labels:
            continue
        # 分组：相邻标签之间只有空白（注释在掩码后也是空白）→ 视为同一组
        runs, cur = [], [labels[0]]
        for prev, nxt in zip(labels, labels[1:]):
            if body[prev[2] + 1:nxt[0]].strip() == '':
                cur.append(nxt)
            else:
                runs.append(cur)
                cur = [nxt]
        runs.append(cur)

        for k, run in enumerate(runs):
            col = base + run[-1][2]
            lbl_lines = {lof(base + x[0]) for x in run}
            lk = lof(col)
            seg_end = base + runs[k + 1][0][0] if k + 1 < len(runs) else jclose
            seg = text[col + 1:seg_end]
            fs = next_sig(seg, 0)
            if fs < len(seg) and seg[fs] == '{':
                n_braced += 1
                hits_braced += len(run) - 1          # 末标签合规，前面标签仍被 style_check 计 C1
                continue
            hits_cand += len(run)
            snip = lines[lk].strip()[:70]
            reason = None

            # ---- SL：单行 case（标签与整个体同行）→ 展开成多行 + 一对 {} ----
            if fs < len(seg) and lof(col + 1 + fs) == lk:
                cand, reason = build_single_case(
                    lines, text, rawtext, starts, base, run, col, lk, seg,
                    types, unit, pp, lof)
                if cand is not None:
                    sl_cands.append(cand)
                else:
                    sl_skips.append((reason, lk, snip))
                continue

            # 4 标签行带注释
            if any(has_comment(lines[li]) for li in lbl_lines):
                reason = '标签行带注释'
            # 5 标签后无语句
            if reason is None and fs >= len(seg):
                reason = '标签后无任何语句（switch 体尾）'
            # 3 体内条件编译
            if reason is None:
                last_ch = len(seg) - 1
                while last_ch >= 0 and seg[last_ch] in ' \t\n':
                    last_ch -= 1
                endl = lof(col + 1 + last_ch)
                if any(pp[i] for i in range(lk, endl + 1)):
                    reason = '体内含条件编译（#if/#elif/#else/#endif…）'

            stmts, kind, bk, bws, lws, deind = None, None, -1, '', '', False
            if reason is None:
                stmts = top_stmts(seg)
                if not stmts:
                    reason = '体内无终结语句（疑似故意 fallthrough）'
            if reason is None:
                last = seg[stmts[-1][0]:stmts[-1][1]].strip()
                if BREAK_ONLY_RE.match(last):
                    kind = 'break'
                elif OP_RE.match(last):
                    kind = 'op'
                else:
                    kind = 'fall'
                if kind == 'fall':
                    reason = '体内无终结语句（疑似故意 fallthrough）'
                elif kind == 'op':
                    reason = '以 return/continue/goto 结尾（补括号会新增 style_check C4）'
            # 1 声明（作用域风险）
            if reason is None:
                for a, b in stmts:
                    if is_declaration(seg[a:b], types):
                        reason = '体内含变量/类型声明（补括号会收紧作用域）'
                        break
            # 2 体内顶层标签
            if reason is None and top_labels(seg):
                reason = '体内含 case/default/goto 标签'
            # 5/break 行检查
            if reason is None:
                bk = lof(col + 1 + stmts[-1][1] - 1)
                braw = lines[bk]
                if has_comment(braw):
                    reason = 'break 行带注释'
                elif lof(col + 1 + stmts[-1][0]) != bk:
                    reason = 'break 行还有其它语句 / break 语句跨多行'
                elif not re.fullmatch(r'break\s*;', masked[bk].strip()):
                    reason = 'break 行还有其它语句 / break 语句跨多行'
                else:
                    bws = lead_ws(braw)
                    lws = lead_ws(lines[lk])
                    if bws == lws + unit:
                        deind = True
                    elif bws == lws:
                        deind = False
                    else:
                        reason = 'break 缩进既非标签缩进也非标签缩进+1级'
            if reason is not None:
                skips.append((reason, lk, snip))
                continue
            cands.append({'first_lbl_line': min(lbl_lines), 'lbl_line': lk,
                          'brk_line': bk, 'lbl_ws': lws, 'brk_ws': bws,
                          'brk_raw': braw, 'deindent': deind,
                          'n_labels': len(run), 'empty': len(stmts) == 1,
                          'line': lk})
    return {'cands': cands, 'sl_cands': sl_cands, 'skips': skips,
            'sl_skips': sl_skips, 'n_braced': n_braced,
            'hits_braced': hits_braced, 'hits_cand': hits_cand}


# ---- SL：单行 case 展开的编辑算子 / 安全闸 / 断言 --------------------------------
def build_sl_ops(kept):
    """sets：{行号: [替换后的多行]}；区间冲突返回 True"""
    sets = {}
    for c in kept:
        if c['lbl_line'] in sets:
            return sets, True
        sets[c['lbl_line']] = c['new_lines']
    return sets, False


def apply_sl(lines, sets):
    out = []
    for i, ln in enumerate(lines):
        if i in sets:
            out.extend(sets[i])
        else:
            out.append(ln)
    return out


def gate_sl(lines, cands, base):
    """逐候选贪心：保持 A1/A2/B/C2/C3/C4 不变、C1 恰好 −1，否则丢弃该候选"""
    kept, rejected = [], []
    for c in cands:
        sets_try, conflict = build_sl_ops(kept + [c])
        if conflict:
            rejected.append((c, '与其它单行 case 编辑区间冲突'))
            continue
        cnt = counts(apply_sl(lines, sets_try))
        if any(cnt[k] != base[k] for k in CATS if k != 'C1') \
                or cnt['C1'] != base['C1'] - len(kept) - 1:
            rejected.append((c, '展开会改变 style_check 其它检查项（C3/C4/B/C2…）'))
            continue
        kept.append(c)
    return kept, rejected


def verify_sl(lines, kept):
    """SL 写盘前断言；返回 (ok, msg)"""
    sets, conflict = build_sl_ops(kept)
    if conflict:
        return False, '编辑区间冲突'
    new = apply_sl(lines, sets)

    # 断言 1：除被改写行外，其余行逐字节相同（行映射一一对应）
    idx = 0
    for i, ln in enumerate(lines):
        if i in sets:
            if new[idx:idx + len(sets[i])] != sets[i]:
                return False, '改写行内容异常（行 %d）' % (i + 1)
            idx += len(sets[i])
        else:
            if new[idx] != ln:
                return False, '区间外字节不一致（行 %d）' % (i + 1)
            idx += 1
    if idx != len(new):
        return False, '行数映射不一致'

    # 断言 2：每个候选「只多了一对 {}」，且注释一条不丢
    for c in kept:
        nl = c['new_lines']
        offs, pos = [], 0
        for x in nl:
            offs.append(pos)
            pos += len(x) + 1
        new_text = '\n'.join(nl)
        ot, nt = tokenize(lines[c['lbl_line']]), tokenize(new_text)
        spans = [(offs[c['open_idx']], offs[c['open_idx']] + len(nl[c['open_idx']])),
                 (offs[c['close_idx']], offs[c['close_idx']] + len(nl[c['close_idx']]))]
        ins = [(t, o) for t, o in nt if any(a <= o < b for a, b in spans)]
        if sorted(t for t, _ in ins) != ['{', '}']:
            return False, '新增行内 token 不是恰好一对括号（行 %d）' % (c['lbl_line'] + 1)
        if ''.join(t for t, _ in sorted(ins, key=lambda x: x[1])) != '{}':
            return False, '新增括号顺序异常（行 %d）' % (c['lbl_line'] + 1)
        rm = {o for _t, o in ins}
        if [t for t, o in nt if o not in rm] != [t for t, _o in ot]:
            return False, '区间 token 序列不等于「仅多一对括号」（行 %d）' % (c['lbl_line'] + 1)
        if sorted(CMT_RE.findall(lines[c['lbl_line']])) != sorted(CMT_RE.findall(new_text)):
            return False, '注释不守恒（行 %d）' % (c['lbl_line'] + 1)

    # 断言 3：全文件 { 与 } 各自总数恰好增加「本次展开的 case 数」
    mo, mn = mask_lines(lines), mask_lines(new)
    for ch in '{}':
        d = sum(l.count(ch) for l in mn) - sum(l.count(ch) for l in mo)
        if d != len(kept):
            return False, '%s 总数变化 %+d（应为 +%d）' % (ch, d, len(kept))
    return True, ''


# ---- C3：case 主体大括号内的收尾 break; 外移 --------------------------------------
def pp_depths(masked, pp):
    """每行的条件编译嵌套深度（#if/#ifdef/#ifndef 记当前深度后 +1；#endif 先 -1；
    #else/#elif 与普通行记当前深度）"""
    dep, d = [], 0
    for i, ml in enumerate(masked):
        m = DIRECTIVE_RE.match(ml) if pp[i] else None
        kind = m.group(1) if m else None
        if kind == 'endif':
            d = max(d - 1, 0)
        dep.append(d)
        if kind in ('if', 'ifdef', 'ifndef'):
            d += 1
    return dep


def breaks_inside(inner):
    """inner（掩码文本）内相对深度 0 的所有 break; 下标列表（嵌套块内的不计）"""
    out, depth, i, n = [], 0, 0, len(inner)
    while i < n:
        c = inner[i]
        if c == '{':
            depth += 1
        elif c == '}':
            depth -= 1
        elif depth == 0 and c == 'b':
            m = BREAK_RE.match(inner, i)
            if m:
                out.append(i)
                i = m.end()
                continue
        i += 1
    return out


def line_offsets(lines):
    """每行在 '\\n'.join(lines) 中的起始下标"""
    offs, pos = [], 0
    for ln in lines:
        offs.append(pos)
        pos += len(ln) + 1
    return offs


def analyze_c3_file(lines):
    """找 case 主体 {} 内「相对深度 0 且非无括号控制语句体」的收尾 break; 外移候选。
    返回 {'cands','skips','hits'}；cands 项 {
    'lbl_line','brk_line','close_line','indent','snip'}。"""
    masked = mask_lines(lines)
    pp = pp_flags(masked)
    dep = pp_depths(masked, pp)
    text = '\n'.join(masked)
    btext = blank_pp(text)
    starts, pos = [], 0
    for ml in masked:
        starts.append(pos)
        pos += len(ml) + 1

    def lof(off):
        return bisect.bisect_right(starts, off) - 1

    cands, skips = [], []
    hits = 0
    for m in SWITCH_RE.finditer(text):
        if pp[lof(m.start())]:
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
            if s >= len(seg) or seg[s] != '{':
                continue                       # 未用 {} 包裹（C1 范畴），不判 C3
            bclose = match_delim(seg, s, '{', '}')
            if bclose < 0:
                continue
            inner = blank_pp(seg[s + 1:bclose])
            bi = find_break_inside(inner)       # 与 style_check C3 同口径：首个顶层 break;
            if bi < 0 or is_ctrl_body(inner, bi):
                continue
            hits += 1
            lbl_line = lof(base + off)
            brk_line = lof(base + colon + 1 + s + 1 + bi)
            close_line = lof(base + colon + 1 + bclose)
            snip = lines[brk_line].strip()[:70]
            reason = None
            others = [x for x in breaks_inside(inner) if not is_ctrl_body(inner, x)]
            if len(others) > 1:
                reason = '体内顶层 break; 多于 1 条（仅处理唯一收尾 break）'
            if reason is None and (has_comment(lines[brk_line])
                                   or masked[brk_line].strip() != 'break;'):
                reason = 'break; 行带注释或与其它语句同行（未动）'
            if reason is None and dep[brk_line] != dep[lbl_line]:
                reason = 'break; 位于体内条件编译分支（移动会改变条件编译语义）'
            if reason is None:
                nxt = [t for t, _o in tokenize(btext[base + colon + 1 + bclose + 1:])[:2]]
                if nxt == ['break', ';']:
                    reason = '括号外已有 break;（括号内多余 break，只报不动，建议人工确认后删除）'
            if reason is None:
                cands.append({'lbl_line': lbl_line, 'brk_line': brk_line,
                              'close_line': close_line, 'snip': snip,
                              'indent': lead_ws(lines[close_line])})
            else:
                skips.append((reason, brk_line, snip))
    return {'cands': cands, 'skips': skips, 'hits': hits}


def build_c3_ops(kept):
    """dels 为待删除行集合；ins 为 {行号: [其后插入的行]}；编辑区间冲突时返回 True"""
    dels, ins = set(), {}
    for c in kept:
        bl, cl = c['brk_line'], c['close_line']
        if bl == cl or bl in dels or cl in dels or bl in ins or cl in ins:
            return dels, ins, True
        dels.add(bl)
        ins[cl] = [c['indent'] + 'break;']
    return dels, ins, False


def apply_c3(lines, dels, ins):
    out = []
    for i, ln in enumerate(lines):
        if i in dels:
            continue
        out.append(ln)
        if i in ins:
            out.extend(ins[i])
    return out


def gate_c3(lines, cands, base):
    """逐候选贪心：保持 style_check 其它检查项不变、C3 恰好 −1，否则丢弃该候选"""
    kept, rejected = [], []
    for c in cands:
        dels, ins, conflict = build_c3_ops(kept + [c])
        if conflict:
            rejected.append((c, '与其它修复编辑区间冲突'))
            continue
        cnt = counts(apply_c3(lines, dels, ins))
        if any(cnt[k] != base[k] for k in CATS if k != 'C3') \
                or cnt['C3'] != base['C3'] - len(kept) - 1:
            rejected.append((c, '移动后会改变 style_check 其它检查项（C4/C1/B…）'))
            continue
        kept.append(c)
    return kept, rejected


def verify_c3(lines, kept):
    """C3 写盘前四项断言；返回 (ok, msg)"""
    dels, ins, conflict = build_c3_ops(kept)
    if conflict:
        return False, '编辑区间冲突'
    new = apply_c3(lines, dels, ins)

    # 断言 1：除被修改的行区间外，其余行逐字节相同（行映射一一对应）
    expected = []
    for i, ln in enumerate(lines):
        if i in dels:
            continue
        expected.append(ln)
        expected.extend(ins.get(i, []))
    if new != expected:
        return False, '行映射不一致'
    idx = 0
    for i, ln in enumerate(lines):
        if i in dels:
            continue
        if new[idx] != ln:
            return False, '行内容异常（行 %d）' % (i + 1)
        idx += 1 + len(ins.get(i, []))
    if idx != len(new):
        return False, '行数映射不一致'

    # 断言 2：token 级——去掉被删/新增的 `break ;` 后全文件 token 序列完全一致
    ooff, noff = line_offsets(lines), line_offsets(new)
    ot = tokenize('\n'.join(lines))
    nt = tokenize('\n'.join(new))
    del_spans = [(ooff[i], ooff[i] + len(lines[i])) for i in dels]
    ins_spans, idx = [], 0
    for i, ln in enumerate(lines):
        if i in dels:
            continue
        idx += 1
        for x in ins.get(i, []):
            ins_spans.append((noff[idx], noff[idx] + len(x)))
            idx += 1
    rm_o = {o for _t, o in ot if any(a <= o < b for a, b in del_spans)}
    rm_n = {o for _t, o in nt if any(a <= o < b for a, b in ins_spans)}
    want = sorted(['break', ';'] * len(kept))
    if sorted(t for t, o in ot if o in rm_o) != want \
            or sorted(t for t, o in nt if o in rm_n) != want:
        return False, '被删/新增 token 不是恰好 %d 个 break;' % len(kept)
    if [t for t, o in ot if o not in rm_o] != [t for t, o in nt if o not in rm_n]:
        return False, 'token 序列（除 break; 位置外）不一致'

    # 断言 3：全文件 { 与 } 各自总数不变
    mo, mn = mask_lines(lines), mask_lines(new)
    for ch in '{}':
        if sum(l.count(ch) for l in mo) != sum(l.count(ch) for l in mn):
            return False, '%s 总数变化（应为 0）' % ch
    return True, ''


# ---- 写盘前断言 ------------------------------------------------------------------
def verify_file(lines, kept):
    opens, closes, sets, conflict = build_ops(kept)
    if conflict:
        return False, '编辑区间冲突'
    touched = set(opens) | set(closes) | set(sets)
    new = apply_ops(lines, opens, closes, sets)

    # 断言 1：区间外行逐字节不变（含行数映射）
    pos = 0
    for i, ln in enumerate(lines):
        if i in closes:
            if new[pos] != closes[i]:
                return False, '新增行内容异常（行 %d 前）' % (i + 1)
            pos += 1
        want = sets.get(i, ln)
        if new[pos] != want:
            return False, '行内容异常（行 %d）' % (i + 1)
        if want != ln and i not in touched:
            return False, '区间外字节不一致（行 %d）' % (i + 1)
        pos += 1
        if i in opens:
            if new[pos] != opens[i]:
                return False, '新增行内容异常（行 %d 后）' % (i + 1)
            pos += 1
    if pos != len(new):
        return False, '行数映射不一致'

    # 断言 2：每个被修改 case「只多了这一对括号」（只用该 case 自己的编辑算子，
    # 避免同一 switch 内层嵌套 case 的编辑混进本 case 的区间）
    for c in kept:
        a, b = c['first_lbl_line'], c['brk_line']
        c_opens, c_closes, c_sets, _cf = build_ops([c])
        pairs = region_pairs(lines, c_opens, c_closes, c_sets, a, b)
        pos, spans = 0, []
        for txt, ins in pairs:
            if ins:
                spans.append((pos, pos + len(txt)))
            pos += len(txt) + 1
        new_text = '\n'.join(t for t, _ in pairs)
        old_text = '\n'.join(lines[a:b + 1])
        t_new = tokenize(new_text)
        ins_toks = [(t, o) for t, o in t_new if any(s <= o < e for s, e in spans)]
        if len(ins_toks) != 2 or sorted(t for t, _ in ins_toks) != ['{', '}']:
            return False, '新插入 token 不是恰好一对括号（行 %d）' % (c['lbl_line'] + 1)
        if ''.join(t for t, _ in sorted(ins_toks, key=lambda x: x[1])) != '{}':
            return False, '新插入括号顺序异常（行 %d）' % (c['lbl_line'] + 1)
        rm = {o for _t, o in ins_toks}
        seq = [t for t, o in t_new if o not in rm]
        if seq != [t for t, _ in tokenize(old_text)]:
            return False, '区间 token 序列不等于「仅多一对括号」（行 %d）' % (c['lbl_line'] + 1)

    # 断言 2'（全文件）：新 token 序列 = 旧序列 + 恰好「修复数」对括号，且新括号都在新增行内
    pos, spans = 0, []
    for i, ln in enumerate(lines):
        if i in closes:
            spans.append((pos, pos + len(closes[i])))
            pos += len(closes[i]) + 1
        txt = sets.get(i, ln)
        pos += len(txt) + 1
        if i in opens:
            spans.append((pos, pos + len(opens[i])))
            pos += len(opens[i]) + 1
    t_new = tokenize('\n'.join(new))
    ins_toks = [(t, o) for t, o in t_new if any(s <= o < e for s, e in spans)]
    if len(ins_toks) != 2 * len(kept) or any(t not in '{}' for t, _ in ins_toks):
        return False, '新增行内 token 不是恰好 %d 对括号' % len(kept)
    rm = {o for _t, o in ins_toks}
    if [t for t, o in t_new if o not in rm] != [t for t, _ in tokenize('\n'.join(lines))]:
        return False, '全文件 token 序列（除新增括号外）不一致'

    # 断言 3：全文件 {} 各自恰好增加「修复数」
    mo, mn = mask_lines(lines), mask_lines(new)
    for ch in '{}':
        d = sum(l.count(ch) for l in mn) - sum(l.count(ch) for l in mo)
        if d != len(kept):
            return False, '%s 总数变化 %+d（应为 +%d）' % (ch, d, len(kept))
    return True, ''


def gate(lines, cands, base):
    """逐候选贪心：保持 style_check 的 A1/A2/B/C2/C3/C4 不变、C1 恰好 −1，否则丢弃该候选"""
    kept, rejected = [], []
    for c in cands:
        trial_cands = kept + [c]
        opens, closes, sets, conflict = build_ops(trial_cands)
        if conflict:
            rejected.append((c, '与其它修复编辑区间冲突'))
            continue
        cnt = counts(apply_ops(lines, opens, closes, sets))
        if any(cnt[k] != base[k] for k in CATS if k != 'C1') \
                or cnt['C1'] != base['C1'] - len(kept) - 1:
            rejected.append((c, '补括号会改变 style_check 其它检查项（C4/C3/B…）'))
            continue
        kept.append(c)
    return kept, rejected


# ---- 报告 ------------------------------------------------------------------------
def run(root, dirs, apply_mode):
    proj = os.path.basename(root.rstrip('\\/'))
    files = walk_files(root, dirs)
    todo = [p for p in files if os.path.basename(p) not in EXEMPT]
    n_exempt = len(files) - len(todo)
    types = collect_types(todo)

    n_file = n_utf8_bad = 0
    n_cand = n_fix = n_braced = 0
    n_group = n_pre_skip = 0
    hits_braced = hits_cand = 0
    skip_cnt = Counter()
    skip_samples = {}
    shape = Counter()
    fixed_shape = Counter()
    file_rows, applied, assert_skip = [], [], []
    c1_hits_total = 0
    n_c3_hit = n_c3_cand = n_c3_fix = n_c3_pre_skip = 0
    c3_skip_cnt = Counter()
    c3_skip_samples = {}
    c3_file_rows, c3_assert_skip = [], []
    n_sl_hit = n_sl_cand = n_sl_fix = n_sl_pre_skip = 0
    sl_skip_cnt = Counter()
    sl_skip_samples = {}
    sl_shape = Counter()
    sl_fixed_shape = Counter()
    sl_file_rows, sl_assert_skip = [], []

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
        res = analyze_file(lines, types, unit)
        base = counts(lines)
        c1_hits_total += base['C1']
        n_braced += res['n_braced']
        hits_braced += res['hits_braced']
        hits_cand += res['hits_cand']
        n_cand += len(res['cands'])
        n_sl_cand += len(res['sl_cands'])
        n_group += (len(res['cands']) + len(res['sl_cands'])
                    + len(res['skips']) + len(res['sl_skips']))
        n_pre_skip += len(res['skips'])
        for reason, ln, snip in res['skips']:
            skip_cnt[reason] += 1
            skip_samples.setdefault(reason, []).append((rel, ln + 1, snip))
        for reason, ln, snip in res['sl_skips']:
            n_sl_pre_skip += 1
            sl_skip_cnt[reason] += 1
            sl_skip_samples.setdefault(reason, []).append((rel, ln + 1, snip))
        n_sl_hit += len(res['sl_cands']) + len(res['sl_skips'])
        for c in res['cands']:
            shape['多标签分组' if c['n_labels'] >= 2 else '单标签'] += 1
            if c['empty']:
                shape['其中空体'] += 1
        for c in res['sl_cands']:
            sl_shape[c['form']] += 1
        kept, rejected = gate(lines, res['cands'], base)
        for c, reason in rejected:
            skip_cnt[reason] += 1
            skip_samples.setdefault(reason, []).append(
                (rel, c['line'] + 1, lines[c['line']].strip()[:70]))
        cur = lines                              # 本文件 C1 修复后的行（干跑即内存结果）
        if kept:
            ok, msg = verify_file(lines, kept)
            if not ok:
                assert_skip.append((rel, msg))
                skip_cnt['断言/写盘未过（整文件跳过）'] += len(kept)
                skip_samples.setdefault('断言/写盘未过（整文件跳过）', []).append(
                    (rel, kept[0]['line'] + 1, msg))
                print('!! 跳过 %s %s' % (rel, msg))
            else:
                opens, closes, sets, _cf = build_ops(kept)
                cur = apply_ops(lines, opens, closes, sets)
                n_fix += len(kept)
                cnt = Counter('多标签分组' if c['n_labels'] >= 2 else '单标签' for c in kept)
                cnt['其中空体'] = sum(1 for c in kept if c['empty'])
                fixed_shape.update(cnt)
                file_rows.append((rel, len(kept), cnt))
                if apply_mode:
                    applied.append((rel, len(kept)))

        # ---- SL：单行 case 展开（标签与整个体同行 → 拆多行 + 补一对 {}）----
        if res['sl_cands']:
            sl_kept, sl_rejected = gate_sl(cur, res['sl_cands'],
                                           base if cur is lines else counts(cur))
            for c, reason in sl_rejected:
                sl_skip_cnt[reason] += 1
                sl_skip_samples.setdefault(reason, []).append(
                    (rel, c['line'] + 1, lines[c['line']].strip()[:70]))
            if sl_kept:
                ok, msg = verify_sl(cur, sl_kept)
                if not ok:
                    sl_assert_skip.append((rel, msg))
                    sl_skip_cnt['断言/写盘未过（整文件跳过）'] += len(sl_kept)
                    sl_skip_samples.setdefault('断言/写盘未过（整文件跳过）', []).append(
                        (rel, sl_kept[0]['line'] + 1, msg))
                    print('!! 跳过 %s %s' % (rel, msg))
                else:
                    sets_sl, _cf = build_sl_ops(sl_kept)
                    cur = apply_sl(cur, sets_sl)
                    n_sl_fix += len(sl_kept)
                    cnt_sl = Counter(c['form'] for c in sl_kept)
                    sl_fixed_shape.update(cnt_sl)
                    sl_file_rows.append((rel, len(sl_kept), cnt_sl))

        # ---- C3：把 case 主体大括号内的收尾 break; 移到 `}` 之后独立成行 ----
        c3 = analyze_c3_file(cur)
        n_c3_hit += c3['hits']
        n_c3_cand += len(c3['cands'])
        n_c3_pre_skip += len(c3['skips'])
        for reason, ln, snip in c3['skips']:
            c3_skip_cnt[reason] += 1
            c3_skip_samples.setdefault(reason, []).append((rel, ln + 1, snip))
        if c3['cands']:
            c3_kept, c3_rejected = gate_c3(cur, c3['cands'],
                                           base if cur is lines else counts(cur))
            for c, reason in c3_rejected:
                c3_skip_cnt[reason] += 1
                c3_skip_samples.setdefault(reason, []).append(
                    (rel, c['brk_line'] + 1, c['snip']))
            if c3_kept:
                ok, msg = verify_c3(cur, c3_kept)
                if not ok:
                    c3_assert_skip.append((rel, msg))
                    c3_skip_cnt['断言/写盘未过（整文件跳过）'] += len(c3_kept)
                    c3_skip_samples.setdefault('断言/写盘未过（整文件跳过）', []).append(
                        (rel, c3_kept[0]['brk_line'] + 1, msg))
                    print('!! 跳过 %s %s' % (rel, msg))
                else:
                    dels, ins, _cf = build_c3_ops(c3_kept)
                    cur = apply_c3(cur, dels, ins)
                    n_c3_fix += len(c3_kept)
                    c3_file_rows.append((rel, len(c3_kept)))

        if apply_mode and cur is not lines:
            meta = load_meta(p)
            if meta is None:
                print('!! 跳过 %s 无法保留原换行/BOM' % rel)
                continue
            save_file(p, cur, meta)

    n_skip = sum(skip_cnt.values())
    print('=== %s：扫描 %d 个文件（EXEMPT 跳过 %d 个，非 UTF-8 跳过 %d 个）'
          % (proj, n_file, n_exempt, n_utf8_bad))
    print('未用 {} 包裹的 case 标签组：%d 组（每个标签组算 1 处；= C1 多行候选 %d + '
          'SL 单行候选 %d + 多行跳过 %d + 单行跳过 %d）'
          % (n_group, n_cand, n_sl_cand, n_pre_skip, n_sl_pre_skip))
    if n_group != c1_hits_total:
        print('!! 标签组 %d 与 style_check C1 命中 %d 不一致' % (n_group, c1_hits_total))
    print('  = 可修 %d + 判定跳过 %d + 安全闸丢弃 %d'
          % (n_cand, n_pre_skip, n_skip - n_pre_skip))
    print('已合规（{ 已在）的标签组 %d 组（其非末标签仍被 style_check 记 C1 共 %d 处）'
          % (n_braced, hits_braced))
    print('折算：style_check C1 命中 %d = 已合规组非末标签 %d + 未包裹组全部标签 %d（%s）'
          % (hits_braced + hits_cand, hits_braced, hits_cand,
             '模型自洽' if hits_braced + hits_cand == c1_hits_total
             else '!! 模型与实际 %d 不一致' % c1_hits_total))
    print('待修候选形态：%s' % ' '.join('%s=%d' % (k, shape[k])
                                    for k in ('单标签', '多标签分组', '其中空体') if shape[k]))
    head = '--- %s %d 处 / %d 个文件' % ('已写盘修复' if apply_mode else '可修',
                                         n_fix, len(file_rows))
    print('%s：%s' % (head, ' '.join('%s=%d' % (k, fixed_shape[k])
                                     for k in ('单标签', '多标签分组', '其中空体')
                                     if fixed_shape[k])))
    for rel, cnt, c in file_rows:
        print('    %s  可修=%d（%s）'
              % (rel, cnt, ' '.join('%s=%d' % (k, c[k]) for k in
                                    ('单标签', '多标签分组', '其中空体') if c[k])))
    print('--- 跳过 %d 处（判定跳过 %d + 安全闸丢弃 %d）'
          % (n_skip, n_pre_skip, n_skip - n_pre_skip))
    for reason in SKIP_REASONS:
        if not skip_cnt[reason]:
            continue
        print('    [%d 处] %s' % (skip_cnt[reason], reason))
        sn = skip_samples.get(reason, [])
        lim = 20 if reason.startswith('体内含变量') else 5
        for rel, ln, snip in sn[:lim]:
            print('        %s:%d  %s' % (rel, ln, snip))
        if len(sn) > lim:
            print('        … 另 %d 条' % (len(sn) - lim))
    for reason in sorted(set(skip_cnt) - set(SKIP_REASONS)):
        print('    [%d 处] %s' % (skip_cnt[reason], reason))
    if assert_skip:
        print('断言未过而未写盘的文件：%d 个' % len(assert_skip))
        for rel, msg in assert_skip[:20]:
            print('    %s  %s' % (rel, msg))

    # ---- SL 报告 ----
    n_sl_skip = sum(sl_skip_cnt.values())
    print('\n=== %s SL：单行 case 展开（标签与整个体同行 → 拆多行 + 补一对 {}）（%s）'
          % (proj, '已写盘' if apply_mode else '干跑，未写盘'))
    print('style_check C1 命中中的单行 case：%d 处 = 可修 %d + 判定跳过 %d + 安全闸丢弃 %d'
          % (n_sl_hit, n_sl_cand, n_sl_pre_skip, n_sl_cand - n_sl_fix))
    if n_sl_hit != n_sl_cand + n_sl_pre_skip:
        print('!! 模型与实际单行 case 命中 %d 不一致' % n_sl_hit)
    print('待展开形态：%s' % ' '.join('%s=%d' % (SL_FORM_DESC[k], sl_shape[k])
                                 for k in SL_FORM_ORDER if sl_shape[k]))
    print('--- %s %d 处 / %d 个文件：%s'
          % ('已写盘展开' if apply_mode else '可展开', n_sl_fix, len(sl_file_rows),
             ' '.join('%s=%d' % (SL_FORM_DESC[k], sl_fixed_shape[k])
                      for k in SL_FORM_ORDER if sl_fixed_shape[k])))
    for rel, cnt, c in sl_file_rows:
        print('    %s  可修=%d（%s）'
              % (rel, cnt, ' '.join('%s=%d' % (SL_FORM_DESC[k], c[k])
                                    for k in SL_FORM_ORDER if c[k])))
    print('--- 跳过 %d 处（判定跳过 %d + 安全闸丢弃 %d）'
          % (n_sl_skip, n_sl_pre_skip, n_sl_cand - n_sl_fix))
    for reason in SL_SKIP_REASONS:
        if not sl_skip_cnt[reason]:
            continue
        sn = sl_skip_samples.get(reason, [])
        lim = 20 if reason.startswith('体内含变量') else 5
        print('    [%d 处] %s' % (sl_skip_cnt[reason], reason))
        for rel, ln, snip in sn[:lim]:
            print('        %s:%d  %s' % (rel, ln, snip))
        if len(sn) > lim:
            print('        … 另 %d 条' % (len(sn) - lim))
    for reason in sorted(set(sl_skip_cnt) - set(SL_SKIP_REASONS)):
        print('    [%d 处] %s' % (sl_skip_cnt[reason], reason))
    if sl_assert_skip:
        print('SL 断言未过而未写盘的文件：%d 个' % len(sl_assert_skip))
        for rel, msg in sl_assert_skip[:20]:
            print('    %s  %s' % (rel, msg))

    # ---- C3 报告 ----
    n_c3_skip = sum(c3_skip_cnt.values())
    print('\n=== %s C3：case 主体大括号内的 break; 外移到 `}` 之后（%s）'
          % (proj, '已写盘' if apply_mode else '干跑，未写盘'))
    print('style_check C3 命中 %d 处 = 可修 %d + 判定跳过 %d + 安全闸丢弃 %d'
          % (n_c3_hit, n_c3_cand, n_c3_pre_skip, n_c3_cand - n_c3_fix))
    if n_c3_hit != n_c3_cand + n_c3_pre_skip:
        print('!! 模型与实际 C3 命中 %d 不一致' % n_c3_hit)
    print('--- %s %d 处 / %d 个文件'
          % ('已写盘修复' if apply_mode else '可修', n_c3_fix, len(c3_file_rows)))
    for rel, cnt in c3_file_rows:
        print('    %s  可修=%d' % (rel, cnt))
    for reason in C3_SKIP_REASONS:
        if not c3_skip_cnt[reason]:
            continue
        sn = c3_skip_samples.get(reason, [])
        print('    [%d 处] %s' % (c3_skip_cnt[reason], reason))
        for rel, ln, snip in sn[:10]:
            print('        %s:%d  %s' % (rel, ln, snip))
        if len(sn) > 10:
            print('        … 另 %d 条' % (len(sn) - 10))
    for reason in sorted(set(c3_skip_cnt) - set(C3_SKIP_REASONS)):
        print('    [%d 处] %s' % (c3_skip_cnt[reason], reason))
    if c3_assert_skip:
        print('C3 断言未过而未写盘的文件：%d 个' % len(c3_assert_skip))
        for rel, msg in c3_assert_skip[:20]:
            print('    %s  %s' % (rel, msg))
    print('SUMMARY %s %s%d 处 跳过=%d 未包裹组=%d 已合规组=%d'
          % (proj, '已修复=' if apply_mode else '可修=', n_fix, n_skip, n_group, n_braced))
    print('SUMMARY-SL %s %s%d 处 跳过=%d 单行命中=%d'
          % (proj, '已修复=' if apply_mode else '可修=', n_sl_fix, n_sl_skip, n_sl_hit))
    print('SUMMARY-C3 %s %s%d 处 跳过=%d 命中=%d'
          % (proj, '已修复=' if apply_mode else '可修=', n_c3_fix, n_c3_skip, n_c3_hit))
    return 0


def main():
    argv = sys.argv[1:]
    apply_mode = '--apply' in argv
    args = [a for a in argv if a != '--apply']
    if len(args) < 2:
        print('用法: python case_brace_fix.py <项目根> <目录列表,逗号分隔> [--apply]',
              file=sys.stderr)
        return 1
    return run(args[0], args[1].split(','), apply_mode)


if __name__ == '__main__':
    sys.exit(main())
