#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""逐行比对「工作区 vs git 索引」，确认改动都是纯空白（无 token 变化、无行数变化）。
usage: python verify_align.py [BOOT|APP] [仓库根]
默认仓库根 = 本脚本上溯 4 级（.agents/skills/<skill>/scripts -> 仓库根）"""
import os
import re
import subprocess
import sys

REPO = sys.argv[2] if len(sys.argv) > 2 else os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
PREFIX = sys.argv[1] if len(sys.argv) > 1 else 'BOOT'

files = [f for f in subprocess.run(['git', '-C', REPO, 'diff', '--name-only', '--', PREFIX],
                                   capture_output=True, text=True, check=True).stdout.split()
         if f.lower().endswith(('.c', '.h'))]
total = 0
bad = []
for rel in files:
    show = subprocess.run(['git', '-C', REPO, 'show', ':' + rel], capture_output=True)
    if show.returncode != 0:
        print('!! not in index: %s' % rel)
        continue
    old = show.stdout.decode('utf-8-sig').replace('\r\n', '\n').split('\n')
    with open(os.path.join(REPO, rel.replace('/', os.sep)), 'rb') as f:
        new = f.read().decode('utf-8-sig').replace('\r\n', '\n').split('\n')
    if len(old) != len(new):
        bad.append('LINE COUNT %s: %d -> %d' % (rel, len(old), len(new)))
        continue
    diff_lines = [i + 1 for i in range(len(old)) if old[i] != new[i]]
    nonws = [i for i in diff_lines if re.sub(r'[ \t]', '', old[i - 1]) != re.sub(r'[ \t]', '', new[i - 1])]
    if nonws:
        bad.append('NON-WHITESPACE %s lines %s' % (rel, nonws[:5]))
    total += len(diff_lines)
print('--- %s files=%d  changed lines=%d' % (PREFIX, len(files), total))
for b in bad:
    print('!! ' + b)
