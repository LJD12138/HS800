/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_task.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示任务外壳头文件 (任务入口/页面导出/记忆参数/兼容接口)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_TASK_H_
#define MD_DISPLAY_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "board_config.h"

#if (boardDISPLAY_EN)
#include <stdbool.h>
#include <stdint.h>
#include "main.h"
#include "uni_disp_core.h"
#include "MD_Display/md_display_api.h"

#if (boardENG_MODE_EN)
#include "MD_Display/md_display_eng_mode.h"
#endif  /* boardENG_MODE_EN */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

/* 显示模块记忆参数, 存入 APP 参数区 */
#pragma pack(1)
typedef struct
{
	uint8_t				ucHighLightValue;
	uint8_t				ucLowLightValue;
	__IO uint16_t		usAutoOffTime;		/* 存储息屏的时间,大于0存在有息屏,0为常亮 */
}DispMemParam_T;
#pragma pack()

//****************************************************Globals*******************************************************************//
#if (boardUSE_OS)
extern TaskHandle_t tDispTaskHandler;
#endif

/* 页面控制块全局导出 */
extern const DispPage_T G_tPageInit;        /* 系统初始化页 */
extern const DispPage_T G_tPageBoot;        /* 开机步进动画页 */
extern const DispPage_T G_tPageWork;        /* 主工作页 */
extern const DispPage_T G_tPageClosing;     /* 关机中页 */
extern const DispPage_T G_tPageShutDown;    /* 关机状态页 */
extern const DispPage_T G_tPageFault;       /* 故障专用页 */

#if (boardUPDATE)
extern const DispPage_T G_tPageUpdate;      /* 升级页 */
#endif  /* boardUPDATE */

#if (boardENG_MODE_EN)
extern const DispPage_T G_tPageEng;         /* 工程模式页 */
#endif  /* boardENG_MODE_EN */

//****************************************************Extern********************************************************************//
s8   cDisp_TaskInit(void);
bool bDisp_Switch(SwitchType_E type, bool fore_en);
void vDisp_TickTimer(void);
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem);
uint16_t usDisp_ErrCodeDisplay(void);
void vDisp_UiInit(void);

#if (boardENG_MODE_EN)
void vDisp_EngModeResetTimeout(void);
#endif

#if (!boardUSE_OS)
void vDisp_Task(void *pvParameters);
#endif  /* !boardUSE_OS */

#if (boardLOW_POWER)
void vLcd_EnterLowPower(void);
void vLcd_ExitLowPower(void);
#endif  /* boardLOW_POWER */

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DISPLAY_TASK_H_ */
