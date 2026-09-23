---
name: c-style-align
description: 按项目 rules 检查或批量重排/修复 C 代码规范（struct/union、enum、#define 列对齐，文件头模板与段落横幅符合度）。Use when asked to align, unify, check or fix C layout and file-header template compliance, 排版对齐, 模板符合度, 规范检查.
---

# C 代码规范（排版对齐 + 文件模板符合度）

本技能只放**流程与脚本**；**口径、列位置、横幅宽度/标签集合一律以项目 rules 为准**，脚本从 rules 文件读取横幅与字段模板，改规则即自动跟随：

- `.agents/rules/c-coding-style.md`（2.1~2.4 排版与命名）
- `.agents/rules/c-code-templates.md`（文件头块、`.c`/`.h` 骨架、段落横幅、函数头注释）
- `.agents/rules/embedded-safety-rules.md`（变更安全准则）

规则真身只有一份（各工程的 `.trae\rules` 是指向它的 Junction），改口径只改 rules。

## 列标准与口径

以规则为准，脚本参数与规则条文的对应关系：

| 脚本模式 | 对应规则 | 默认列参数 |
| :--- | :--- | :--- |
| `check｜apply <名称列> <注释列>` | c-coding-style 2.2 结构体/联合体 | 24 / 44 |
| `enumcheck｜enumapply <注释列>` | 2.3 枚举 | 24 |
| `defcheck｜defapply <名称列> <取值列> <注释列>` | 2.4 `#define` | 16 / 56 / 64 |
| `tmpl_fix` / `tmpl_ban` / `tmpl_skel` | c-code-templates 全篇 | 从 rules 提取 |

脚本自身行为（规则里不写这些）：**只修列不对的行**（已合规行保留原填充风格）；排版段**只改空白**，遇到非空白差异立即中止；缩进统一为 Tab；换行符与 BOM 原样保留。

## 用法

脚本在 `scripts\style_align.py`（`<SKILL_DIR>` = 本 SKILL.md 所在目录）。

```powershell
# 三段一起干跑（只报告会改哪些行）
python "<SKILL_DIR>\scripts\style_align.py" allcheck "<项目根>" "<目录列表>"
# 三段一起应用
python "<SKILL_DIR>\scripts\style_align.py" allapply "<项目根>" "<目录列表>"
```

- `<项目根>`：如 `...\HS800\BOOT`、`...\HS800\APP`
- `<目录列表>`：逗号分隔的相对路径，可含子目录，如 `Application,ComFunc,Hardware,Middlewares/TaskQueue`
- 排除目录：环境变量（相对项目根、逗号分隔）

```powershell
$env:STRUCT_ALIGN_EXCLUDE='Hardware/MD_Display/eez_ui,Middlewares/LVGL'
```

- 单独某段：`check|apply`（结构体，可再跟 名称列 注释列）、`enumcheck|enumapply <注释列>`、`defcheck|defapply <名称列> <取值列> <注释列>`
- 调查现状：`stats`、`enumstats`、`definestats`、`indentstats`（列分布直方图）、`dump <文件关键字>`
- 验证：`python "<SKILL_DIR>\scripts\verify_align.py" <BOOT|APP>` 逐行比对工作区与 git 索引

## 执行流程

1. **定范围**：与用户确认 `<目录列表>` 与排除项。第三方库、官方 SDK、生成代码默认排除。
2. **干跑**：`allcheck` 看改动量与文件清单；若某文件行数异常多，先打开该文件确认没踩保护清单。
3. **记基线**：`git status --short -- <项目>` 确认「工作区相对索引没有额外的未暂存改动」，否则 `git diff` 基线不干净，先与用户确认（工作区相对索引无差异时，`git diff` 就只反映本次排版改动）。
4. **应用**：`allapply`。
5. **验证三项都要过**：
   - `git diff -w --numstat -- <项目>` 应**无源码输出**（证明纯空白改动）
   - `verify_align.py <BOOT|APP>` 应**无 `!!` 行**（无空白外差异、无行数变化）
   - 再跑一次 `allcheck`，应为 `files=0 lines=0`（幂等）
6. 报告：段 × 文件数 × 行数、跳过项、未提交状态。不要擅自 `git add`/commit。

## 保护清单

脚本已内置其中两条判定，其余需人工指定排除：

- **Keil `Configuration Wizard` 文件**（含 `<<< Use Configuration Wizard in Context Menu >>>`）：`#define` 段自动跳过，如 `Application\board_config.h`、`Application\FreeRTOSConfig.h`
- **被外部工具解析的宏**：`APP\MergeTool.bat` 用 `for /f "tokens=3 delims= "` 读取 `boardSOFTWARE_VERSION`、`flashBOOT_STACK_SIZE`，这些行必须保留**空格**分隔，禁止改 Tab
- **无值宏**（`#define XXX_H` 头文件保护等）：保持原样，不右移到第 16 列
- **函数式宏**：`NAME(params)` 视为一个整体，参数表内部不参与对齐（禁止用 Tab 撑开）
- **生成/第三方代码**：`eez_ui`、`eez_project`、`LVGL`、`FreeRTOS`、`EasyFlash`、`EasyLogger`、`CmBacktrace`、`lwrb`、`SeggerRtt`、`MultiFuncKey`、`Firmware`(GD32 官方库)、`.cmsis`、`.pack`、`RTE`

## 已知约束

- 只处理 UTF-8 文件；非 UTF-8 文件跳过并在 stderr 打印 `SKIP (not utf-8)`
- 多词类型（`const char`、`unsigned int`、`struct X`）合并为一个类型整体参与对齐
- 行尾换行符（LF/CRLF）与 BOM 原样保留；不处理函数体内部、语句级排版
- 典型效果（`#define`）：`#define\t\t\tADC_DMAX\t\t\t\t\t\t\t\t2`

## 文件模板符合度（c-code-templates.md）

脚本（`<SKILL_DIR>` = 本 SKILL.md 所在目录；横幅字符串与字段模板直接从 rules 文件提取，改规则自动跟随）：

```powershell
# 1) 审计：逐文件列出不符合项（按类别汇总 + 按文件明细）
python "<SKILL_DIR>\scripts\tmpl_audit.py" "<项目根>" "<目录列表>"
# 2) 批改头块：先干跑（不加 --apply 只打印计划），确认后加 --apply 写盘
python "<SKILL_DIR>\scripts\tmpl_fix.py" "<项目根>" "<目录列表>" [--apply]
# 3) 批改段落横幅与 #endif 注释（口径以 rules 为准）
python "<SKILL_DIR>\scripts\tmpl_ban.py" "<项目根>" "<目录列表>" [--apply]
# 4) 完整性校验：除「头块 + 段落横幅 + #endif 注释」外，其余行必须与 git 索引版本逐行一致
python "<SKILL_DIR>\scripts\tmpl_verify.py" "<仓库根>"
# 5) 旧式函数头注释块（-----函数功能 …）就地规范化为 3.1 标准块（内容原样保留，先干跑）
python "<SKILL_DIR>\scripts\funcblock_norm.py" "<项目根>" "<目录列表>" [--apply]
```

**A 类（脚本可自动修）**：头块字段值与缺失字段（`Project`/`Module`/`File`/`Date`/`Author`/`Desc`/`todo`/`Copyright`）、装饰性头框或 `-----` 旧式块重建、无头块文件补标准块、头块与段落横幅长度归一、`.c` 横幅统一 `//****...****//`、`#endif` 注释改条件名。

**口径**：一律以 `.agents/rules/c-code-templates.md` 为准（`.c`/`.h` 骨架、段落横幅风格与宽度、标签集合、`#endif` 命名、行内注释）。`tmpl_ban.py` 内置 `LABEL_MAP` 把历史标签按语义归并到规范集合、归不进的横幅行直接删除；横幅字符串与头块字段模板由脚本从 rules 提取，**改规则即自动跟随**。

**尚未自动处理**：**缺**函数头注释块（3.1，需按代码逐函数撰写内容，不可编造）；另有少量特殊情况需人工看（如 `#endif` 注释与实际条件/嵌套不符的 HAL 移植遗留文件、注释块挂在已注释掉的死代码上）。

**`funcblock_norm.py`（旧式函数块规范化）**：把 `-----函数功能 …` 旧式块就地转成 3.1 标准块——字段名映射（`函数功能/说明(备注)/传入参数/输出参数/返回值`）、缩进续行改为 ` *` + 15 空格、首尾横幅取自 rules 3.1、冒号按**显示宽度**对齐到第 12 列；注释文字原样保留。判定与审计同一套（`attached_comments`）。内置断言：**剥离注释后的代码特征串必须完全一致**，否则跳过写盘；已标准的块跳过（幂等）。

**批改注意**：
- 只动头块内部、横幅行与 `#endif` 注释，其余行字节不变（脚本内置断言 + `tmpl_verify.py` 复核）
- `Desc` 无法自动生成时插入占位 `[文件功能描述]`，须人工回填（审核时列出清单）
- 生成文件/Keil 配置向导文件默认跳过（`gd32f50x_it.c/.h`、`gd32f50x_libopt.h`、`FreeRTOSConfig.h`、`board_config.c/.h`）
- 保留头块内的自定义段（如 API 规约表）与原有 Doxygen 注释，不覆盖
- 报告 A 类结果与"豁免待确认"项；不要擅自 `git add`/commit

## 代码规范符合度（c-coding-style.md）

```powershell
# 2.1 排版/控制结构体检（只读）
python "<SKILL_DIR>\scripts\style_check.py" "<项目根>" "<目录列表>"
# 2.1 只修 A 类：块内 #if 缩进 + #endif 补条件注释（先干跑看预计量，再加 --apply）
python "<SKILL_DIR>\scripts\style_check.py" "<项目根>" "<目录列表>" [--apply]
# 3.x 命名规范体检（只读）
python "<SKILL_DIR>\scripts\name_check.py" "<项目根>" "<目录列表>"
```

**`style_check.py` 检查项**：`A1` 块内条件编译顶格、`A2` 块内 `#endif` 无行尾注释、`B` 单语句分支却带大括号、`C1~C4` switch-case 大括号/break 位置/疑似 fallthrough。
`--apply` **只修 A1/A2**，缩进口径是「抄它包裹的那行代码的行首空白」（Tab / 4 空格 / 8 空格自动贴合），`#endif` 注释取配对条件的首个标识符；写盘前做「剥离注释后代码特征串一致」断言，幂等。

**`name_check.py` 检查项**：`N1` 全局函数名形状、`N2` 静态函数名、`N4~N8` 变量/指针前缀、`N9/N10/N15` 类型后缀（`_T/_E/_U`）、`N11/N12/N15` 结构体/枚举/联合变量前缀、`N13a/b` 宏名；`N3` 出返回值前缀统计、`N14` 单列"规则允许保留的存量 `_t`"（不计违规）。
口径全部来自 rules 第 3 节（含：返回类型前缀表、静态全局 `S_`/`s_` 均可、**类型前缀优先**——文件作用域结构体/枚举/联合变量只用 `t`/`e`/`u`，不叠加 `G_`/`S_`/`s_`）。

> 两个脚本均**只读**（`--apply` 除外）、可重复运行结果一致、沿用同一套 `SKIP_DIRS`/`EXEMPT`；`B/C1` 两类改动量大且会动代码结构，**不自动修**，只出清单。
