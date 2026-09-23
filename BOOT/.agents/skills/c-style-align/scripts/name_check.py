#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""HS800 命名规范只读体检：按 .agents/rules/c-coding-style.md 第 3 节检查命名。

用法: python name_check.py <项目根> <目录列表,逗号分隔>
  例: python name_check.py "G:\\...\\HS800\\BOOT" "Application,ComFunc,Hardware,Middlewares"

只读脚本：只解析、只报告，不写任何文件；解析前先用字符状态机剥离注释与字符串字面量。
"""
import os
import re
import sys
from collections import Counter, OrderedDict

# ------------------------------------------------------------------ 工程约定 --
SKIP_DIRS = {'.git', '.cmsis', '.pack', 'RTE', 'build', 'DebugConfig', 'Objects', 'Listings',
             'eez_ui', 'eez_project', 'lvgl', 'FreeRTOS', 'EasyFlash', 'EasyLogger',
             'CmBacktrace', 'lwrb', 'LightweightRingBuffer', 'SeggerRtt',
             'MultiFuncKey', 'Firmware', '.agents', '.trae', '.vscode'}
SKIP_LOWER = {x.lower() for x in SKIP_DIRS}          # 目录名大小写不敏感
EXEMPT = {'gd32f50x_it.c', 'gd32f50x_it.h', 'gd32f50x_libopt.h',
          'FreeRTOSConfig.h', 'board_config.h', 'board_config.c'}

# 3.1 返回值「返回类型 -> 首前缀」表（新口径：ui 已废弃，uint32_t/u32 统一用 ul；
#      表中没有的返回类型只统计、单独归类，不算 N3 违规）
RET_MAP = {('void', False): 'v', ('void', True): 'pv',
           ('bool', False): 'b', ('BaseType_t', False): 'b',
           ('s8', False): 'c', ('int8_t', False): 'c',
           ('u8', False): 'uc', ('uint8_t', False): 'uc',
           ('s16', False): 's', ('int16_t', False): 's',
           ('u16', False): 'us', ('uint16_t', False): 'us',
           ('s32', False): 'l', ('int32_t', False): 'l',
           ('u32', False): 'ul', ('uint32_t', False): 'ul',
           ('float', False): 'f', ('double', False): 'd',
           ('u64', False): 'u64', ('uint64_t', False): 'u64'}

# 标准库/固件库中同样以 _t 结尾的 typedef：不算「存量业务类型」
STD_TYPES = {'uint8_t', 'int8_t', 'uint16_t', 'int16_t', 'uint32_t', 'int32_t',
             'uint64_t', 'int64_t', 'size_t', 'ssize_t', 'intptr_t', 'uintptr_t', 'time_t'}

# 非业务函数（不计入 3.1 判定）
EXCL_NAMES = {'main', 'Reset_Handler', 'SystemInit', '_sys_exit', 'fputc'}
# C 关键字：出现在「名字」或「返回类型」位置一律不当作函数名
KEYWORDS = {'if', 'else', 'while', 'for', 'switch', 'case', 'default', 'do', 'return',
            'sizeof', 'typedef', 'struct', 'union', 'enum', 'goto', 'break', 'continue',
            'defined', 'volatile', 'static', 'const', 'extern', 'register', 'inline'}

# ------------------------------------------------------------------ 正则表 ----
# 单行声明（type [*] name[arr] [= init]；类型与名字之间必须有空白或 *，
# 且名字后必须是 = ; , 或行尾 —— 天然排除函数、调用、枚举常量行）
DECL_RE = re.compile(
    r'^\s*(?P<pre>(?:(?:static|extern|volatile|const|register|_IO|__IO|__I|__O|unsigned|signed)\s+)*)'
    r'(?P<type>[A-Za-z_]\w*)(?P<gap>\s*\*\s*|\s+)(?P<name>[A-Za-z_]\w*)\s*(?P<arr>\[[^\]]*\])?\s*'
    r'(?:=|;|,|$)')
# 函数签名行（返回类型与函数名之间必须有空白或 *，避免把调用/条件语句当函数）
RET_TOK = (r'(?:(?:const|volatile|struct|union|enum|unsigned|signed|register|_IO|__IO|__I|__O)\s+)*'
           r'[A-Za-z_]\w*')
FUNC_RE = re.compile(
    r'^\s*(?P<pre>(?:(?:static|inline|extern|__STATIC_INLINE|__STATIC_FORCEINLINE|'
    r'__weak|__WEAK|__INLINE)\s+)*)'
    r'(?P<ret>' + RET_TOK + r')(?P<gap>\s*\*\s*|\s+)(?P<name>[A-Za-z_]\w*)\s*\((?P<params>.*)$')
TYPEDEF_RE = re.compile(r'^(\s*)typedef\s+(struct|enum|union)\b')
DEF_RE = re.compile(r'^\s*#\s*define\s+([A-Za-z_]\w*)')
GUARD_RE = re.compile(r'^[A-Z][A-Z0-9_]*_H_?$')

ITEMS = [('N1', 'N1 全局函数命名（返回类型小写前缀 + 模块名 + _动宾）'),
         ('N2', 'N2 静态（局部）函数命名（全小写下划线）'),
         ('N3', 'N3 返回值首前缀与规则表不符（v/b/t/e/u/pv/p/c/uc/s/us/l/ul/f/d/u64；'
                'uint32_t→ul，ui 已废弃）'),
         ('N4', 'N4 静态全局变量命名（应以 S_ 或 s_ 开头；结构体/枚举/联合变量走 N11/N12/N15）'),
         ('N5', 'N5 全局变量命名（应以 G_ 开头；结构体/枚举/联合变量走 N11/N12/N15）'),
         ('N6', 'N6 全局指针命名（应以 P_ 开头）'),
         ('N7', 'N7 静态局部变量命名（应以 s_ 开头）'),
         ('N8', 'N8 指针 * 与变量名之间不得有空格'),
         ('N9', 'N9 typedef struct 类型名（应以 _T 结尾；小写 _t 存量入 N14）'),
         ('N10', 'N10 typedef enum 类型名（应以 _E 结尾；小写 _t 存量入 N14）'),
         ('N11', 'N11 结构体变量命名（应以 t 开头大驼峰，不再叠加 G_/S_/s_）'),
         ('N12', 'N12 枚举变量命名（应以 e 开头大驼峰，不再叠加 G_/S_/s_）'),
         ('N13a', 'N13a 宏名全小写（应为 小写模块前缀 + 全大写下划线）'),
         ('N13b', 'N13b 宏名含「大写+小写」段（应为 小写模块前缀 + 全大写下划线）'),
         ('N14', 'N14 存量 _t 类型（规则允许保留，仅提示；不计入违规）'),
         ('N15', 'N15 联合类型名（应以 _U 结尾）/ 联合变量命名（应以 u 开头大驼峰）')]


# ------------------------------------------------------------------ 基础函数 --
def load(path):
    """读 UTF-8（含 BOM）文件；非 UTF-8 打印跳过并返回 None"""
    with open(path, 'rb') as f:
        raw = f.read()
    try:
        text = raw.decode('utf-8-sig')
    except UnicodeDecodeError:
        sys.stderr.write('SKIP (not utf-8): %s\n' % path)
        return None
    lines = text.replace('\r\n', '\n').replace('\r', '\n').split('\n')
    if lines and lines[-1] == '':
        lines.pop()
    return lines


def strip_cmt_str(lines):
    """字符状态机：把注释与字符串/字符字面量替换成等长空格（保留 Tab 与列位置）"""
    out = []
    state = None                                  # None | 'block' | 'line' | 'str' | 'chr'
    for line in lines:
        buf = []
        i, n = 0, len(line)
        while i < n:
            ch = line[i]
            if state is None:
                if ch == '/' and i + 1 < n and line[i + 1] == '*':
                    buf.append('  ')
                    i += 2
                    state = 'block'
                    continue
                if ch == '/' and i + 1 < n and line[i + 1] == '/':
                    buf.append('  ')
                    i += 2
                    state = 'line'
                    continue
                if ch == '"':
                    buf.append(' ')
                    i += 1
                    state = 'str'
                    continue
                if ch == "'":
                    buf.append(' ')
                    i += 1
                    state = 'chr'
                    continue
                buf.append(ch)
                i += 1
                continue
            if state == 'line':
                buf.append(' ' if ch != '\t' else '\t')
                i += 1
                continue
            if state == 'block':
                if ch == '*' and i + 1 < n and line[i + 1] == '/':
                    buf.append('  ')
                    i += 2
                    state = None
                    continue
                buf.append(' ' if ch != '\t' else '\t')
                i += 1
                continue
            # 字符串 / 字符字面量
            if ch == '\\' and i + 1 < n:
                buf.append('  ')
                i += 2
                continue
            if (state == 'str' and ch == '"') or (state == 'chr' and ch == "'"):
                buf.append(' ')
                i += 1
                state = None
                continue
            buf.append(' ' if ch != '\t' else '\t')
            i += 1
        out.append(''.join(buf))
        if state == 'line':                       # 行注释不跨行
            state = None
    return out


def classify(cl):
    """返回 (depth_before, paren_before, in_func, in_type)：行首花括号深度 / 括号深度 / 是否在函数体内 / 是否在类型体内"""
    n = len(cl)
    depth_before, paren_before = [0] * n, [0] * n
    in_func, in_type = [False] * n, [False] * n
    stack, paren, hdr = [], 0, []
    for i in range(n):
        depth_before[i] = len(stack)
        paren_before[i] = paren
        in_func[i] = 'f' in stack
        in_type[i] = 't' in stack
        for ch in cl[i]:
            if ch == '(':
                paren += 1
                hdr.append(ch)
            elif ch == ')':
                paren = max(0, paren - 1)
                hdr.append(ch)
            elif ch == '{':
                head = ''.join(hdr)
                if 'typedef' in head and re.search(r'\b(struct|enum)\b', head):
                    stack.append('t')
                elif '(' in head and '=' not in head:
                    stack.append('f')
                else:
                    stack.append('b')
                hdr = []
            elif ch == '}':
                if stack:
                    stack.pop()
                hdr = []
            elif ch == ';':
                hdr = []
            else:
                hdr.append(ch)
    return depth_before, paren_before, in_func, in_type


def star_attached(cline, m):
    """`*` 是否紧贴变量名：name 组前一个字符必须就是 *（中间不允许空格）"""
    p = m.start('name') - 1
    return p >= 0 and cline[p] == '*'


def name_prefix(name):
    """取名字的首小写前缀：cI2C_WriteData->c、usFunc_SwapU16->us、chk_add8->chk；全大写开头返回 None"""
    m = re.match(r'^([a-z][a-z0-9]*)(?=[A-Z_]|$)', name)
    return m.group(1) if m else None


def norm_ret(ret):
    """返回 (基础类型, 是否指针)"""
    r = ret
    for kw in ('const', 'volatile', 'register', '_IO', '__IO', '__I', '__O'):
        r = re.sub(r'\b%s\b' % kw, ' ', r)
    ptr = '*' in r
    r = re.sub(r'^(struct|union|enum)\s+', '', r.replace('*', ' ').strip())
    return ' '.join(r.split()), ptr


def expect_prefix(base, ptr):
    """规则表里该返回类型应有的首前缀；返回类型取不出时返回 None（单独归类，不算 N3 违规）"""
    if ptr:
        return 'pv' if base == 'void' else 'p'       # pv=void*，p=其它指针
    if (base, False) in RET_MAP:
        return RET_MAP[(base, False)]
    if base.endswith('_T'):
        return 't'
    if base.endswith('_E'):
        return 'e'
    if base.endswith('_U'):
        return 'u'
    return None


def is_excluded_name(name):
    if name in EXCL_NAMES:
        return True
    if name.startswith('_'):                       # _sys_exit / __aeabi_* / 启动文件符号
        return True
    if name.endswith('_IRQHandler') or name.endswith('_Handler'):
        return True
    return False


# ------------------------------------------------------------------ 单文件体检 --
def analyze(rel, o_lines, is_wizard, fnptr_types, legacy_t):
    cl = strip_cmt_str(o_lines)
    n = len(cl)
    depth_before, paren_before, in_func, in_type = classify(cl)
    res = OrderedDict((k, []) for k, _ in ITEMS)
    stats = Counter()                              # (返回类型, 实际前缀) -> 次数
    unres = []                                     # 取不出合规前缀：返回类型无法识别 / 函数名无小写前缀
    seen_func = set()

    def add(key, i, name, extra=''):
        res[key].append((rel, i + 1, name, o_lines[i].strip()[:60], extra))

    # ---- 变量 / 指针 / 类型变量（N4~N8、N11、N12）----
    for i in range(n):
        s = cl[i]
        if not s.strip() or s.lstrip().startswith('#') or paren_before[i] > 0:
            continue
        if 'typedef' in s:
            continue
        m = DECL_RE.match(s)
        if not m:
            continue
        pre = m.group('pre') or ''
        typ = m.group('type')
        name = m.group('name')
        ptr = '*' in m.group('gap')
        if typ in ('struct', 'union', 'enum') or typ in KEYWORDS:
            continue                               # `struct __FILE` 之类的类型头，不是变量声明
        if typ.isupper() and not re.search(r'_(T|E|U)$', typ):
            continue                               # 形如 MODULE_REGISTER(x, y) 的宏调用，不是声明
        if 'extern' in pre:
            continue                               # extern 只声明不定义，不算本文件命名
        if not (depth_before[i] == 0 or in_func[i] or in_type[i]):
            continue                               # 初始化列表内部
        if depth_before[i] == 0 and not in_func[i] and not in_type[i]:
            scope = 'file'
        elif in_type[i]:
            scope = 'type'
        elif in_func[i]:
            scope = 'local'
        else:
            scope = 'other'
        if scope == 'other':
            continue
        # 3.3 类型后缀统一 _T/_E/_U；历史小写 _t 类型同样按类型前缀判（类型名本身归 N14 仅提示）
        if typ in fnptr_types:
            kind = None
        elif typ.endswith('_T'):
            kind = 'struct'
        elif typ.endswith('_E'):
            kind = 'enum'
        elif typ.endswith('_U'):
            kind = 'union'
        elif typ in legacy_t:
            kind = legacy_t[typ]                 # 本工程内的存量小写 _t 类型（外部库如 TaskHandle_t 不在此列）
        else:
            kind = None
        if ptr:
            if scope == 'file':
                if not name.startswith('P_'):
                    add('N6', i, name)
                if not star_attached(s, m):
                    add('N8', i, name)
            elif scope == 'local' and 'static' in pre and not star_attached(s, m):
                add('N8', i, name)                 # static 变量才查 * 贴名
            continue
        # 3.3 前缀优先级：文件作用域的结构体/枚举/联合变量一律用类型前缀 t/e/u，不再叠加 G_/S_/s_
        if kind == 'struct':
            if not re.match(r'^t[A-Z]', name) and name != typ[:-2]:
                add('N11', i, name)
            continue
        if kind == 'enum':
            if not re.match(r'^e[A-Z]', name) and name != typ[:-2]:
                add('N12', i, name)
            continue
        if kind == 'union':
            if not re.match(r'^u[A-Z]', name) and name != typ[:-2]:
                add('N15', i, name)
            continue
        if scope == 'file':
            if 'static' in pre:
                if not (name.startswith('S_') or name.startswith('s_')):   # 3.2 S_/s_ 均可
                    add('N4', i, name)
            else:
                if not name.startswith('G_'):
                    add('N5', i, name)
        elif scope == 'local' and 'static' in pre:
            if not name.startswith('s_'):
                add('N7', i, name)

    # ---- 函数（N1/N2/N3）：只看文件作用域 ----
    for i in range(n):
        if depth_before[i] != 0 or paren_before[i] != 0 or in_func[i] or in_type[i]:
            continue
        s = cl[i]
        if not s.strip() or s.lstrip().startswith('#'):
            continue
        m = FUNC_RE.match(s)
        if not m:
            continue
        name = m.group('name')
        ret = m.group('ret').strip()
        pre = m.group('pre') or ''
        if name in KEYWORDS or ret.split()[0] in KEYWORDS:
            continue
        if is_excluded_name(name):
            continue
        if '__weak' in pre or '__WEAK' in pre:
            continue                               # __weak 重写函数不判定
        if (rel, name) in seen_func:               # 同一文件同名只报一次（原型+定义）
            continue
        seen_func.add((rel, name))
        is_static = 'static' in pre or '__STATIC_INLINE' in pre or '__STATIC_FORCEINLINE' in pre
        if is_static:
            if not re.match(r'^[a-z][a-z0-9]*_[a-z0-9_]+$', name):
                add('N2', i, name)
        elif not re.match(r'^[a-z][a-z0-9]*[A-Z][A-Za-z0-9]*_[A-Za-z0-9]+$', name):
            add('N1', i, name)
        base, ptr = norm_ret(ret)
        actual = name_prefix(name)
        exp = expect_prefix(base, ptr)
        stats[(ret, actual or '<无小写前缀>')] += 1
        if exp is None:
            unres.append((rel, i + 1, name, '返回类型无法识别: %s' % ret))
        elif actual is None:
            unres.append((rel, i + 1, name, '函数名无小写前缀: %s' % name))
        elif actual != exp:
            add('N3', i, name, '(返回类型=%s 期望前缀=%s 实际前缀=%s)' % (ret, exp, actual))

    # ---- typedef struct / enum / union（N9/N10/N14/N15）----
    for i in range(n):
        mt = TYPEDEF_RE.match(cl[i])
        if not mt:
            continue
        kind, name = mt.group(2), None
        depth, saw_brace, done = 0, False, False
        for j in range(i, n):
            c0 = mt.end() if j == i else 0
            for ci in range(c0, len(cl[j])):
                ch = cl[j][ci]
                if ch == '{':
                    depth += 1
                    saw_brace = True
                elif ch == '}':
                    depth -= 1
                    if saw_brace and depth == 0:
                        mm = re.search(r'[A-Za-z_]\w*', cl[j][ci + 1:])
                        if mm:
                            name = mm.group(0)
                        else:
                            for k in range(j + 1, min(j + 4, n)):
                                mm2 = re.search(r'^[\s\*]*([A-Za-z_]\w*)', cl[k])
                                if mm2:
                                    name = mm2.group(1)
                                    break
                                if ';' in cl[k]:
                                    break
                        done = True
                        break
                elif ch == ';' and depth == 0:
                    toks = re.findall(r'[A-Za-z_]\w*', cl[j][:ci])
                    if len(toks) >= 3:
                        name = toks[-1]
                    done = True
                    break
            if done:
                break
        if not name:
            continue
        if kind == 'struct':
            if name.endswith('_T'):
                pass
            elif name.endswith('_t'):
                add('N14', i, name, '(存量小写 _t 结构体类型，规则允许保留，仅提示)')
            else:
                add('N9', i, name, '(结构体类型名应以 _T 结尾)')
        elif kind == 'enum':
            if name.endswith('_E'):
                pass
            elif name.endswith('_t'):
                add('N14', i, name, '(存量小写 _t 枚举类型，规则允许保留，仅提示)')
            else:
                add('N10', i, name, '(枚举类型名应以 _E 结尾)')
        else:
            if name.endswith('_U'):
                pass
            elif name.endswith('_t'):
                add('N14', i, name, '(存量小写 _t 联合类型，规则允许保留，仅提示)')
            else:
                add('N15', i, name, '(联合类型名应以 _U 结尾)')

    # ---- 宏（N13a/N13b）----
    if not is_wizard:
        for i in range(n):
            md = DEF_RE.match(cl[i])
            if not md:
                continue
            name = md.group(1)
            if name.startswith('_') or GUARD_RE.match(name):
                continue                           # 头文件保护宏跳过
            if not re.search(r'[A-Z]', name):
                add('N13a', i, name)
            elif re.search(r'[A-Z][a-z]', name):
                add('N13b', i, name)
    return res, stats, unres


# ------------------------------------------------------------------ 主流程 ----
def walk(root, dirs):
    for d in dirs:
        d = d.strip()
        if not d:
            continue
        base = os.path.join(root, d.replace('/', os.sep))
        if not os.path.isdir(base):
            sys.stderr.write('SKIP (no such dir): %s\n' % base)
            continue
        for dp, dns, fns in os.walk(base):
            dns[:] = sorted(x for x in dns if x.lower() not in SKIP_LOWER)
            for fn in sorted(fns):
                if fn.lower().endswith(('.c', '.h')):
                    yield os.path.join(dp, fn)


def main():
    if len(sys.argv) < 3:
        sys.stderr.write('用法: python name_check.py <项目根> <目录列表,逗号分隔>\n')
        return 2
    root = os.path.abspath(sys.argv[1].rstrip('\\/'))
    dirs = sys.argv[2].split(',')
    proj = os.path.basename(root)
    hits = OrderedDict((k, []) for k, _ in ITEMS)
    stats = Counter()
    nfiles, bad_files = 0, set()
    unres = []
    files = []
    for path in walk(root, dirs):
        if os.path.basename(path) in EXEMPT:
            continue
        o_lines = load(path)
        if o_lines is None:
            continue
        files.append((os.path.relpath(path, root), o_lines))
    # 预扫：收集函数指针 typedef 名（如 typedef void (*fnKeyAction_T)(void)）
    #       与历史小写 _t 类型名（kind: struct/enum/union，用于把存量 _t 归入 N14 而非 N9/N10）
    fnptr_types = set()
    legacy_t = {}
    typedef_body = re.compile(r'typedef\s+(struct|enum|union)\b[\s\S]*?\}\s*([A-Za-z_]\w*)\s*;')
    for rel, o_lines in files:
        cl = strip_cmt_str(o_lines)
        for line in cl:
            if 'typedef' in line:
                for mm in re.finditer(r'\(\s*\*\s*([A-Za-z_]\w*)\s*\)', line):
                    fnptr_types.add(mm.group(1))
        for mm in typedef_body.finditer('\n'.join(cl)):
            nm = mm.group(2)
            if nm.endswith('_t') and nm not in STD_TYPES:
                legacy_t[nm] = mm.group(1)
    for rel, o_lines in files:
        nfiles += 1
        is_wizard = any('Use Configuration Wizard in Context Menu' in x for x in o_lines)
        try:
            fres, fstats, funres = analyze(rel, o_lines, is_wizard, fnptr_types, legacy_t)
        except Exception as exc:                   # 单文件异常不影响整体
            sys.stderr.write('ERROR %s: %s\n' % (rel, exc))
            continue
        for k, items in fres.items():
            if items:
                hits[k].extend(items)
                if k != 'N14':                     # N14 存量 _t 仅提示，不计入「有违规文件数」
                    bad_files.add(rel)
        stats.update(fstats)
        unres.extend(funres)

    print('=== %s：%d 个文件，%d 个文件有违规 ===' % (proj, nfiles, len(bad_files)))
    for key, title in ITEMS:
        items = hits[key]
        nf = len(set(x[0] for x in items))
        print('\n--- [%d 处 / %d 个文件] %s' % (len(items), nf, title))
        for rel, ln, name, sn, extra in items[:10]:
            print('    %s:%d  %s  %s%s' % (rel.replace(os.sep, '\\'), ln, name, sn,
                                           ('  ' + extra) if extra else ''))
        if len(items) > 10:
            print('    … 另 %d 处' % (len(items) - 10))
    print('\n--- [%d 处 / %d 个文件] N3x 取不出合规返回值前缀（单独归类，不计违规）'
          % (len(unres), len(set(x[0] for x in unres))))
    rc = Counter(x[3].split(':')[0] for x in unres)
    print('    原因分类: %s' % ('；'.join('%s=%d' % (k, v) for k, v in rc.most_common()) or '(无)'))
    for rel, ln, name, reason in unres[:10]:
        print('    %s:%d  %s  %s' % (rel.replace(os.sep, '\\'), ln, name, reason))
    if len(unres) > 10:
        print('    … 另 %d 处' % (len(unres) - 10))
    print('\n--- [统计] N3 返回值前缀使用统计（3.1 新表：v/b/t/e/u/pv/p/c/uc/s/us/l/ul/f/d/u64；'
          'uint32_t→ul，ui 已废弃）')
    print('    %-24s %-18s %s' % ('返回类型', '实际前缀', '次数'))
    for (ret, pfx), c in sorted(stats.items(), key=lambda kv: (-kv[1], kv[0])):
        print('    %-24s %-18s %d' % (ret, pfx, c))
    tot = Counter()
    for (ret, pfx), c in stats.items():
        tot[pfx] += c
    print('    前缀合计: %s' % (', '.join('%s=%d' % (p, c) for p, c in tot.most_common()) or '(无)'))
    print('\nSUMMARY %s N1=%d N2=%d N4=%d N5=%d N6=%d N7=%d N8=%d N9=%d N10=%d '
          'N11=%d N12=%d N13a=%d N13b=%d N14=%d N15=%d'
          % (proj, len(hits['N1']), len(hits['N2']), len(hits['N4']), len(hits['N5']),
             len(hits['N6']), len(hits['N7']), len(hits['N8']), len(hits['N9']),
             len(hits['N10']), len(hits['N11']), len(hits['N12']), len(hits['N13a']),
             len(hits['N13b']), len(hits['N14']), len(hits['N15'])))
    return 0


if __name__ == '__main__':
    sys.exit(main())
