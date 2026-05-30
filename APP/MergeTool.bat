:: 开/关显示cli信息
@echo off

::echo -----------------------------------------------------------------------------------------
::echo 合并bin工具MergeTool.Bat文件
::echo 使用步骤：
::echo 【1】 该文件需要放在APP的项目工程文件夹目录下。
::echo 【2】 在keil的options选项的user中After Build里填写$PMergeTool.BAT并勾选Run#2。
::echo 【3】 在项目工程BOOT\Application\flash_allot_table.h中修改修改flashBOOT_STACK_SIZE的大小。
::echo Data: 2023-05-13
::echo -----------------------------------------------------------------------------------------

echo -----------------------------------【一、 生成Bin文件】------------------------------------
::删除前APP.bin文件
del .\APP.bin


echo $K

::生成Bin文件
::$K\ARM\ARMCC\bin\fromelf.exe --bin --output=@L.bin !L
%1\ARM\ARMCC\bin\fromelf.exe --bin --output .\APP.bin .\Objects\APP.axf

::判断BIN文件是否存在
if not exist .\APP.bin (
	echo !!!!!!!APP.bin文件不存在!!!!!!!!
	goto failure
) else (
	echo *******成功**********
)

echo -----------------------------------【二、 获取工程文件】------------------------------------
::获取主工程的路径(APP+BOOT)
cd..
::获取当前路径
set ProjectPath=%cd%\

:: 软件版本文件路径及BOOT大小
set BootSizePath=%ProjectPath%BOOT\Application\flash_allot_table.h
set BootVersionPath=%ProjectPath%BOOT\Application\board_config.h
set AppVersionPath=%ProjectPath%APP\Application\board_config.h

::<-----定义指定工程下的app和boot的.bin文件路径----->
set AppFile=%ProjectPath%APP\APP.bin
set BootFile=%ProjectPath%BOOT\BOOT.bin

::判断文件是否存在
if not exist %BootSizePath% (
	echo !!!!!!!BOOT\Application\flash_allot_table.h文件不存在!!!!!!!!
	goto failure
)
if not exist %BootVersionPath% (
	echo !!!!!!!BOOT\Application\board_config.h文件不存在!!!!!!!!
	goto failure
)
if not exist %AppVersionPath% (
	echo !!!!!!!APP\Application\board_config.h文件不存在!!!!!!!!
	goto failure
)
if not exist %AppFile% (
	echo !!!!!!!APP\APP.bin文件不存在!!!!!!!!
	goto failure
)
if not exist %BootFile% (
	echo !!!!!!!BOOT\BOOT.bin文件不存在!!!!!!!!
	goto failure
) else (
	echo *******成功**********
)

::<-----打印出字段(调试使用)----------------->
::echo BOOT堆栈大小路径:%BootSizePath%
::echo BOOT软件版本路径:%BootVersionPath%
::echo APP软件版本路径:%AppVersionPath%
::echo Project目录:%ProjectPath%
::<-----打印出字段(调试使用)----------------->

:: 获取BOOT软件版本
for /f "tokens=3 delims= " %%i in ('findstr "boardSOFTWARE_VERSION" %BootVersionPath%') do set BootVer=%%i
set BootName=%BootVer:~1,-1%

:: 获取APP软件版本
for /f "tokens=3 delims= " %%i in ('findstr "boardSOFTWARE_VERSION" %AppVersionPath%') do set AppVer=%%i
set ProjectName=%AppVer:~1,-1%

:: 获取BOOT堆栈大小
for /f "tokens=3 delims= " %%i in ('findstr "flashBOOT_STACK_SIZE" %BootSizePath%') do set BootStack=%%i
set BootFlashSize=%BootStack%

::判断堆栈是否为空
if %BootFlashSize% == 0 (
	echo 堆栈不可以为0
	goto failure
)


::<-----打印出字段(调试使用)----------------->
::echo 获取BOOT软件版本:%BootName%
::echo 获取APP软件版本:%ProjectName%
::echo 获取BOOT堆栈大小:%BootFlashSize%
::<-----打印出字段(调试使用)----------------->


::<-----定义自动生成由时间组成的文件夹名称字段DataField，列:2020-11-23-11-31-28----->
set DataField=%date:~0,4%-%date:~5,2%-%date:~8,2%-%time:~0,2%-%time:~3,2%-%time:~6,2%

::<-----定义文件夹名称字段OutField=Output----->
set OutField=Output

::<-----定义Merge为最终生成的生产固件字段----->
::加/表示当下的目录
set MergeFile="%ProjectPath%%OutField%/%ProjectName%_Boot_App(%DataField%).bin"

::<-----打印出字段(调试使用)----------------->
::echo %AppFile%
::echo %BootFile%
::echo %MergeFile%
::<-----打印出字段(调试使用)----------------->


::如果存在,先删除
if exist %ProjectPath%%OutField%  rd /s /q %ProjectPath%%OutField%

::<-----创建目标文件夹Output，包含创建所需的文件夹----->
if not exist %ProjectPath%%OutField% mkdir %ProjectPath%%OutField%

echo -----------------------------------【三、 复制文件】--------------------------------------
::<-----将工程下的boot和app的bin文件分别复制到对应的文件夹----->
copy %AppFile% %ProjectPath%%OutField%
copy %BootFile% %ProjectPath%%OutField%

::修改文件名
ren %ProjectPath%%OutField%\BOOT.bin %BootName%_Boot.bin
ren %ProjectPath%%OutField%\APP.bin %ProjectName%_App.bin

::<-----准备boot文件 空bin文件撑开不需要写部分，size取决于app在flash起始地址之前的空间大小----->
set /a bootsize = %BootFlashSize%*1024
for %%a in (%BootFile%) do set /a size="%bootsize%"-%%~za
echo Boot文件大小：【%BootFlashSize% Kb】

fsutil file createnew temp.bin %size%

copy /b %BootFile% + temp.bin boot.bin


echo -----------------------------------【四、 合并文件】----------------------------------------
::<-----生成合并文件----->
copy /b boot.bin + %AppFile% %MergeFile%


::<-----删除临时文件----->
del temp.bin
del boot.bin

::<-----检查执行成功与否----->
if exist %MergeFile% (goto success) else goto failure

:success
echo ********************************************************************************************
echo *********************Merger success!合并并生成Bin文件成功***********************************
echo ********************************************************************************************

%ProjectPath%APP\Keil5_disp_size_bar_v0.4.exe

::Pause
exit

:failure
echo --------------------------------------------------------------------------------------------
echo !!!!!!!!!!!!!!!!!!!!!!Merger failure!合并并生成Bin文件失败!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
echo --------------------------------------------------------------------------------------------

::Pause
exit