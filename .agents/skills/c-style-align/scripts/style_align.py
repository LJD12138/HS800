#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""C 代码排版对齐（rules 2.2 / 2.3 / 2.4）
   struct/union 成员 -> 名称列 24、注释列 44
   enum 项           -> 注释列 24
   #define          -> 名称列 16、取值列 56、注释列 64（过长顺延到下一个 Tab 位）
只改空白字符；任何非空白差异立即中止。

用法:
  python style_align.py allcheck <项目根> <目录列表>
  python style_align.py allapply <项目根> <目录列表>
  python style_align.py check|apply <项目根> <目录列表> [名称列 注释列]
  python style_align.py enumcheck|enumapply <项目根> <目录列表> <注释列>
  python style_align.py defcheck|defapply <项目根> <目录列表> <名称列> <取值列> <注释列>
  python style_align.py stats|enumstats|definestats|indentstats <项目根> <目录列表>
  python style_align.py dump <项目根> <文件关键字>
排除目录: 环境变量 STRUCT_ALIGN_EXCLUDE=<相对路径,逗号分隔>
"""
import os
import re
import sys

SKIP_DIRS = {'objects', 'listings', 'build', 'rte', 'debugconfig', '.git', '.eide', '.cmsis', '.pack', '.agents', '.trae', '.vscode'}
TAB = 4
TYPE_QUAL = {'const', 'volatile', 'static', 'unsigned', 'signed', 'long', 'short',
             'struct', 'union', 'enum', 'register', 'auto', 'extern', '__IO', '__I', '__O'}


def col_of(ws):
    c = 0
    for ch in ws:
        if ch == '\t':
            c = (c // TAB + 1) * TAB
        else:
            c += 1
    return c


def strip_comment(s):
    """return (code, comment) splitting off first // or /*"""
    i1 = s.find('//')
    i2 = s.find('/*')
    cands = [i for i in (i1, i2) if i >= 0]
    if not cands:
        return s.rstrip(), ''
    idx = min(cands)
    return s[:idx].rstrip(), s[idx:].rstrip()


def split_lines(text):
    """split keeping EOL, without assuming a single kind of newline"""
    out = []
    start = 0
    for m in re.finditer(r'\r\n|\n|\r', text):
        out.append((text[start:m.end()], text[start:m.start()]))
        start = m.end()
    if start < len(text):
        out.append((text[start:], text[start:]))
    return out


def _scan(lines):
    """扫描一次，返回 (kind_map, cid_map):
    kind_map[行] = 行首所处容器类型(struct/union/enum 或 None)
    cid_map[行]  = 该容器的编号(开括号的文本偏移)，同一容器内的行编号相同，用于块内对齐"""
    text = ''.join(l[0] for l in lines)
    n = len(text)
    starts = {}
    off = 0
    for idx, (full, _content) in enumerate(lines):
        starts[off] = idx
        off += len(full)
    state = {}
    cids = {}
    stack = []
    stmt_start = 0            # index in text of start of current statement-ish prefix
    i = 0
    while i < n:
        if i in starts:
            li = starts[i]
            state[li] = stack[-1][0] if stack else None
            cids[li] = stack[-1][1] if stack else None
        ch = text[i]
        if ch == '/' and i + 1 < n and text[i + 1] == '/':
            nl = text.find('\n', i)
            i = n if nl < 0 else nl
            continue
        if ch == '/' and i + 1 < n and text[i + 1] == '*':
            close = text.find('*/', i + 2)
            i = n if close < 0 else close + 2
            continue
        if ch == '"' or ch == "'":
            quote = ch
            i += 1
            while i < n and text[i] != quote:
                if text[i] == '\\':
                    i += 1
                i += 1
            i += 1
            continue
        if ch == '{':
            prefix = text[stmt_start:i]
            kw = re.search(r'\b(struct|union|enum)\b', prefix)
            stack.append((kw.group(1) if kw else None, i))
        elif ch == '}':
            if stack:
                stack.pop()
        elif ch == ';':
            stmt_start = i + 1
        i += 1
    return state, cids


def line_states(lines):
    """每行开始时所处的容器类型（struct/union/enum 或 None）"""
    return _scan(lines)[0]


def line_blocks(lines):
    """每行开始时所处的容器编号（用于块内注释列统一）"""
    return _scan(lines)[1]


def member_parts(content):
    """if content is a struct member declaration return (indent, startcol, type, decl, comment)"""
    m = re.match(r'^([ \t]*)(.*)$', content)
    indent, rest = m.group(1), m.group(2)
    if not rest or rest.startswith('#'):
        return None
    code, comment = strip_comment(rest)
    if not code.endswith(';') or code.endswith('};'):
        return None
    toks = list(re.finditer(r'\S+', code))
    if len(toks) < 2:
        return None
    # multi-token type: keep leading qualifiers together with the base type
    # e.g. "const char *id_str;" -> type "const char", declarator "*id_str;"
    n = 1
    while n < len(toks) and toks[n - 1].group(0) in TYPE_QUAL and n < 3:
        n += 1
    typ = ' '.join(t.group(0) for t in toks[:n])
    decl = code[toks[n].start():]
    return indent, col_of(indent), typ, decl, comment


def walk(root, dirs=None):
    exclude = [e for e in os.environ.get('STRUCT_ALIGN_EXCLUDE', '').split(',') if e]
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [d for d in dirnames if d.lower() not in SKIP_DIRS]
        rel = os.path.relpath(dirpath, root).replace('\\', '/')
        if any(rel == e or rel.startswith(e + '/') for e in exclude):
            dirnames[:] = []
            continue
        if dirs is not None and rel != '.':
            keep = False
            for d in dirs:
                if rel == d or rel.startswith(d + '/') or d.startswith(rel + '/'):
                    keep = True
                    break
            if not keep:
                continue
        for fn in sorted(filenames):
            if fn.lower().endswith(('.c', '.h')):
                yield os.path.join(dirpath, fn)


def load(path):
    with open(path, 'rb') as f:
        data = f.read()
    bom = data.startswith(b'\xef\xbb\xbf')
    try:
        text = data.decode('utf-8-sig')
    except UnicodeDecodeError:
        sys.stderr.write('SKIP (not utf-8): %s\n' % path)
        return None
    return bom, text, split_lines(text)


def stats(root, dirs=None):
    hist = {}
    for path in walk(root, dirs):
        loaded = load(path)
        if loaded is None:
            continue
        bom, text, lines = loaded
        st = line_states(lines)
        for idx, (full, content) in enumerate(lines):
            if st.get(idx) not in ('struct', 'union'):
                continue
            p = member_parts(content)
            if not p:
                continue
            indent, scol, typ, decl, comment = p
            name_col = col_of(content[:content.index(decl)])
            ccol = col_of(content[:content.index(comment)]) if comment else -1
            hist.setdefault((scol, name_col, ccol), []).append((os.path.relpath(path, root), idx + 1))
    for key in sorted(hist):
        v = hist[key]
        print('indent=%2d name=%2d comment=%3d  n=%3d  e.g. %s:%d' % (key[0], key[1], key[2], len(v), v[0][0], v[0][1]))


def enum_item(content):
    """if content is an enum constant line return (indent, code, comment)"""
    m = re.match(r'^([ \t]*)(.*)$', content)
    indent, rest = m.group(1), m.group(2)
    if not rest or rest.startswith('#'):
        return None
    code, comment = strip_comment(rest)
    if not re.match(r'^[A-Za-z_]\w*(\s*=\s*[^;]+?)?\s*,?$', code):
        return None
    return indent, code, comment


def norm_indent(indent):
    """leading indent must be tabs (rules: 1Tab per level); keep as-is when not 4-column aligned"""
    c = col_of(indent)
    if c % TAB == 0:
        return '\t' * (c // TAB)
    return indent


def pad_to(cur, target, min_tab=False):
    """tab padding reaching `target` column exactly (+1 tab when already past).
    min_tab=True guarantees at least one tab, i.e. never glue two tokens together."""
    pad = ''
    while cur < target:
        pad += '\t'
        cur = (cur // TAB + 1) * TAB
    if cur > target or (min_tab and not pad):
        pad += '\t'
        cur = (cur // TAB + 1) * TAB
    return pad, cur


def comment_target(code_end, cmt_abs):
    """单行需要的注释列：基准列；被代码占满时顺延到下一个 Tab 位"""
    col = cmt_abs
    while col <= code_end:
        col = (col // TAB + 1) * TAB
    return col


def rebuild_item(content, kind, item_abs=24, cmt_abs=44, block_col=None):
    """按规则重建一条结构体成员 / 枚举项行。
    返回 (新内容, 代码结束列, 是否有注释)；不是成员/枚举项时返回 None。
    block_col 非空时用该列（块内统一），否则按单行基准列 + 顺延。"""
    if kind == 'enum':
        it = enum_item(content)
        if not it:
            return None
        indent, code, comment = it
        # rules 2.3：常量与赋值的内部空白压成单空格（去掉 = 号竖排对齐）
        tight = re.sub(r'\s+', ' ', code).strip()
        body = norm_indent(indent) + tight
        code_end = col_of(indent + tight)
    else:
        p = member_parts(content)
        if not p:
            return None
        indent, scol, typ, decl, comment = p
        pad, _cur = pad_to(scol + len(typ), item_abs, min_tab=True)
        body = norm_indent(indent) + typ + pad + decl
        code_end = col_of(indent + typ + pad) + len(decl)
    if not comment:
        return body, code_end, False
    col = block_col if block_col else comment_target(code_end, cmt_abs)
    pad2, _cur = pad_to(code_end, col, min_tab=True)
    return body + pad2 + comment, code_end, True


def realign(root, apply=False, dirs=None, kind='struct', item_abs=24, cmt_abs=44):
    if kind == 'enum':
        in_kind = lambda v: v == 'enum'
    else:
        in_kind = lambda v: v in ('struct', 'union')
    report = []
    for path in walk(root, dirs):
        loaded = load(path)
        if loaded is None:
            continue
        bom, text, lines = loaded
        st, blocks = _scan(lines)
        # 预扫：块内注释列统一（基准列或基准列+1 个 Tab；需要 ≥2 个 Tab 的块保持逐行顺延）
        block_cols = {}
        req = {}
        for idx, (full, content) in enumerate(lines):
            if not in_kind(st.get(idx)):
                continue
            r = rebuild_item(content, kind, item_abs, cmt_abs)
            if r is None or not r[2]:
                continue
            cid = blocks.get(idx)
            req[cid] = max(req.get(cid, cmt_abs), comment_target(r[1], cmt_abs))
        for cid, col in req.items():
            block_cols[cid] = col if col <= cmt_abs + TAB else None
        changed = 0
        out = []
        for idx, (full, content) in enumerate(lines):
            eol = full[len(content):]
            new_content = content
            if in_kind(st.get(idx)) and content.lstrip().startswith('}'):
                # closing brace line: "}Type_T;" without padding and without trailing blanks
                m = re.match(r'^([ \t]*)\}[ \t]*(\w*)[ \t]*(;[ \t]*)(.*)$', content)
                if m:
                    tail = m.group(4).rstrip()
                    new_content = norm_indent(m.group(1)) + '}' + m.group(2) + ';'
                    if tail:
                        new_content += ' ' + tail
                    full_new = new_content + eol
                    if full_new != full:
                        if re.sub(r'[ \t]', '', full_new) != re.sub(r'[ \t]', '', full):
                            raise SystemExit('NON-WHITESPACE CHANGE %s:%d\n%r\n%r' % (path, idx + 1, full, full_new))
                        changed += 1
                    out.append((full_new, new_content))
                    continue
            if in_kind(st.get(idx)):
                r = rebuild_item(content, kind, item_abs, cmt_abs,
                                 block_col=block_cols.get(blocks.get(idx)))
                if r is not None:
                    new_content = r[0]
            full_new = new_content + eol
            if full_new != full:
                if re.sub(r'[ \t]', '', full_new) != re.sub(r'[ \t]', '', full):
                    raise SystemExit('NON-WHITESPACE CHANGE %s:%d\n%r\n%r' % (path, idx + 1, full, full_new))
                changed += 1
            out.append((full_new, new_content))
        if changed:
            report.append((os.path.relpath(path, root), changed))
            if apply:
                new_text = ''.join(l[0] for l in out)
                data = new_text.encode('utf-8')
                if bom:
                    data = b'\xef\xbb\xbf' + data
                with open(path, 'wb') as f:
                    f.write(data)
    for p, c in report:
        print('%-72s %5d lines' % (p, c))
    print('--- files=%d lines=%d' % (len(report), sum(c for _p, c in report)))


def indent_stats(root, dirs=None):
    """count items whose leading indent contains spaces (not pure tabs)"""
    res = {'struct': [], 'enum': []}
    for path in walk(root, dirs):
        loaded = load(path)
        if loaded is None:
            continue
        _bom, _text, lines = loaded
        st = line_states(lines)
        for idx, (full, content) in enumerate(lines):
            kind = st.get(idx)
            p = member_parts(content) if kind in ('struct', 'union') else (enum_item(content) if kind == 'enum' else None)
            if not p:
                continue
            indent = p[0]
            if ' ' in indent:
                key = 'enum' if kind == 'enum' else 'struct'
                res[key].append((os.path.relpath(path, root), idx + 1, content))
    for key in res:
        print('%s: %d 行使用空格缩进' % (key, len(res[key])))
        for rel, ln, content in res[key][:15]:
            print('   %s:%d  %r' % (rel, ln, content[:70]))


def in_block_comment_lines(lines):
    """line indexes that start inside a /* */ comment"""
    inside = False
    res = set()
    for idx, (full, content) in enumerate(lines):
        if inside:
            res.add(idx)
        i = 0
        n = len(content)
        while i < n:
            if not inside and content.startswith('//', i):
                break
            if not inside and content.startswith('/*', i):
                inside = True
                i += 2
                continue
            if inside and content.startswith('*/', i):
                inside = False
                i += 2
                continue
            i += 1
    return res


def split_define_head(s):
    """split '#define' head into (macro_name_unit, rest).
    Function-like macro: name unit is IDENT(params) with the parameter list kept
    together (internal whitespace normalised), so parameters never get torn apart."""
    mm = re.match(r'^([A-Za-z_]\w*)\(', s)
    if not mm:
        tok = re.match(r'^[ \t]*(\S+)', s)
        return tok.group(1), s[tok.end():]
    ident = mm.group(1)
    i = mm.end() - 1
    depth = 0
    j = i
    while j < len(s):
        if s[j] == '(':
            depth += 1
        elif s[j] == ')':
            depth -= 1
            if depth == 0:
                j += 1
                break
        j += 1
    params = s[i + 1:j - 1] if j > i else ''
    params = re.sub(r'\s*,\s*', ', ', re.sub(r'\s+', ' ', params)).strip()
    return ident + '(' + params + ')', s[j:]


def realign_defines(root, apply=False, dirs=None, name_abs=16, val_abs=56, cmt_abs=64):
    """rules 2.4: #define flush left, macro name at 4Tab, value at 10Tab, comment at 64.
    Only lines whose columns are wrong get rebuilt (tab padded); others stay untouched."""
    rx = re.compile(r'^([ \t]*)#[ \t]*define([ \t]+)(.*)$')
    report = []
    skipped = []
    stat = {'total': 0, 'indented': 0, 'no_value_with_comment': 0, 'no_value_skipped': 0}
    for path in walk(root, dirs):
        loaded = load(path)
        if loaded is None:
            continue
        bom, _text, lines = loaded
        if 'Use Configuration Wizard' in _text:
            skipped.append(os.path.relpath(path, root))
            continue
        cmt_lines = in_block_comment_lines(lines)
        changed = 0
        out = []
        for idx, (full, content) in enumerate(lines):
            eol = full[len(content):]
            new_content = content
            if idx not in cmt_lines:
                m = rx.match(content)
                if m:
                    stat['total'] += 1
                    if m.group(1):
                        stat['indented'] += 1
                    name, tail = split_define_head(m.group(3))
                    head_start = m.start(3)
                    code, comment = strip_comment(tail)
                    value = code.strip()
                    if not value:
                        # 无值宏（头文件保护等）保持原样，与 APP 工程一致：#define XXX_H
                        stat['no_value_skipped'] += 1
                        out.append((full, content))
                        continue
                    lstrip_tail = tail.lstrip()
                    pad_len = len(tail) - len(lstrip_tail)
                    name_col = col_of(content[:head_start])
                    vpos = head_start + len(name) + pad_len
                    val_col = col_of(content[:vpos]) if value else None
                    cmt_col = col_of(content[:vpos + lstrip_tail.index(comment)]) if comment else None
                    need = (name_col != name_abs
                            or (val_col is not None and val_col != val_abs)
                            or (cmt_col is not None and cmt_col != cmt_abs)
                            or '\t' in name or '  ' in name)
                    if need:
                        body = '#define'
                        pad1, _c = pad_to(col_of('#define'), name_abs, min_tab=True)
                        body += pad1 + name
                        if value:
                            pad2, _c = pad_to(col_of(body), val_abs, min_tab=True)
                            body += pad2 + value
                        if comment:
                            if not value:
                                stat['no_value_with_comment'] += 1
                            pad3, _c = pad_to(col_of(body), cmt_abs, min_tab=True)
                            body += pad3 + comment
                        new_content = body
            full_new = new_content + eol
            if full_new != full:
                if re.sub(r'[ \t]', '', full_new) != re.sub(r'[ \t]', '', full):
                    raise SystemExit('NON-WHITESPACE CHANGE %s:%d\n%r\n%r' % (path, idx + 1, full, full_new))
                changed += 1
            out.append((full_new, new_content))
        if changed:
            report.append((os.path.relpath(path, root), changed))
            if apply:
                new_text = ''.join(l[0] for l in out)
                data = new_text.encode('utf-8')
                if bom:
                    data = b'\xef\xbb\xbf' + data
                with open(path, 'wb') as f:
                    f.write(data)
    for p, c in report:
        print('%-72s %5d lines' % (p, c))
    print('--- files=%d lines=%d  (宏总数=%d, 顶格外的缩进宏=%d, 无值宏保持原样=%d)' % (
        len(report), sum(c for _p, c in report), stat['total'], stat['indented'],
        stat['no_value_skipped']))
    for s in skipped:
        print('--- 跳过(Keil 配置向导文件)=%s' % s)


def define_parts(content):
    """if content is a #define line return (indent, macro_name, value, comment)"""
    m = re.match(r'^([ \t]*)#[ \t]*define([ \t]+)(\S+)[ \t]*(.*)$', content)
    if not m:
        return None
    indent, name, tail = m.group(1), m.group(3), m.group(4)
    code, comment = strip_comment(tail)
    return indent, name, code.strip(), comment


def define_stats(root, dirs=None):
    """column histogram of #define lines (indent, name col, value col, comment col)"""
    hist = {}
    rx = re.compile(r'^([ \t]*)#[ \t]*define([ \t]+)(\S+)[ \t]*(.*)$')
    for path in walk(root, dirs):
        loaded = load(path)
        if loaded is None:
            continue
        _bom, _text, lines = loaded
        for idx, (full, content) in enumerate(lines):
            m = rx.match(content)
            if not m:
                continue
            indent, tail = m.group(1), m.group(4)
            ncol = col_of(content[:m.start(3)])
            stripped = tail.lstrip()
            if not stripped:
                hist.setdefault((col_of(indent), ncol, -1, -1), []).append(
                    (os.path.relpath(path, root), idx + 1))
                continue
            vstart = m.start(4) + (len(tail) - len(stripped))
            code, comment = strip_comment(stripped)
            value = code.strip()
            vcol = col_of(content[:vstart]) if value else -1
            ccol = col_of(content[:vstart + stripped.index(comment)]) if comment else -1
            hist.setdefault((col_of(indent), ncol, vcol, ccol), []).append(
                (os.path.relpath(path, root), idx + 1))
    for key in sorted(hist):
        v = hist[key]
        print('indent=%2d name=%2d value=%3d comment=%3d  n=%4d  e.g. %s:%d' % (
            key[0], key[1], key[2], key[3], len(v), v[0][0], v[0][1]))


def enum_stats(root, dirs=None):
    """column histogram of enum constant lines (indent, end-of-item col, comment col)"""
    hist = {}
    for path in walk(root, dirs):
        loaded = load(path)
        if loaded is None:
            continue
        _bom, _text, lines = loaded
        st = line_states(lines)
        for idx, (full, content) in enumerate(lines):
            if st.get(idx) != 'enum':
                continue
            it = enum_item(content)
            if not it:
                continue
            indent, code, comment = it
            ccol = col_of(content[:content.index(comment)]) if comment else -1
            hist.setdefault((col_of(indent), col_of(indent + code), ccol), []).append(
                (os.path.relpath(path, root), idx + 1))
    for key in sorted(hist):
        v = hist[key]
        print('indent=%2d item_end=%2d comment=%3d  n=%3d  e.g. %s:%d' % (
            key[0], key[1], key[2], len(v), v[0][0], v[0][1]))


if __name__ == '__main__':
    mode, root = sys.argv[1], sys.argv[2]
    dirs = sys.argv[3].split(',') if len(sys.argv) > 3 else None
    if mode == 'stats':
        stats(root, dirs)
    elif mode == 'enumstats':
        enum_stats(root, dirs)
    elif mode == 'indentstats':
        indent_stats(root, dirs)
    elif mode == 'definestats':
        define_stats(root, dirs)
    elif mode == 'dump':
        pat = sys.argv[3]
        for path in walk(root, None):
            if pat not in path:
                continue
            loaded = load(path)
            if loaded is None:
                continue
            _b, _t, lines = loaded
            st = line_states(lines)
            for idx, (full, content) in enumerate(lines):
                p = member_parts(content)
                print('%4d in=%-8s %s' % (idx + 1, st.get(idx), content))
    elif mode == 'check':
        realign(root, apply=False, dirs=dirs)
    elif mode == 'apply':
        realign(root, apply=True, dirs=dirs)
    elif mode == 'enumcheck':
        realign(root, apply=False, dirs=dirs, kind='enum', cmt_abs=int(sys.argv[4]))
    elif mode == 'enumapply':
        realign(root, apply=True, dirs=dirs, kind='enum', cmt_abs=int(sys.argv[4]))
    elif mode in ('defcheck', 'defapply'):
        kw = dict(name_abs=int(sys.argv[4]), val_abs=int(sys.argv[5]), cmt_abs=int(sys.argv[6]))
        realign_defines(root, apply=(mode == 'defapply'), dirs=dirs, **kw)
    elif mode in ('allcheck', 'allapply'):
        do_apply = mode == 'allapply'
        print('===== 1/3 结构体/联合体 (名称列24 / 注释列44) =====')
        realign(root, apply=do_apply, dirs=dirs, kind='struct', item_abs=24, cmt_abs=44)
        print('===== 2/3 枚举 (注释列24) =====')
        realign(root, apply=do_apply, dirs=dirs, kind='enum', cmt_abs=24)
        print('===== 3/3 宏定义 (名称列16 / 取值列56 / 注释列64) =====')
        realign_defines(root, apply=do_apply, dirs=dirs, name_abs=16, val_abs=56, cmt_abs=64)
    else:
        raise SystemExit('mode: allcheck|allapply|check|apply|enumcheck|enumapply|defcheck|defapply|stats|enumstats|definestats|indentstats|dump')
