#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""按 .agents/rules/c-code-templates.md 检查工程文件符合度。
横幅字符串直接从 rules 文件里提取，保证与规则同源。"""
import os
import re
import sys

RULES = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      '..', '..', '..', 'rules', 'c-code-templates.md'))
SKIP_DIRS = {'.git', '.cmsis', '.pack', 'RTE', 'build', 'DebugConfig', 'Objects', 'Listings',
             'eez_ui', 'eez_project', 'lvgl', 'LVGL', 'FreeRTOS', 'EasyFlash', 'easyflash',
             'EasyLogger', 'easylogger', 'CmBacktrace', 'cm_backtrace', 'lwrb',
             'LightweightRingBuffer', 'SeggerRtt',
             'MultiFuncKey', 'Firmware', 'RTT', 'SEGGER'}
SKIP_LOWER = {x.lower() for x in SKIP_DIRS}   # 目录名大小写不敏感
HDR_FIELDS = ['Project', 'Module', 'File', 'Date', 'Author', 'Desc', 'todo']
FUNC_RE = re.compile(r'^(static\s+)?[A-Za-z_][\w \t\*]*?[\w\*]\s*\([^;{]*$')
CMT_RE = re.compile(r'^\s*(/\*|\*|//|-{2,}|#\s*(?:if|ifdef|ifndef|else|elif|endif)\b)')  # 注释行 + 条件编译行（不含 #include/#define）
STD_FIELD_RE = re.compile(r'^ \* (函数功能|说明\(备注\)|传入参数|输出参数|返回值)\s*:')
FUNC_FIELDS = ['函数功能', '说明(备注)', '传入参数', '输出参数', '返回值']


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
    with open(RULES, encoding='utf-8') as f:
        txt = f.read()
    fences = re.findall(r'```c\n(.*?)```', txt, re.S)
    c_tpl, h_tpl = fences[0].split('\n'), fences[1].split('\n')
    c_ban = [l for l in c_tpl if l.startswith('//****')]
    h_ban = [l for l in h_tpl if l.startswith('//****')]
    h_hdr = [l for l in h_tpl if l.startswith('/****')]
    c_hdr = [l for l in c_tpl if l.startswith('/****')]
    return {'c_ban': c_ban, 'h_ban': h_ban, 'c_hdr_len': len(c_hdr[0]), 'h_hdr_len': len(h_hdr[0]),
            'func_ban_len': len(_func_tpl(txt)[0]), 'func_close_len': len(_func_tpl(txt)[1]),
            'c_hdr_close': [l for l in c_tpl if l.rstrip().endswith('*/') and l.startswith(' ***')][-1],
            'h_hdr_close': [l for l in h_tpl if l.rstrip().endswith('*/') and l.startswith(' ***')][-1],
            'c_body_endif': [l for l in c_tpl if l.startswith('#endif')][-1],
            'h_body_endif': [l for l in h_tpl if l.startswith('#endif')][0]}


def _func_tpl(txt):
    """rules 3.1 函数头注释块的首尾横幅"""
    m = re.search(r'```c\n(/\*{10,}\n \* 函数功能.*?\n \*{10,}/)\n```', txt, re.S)
    blk = m.group(1).split('\n')
    return blk[0], blk[-1]


def load(path):
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


def chk_header(lines, proj, rel, base, R):
    errs = []
    close = next((j for j in range(1, min(len(lines), 45)) if re.match(r'^\s*\*+/\s*$', lines[j])), 15)
    head = '\n'.join(lines[:close + 1])
    if not lines:
        return ['空文件']
    if len(lines[0]) < 100 or not re.match(r'^/\*{50,}$', lines[0]):
        return ['缺文件头注释块（首行非 /*** 横幅）']
    if len(lines[0]) != R['c_hdr_len'] and len(lines[0]) != R['h_hdr_len']:
        errs.append('头块横幅长度 %d 与规范(%d/%d)不符' % (len(lines[0]), R['c_hdr_len'], R['h_hdr_len']))
    fld = {}
    for name in HDR_FIELDS:
        m = re.search(r'^ \* %s\s*:\s*(.*)$' % re.escape(name), head, re.M)
        fld[name] = m.group(1).strip() if m else None
        if m is None:
            errs.append('头块缺字段 %s' % name)
    if fld['Project'] not in (None, proj):
        errs.append('Project 字段为 %r，应为 %r' % (fld['Project'], proj))
    if fld['File'] not in (None, base):
        errs.append('File 字段为 %r，应为 %r' % (fld['File'], base))
    d = os.path.dirname(rel)
    exp = (proj + '\\' + d.replace('/', '\\')) if d else proj
    if fld['Module'] is not None and fld['Module'].replace('/', '\\') != exp:
        errs.append('Module 字段为 %r，应为 %r' % (fld['Module'], exp))
    if fld['Date'] is not None and not re.match(r'^\d{4}-\d{2}-\d{2}$', fld['Date']):
        errs.append('Date 非 YYYY-MM-DD: %r' % fld['Date'])
    if fld['Author'] is not None and 'LJD(291483914@qq.com)' not in fld['Author']:
        errs.append('Author 不规范: %r' % fld['Author'])
    if not re.search(r'^ \* Copyright \(c\) \d{4} -inc\s*$', head, re.M):
        m = re.search(r'^ \* Copyright.*$', head, re.M)
        errs.append('Copyright 行缺失' if m is None else 'Copyright 行不规范: %r' % m.group(0).strip())
    if '---' not in head:
        errs.append('头块缺 ---- 分隔线')
    return errs


def chk_endif_comments(lines):
    """新口径：`#endif` 的行尾注释（若有）必须是对应条件名（允许写条件里的某个标识符）"""
    errs = []
    stack = []
    for i, l in enumerate(lines):
        s = l.strip()
        if s.startswith('#if'):
            m = re.match(r'^#if\s*\((.*)\)\s*$', s)
            if m:
                stack.append(m.group(1).strip())
            else:
                m2 = re.match(r'^#ifn?def\s+(\w+)', s)
                stack.append(m2.group(1) if m2 else None)
        elif s.startswith('#endif'):
            cond = stack.pop() if stack else None
            m = re.match(r'^#endif\s+/\*\s*(.+?)\s*\*/', s)
            if m and cond:
                cmt = m.group(1)
                same = (cmt == cond or cmt in cond or cond in cmt
                        or set(cmt.split()) == set(cond.split()))
                if not same:
                    errs.append('L%d #endif 注释 %r 与条件 %r 不符' % (i + 1, cmt, cond))
                    break
    return errs


def chk_h(lines, base, R):
    errs = []
    txt = '\n'.join(lines)
    if not re.search(r'^#ifndef\s+(\w+)', txt, re.M):
        errs.append('缺 #ifndef 保护')
    else:
        guard = re.search(r'^#ifndef\s+(\w+)', txt, re.M).group(1)
        if not re.search(r'^#endif\s+/\*\s*%s\s*\*/' % re.escape(guard), txt, re.M):
            errs.append('末尾缺 #endif  /* %s */' % guard)
    if '#ifdef __cplusplus' not in txt:
        errs.append('缺 #ifdef __cplusplus / extern "C"')
    if not re.search(r'^#if\s*\(', txt, re.M):
        errs.append('缺 #if (x) 主体包裹')
    errs += chk_endif_comments(lines)
    label_re = re.compile(r'^//\*+([^*]+?)\*+/*$')
    allowed = {}
    for l in R['h_ban']:
        m = label_re.match(l)
        if m:
            allowed[m.group(1).lower()] = len(l)
    for l in [x for x in lines if x.startswith('//****')]:
        m = label_re.match(l)
        if not m:
            errs.append('段落横幅格式不符: %s' % l[:46])
            break
        lab = m.group(1)
        if lab.lower() not in allowed:
            errs.append('段落标签不在规范集合(.h 5 个): %s' % lab)
            break
        if len(l) != allowed[lab.lower()]:
            errs.append('段落横幅长度 %d 与规范 %d 不符: %s' % (len(l), allowed[lab.lower()], lab))
            break
    return errs


def chk_c(lines, base, R):
    errs = []
    txt = '\n'.join(lines)
    label_re = re.compile(r'^//\*+([^*]+?)\*+/*$')
    allowed = {}
    for l in R['c_ban']:
        m = label_re.match(l)
        if m:
            allowed[m.group(1).lower()] = len(l)
    ban = [l for l in lines if l.startswith('//****')]
    if not ban:
        if any(l.startswith('/* ====') for l in lines):
            errs.append('.c 段落横幅用 /* ==== */ 风格（口径：统一 //****...****//）')
        else:
            errs.append('缺 //**** 段落横幅')
    else:
        inc = [l for l in ban if 'includes' in l.lower()]
        if not inc:
            errs.append('缺 Includes 段落横幅')
        elif len(inc[0]) != len(R['c_ban'][0]):
            errs.append('Includes 横幅长度 %d 与规范 %d 不符' % (len(inc[0]), len(R['c_ban'][0])))
        for l in ban:
            m = label_re.match(l)
            if not m:
                errs.append('段落横幅格式不符: %s' % l[:46])
                break
            lab = m.group(1)
            if lab.lower() not in allowed:
                errs.append('段落标签不在规范集合(.c 4 个): %s' % lab)
                break
            if len(l) != allowed[lab.lower()]:
                errs.append('段落横幅长度 %d 与规范 %d 不符: %s' % (len(l), allowed[lab.lower()], lab))
                break
    if not re.search(r'^#if\s*\(', txt, re.M):
        errs.append('缺 #if (x) 主体包裹')
    errs += chk_endif_comments(lines)
    return errs


def chk_funcs(lines, R):
    miss = []
    for i, ln in enumerate(lines):
        if not FUNC_RE.match(ln):
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
                    reg = attached_comments(lines, i)[1]
                    txt = '\n'.join(reg)
                    if '函数功能' not in txt:
                        miss.append((i + 1, ln[:52].strip(), '缺注释块'))
                    elif not any(STD_FIELD_RE.match(x) for x in reg):
                        miss.append((i + 1, ln[:52].strip(), '旧式格式需规范化'))
                    else:
                        lack = [f for f in FUNC_FIELDS
                                if not any(re.match(r'^ \* %s\s*:' % re.escape(f), x) for x in reg)]
                        if lack:
                            miss.append((i + 1, ln[:52].strip(), '注释块字段不全(缺 %s)' % '、'.join(lack)))
                        ban = next((x for x in reg if x.lstrip().startswith('/****')), None)
                        if ban is not None and len(ban) != R['func_ban_len']:
                            miss.append((i + 1, ln[:52].strip(),
                                         '函数块横幅长度 %d 与规范 %d 不符' % (len(ban), R['func_ban_len'])))
                    break
            j += 1
    return miss


def main():
    root, dirs = sys.argv[1], sys.argv[2].split(',')
    R = read_rules()
    proj = os.path.basename(root.rstrip('\\/'))
    rows = []
    for d in dirs:
        for dp, dns, fns in os.walk(os.path.join(root, d)):
            dns[:] = [x for x in dns if x.lower() not in SKIP_LOWER]
            for fn in sorted(fns):
                if not fn.lower().endswith(('.c', '.h')):
                    continue
                p = os.path.join(dp, fn)
                rel = os.path.relpath(p, root)
                lines = load(p)
                if lines is None:
                    rows.append((rel, ['非 UTF-8，跳过'], 0, 0))
                    continue
                errs = chk_header(lines, proj, rel, fn, R)
                if fn.lower().endswith('.h'):
                    errs += chk_h(lines, fn, R)
                else:
                    errs += chk_c(lines, fn, R)
                miss = chk_funcs(lines, R)
                if miss:
                    kinds = {}
                    for ln, sig, kind in miss:
                        kinds.setdefault(kind, []).append(ln)
                    parts = ['%s %d 处 (L%s…)' % (k, len(v), '/'.join(str(x) for x in v[:5]))
                             for k, v in kinds.items()]
                    errs.append('函数头注释块: ' + '; '.join(parts))
                rows.append((rel, errs, len(re.findall(r'//', '\n'.join(lines))), len(miss)))
    bad = [r for r in rows if r[1]]
    print('=== %s：%d 个文件，符合 %d，不符合 %d ===' % (proj, len(rows), len(rows) - len(bad), len(bad)))
    cats = {}
    for rel, errs, *_ in bad:
        for e in errs:
            key = re.sub(r'\d+', 'N', e)
            key = re.sub(r"'.*?'", "'X'", key).split('(')[0].strip()
            cats.setdefault(key, []).append((rel, e))
    for key, items in sorted(cats.items(), key=lambda kv: -len(kv[1])):
        print('\n--- [%d 个文件] %s' % (len(items), key))
        for rel, e in items[:8]:
            extra = '' if e == key else ' | ' + e
            print('    %-50s%s' % (rel, extra))
        if len(items) > 8:
            print('    … 另 %d 个' % (len(items) - 8))
    print('\n=== 按文件（忽略 #endif 注释口径类）===')
    for rel, errs, *_ in bad:
        e2 = [e for e in errs if '#endif' not in e]
        if e2:
            print('  %-52s %s' % (rel, ' ; '.join(e2)))
    nz = sum(1 for r in rows if r[2] > 0)


if __name__ == '__main__':
    main()
