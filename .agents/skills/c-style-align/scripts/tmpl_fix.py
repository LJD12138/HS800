#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""按 .agents/rules/c-code-templates.md 批量修 A 类问题（文件头注释块、段落横幅宽度）。
只动头块内部与横幅行；其余行必须字节不变（内置断言）。用法: ctmpl_fix.py <root> <dirs> [--apply]"""
import os
import re
import sys

RULES = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      '..', '..', '..', 'rules', 'c-code-templates.md'))
SKIP_DIRS = {'.git', '.cmsis', '.pack', 'RTE', 'build', 'DebugConfig', 'Objects', 'Listings',
             'eez_ui', 'eez_project', 'lvgl', 'FreeRTOS', 'EasyFlash', 'EasyLogger',
             'CmBacktrace', 'lwrb', 'LightweightRingBuffer', 'SeggerRtt', 'MultiFuncKey', 'Firmware'}
SKIP_LOWER = {x.lower() for x in SKIP_DIRS}   # 目录名大小写不敏感
# 生成文件 / Keil 配置向导文件：不自动改头块
EXEMPT = {'gd32f50x_it.c', 'gd32f50x_it.h', 'gd32f50x_libopt.h',
          'FreeRTOSConfig.h', 'board_config.h', 'board_config.c'}
SEP = ' * -------------------------------------------------------'
FIELD_RE = re.compile(r'^ \* (Project|Module|File|Date|Author|Desc|todo)\s*:\s*(.*)$')
CLOSE_RE = re.compile(r'^\s*\*+/\s*$')
BAN_RE = re.compile(r'^(//\*+(.+?)\*+//|/\* =+(.+?)=+\s*\*/)$')


def read_rules():
    with open(RULES, encoding='utf-8') as f:
        txt = f.read()
    fences = re.findall(r'```c\n(.*?)```', txt, re.S)
    out = {}
    for tag, tpl in (('c', fences[0]), ('h', fences[1])):
        ls = tpl.split('\n')
        a = next(i for i, l in enumerate(ls) if l.startswith('/****'))
        b = next(i for i, l in enumerate(ls) if i > a and CLOSE_RE.match(l))
        out[tag + '_blk'] = ls[a:b + 1]
        out[tag + '_ban'] = [l for l in ls if l.startswith('//****') or l.startswith('/* ====')]
    return out


def read(path):
    with open(path, 'rb') as f:
        raw = f.read()
    bom = raw.startswith(b'\xef\xbb\xbf')
    text = raw.decode('utf-8-sig')
    if '\r\n' in text:
        eol = '\r\n'
    elif '\r' in text:
        eol = '\r'
    else:
        eol = '\n'
    lines = text.replace('\r\n', '\n').replace('\r', '\n').split('\n')
    return bom, eol, lines


def write(path, bom, eol, lines):
    data = eol.join(lines)
    with open(path, 'wb') as f:
        f.write(data.encode('utf-8-sig' if bom else 'utf-8'))


def tpl_field_lines(R, tag):
    return list(R[tag + '_blk'])


def canon_date(path, cur):
    if cur and re.match(r'^\d{4}-\d{2}-\d{2}$', cur):
        return cur, None
    if cur:
        m = re.match(r'^(\d{4}-\d{2}-\d{2})', cur)
        if m:
            return m.group(1), 'Date 去掉时分秒'
    import time
    return time.strftime('%Y-%m-%d', time.localtime(os.path.getmtime(path))), 'Date 取文件日期'


def fix_file(path, root, R, apply=False):
    proj = os.path.basename(root.rstrip('\\/'))
    rel = os.path.relpath(path, root)
    base = os.path.basename(path)
    tag = 'h' if base.lower().endswith('.h') else 'c'
    bom, eol, lines = read(path)
    ch = []
    d = os.path.dirname(rel)
    module = (proj + '\\' + d.replace('/', '\\')) if d else proj
    todo_item = '1. 无' if proj == 'APP' else '1. none'

    if base in EXEMPT:
        return lines, ['跳过（生成/配置向导文件，需人工确认豁免）'], bom, eol, 0

    # ---- 头块定位 ----
    has_blk = bool(lines) and re.match(r'^/\*{50,}', lines[0])
    if not has_blk:
        # 生成标准头块，插到文件最前（保留原有 Doxygen/注释）
        desc = '[文件功能描述]'
        for l in lines[:12]:
            m = re.match(r'^\s*\*?\s*@brief\s+(.*)$', l)
            if m:
                desc = m.group(1).strip()
                ch.append('Desc 取自 @brief')
                break
        blk = []
        for l in R[tag + '_blk']:
            l2 = l
            if re.match(r'^ \* Project', l):
                l2 = ' * Project : ' + proj
            elif re.match(r'^ \* Module', l):
                l2 = ' * Module  : ' + module
            elif re.match(r'^ \* File', l):
                l2 = ' * File    : ' + base
            elif re.match(r'^ \* Date', l):
                l2 = ' * Date    : ' + canon_date(path, None)[0]
            elif re.match(r'^ \* Desc', l):
                l2 = ' * Desc    : ' + desc
            elif re.match(r'^ \* 1\. none', l):
                l2 = ' * ' + todo_item
            blk.append(l2)
        new = blk + [''] + lines
        ch.append('新建标准头块')
        return new, ch, bom, eol, len(blk) + 1

    close = next((j for j in range(1, min(len(lines), 40)) if CLOSE_RE.match(lines[j])), None)
    if close is None:
        return lines, ['头块未闭合，跳过'], bom, eol, 0
    block = lines[:close + 1]
    inner = block[1:-1]
    fields = {}
    for l in inner:
        m = FIELD_RE.match(l)
        if m:
            fields[m.group(1)] = m.group(2).strip()
    # 装饰性标题框（只有标题/空框线、无字段）-> 整块重建，标题当 Desc
    if not fields:
        title = ''
        for l in inner:
            t = re.sub(r'^\*+\s*', '', l.strip())
            t = re.sub(r'\s*\*+$', '', t)
            t = re.sub(r'^-+\s*', '', t).strip()
            if not t:
                continue
            m = re.match(r'^(文件说明|说明\(备注\)|说明|描述|Desc)\s*[:：]?\s*(.+)$', t)
            if m:
                t = m.group(2).strip()
            title = t
            break
        newblk = []
        for l in R[tag + '_blk']:
            if re.match(r'^ \* Project', l):
                l = ' * Project : ' + proj
            elif re.match(r'^ \* Module', l):
                l = ' * Module  : ' + module
            elif re.match(r'^ \* File', l):
                l = ' * File    : ' + base
            elif re.match(r'^ \* Date', l):
                l = ' * Date    : ' + canon_date(path, None)[0]
            elif re.match(r'^ \* Desc', l):
                l = ' * Desc    : ' + (title or '[文件功能描述]')
            elif re.match(r'^ \* 1\. none', l):
                l = ' * ' + todo_item
            newblk.append(l)
        ch.append('装饰性头框 -> 标准头块(Desc=%s)' % (title or '[文件功能描述]'))
        rest = lines[close + 1:]
        add = 0
        if rest and rest[0].strip():
            rest = [''] + rest
            add = 1
        return newblk + rest, ch, bom, eol, len(newblk) + add

    # ---- 已有字段：纠正值 + 补齐缺失（最小插入）----
    block = list(block)
    if block[0] != R[tag + '_blk'][0]:
        ch.append('头块横幅长度归一(%d->%d)' % (len(block[0]), len(R[tag + '_blk'][0])))
    if block[-1] != R[tag + '_blk'][-1]:
        ch.append('头块闭合行长度归一(%d->%d)' % (len(block[-1]), len(R[tag + '_blk'][-1])))
    block[0] = R[tag + '_blk'][0]           # 横幅长度归一
    block[-1] = R[tag + '_blk'][-1]         # 闭合行归一
    idx = {k: i for i, l in enumerate(block) if (m := FIELD_RE.match(l)) and (k := m.group(1))}
    want = {'Project': proj, 'Module': module, 'File': base, 'Author': 'LJD(291483914@qq.com)'}
    for k, v in want.items():
        if k in idx:
            if fields.get(k) != v:
                line = block[idx[k]]
                block[idx[k]] = line[:line.index(':', 4) + 1] + ' ' + v
                ch.append('%s: %r -> %r' % (k, fields.get(k), v))
        else:
            # 插到规范顺序的位置
            order = ['Project', 'Module', 'File', 'Date', 'Author', 'Desc']
            pos = len(block) - 1
            for o in order[order.index(k) + 1:]:
                if o in idx:
                    pos = idx[o]
                    break
            else:
                pos = next((i for i, l in enumerate(block) if l.startswith(' * ---')), len(block) - 1)
            lab = {'Project': ' * Project : ', 'Module': ' * Module  : ', 'File': ' * File    : ',
                   'Date': ' * Date    : ', 'Author': ' * Author  : ', 'Desc': ' * Desc    : '}[k]
            val = canon_date(path, fields.get('Date'))[0] if k == 'Date' else v
            block.insert(pos, lab + val)
            idx = {kk: i + (1 if i >= pos else 0) for kk, i in idx.items()}
            ch.append('补 %s = %s' % (k, val))
    if 'Date' in idx and fields.get('Date') is not None:
        nd, why = canon_date(path, fields.get('Date'))
        if why:
            line = block[idx['Date']]
            block[idx['Date']] = line[:line.index(':', 4) + 1] + ' ' + nd
            ch.append('%s (%r -> %r)' % (why, fields.get('Date'), nd))
    if 'Desc' not in idx:
        pos = next((i for i, l in enumerate(block) if l.startswith(' * ---')), len(block) - 1)
        block.insert(pos, ' * Desc    : [文件功能描述]')
        ch.append('补 Desc（占位，需人工填写）')
    # ---- 尾部：规范为 (---) todo: 1.xx (---) Copyright ----
    ti = next((i for i, l in enumerate(block) if FIELD_RE.match(l) and FIELD_RE.match(l).group(1) == 'todo'), None)
    has_cop = any(re.match(r'^ \* Copyright', l) for l in block)
    seps = [i for i, l in enumerate(block[:-1]) if l.startswith(' * ---')]
    if not seps:
        ins = [SEP, ' * todo    :', ' * ' + todo_item, SEP]
        if not has_cop:
            ins.append(' * Copyright (c) 2026 -inc')
        block[len(block) - 1:len(block) - 1] = ins
        ch.append('补 todo / Copyright（含分隔线）')
    else:
        if ti is None:
            p = seps[-1] + 1
            block[p:p] = [' * todo    :', ' * ' + todo_item]
            ti = p
            ch.append('补 todo 字段')
        else:
            nxt = block[ti + 1] if ti + 1 < len(block) - 1 else ''
            if not re.match(r'^ \* \S', nxt) or FIELD_RE.match(nxt) or nxt.startswith(' * ---'):
                block.insert(ti + 1, ' * ' + todo_item)
                ch.append('补 todo 条目')
        # todo 条目与紧随其后的 Copyright 之间要有分隔线
        j = ti + 1
        while j < len(block) - 1 and not block[j].startswith(' * ---') and not re.match(r'^ \* Copyright', block[j]):
            j += 1
        if j < len(block) - 1 and re.match(r'^ \* Copyright', block[j]) and not block[j - 1].startswith(' * ---'):
            block.insert(j, SEP)
            ch.append('补 todo 后分隔线')
        if not has_cop:
            k = max(i for i, l in enumerate(block[:-1]) if l.startswith(' * ---'))
            block.insert(k + 1, ' * Copyright (c) 2026 -inc')
            ch.append('补 Copyright 行')
    # Copyright 行格式归一（保留年份）
    for i, l in enumerate(block):
        if re.match(r'^ \* Copyright', l) and not re.match(r'^ \* Copyright \(c\) \d{4} -inc\s*$', l):
            m = re.search(r'\(c\)\s*(\d{4})', l)
            block[i] = ' * Copyright (c) %s -inc' % (m.group(1) if m else '2026')
            ch.append('Copyright 行格式归一')
            break
    new = block + lines[close + 1:]
    # 去重：连续多行的分隔线只保留一条
    ded = []
    for l in new[:close + 1]:
        if ded and l.startswith(' * ---') and ded[-1].startswith(' * ---'):
            ch.append('去重多余分隔线')
            continue
        ded.append(l)
    new = ded + new[close + 1:]

    return new, ch, bom, eol, len(ded)


def main():
    root, dirs = sys.argv[1], sys.argv[2].split(',')
    apply = '--apply' in sys.argv
    R = read_rules()
    files = []
    for d in dirs:
        for dp, dns, fns in os.walk(os.path.join(root, d)):
            dns[:] = [x for x in dns if x.lower() not in SKIP_LOWER]
            files += [os.path.join(dp, f) for f in sorted(fns) if f.lower().endswith(('.c', '.h'))]
    nch = 0
    for p in files:
        before = open(p, 'rb').read()
        new, ch, bom, eol, body_start = fix_file(p, root, R, apply)
        if not ch or all(c.startswith('跳过') for c in ch):      # 豁免文件不计入改动数
            continue
        nch += 1
        print('%-58s %s' % (os.path.relpath(p, root), ' ; '.join(ch)))
        if apply and any(not c.startswith('跳过') for c in ch) and body_start:
            # 断言：头块之外只允许段落横幅行变化（其余内容必须逐行一致）
            ob = before.decode('utf-8-sig').replace('\r\n', '\n').split('\n')
            b_re = re.compile(r'^(//\*{10,}.*|/\* =+.*=+ ?\*/)$')
            old_body = ob[body_start + (len(ob) - len(new)):]
            new_body = new[body_start:]
            ok = len(old_body) == len(new_body) and all(
                a == b or (b_re.match(a) and b_re.match(b)) for a, b in zip(old_body, new_body))
            if not ok:
                print('   !! 头块之外内容变化，跳过写盘')
                continue
            write(p, bom, eol, new)
    print('\n%s：%d 个文件有改动%s' % (os.path.basename(root.rstrip('\\/')), nch, '' if apply else '（干跑，未写盘）'))


if __name__ == '__main__':
    main()
