:: Close display of cli info
@echo off

::echo -----------------------------------------------------------------------------------------
::echo Merge bin file MergeTool.Bat
::echo Steps:
::echo (1) The file must be in the APP project root directory.
::echo (2) In Keil Options -> User -> After Build, write $PMergeTool.BAT, check Run #2.
::echo (3) Open BOOT\Application\flash_allot_table.h and modify flashBOOT_STACK_SIZE.
::echo Date: 2023-05-13
::echo -----------------------------------------------------------------------------------------

echo -----------------------------------Step 1: Generate Bin File------------------------------------
:: Delete previous APP.bin file/directory
if exist .\APP.bin\ (
	rd /s /q .\APP.bin
) else if exist .\APP.bin (
	del /f /q .\APP.bin
)

echo %1

:: Generate Bin file
:: Auto-detect ARMCLANG (AC6) or ARMCC (AC5) fromelf utility
set FROMELF_EXE=""
if exist "%~1\ARM\ARMCLANG\bin\fromelf.exe" (
	set FROMELF_EXE="%~1\ARM\ARMCLANG\bin\fromelf.exe"
) else (
	set FROMELF_EXE="%~1\ARM\ARMCC\bin\fromelf.exe"
)

%FROMELF_EXE% --bincombined --output .\APP.bin .\Objects\APP.axf

:: Check if BIN file exists
if not exist .\APP.bin (
	echo !!!!!!!APP.bin file does not exist!!!!!!!!
	goto failure
) else (
	echo *******Success**********
)

echo -----------------------------------Step 2: Read Files------------------------------------
:: Get project path (APP+BOOT)
cd..
:: Get current path
set ProjectPath=%cd%\

:: Version file paths
set BootSizePath=%ProjectPath%BOOT\Application\flash_allot_table.h
set BootVersionPath=%ProjectPath%BOOT\Application\board_config.h
set AppVersionPath=%ProjectPath%APP\Application\board_config.h

:: Set new app and boot .bin file paths
set AppFile=%ProjectPath%APP\APP.bin
set BootFile=%ProjectPath%BOOT\BOOT.bin

:: Check if files exist
if not exist %BootSizePath% (
	echo !!!!!!!BOOT\Application\flash_allot_table.h does not exist!!!!!!!!
	goto failure
)
if not exist %BootVersionPath% (
	echo !!!!!!!BOOT\Application\board_config.h does not exist!!!!!!!!
	goto failure
)
if not exist %AppVersionPath% (
	echo !!!!!!!APP\Application\board_config.h does not exist!!!!!!!!
	goto failure
)
if not exist %AppFile% (
	echo !!!!!!!APP\APP.bin does not exist!!!!!!!!
	goto failure
)
if not exist %BootFile% (
	echo !!!!!!!BOOT\BOOT.bin does not exist!!!!!!!!
	goto failure
) else (
	echo *******Success**********
)

:: Get BOOT software version
for /f "tokens=3 delims= " %%i in ('findstr "boardSOFTWARE_VERSION" %BootVersionPath%') do set BootVer=%%i
set BootName=%BootVer:~1,-1%

:: Get APP software version
for /f "tokens=3 delims= " %%i in ('findstr "boardSOFTWARE_VERSION" %AppVersionPath%') do set AppVer=%%i
set ProjectName=%AppVer:~1,-1%

:: Get BOOT stack size
for /f "tokens=3 delims= " %%i in ('findstr "flashBOOT_STACK_SIZE" %BootSizePath%') do set BootStack=%%i
set /a BootFlashSize=%BootStack% >nul

:: Check if stack size is 0
if %BootFlashSize% == 0 (
	echo Stack size is 0
	goto failure
)

:: Auto-generate folder timestamp DataField: 2020-11-23-11-31-28
set DataField=%date:~0,4%-%date:~5,2%-%date:~8,2%-%time:~0,2%-%time:~3,2%-%time:~6,2%

:: Output folder field
set OutField=Output

:: Merge output file path
set MergeFile="%ProjectPath%%OutField%/%ProjectName%_Boot_App(%DataField%).bin"

:: Delete output folder if exists
if exist %ProjectPath%%OutField% rd /s /q %ProjectPath%%OutField%

:: Create output directory
if not exist %ProjectPath%%OutField% mkdir %ProjectPath%%OutField%

echo -----------------------------------Step 3: Copy Files--------------------------------------
:: Copy boot and app bin files to output directory
copy %AppFile% %ProjectPath%%OutField%
copy %BootFile% %ProjectPath%%OutField%

:: Rename files
ren %ProjectPath%%OutField%\BOOT.bin %BootName%_Boot.bin
ren %ProjectPath%%OutField%\APP.bin %ProjectName%_App.bin

:: Prepare boot file with padding
set /a bootsize = %BootFlashSize%*1024
for %%a in (%BootFile%) do set /a size="%bootsize%"-%%~za
echo Boot file size limit: %BootFlashSize% Kb

powershell -Command "[System.IO.File]::WriteAllBytes('temp.bin', (New-Object byte[] %size%))"

copy /b %BootFile% + temp.bin boot.bin


echo -----------------------------------Step 4: Merge Files----------------------------------------
:: Create merged file
copy /b boot.bin + %AppFile% %MergeFile%


:: Delete temporary files
del temp.bin
del boot.bin

:: Check if merge succeeded
if exist %MergeFile% (goto success) else goto failure

:success
echo ********************************************************************************************
echo *********************Merger success! Merged Bin file successfully***************************
echo ********************************************************************************************

%ProjectPath%APP\Keil5_disp_size_bar_v0.4.exe

exit /b 0

:failure
echo --------------------------------------------------------------------------------------------
echo !!!!!!!!!!!!!!!!!!!!!!Merger failure! Merged Bin file failed!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo --------------------------------------------------------------------------------------------

exit /b 1