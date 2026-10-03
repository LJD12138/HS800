/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Display
 * File    : md_display_task.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 显示屏任务管理与状态控制头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DISPLAY_TASK_H_
#define MD_DISPLAY_TASK_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDISPLAY_EN)

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//
typedef struct
{
	bool				bLight;				//1:打开   0:关闭
	bool				bSleepShow;			//1:打开   0:关闭
	vu16				usAutoOffTime;		//息屏时间
	vu16				usAutoOffCnt;		//息屏倒计时
	#if (boardENG_MODE_EN)
	DispTypeSet_E		eLightSetType;		//亮度设置
	#endif  /* boardENG_MODE_EN */
}Disp_T;

#pragma pack(1)
typedef struct
{
	u8					ucHighLightValue;
	u8					ucLowLightValue;
	vu16				usAutoOffTime;		//存储息屏的时间,大于0存在有息屏,0为常亮
}DispMemParam_T;
#pragma pack()

//****************************************************Globals*******************************************************************//
extern Disp_T tDisp; 

//****************************************************Extern********************************************************************//
s8   cDisp_TaskInit(void);
bool bDisp_Switch(SwitchType_E type, bool fore_en);
void vDisp_TickTimer(void);
bool bDisp_MemParamInit(DispMemParam_T *p_disp_mem);
u16  usDisp_ErrCodeDisplay(void);

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
