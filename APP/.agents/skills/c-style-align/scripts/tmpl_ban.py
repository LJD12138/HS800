#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""按已定口径统一段落横幅与 #endif 注释：
  1) .c 文件：`/* ====label==== */` 与 `//****label****//` 统一为 `//` + 52* + 标签 + * + `//`（总宽 130）
  2) .c/.h：主体结束 `#endif  /* <文件名> */` -> `#endif  /* <对应条件名> */`（不写文件名）
用法: tmpl_ban.py <root> <dirs> [--apply]
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
TOTAL = 130
LEFT = 52
# 标签归并：(正则, .c 规范标签, .h 规范标签)；顺序敏感，取首个命中
LABEL_MAP = [
    (r'include|头文件|包含', 'Includes', 'Includes'),
    (r'macro|宏', 'Macros', 'Macros'),
    (r'extern|prototype|api|函数声明|接口', 'Function Declaration', 'Extern'),
    (r'function|函数|回调|callback', 'Function Declaration', 'Extern'),
    (r'parameter|参数|init|初始化', 'Parameter Initialization', 'Globals'),
    (r'type|类型|枚举|结构|table|表', 'Parameter Initialization', 'Types'),
    (r'global|variable|变量|常量|静态|const', 'Parameter Initialization', 'Globals'),
]
BAN_EQ = re.compile(r'^/\* =+([^=]+?)=+\s*\*/$')
BAN_ST = re.compile(r'^//\*+([^*]+?)\*+/*$')
IF_RE = re.compile(r'^#if\s*\((.*)\)\s*$')
IFDEF_RE = re.compile(r'^#ifn?def\s+(\w+)')


def build(label):
    """段落横幅：// + 52 个 * + 标签 + * + //（总宽 130，.c/.h 同风格）"""
    return '//' + '*' * LEFT + label + '*' * max(1, TOTAL - 4 - LEFT - len(label)) + '//'


def norm_label(raw, is_h):
    """把标签归并到规范集合；归不进则返回 None（调用方删除该横幅行）"""
    t = raw.strip().lower()
    for pat, c_lab, h_lab in LABEL_MAP:
        if re.search(pat, t):
            return h_lab if is_h else c_lab
    return None


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


def fix(path, apply=False):
    base = os.path.basename(path)
    if base in EXEMPT:
        return ['跳过（生成/配置向导文件）']
    bom, eol, lines = read(path)
    ch = []
    is_h = base.lower().endswith('.h')
    edits = []
    for i, l in enumerate(lines):
        m = BAN_EQ.match(l) or BAN_ST.match(l)
        if not m:
            continue
        raw = m.group(1).strip()
        lab = norm_label(raw, is_h)
        if lab is None or len(lab) > TOTAL - 6:
            edits.append((i, None, raw))
            continue
        new = build(lab)
        if new != l:
            edits.append((i, new, raw))
    for i, new, raw in reversed(edits):
        if new is None:
            lines.pop(i)
            ch.append('删横幅 %r（归不进规范标签）' % raw)
        else:
            lines[i] = new
            ch.append('横幅 %r -> %r' % (raw, norm_label(raw, is_h)))
    # #endif 注释：文件名 -> 条件名
    stack = []
    for i, l in enumerate(lines):
        s = l.strip()
        if s.startswith('#if'):
            m = IF_RE.match(s)
            if m:
                stack.append(m.group(1).strip())
            else:
                m2 = IFDEF_RE.match(s)
                stack.append(m2.group(1) if m2 else '?')
        elif s.startswith('#endif'):
            cond = stack.pop() if stack else '?'
            m = re.match(r'^(#endif)\s+/\*\s*%s\s*\*/' % re.escape(base), s)
            if m and cond != '?':
                new = m.group(1) + '  /* ' + cond + ' */'
                if new != s:
                    ch.append('#endif /* %s */ -> /* %s */' % (base, cond))
                    lines[i] = l[:len(l) - len(l.lstrip())] + new
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
