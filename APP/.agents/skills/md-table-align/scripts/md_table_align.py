#!/usr/bin/env python3
"""Markdown 表格排版工具：列宽对齐（空格填充 / Tab 对齐），表格内容零改动。

用法：
    python md_table_align.py check <文件|目录> [...] [--mode spaces|tabs] [--ambiguous-width 2]
    python md_table_align.py apply <文件|目录> [...] [--mode spaces|tabs] [--tab-size 4] [--pad-last|--no-pad-last]

    check  只检查表格是否需要重排，不写文件（退出码 1 = 有表格待重排或跳过）
    apply  重排表格空白后写回；写回前逐格校验内容未变，不一致则整表跳过

约定：
    - 只改表格行的空白；默认 spaces（比例字体下 Tab 无法保证对齐）
    - 默认末列补齐（矩形对齐）；表格原本紧凑时沿用紧凑，可用 --pad-last/--no-pad-last 强制
    - 保留行首缩进与引用前缀（> | ...），缩进达到 4 列的行按缩进代码块处理，不动
    - 围栏代码块（``` / ~~~）内的 | 行不处理
    - 换行符（LF/CRLF）、UTF-8 BOM 原样保留
"""

import argparse
import re
import sys
import unicodedata
from pathlib import Path

PREFIX_RE = re.compile(r"^(?P<prefix>(?:[ \t]*>[ \t]?)*[ \t]*)(?P<content>\|.*)$")
SEP_CELL_RE = re.compile(r"^:?-+:?$")
FENCE_RE = re.compile(r"^[ ]{0,3}(?P<marker>`{3,}|~{3,})")
QUOTE_RE = re.compile(r"^[ \t]{0,3}>[ \t]?")


def char_width(ch, ambiguous):
    if unicodedata.combining(ch) or ch in "\u200b\u200c\u200d\ufeff":
        return 0
    ea = unicodedata.east_asian_width(ch)
    if ea in ("W", "F"):
        return 2
    if ea == "A":
        return ambiguous
    return 1


def disp_width(text, ambiguous):
    return sum(char_width(ch, ambiguous) for ch in text)


def text_width(text, base, ambiguous, tab_size):
    """从 base 列开始，按 Tab 展开规则计算文本结束列。"""
    col = base
    for ch in text:
        if ch == "\t":
            col = (col // tab_size + 1) * tab_size
        else:
            col += char_width(ch, ambiguous)
    return col


def strip_quotes(line):
    s = line
    while True:
        m = QUOTE_RE.match(s)
        if not m:
            return s
        s = s[m.end():]


def parse_table_line(line, tab_size, ambiguous):
    """返回 (前缀, 以 | 开头的内容)；不是表格行或缩进像代码块时返回 None。"""
    m = PREFIX_RE.match(line)
    if not m:
        return None
    prefix, content = m.group("prefix"), m.group("content")
    lead = prefix[:len(prefix) - len(prefix.lstrip(" \t"))]
    tail = prefix[len(prefix.rstrip(" \t")):]
    if (text_width(lead, 0, ambiguous, tab_size) > 3
            or text_width(tail, 0, ambiguous, tab_size) > 3):
        return None
    return prefix, content


def is_escaped(s, idx):
    n, j = 0, idx - 1
    while j >= 0 and s[j] == "\\":
        n += 1
        j -= 1
    return n % 2 == 1


def split_row(content):
    s = content.strip()
    if s.startswith("|"):
        s = s[1:]
    if s.endswith("|") and not is_escaped(s, len(s) - 1):
        s = s[:-1]
    cells, buf = [], []
    for idx, ch in enumerate(s):
        if ch == "|" and not is_escaped(s, idx):
            cells.append("".join(buf).strip(" \t"))
            buf = []
        else:
            buf.append(ch)
    cells.append("".join(buf).strip(" \t"))
    return cells


def is_separator(cells):
    return bool(cells) and all(SEP_CELL_RE.match(c) for c in cells)


def align_of(cell):
    c = cell.strip(" \t")
    left, right = c.startswith(":"), c.endswith(":")
    if left and right:
        return "center"
    if right:
        return "right"
    if left:
        return "left"
    return None


def find_tables(lines, tab_size, ambiguous):
    """返回 [(起始行索引, 前缀, 原始行列表, 单元格列表)]，跳过代码块。"""
    tables = []
    in_fence, fence_char, fence_len = False, "", 0
    i = 0
    while i < len(lines):
        stripped = strip_quotes(lines[i])
        fm = FENCE_RE.match(stripped)
        if fm:
            marker = fm.group("marker")
            if not in_fence:
                in_fence, fence_char, fence_len = True, marker[0], len(marker)
            elif marker[0] == fence_char and len(marker) >= fence_len:
                in_fence = False
            i += 1
            continue
        if in_fence:
            i += 1
            continue
        parsed = parse_table_line(lines[i], tab_size, ambiguous)
        if parsed is None:
            i += 1
            continue
        start, prefix = i, parsed[0]
        block, rows = [], []
        while i < len(lines):
            p = parse_table_line(lines[i], tab_size, ambiguous)
            if p is None or p[0] != prefix:
                break
            block.append(lines[i])
            rows.append(split_row(p[1]))
            i += 1
        if len(block) >= 2 and is_separator(rows[1]):
            tables.append((start, prefix, block, rows))
    return tables


def plan_widths(rows, mode, tab_size, ambiguous, base, pad_last):
    """返回每列用于渲染的宽度（tabs 模式为到制表位的距离）。"""
    cols = len(rows[0])
    content_w = [0] * cols
    for row in rows:
        if is_separator(row):
            continue
        for idx, cell in enumerate(row):
            content_w[idx] = max(content_w[idx], disp_width(cell, ambiguous))
    widths = [max(3, w) for w in content_w]
    last = cols - 1
    if not pad_last:
        widths[last] = max(1, content_w[last])
    if mode == "spaces":
        return widths
    start = base + 2  # "| " 之后内容起始列
    for idx in range(cols):
        if idx == last and not pad_last:
            break
        w = widths[idx] + 1
        while (start + w) % tab_size:
            w += 1
        widths[idx] = w
        start += w + 2
    return widths


def tabs_needed(start_col, content_w, target_col, tab_size):
    pos, count = start_col + content_w, 0
    while pos < target_col:
        pos = (pos // tab_size + 1) * tab_size
        count += 1
    return count


def sep_segment(width, align, trailing_space):
    core = max(1, width - 1) if trailing_space else width
    if align == "center" and core >= 3:
        body = ":" + "-" * (core - 2) + ":"
    elif align == "left" and core >= 2:
        body = ":" + "-" * (core - 1)
    elif align == "right" and core >= 2:
        body = "-" * (core - 1) + ":"
    else:
        body = "-" * core
    return body + (" " if trailing_space else "")


def infer_pad_last(block):
    """沿用表格现状：行尾有多余空白/Tab，或分隔行末格多于 3 个破折号，视为补齐。"""
    for line in block:
        s = line.rstrip()
        if not s.endswith("|") or s.endswith("\\|"):
            continue
        before = s[:-1]
        run = before[len(before.rstrip(" \t")):]
        if "\t" in run or len(run) > 1:
            return True
    for line in block:
        parsed = parse_table_line(line, 4, 1)
        if parsed is None:
            continue
        cells = split_row(parsed[1])
        if is_separator(cells):
            return len(cells[-1].replace(":", "")) > 3
    return False


def render_table(prefix, block, rows, mode, tab_size, ambiguous, pad_last):
    if pad_last is None:
        pad_last = infer_pad_last(block)
    cols = len(rows[0])
    aligns = [align_of(c) for c in rows[1]]
    base = text_width(prefix, 0, ambiguous, tab_size)
    widths = plan_widths(rows, mode, tab_size, ambiguous, base, pad_last)
    last = cols - 1
    out = []
    for row in rows:
        if is_separator(row):
            segs = []
            for idx in range(cols):
                if mode == "spaces":
                    w = 3 if (idx == last and not pad_last) else widths[idx]
                    segs.append(sep_segment(w, aligns[idx], False))
                elif idx == last and not pad_last:
                    segs.append(sep_segment(4, aligns[idx], True))
                else:
                    segs.append(sep_segment(widths[idx], aligns[idx], True))
            sep = " | ".join(segs) if mode == "spaces" else "| ".join(segs)
            out.append(prefix + "| " + sep + (" |" if mode == "spaces" else "|"))
            continue
        if mode == "tabs":
            parts, start = [], base + 2
            for idx, cell in enumerate(row):
                if idx == last and not pad_last:
                    parts.append(cell)
                    break
                target = start + widths[idx]
                parts.append(cell + "\t" * tabs_needed(
                    start, disp_width(cell, ambiguous), target, tab_size))
                start = target + 2
            out.append(prefix + "| " + "| ".join(parts)
                       + ("|" if pad_last else " |"))
        else:
            parts = []
            for idx, cell in enumerate(row):
                if idx == last and not pad_last:
                    parts.append(cell)
                    continue
                pad = widths[idx] - disp_width(cell, ambiguous)
                if aligns[idx] == "right":
                    parts.append(" " * pad + cell)
                elif aligns[idx] == "center":
                    left = pad // 2
                    parts.append(" " * left + cell + " " * (pad - left))
                else:
                    parts.append(cell + " " * pad)
            out.append(prefix + "| " + " | ".join(parts) + " |")
    return out


def verify_render(rows, rendered, prefix):
    """逐格比对：除分隔行空白外，内容必须完全一致。"""
    if len(rows) != len(rendered):
        return False
    for old, line in zip(rows, rendered):
        if not line.startswith(prefix):
            return False
        new = split_row(line[len(prefix):])
        if is_separator(old) or is_separator(new):
            if not (is_separator(old) and is_separator(new)):
                return False
            if [align_of(c) for c in old] != [align_of(c) for c in new]:
                return False
            continue
        if [c.strip(" \t") for c in old] != [c.strip(" \t") for c in new]:
            return False
    return True


def process(text, mode, tab_size, ambiguous, pad_last, apply_changes):
    newline = "\r\n" if "\r\n" in text else "\n"
    lines = text.replace("\r\n", "\n").split("\n")
    report, changed = [], 0
    for start, prefix, block, rows in reversed(find_tables(lines, tab_size, ambiguous)):
        cols = len(rows[0])
        if not all(len(r) == cols for r in rows):
            counts = sorted({len(r) for r in rows})
            report.append((start + 1, "SKIP", f"列数不一致 {counts}"))
            continue
        rendered = render_table(prefix, block, rows, mode, tab_size, ambiguous, pad_last)
        if not verify_render(rows, rendered, prefix):
            report.append((start + 1, "SKIP", "内容校验失败，保持原样"))
            continue
        if rendered == block:
            report.append((start + 1, "OK", "已对齐"))
            continue
        changed += 1
        report.append((start + 1, "TABLE", "已重排" if apply_changes else "需要重排"))
        if apply_changes:
            lines[start:start + len(block)] = rendered
    return newline.join(lines), report, changed


def iter_files(paths):
    for raw in paths:
        p = Path(raw)
        if p.is_dir():
            found = sorted(x for x in p.glob("*.md") if x.is_file())
            if not found:
                print(f"{p}: 目录下没有 .md 文件", file=sys.stderr)
            yield from found
        else:
            yield p


def main():
    parser = argparse.ArgumentParser(description="Markdown 表格排版（内容零改动）")
    parser.add_argument("action", choices=["check", "apply"])
    parser.add_argument("paths", nargs="+", help="文件或目录（目录只处理顶层 *.md）")
    parser.add_argument("--mode", choices=["tabs", "spaces"], default="spaces",
                        help="默认 spaces：本项目编辑器用比例字体（黑体），Tab 宽度不可控")
    parser.add_argument("--tab-size", type=int, default=4)
    parser.add_argument("--ambiguous-width", type=int, choices=[1, 2], default=2,
                        help="东亚歧义宽度字符（§ ℃ → “” 等）按几列算，默认 2"
                             "（黑体等中文字体下这些字符实际为全角；西文字体可改 1）")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--pad-last", dest="pad_last", action="store_true",
                       help="强制补齐最后一列（矩形对齐）")
    group.add_argument("--no-pad-last", dest="pad_last", action="store_false",
                       help="强制不补最后一列（行更短）")
    parser.set_defaults(pad_last=None)
    args = parser.parse_args()

    exit_code, io_errors = 0, 0
    pad_txt = "auto" if args.pad_last is None else ("pad" if args.pad_last else "off")
    for path in iter_files(args.paths):
        try:
            raw = path.read_bytes()
        except OSError as exc:
            print(f"{path}: 读取失败：{exc}", file=sys.stderr)
            io_errors += 1
            continue
        bom = raw.startswith(b"\xef\xbb\xbf")
        try:
            text = raw.decode("utf-8-sig")
        except UnicodeDecodeError:
            print(f"{path}: SKIP 非 UTF-8 文件")
            exit_code = 1
            continue
        out, report, changed = process(
            text, args.mode, args.tab_size, args.ambiguous_width,
            args.pad_last, apply_changes=(args.action == "apply"))
        skipped = [r for r in report if r[1] == "SKIP"]
        total = len(report)
        if args.action == "apply":
            if out != text:
                encoded = out.encode("utf-8")
                if bom:
                    encoded = b"\xef\xbb\xbf" + encoded
                try:
                    path.write_bytes(encoded)
                except OSError as exc:
                    print(f"{path}: 写入失败：{exc}", file=sys.stderr)
                    io_errors += 1
                    continue
            print(f"{path}: 表 {total} 张，改写 {changed} 张，跳过 {len(skipped)} 张"
                  f"（mode={args.mode}, tab={args.tab_size}, pad_last={pad_txt}）")
            if skipped:
                exit_code = 1
        else:
            print(f"{path}: 表 {total} 张，待重排 {changed} 张，跳过 {len(skipped)} 张")
            if changed or skipped:
                exit_code = 1
        for line_no, kind, msg in skipped:
            print(f"  line {line_no}: {kind} {msg}")
    return 2 if io_errors else exit_code


if __name__ == "__main__":
    sys.exit(main())
