#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把函数头注释块向 c-code-templates 3.1 标准块归一：
  1) 旧式块（-----函数功能 …）就地重建为标准块，注释内容原样保留；
  2) 已是标准字段格式、但首尾横幅宽度与 rules 不符的块，只替换首尾两行；
其余行字节不变（内置断言）。缺失字段的内容需人工撰写，脚本不猜。
用法: funcblock_norm.py <root> <dirs> [--report] [--apply]"""
import os
import re
import sys
import unicodedata

RULES = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      '..', '..', '..', 'rules', 'c-code-templates.md'))
SKIP_DIRS = {'.git', '.cmsis', '.pack', 'RTE', 'build', 'DebugConfig', 'Objects', 'Listings',
             'eez_ui', 'eez_project', 'lvgl', 'FreeRTOS', 'EasyFlash', 'EasyLogger',
             'CmBacktrace', 'lwrb', 'LightweightRingBuffer', 'SeggerRtt', 'MultiFuncKey', 'Firmware'}
SKIP_LOWER = {x.lower() for x in SKIP_DIRS}   # 目录名大小写不敏感
EXEMPT = {'gd32f50x_it.c', 'gd32f50x_it.h', 'gd32f50x_libopt.h',
          'FreeRTOSConfig.h', 'board_config.h', 'board_config.c'}

FUNC_RE = re.compile(r'^(static\s+)?[A-Za-z_][\w \t\*]*?[\w\*]\s*\([^;{]*$')
CMT_RE = re.compile(r'^\s*(/\*|\*|//|-{2,}|#\s*(?:if|ifdef|ifndef|else|elif|endif)\b)')
STD_FIELD_RE = re.compile(r'^ \* (函数功能|说明\(备注\)|传入参数|输出参数|返回值)\s*:')
OLD_FIELD_RE = re.compile(r'^-{2,}\s*(\S+)(?:[\t ]+(.*))?$')
OPEN_RE = re.compile(r'^\s*/\*{10,}$')
CLOSE_RE = re.compile(r'^\s*\*{10,}/\s*$')
FIELDS = ['函数功能', '说明(备注)', '传入参数', '输出参数', '返回值']
CONT = ' *' + ' ' * 15


def canon_label(lab):
    lab = lab.strip()
    for key in FIELDS:
        if lab.startswith(key[:2]) and (key in lab or lab in key):
            return key
    for key, aliases in (('说明(备注)', ('说明', '备注')),
                         ('传入参数', ('输入参数', '入参')),
                         ('输出参数', ('出参',)),
                         ('返回值', ('返回',)),
                         ('函数功能', ('功能',))):
        if any(a in lab for a in aliases):
            return key
    return lab


def disp_w(s):
    """显示宽度（中日韩全角按 2 列）"""
    return sum(2 if unicodedata.east_asian_width(c) in 'WF' else 1 for c in s)


def indent_w(s):
    """缩进宽度（Tab 按 4 列展开）"""
    lead = s[:len(s) - len(s.lstrip())]
    return len(lead.expandtabs(4))


def attached_comments(lines, i):
    """紧贴第 i 行上方的连续注释区（允许夹空行、条件编译行、旧式块的缩进续行）"""
    j = i - 1
    while j >= 0:
        s = lines[j]
        if CMT_RE.match(s) or s.strip() == '':
            j -= 1
            continue
        if indent_w(s) >= 8:                     # 旧式注释块的多行续行（缩进对齐）
            k = j - 1
            while k >= 0 and (lines[k].strip() == ''
                              or (not CMT_RE.match(lines[k]) and indent_w(lines[k]) >= 8)):
                k -= 1
            if k >= 0 and CMT_RE.match(lines[k]):
                j -= 1
                continue
        break
    return j + 1, lines[j + 1:i]


def read_rules():
    """3.1 函数头注释块的首尾横幅（与规则同源）"""
    with open(RULES, encoding='utf-8') as f:
        txt = f.read()
    m = re.search(r'```c\n(/\*{10,}\n \* 函数功能.*?\n \*{10,}/)\n```', txt, re.S)
    blk = m.group(1).split('\n')
    return blk[0], blk[-1]


def read(path):
    with open(path, 'rb') as f:
        raw = f.read()
    bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig')
    eol = '\r\n' if '\r\n' in text else ('\r' if '\r' in text else '\n')
    return bom, eol, text.replace('\r\n', '\n').replace('\r', '\n').split('\n')


def write(path, bom, eol, lines):
    with open(path, 'wb') as f:
        f.write(eol.join(lines).encode('utf-8-sig' if bom else 'utf-8'))


def parse_old(reg):
    """旧式块 -> [(字段, [内容行…])]；非旧式返回 None"""
    items = []
    for s in reg:
        t = s.strip()
        if not t or OPEN_RE.match(s) or CLOSE_RE.match(s):
            continue
        m = OLD_FIELD_RE.match(t)
        if m:
            items.append([canon_label(m.group(1)), [(m.group(2) or '').strip()]])
            continue
        if not items:                        # 块内说明性文字，挂到 说明(备注)
            items.append(['说明(备注)', [t]])
            continue
        items[-1][1].append(t)
    if not items:
        return None
    return [('函数功能', v) if k == '函数功能' else (k, v) for k, v in items]


def build_block(items, open_ban, close_ban):
    out = [open_ban]
    for lab, vals in items:
        pad = ' ' * max(1, 12 - disp_w(lab))          # 冒号对齐到第 12 显示列
        out.append(' * %s%s: %s' % (lab, pad, vals[0]))
        out.extend(CONT + v for v in vals[1:] if v)
    out.append(close_ban)
    return out


def std_block_span(reg):
    """标准块（含 3.1 字段）的首尾横幅下标；找不到或跨块返回 (None, None)"""
    fi = [j for j, s in enumerate(reg) if STD_FIELD_RE.match(s)]
    if not fi:
        return None, None
    oi = next((j for j in range(fi[0] - 1, -1, -1) if OPEN_RE.match(reg[j])), None)
    ci = next((j for j in range(fi[-1] + 1, len(reg)) if CLOSE_RE.match(reg[j])), None)
    if oi is None or ci is None:
        return None, None
    inner = reg[oi + 1:ci]
    if not all(CMT_RE.match(x) or x.strip() == '' or indent_w(x) >= 8 for x in inner):  # 不许夹正文
        return None, None
    return oi, ci


def iter_funcs(lines):
    """函数实现的首行下标（支持多行签名）：与 tmpl_audit.chk_funcs 同一套判定"""
    for i in range(len(lines)):
        if not FUNC_RE.match(lines[i]):
            continue
        j = i
        while j < len(lines) and j < i + 12:
            if lines[j].rstrip().endswith(';') and ')' not in lines[j]:
                break
            if lines[j].rstrip().endswith(')'):
                k = j + 1
                while k < len(lines) and lines[k].strip() == '':
                    k += 1
                if k < len(lines) and lines[k].lstrip().startswith('{'):
                    yield i
                break
            j += 1


def report(path, open_ban, close_ban):
    """打印标准块的横幅/字段问题（只读，不改盘）"""
    lines = read(path)[2]
    if os.path.basename(path) in EXEMPT:
        return []
    msgs = []
    for i in reversed(list(iter_funcs(lines))):
        reg = attached_comments(lines, i)[1]
        txt = '\n'.join(reg)
        sig = lines[i].strip()[:60]
        if '函数功能' not in txt:
            msgs.append((i + 1, sig, '缺注释块'))
            continue
        if not any(STD_FIELD_RE.match(x) for x in reg):
            msgs.append((i + 1, sig, '旧式格式'))
            continue
        lack = [f for f in FIELDS
                if not any(re.match(r'^ \* %s\s*:' % re.escape(f), x) for x in reg)]
        oi, ci = std_block_span(reg)
        bad_ban = oi is None or ci is None or len(reg[oi]) != len(open_ban) or len(reg[ci]) != len(close_ban)
        if lack or bad_ban:
            msgs.append((i + 1, sig, '缺字段[%s]%s' % ('、'.join(lack), ' 横幅不符' if bad_ban else '')))
    return msgs


def process(path, open_ban, close_ban):
    bom, eol, lines = read(path)
    if os.path.basename(path) in EXEMPT:
        return lines, [], bom, eol
    ch = []
    out = list(lines)
    for i in reversed(list(iter_funcs(lines))):  # 自底向上，避免行号漂移
        rs, reg = attached_comments(lines, i)
        if not reg or '函数功能' not in '\n'.join(reg):
            continue
        if any(STD_FIELD_RE.match(x) for x in reg):
            oi, ci = std_block_span(reg)          # 标准块：只归一横幅宽度
            if oi is None or ci is None:
                continue
            if len(reg[oi]) == len(open_ban) and len(reg[ci]) == len(close_ban):
                continue
            out[rs + oi] = open_ban
            out[rs + ci] = close_ban
            ch.append('L%d %s -> 横幅归一(%d/%d)' % (i + 1, lines[i][:40].strip(),
                                                 len(open_ban), len(close_ban)))
            continue
        start = next((j for j, s in enumerate(reg) if OPEN_RE.match(s)), None)
        end = next((j for j in range(len(reg) - 1, -1, -1) if CLOSE_RE.match(reg[j])), None)
        if start is None or end is None or end < start:
            continue
        items = parse_old(reg[start:end + 1])
        if not items:
            continue
        new = build_block(items, open_ban, close_ban)
        out[rs + start:rs + end + 1] = new
        ch.append('L%d %s -> 3.1 标准块(%d 行)' % (i + 1, lines[i][:40].strip(), len(new)))
    return out, ch, bom, eol


def code_sig(lines):
    """剥离所有注释后的代码特征串（用于断言"只改了注释"）"""
    txt = '\n'.join(lines)
    out, i, n, st = [], 0, len(txt), 0        # st: 0 代码 1 行注释 2 块注释 3 字符串 4 字符
    while i < n:
        c = txt[i]
        nx = txt[i + 1] if i + 1 < n else ''
        if st == 0:
            if c == '/' and nx == '/':
                st = 1
                i += 2
                continue
            if c == '/' and nx == '*':
                st = 2
                i += 2
                continue
            if c in '"\'':
                st = 3 if c == '"' else 4
            out.append(c)
        elif st == 1:
            if c == '\n':
                st = 0
                out.append(c)
        elif st == 2:
            if c == '*' and nx == '/':
                st = 0
                i += 2
                continue
        elif st == 3 and c == '\\':
            out.append(c)
            if i + 1 < n:
                out.append(txt[i + 1])
                i += 2
                continue
        elif (st == 3 and c == '"') or (st == 4 and c == "'"):
            st = 0
            out.append(c)
        i += 1
    return re.sub(r'\s+', ' ', ''.join(out)).strip()


def main():
    root, dirs = sys.argv[1], sys.argv[2].split(',')
    apply = '--apply' in sys.argv
    show_report = '--report' in sys.argv
    open_ban, close_ban = read_rules()
    files = []
    for d in dirs:
        for dp, dns, fns in os.walk(os.path.join(root, d)):
            dns[:] = [x for x in dns if x.lower() not in SKIP_LOWER]
            files += [os.path.join(dp, f) for f in sorted(fns) if f.lower().endswith(('.c', '.h'))]
    n = 0
    for p in files:
        rel = os.path.relpath(p, root)
        if show_report:
            for ln, sig, msg in reversed(report(p, open_ban, close_ban)):
                print('%-52s L%-5d %-58s %s' % (rel, ln, sig, msg))
        bom, eol, before = read(p)
        new, ch, bom2, eol2 = process(p, open_ban, close_ban)
        if not ch:
            continue
        n += 1
        print('%-56s %s' % (rel, ' ; '.join(ch)))
        if apply:
            if code_sig(before) != code_sig(new):
                print('   !! 块外代码变化，跳过写盘')
                continue
            write(p, bom, eol, new)
    print('\n%s：%d 个文件有改动%s' % (os.path.basename(root.rstrip('\\/')), n,
                                  '' if apply else '（干跑，未写盘）'))


if __name__ == '__main__':
    main()
