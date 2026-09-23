#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""头块/骨架批改的完整性校验：剥离注释（含头块/横幅）、空行、预处理条件指令、extern "C" 包裹后，
工作区与 git 索引版本的**代码 token 序列**必须完全一致（即只动注释与包裹，没动代码）。
用法: tmpl_verify.py [仓库根]
"""
import os
import re
import subprocess
import sys

REPO = (sys.argv[1] if len(sys.argv) > 1 else
        os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                      '..', '..', '..', '..')))
COND = re.compile(r'^#\s*(if|ifdef|ifndef|else|elif|endif)\b')


def strip_comments(text):
    """按字符状态剥离 /* */ 与 // 注释，正确跳过字符串/字符字面量"""
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


def tokens(text):
    text = strip_comments(text.replace('\r\n', '\n').replace('\r', '\n'))
    out, prev_cpp = [], False
    for l in text.split('\n'):
        s = re.sub(r'\s+', ' ', l.strip())
        prev = prev_cpp
        prev_cpp = bool(re.match(r'^#\s*ifdef\s+__cplusplus$', s))
        if not s or COND.match(s):
            continue
        if prev and s in ('extern "C" {', '}'):
            continue
        out.append(s)
    return out


files = subprocess.run(['git', '-C', REPO, 'diff', '--name-only', '--', 'BOOT', 'APP'],
                       capture_output=True, text=True, check=True).stdout.split()
files = [f for f in files if f.lower().endswith(('.c', '.h'))]
bad = []
for f in files:
    old = subprocess.run(['git', '-C', REPO, 'show', ':' + f], capture_output=True, text=True,
                         encoding='utf-8', errors='replace').stdout
    try:
        new = open(os.path.join(REPO, f), encoding='utf-8-sig').read()
    except Exception:
        continue
    a, b = tokens(old), tokens(new)
    if a != b:
        d = next((i for i, (x, y) in enumerate(zip(a, b)) if x != y), min(len(a), len(b)))
        bad.append((f, len(a), len(b), a[d] if d < len(a) else '(末尾)', b[d] if d < len(b) else '(末尾)'))

print('参与校验文件: %d' % len(files))
if bad:
    print('!! 代码 token 不一致: %d 个' % len(bad))
    for f, n1, n2, x, y in bad[:15]:
        print('   %-52s 旧 %d token / 新 %d token' % (f, n1, n2))
        print('        旧: %s' % x[:72])
        print('        新: %s' % y[:72])
else:
    print('OK：全部文件在注释/包裹以外代码 token 完全一致')
