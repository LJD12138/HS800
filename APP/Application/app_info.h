/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : app_info.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : APP运行信息、固件版本、持久化记忆参数定义与交互接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef APP_INFO_H_
#define APP_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#include "Sys/sys_task.h"

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  /* boardDC_EN */

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  /* boardUSB_EN */

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  /* boardBMS_EN */

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  /* boardMPPT_EN */

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  /* boardDCAC_EN */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

//****************************************************Macros********************************************************************//
/* APP参数初始化完成标志值:写入tAppMemParam.tParam.usInitFinish表示已完成出厂配置 */
#define			appPARAM_INIT_MAGIC						0xAAAA

//****************************************************Types*********************************************************************//
/* APP运行与升级状态枚举（存储于BOOT信息区） */
typedef enum
{
	AS_NULL = 0,		/* 未选择 */
	AS_FINISH,			/* 刚升级完成 */
	AS_OK,				/* 当前完整可用 */
	AS_ERASE,			/* 已经擦除 */
}AppState_E;

/* 记忆参数操作统一返回码 */
typedef enum
{
	APPINFO_ERR_ID = -99,	/* 未知的参数对象ID */
	APPINFO_ERR_NULL = -1,	/* 传入空指针 */
	APPINFO_ERR_INIT = -2,	/* 出厂默认值初始化失败 */
	APPINFO_ERR_WRITE = -3,	/* ENV/Flash写入失败 */
	APPINFO_ERR_READ = -4,	/* ENV读取长度不匹配(数据损坏) */
	APPINFO_ERR_FLASH = -5,	/* 裸Flash擦/写/读失败 */
	APPINFO_UNINIT = 0,		/* ENV中无该键(未初始化,合法状态,仅Get返回) */
	APPINFO_OK = 1,			/* 操作成功 */
}AppInfoErr_E;

/* 固件版本与编译时间信息 (纯字符数组，共96字节，天然单字节对齐) */
typedef struct
{
	char				saVersion[32];		/* 软件版本 */
	char				saBuildDate[32];	/* 程序编译日期 */
	char				saBuildTime[32];	/* 程序编译时间 */
}VerInfo_T;

#pragma pack(4)  /* 4字节自然对齐：与BOOT信息区及EasyFlash环境持久化严格对应 */
typedef struct
{
	vu32				ulCmd;				/* 0xAAAA_AAAA需要升级, 其他不需要升级 */
	AppState_E			eAppState;			/* APP状态 */
	vu8					ucAppFaultCnt;		/* APP启动失败计数 */
}BootParam_T;
#pragma pack()

/* APP参数：初始化标志与芯片唯一ID (共14字节，天然2字节对齐) */
typedef struct
{
	uint16_t			usInitFinish;		/* APP参数初始化完成标志 (appPARAM_INIT_MAGIC代表已完成) */
	uint16_t			usUniqueID[6];		/* 芯片全局唯一ID (12字节) */
}AppParam_T;

/* BOOT共享参数内存镜像 (4字节自然对齐，共108字节) */
typedef struct
{
	VerInfo_T			tVerInfo;			/* 版本与编译时间信息 (96字节) */
	BootParam_T			tParam;				/* BOOT引导升级参数 (12字节) */
}BootMemParam_T;

typedef struct
{
	VerInfo_T			tVerInfo;			/* 版本与编译时间信息 (96字节) */
	AppParam_T			tParam;				/* 初始化标志与芯片UID (14字节) */
}AppVerAndParam_T;

/* APP全部外设与系统持久化参数RAM镜像集合 (4字节自然对齐) */
typedef struct
{
	VerInfo_T			tVerInfo;			/* 固件版本信息 */
	AppParam_T			tParam;				/* 初始化标志与UID */
	#if (boardDISPLAY_EN)
	DispMemParam_T		tDISP;				/* 显示模块参数 */
	#endif  /* boardDISPLAY_EN */
	#if (boardDC_EN)
	DcMemParam_T		tDC;				/* DC模块参数 */
	#endif  /* boardDC_EN */
	#if (boardUSB_EN)
	UsbMemParam_T		tUSB;				/* USB模块参数 */
	#endif  /* boardUSB_EN */
	#if (boardBMS_EN)
	BmsMemParam_T		tBMS;				/* BMS模块参数 */
	#endif  /* boardBMS_EN */
	#if (boardMPPT_EN)
	MpptMemParam_T		tMPPT;				/* MPPT模块参数 */
	#endif  /* boardMPPT_EN */
	#if (boardDCAC_EN)
	DcacMemParam_T		tDCAC;				/* 逆变器模块参数 */
	#endif  /* boardDCAC_EN */
	SysMemParam_T		tSYS;				/* 系统运行参数 */
}AppMemParam_T;

//****************************************************Globals*******************************************************************//
extern BootMemParam_T                   tBootMemParam;
extern AppMemParam_T                    tAppMemParam;

extern const char                       tAppMemParamStr[];
extern const char                       tAppVerInfoStr[];
extern const char                       tAppParamStr[];
extern const char                       tAppVerAndParamStr[];
#if (boardDISPLAY_EN)
extern const char                       tDispMemParamStr[];
#endif  /* boardDISPLAY_EN */
#if (boardDC_EN)
extern const char                       tDcMemParamStr[];
#endif  /* boardDC_EN */
#if (boardUSB_EN)
extern const char                       tUsbMemParamStr[];
#endif  /* boardUSB_EN */
#if (boardBMS_EN)
extern const char                       tBmsMemParamStr[];
#endif  /* boardBMS_EN */
#if (boardMPPT_EN)
extern const char                       tMpptMemParamStr[];
#endif  /* boardMPPT_EN */
#if (boardDCAC_EN)
extern const char                       tDcacMemParamStr[];
#endif  /* boardDCAC_EN */
extern const char                       tSysMemParamStr[];

#if (boardEASY_FLASH)
#include "ef_def.h"
extern const ef_env                     default_env_set[];
#endif  /* boardEASY_FLASH */

//****************************************************Extern********************************************************************//
/* 统一返回码见 AppInfoErr_E:APPINFO_OK(>0)成功 / APPINFO_UNINIT(0)未初始化 / APPINFO_ERR_*(<0)失败 */
void vApp_JumpToBoot(uint32_t ul_cmd);
s8 cApp_BootInfoInit(void);
s8 cApp_AppInfoInit(void);                          /* 1:参数已存在  2:刚完成出厂初始化  <0:失败 */
s8 cApp_MemParamInit(const char *p_id_str);         /* 将RAM镜像置为出厂默认值 */
s8 cApp_UpdateMemParam(const char *p_id_str);       /* 将RAM镜像写入ENV/Flash */
s8 cApp_GetMemParam(const char *p_id_str);          /* 将ENV/Flash数据读入RAM镜像 */
u16 sApp_GetMemParamSize(void);
s8 cApp_BootUpdateMemParam(const char *p_id_str);
s8 cApp_BootGetMemParam(const char *p_id_str);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* APP_INFO_H_ */
