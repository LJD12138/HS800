/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : boot_info.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Bootloader共享参数、版本信息与持久化配置声明
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef __BOOT_INFO_H
#define __BOOT_INFO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"
#include "Sys/sys_task.h"

#if (boardEASY_FLASH)
#include "easyflash.h"
#endif  /* boardEASY_FLASH */



//****************************************************Types*********************************************************************//
/* APP状态枚举 */
typedef enum
{
	AS_NULL = 0,		//未选择
	AS_FINISH,			//刚升级完成
	AS_OK,				//当前是完整的
	AS_ERASE,			//已经擦除
}AppState_E;

/* 固件版本与编译时间信息 (纯字符数组，共96字节，天然单字节对齐) */
typedef struct
{
	char				saVersion[32];		//软件版本
	char				saBuildDate[32];	//程序编译日期
	char				saBuildTime[32];	//程序编译时间
}VerInfo_T;

#pragma pack(4)  // 4字节自然对齐：与BOOT信息区及EasyFlash环境持久化严格对应
typedef struct
{
	vu32				ulCmd;				//0xAAAAAAAA需要升级,其他不需要升级
	AppState_E			eAppState;			//APP状态
	vu8					ucAppFaultCnt;		//APP启动失败计数
}BootParam_T;
#pragma pack()

/* BOOT共享参数内存镜像 (4字节自然对齐，共108字节) */
typedef struct
{
	VerInfo_T			tVerInfo;
	BootParam_T			tParam;
}BootMemParam_T;

//****************************************************Globals*******************************************************************//
extern BootMemParam_T tBootMemParam;

extern const char tBootMemParamStr[];
extern const char tBootVerInfoStr[];
extern const char tBootParamStr[];

#if (boardEASY_FLASH)
extern const ef_env default_env_set[];
#endif  /* boardEASY_FLASH */

//****************************************************Extern********************************************************************//
SysTaskId_E eBoot_InfoInit(bool init);
s8 cBoot_MemParamInit(const char* id_str);
s16 cBoot_UpdateMemParam(const char* id_str);
s16 cBoot_GetMemParam(const char* id_str);
u16 usBoot_GetMemParamSize(void);
bool bBoot_CmdExist(u32 cmd);

#if (boardUPDATE)
s8 cBoot_CtrlUpdate(bool en, AppState_E state);
#endif  /* boardUPDATE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __BOOT_INFO_H */
