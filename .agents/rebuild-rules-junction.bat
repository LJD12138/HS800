@echo off
chcp 936 >nul
setlocal enabledelayedexpansion

rem ============================================================
rem 一键重建 BOOT / APP 工作区到 .agents 唯一信息源的目录联接
rem   规则: <工程>\.trae\rules   -> .agents\rules
rem   技能: <工程>\.agents\skills -> .agents\skills
rem 物理文件只保留在 .agents, 本脚本不复制任何文件
rem 用法: 可直接在 .agents 目录下双击运行，也支持在工程根目录运行
rem ============================================================

cd /d "%~dp0"

rem --- 自动识别目录层级（兼容 .agents 目录内部与工程根目录） ---
if exist "%~dp0rules\" (
    rem 脚本位于 .agents 目录中
    for %%I in ("%~dp0..") do set "ROOT_DIR=%%~fI"
    set "AGENTS_DIR=%~dp0"
) else if exist "%~dp0.agents\rules\" (
    rem 脚本位于工程根目录中
    for %%I in ("%~dp0.") do set "ROOT_DIR=%%~fI"
    set "AGENTS_DIR=%~dp0.agents\"
) else (
    goto :SRC_MISSING
)

set "RULES_SRC=%AGENTS_DIR%rules"
set "SKILLS_SRC=%AGENTS_DIR%skills"
set /a FAILCNT=0

if not exist "%RULES_SRC%\" goto :SRC_MISSING
if not exist "%SKILLS_SRC%\" goto :SRC_MISSING

call :LINK_ONE "BOOT"
call :LINK_ONE "APP"

echo.
echo ============================================================
if !FAILCNT! equ 0 (
    echo [成功] 全部联接已就绪:
    echo   BOOT\.trae\rules    和 APP\.trae\rules    -^> .agents\rules
    echo   BOOT\.agents\skills 和 APP\.agents\skills -^> .agents\skills
    echo 请在 IDE 中重新打开工作区窗口使配置生效。
) else (
    echo [完成但有失败] 失败项数量: !FAILCNT!, 请查看上方日志。
)
echo ============================================================
echo.
pause
exit /b !FAILCNT!

:LINK_ONE
set "PROJ=%~1"
set "PROJDIR=%ROOT_DIR%\%PROJ%"
echo.
echo ########## 工程: %PROJ% ##########
if not exist "%PROJDIR%\" (
    echo [跳过] 目录不存在: %PROJDIR%
    goto :eof
)

rem --- 规则联接 ---
set "RULES_DST=%PROJDIR%\.trae\rules"
if not exist "%PROJDIR%\.trae" mkdir "%PROJDIR%\.trae"
call :SAFE_RELINK "%RULES_DST%" "%RULES_SRC%"
if errorlevel 1 set /a FAILCNT+=1

rem --- 技能联接 ---
set "SKILLS_DST=%PROJDIR%\.agents\skills"
if not exist "%PROJDIR%\.agents" mkdir "%PROJDIR%\.agents"
call :SAFE_RELINK "%SKILLS_DST%" "%SKILLS_SRC%"
if errorlevel 1 set /a FAILCNT+=1

goto :eof

rem ------------------------------------------------------------
rem SAFE_RELINK "目标路径" "源路径"
rem 目标若是联接则删链接重建; 若是真实目录则拒绝, 返回 errorlevel 1
rem ------------------------------------------------------------
:SAFE_RELINK
set "DST=%~1"
set "SRC=%~2"
if not exist "%DST%\" goto :DO_MKLINK
fsutil reparsepoint query "%DST%" >nul 2>&1
if errorlevel 1 goto :REAL_DIR
echo   删除旧联接: %DST%
rmdir "%DST%"
if errorlevel 1 goto :RM_FAIL
:DO_MKLINK
echo   创建联接: %DST%
mklink /J "%DST%" "%SRC%" >nul
if errorlevel 1 goto :MK_FAIL
dir /b "%DST%" >nul 2>&1
echo   完成。
exit /b 0
:REAL_DIR
echo   [错误] 目标是真实目录而非联接, 为防误删已跳过:
echo          %DST%
exit /b 1
:RM_FAIL
echo   [错误] 旧联接删除失败, 请关闭占用程序后重试:
echo          %DST%
exit /b 1
:MK_FAIL
echo   [错误] 联接创建失败, 请检查 NTFS 与权限:
echo          %DST%
exit /b 1

:SRC_MISSING
echo [错误] 找不到源目录 rules 或 skills。
echo        当前脚本所在目录: %~dp0
echo        请确认 rules 和 skills 目录位于当前 .agents 目录下。
pause
exit /b 1
