#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""按 c-code-templates 补齐文件骨架（模板化插入，不改语义）：
  1) 无 Includes 段落横幅 -> 在首个 #include 之前插入
  2) .h 缺 #ifdef __cplusplus / extern "C" -> 补开闭两块
  3) 无 #if (x) 主体包裹 -> 补 #if (1) ... #endif  /* 1 */
  4) 守卫 #endif 缺行尾注释 -> 补 /* GUARD */
用法: tmpl_skel.py <root> <dirs> [--apply]
"""
import os
import re
import sys

SKIP_DIRS = {'.git', '.cmsis', '.pack', 'RTE', 'build', 'DebugConfig', 'Objects', 'Listings',
             'eez_ui', 'eez_project', 'lvgl', 'FreeRTOS', 'EasyFlash', 'EasyLogger',
             'CmBacktrace', 'lwrb', 'LightweightRingBuffer', 'SeggerRtt', 'MultiFuncKey', 'Firmware'}
SKIP_LOWER = {x.lower() for x in SKIP_DIRS}   # 目录名大小写不敏感
EXEMPT = {'gd32f50x_it.c', 'gd32f50x_it.h', 'gd32f50x_libopt.h',
          'FreeRTOSConfig.h', 'board_config.h', 'board_config.c'}
GUARD_RE = re.compile(r'^#ifndef\s+(\w+)')
DEF_RE = re.compile(r'^#define\s+\w+\s*$')
INC_BAN_C = '//' + '*' * 52 + 'Includes' + '*' * (74 - 8) + '//'
INC_BAN_H = INC_BAN_C          # .h 与 .c 同风格


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


def pair_map(lines):
    """返回 {endif_index: if_index}"""
    stack, pairs = [], {}
    for i, l in enumerate(lines):
        s = l.strip()
        if s.startswith('#if'):
            stack.append(i)
        elif s.startswith('#endif') and stack:
            pairs[i] = stack.pop()
    return pairs


def fix(path, apply=False):
    base = os.path.basename(path)
    if base in EXEMPT:
        return ['跳过（生成/配置向导文件）']
    is_h = base.lower().endswith('.h')
    bom, eol, lines = read(path)
    ch = []

    def blank_after(i):
        return i + 1 < len(lines) and lines[i + 1].strip() == ''

    # --- 2) .h 补 extern "C" ---
    if is_h and '#ifdef __cplusplus' not in '\n'.join(lines):
        g = next((i for i, l in enumerate(lines) if GUARD_RE.match(l.strip())), None)
        if g is not None:
            j = g + 1
            while j < len(lines) and (lines[j].strip() == '' or DEF_RE.match(lines[j].strip())):
                j += 1
            lines[j:j] = ['#ifdef __cplusplus', 'extern "C" {', '#endif', '']
            ch.append('补 extern "C" 开头块')
            pairs = pair_map(lines)
            g_end = next((e for e, s in pairs.items() if s == g), None)
            if g_end is not None:
                lines[g_end:g_end] = ['#ifdef __cplusplus', '}', '#endif', '']
                ch.append('补 extern "C" 结尾块')
    # --- 3) 补 #if (1) 主体包裹 ---
    if not re.search(r'^#if\s*\(', '\n'.join(lines), re.M):
        anchor = None
        for i, l in enumerate(lines[:60]):
            s = l.strip()
            if s.startswith('#include') or s == 'extern "C" {' or GUARD_RE.match(s) or DEF_RE.match(s):
                anchor = i
        if anchor is not None:
            lines[anchor + 1:anchor + 1] = ['', '#if (1)']
            ch.append('补 #if (1) 开头')
            pairs = pair_map(lines)
            g = next((i for i, l in enumerate(lines) if GUARD_RE.match(l.strip())), None)
            ins = None
            if g is not None:
                if is_h:
                    cp = [i for i, l in enumerate(lines) if l.strip() == '#ifdef __cplusplus']
                    ins = cp[-1] if cp else next((e for e, s in pairs.items() if s == g), None)
                else:
                    ins = next((e for e, s in pairs.items() if s == g), None)
            if ins is None:
                ins = len(lines) - 1 if lines and lines[-1].strip() == '' else len(lines)
            lines[ins:ins] = ['#endif  /* 1 */', '']
            ch.append('补 #endif  /* 1 */')
    # --- 4) 守卫 #endif 补注释 ---
    g = next((i for i, l in enumerate(lines) if GUARD_RE.match(l.strip())), None)
    if g is not None:
        guard = GUARD_RE.match(lines[g].strip()).group(1)
        pairs = pair_map(lines)
        g_end = next((e for e, s in pairs.items() if s == g), None)
        if g_end is not None and not re.match(r'^#endif\s+/\*\s*.+\*/\s*$', lines[g_end].strip()):
            lines[g_end] = '#endif  /* %s */' % guard
            ch.append('守卫 #endif 补 /* %s */' % guard)
    # --- 1) Includes 横幅 ---
    ban_re = re.compile(r'^(?://\*+(.+?)\*+//|/\* =+(.+?)=+\s*\*/)$')
    has_ban = False
    for l in lines:
        m = ban_re.match(l)
        if m:
            lab = m.group(1) or m.group(2) or ''
            if 'include' in lab.lower() or '头文件' in lab:
                has_ban = True
                break
    first_inc = next((i for i, l in enumerate(lines) if l.startswith('#include')), None)
    if not has_ban and first_inc is not None:
        pos = first_inc
        k = first_inc - 1
        while k >= 0:
            s = lines[k].strip()
            if s == '' or s == 'extern "C" {' or s.startswith('#endif') or s.startswith('#ifdef __cplusplus'):
                k -= 1
                continue
            if s.startswith('#if'):          # 条件包含块（#if (boardX) / #if defined(...)）
                pos = k
            break
        lines[pos:pos] = [INC_BAN_H if is_h else INC_BAN_C]
        ch.append('插 Includes 段落横幅')
    if ch and apply:
        write(path, bom, eol, lines)
    return ch


def main():
    root, dirs = sys.argv[1], sys.argv[2].split(',')
    apply = '--apply' in sys.argv
    files = []
    for d in dirs:
        for dp, dns, fns in os.walk(os.path.join(root, d)):
            dns[:] = [x for x in dns if x.lower() not in SKIP_LOWER]
            files += [os.path.join(dp, f) for f in sorted(fns) if f.lower().endswith(('.c', '.h'))]
    n = 0
    for p in files:
        ch = fix(p, apply)
        if ch and all(c.startswith('跳过') for c in ch):     # 豁免文件不计入改动数
            continue
        if ch:
            n += 1
            print('%-56s %s' % (os.path.relpath(p, root), ' ; '.join(ch)))
    print('\n%s：%d 个文件有改动%s' % (os.path.basename(root.rstrip('\\/')), n,
                                     '' if apply else '（干跑，未写盘）'))


if __name__ == '__main__':
    main()
