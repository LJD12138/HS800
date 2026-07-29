#ifndef SYS_QUEUE_TASK_ENG_H_
#define SYS_QUEUE_TASK_ENG_H_

#include "board_config.h"

#if(boardENG_MODE_EN)

#include "main.h"
#include "queue_task.h"

//工程模式后台步骤(对应PARAM_SET各Tab, 由S_aucPsTabToEms映射)
typedef enum
{
	EMS_INIT = 0,	/* 初始(系统任务启动时) */
	EMS_SYS,		/* SYS Tab: 风扇/蜂鸣器持续控制 */
	EMS_LCD,		/* LCD Tab */
	EMS_BAT,		/* BAT Tab */
	EMS_DCAC,		/* DCAC Tab */
	EMS_MPPT,		/* MPPT Tab */
	EMS_USB,		/* USB Tab */
	EMS_DC,			/* DC Tab */
}EngModeStep_E;


typedef struct
{
	vu16 usEngModeCnt;
	vu8  ucEngModeItem;
	s8   cEngModeState;
}EngMode_T;

extern EngMode_T tEngMode;

void vEng_RefreshEngModeTime(void);
void vEng_AdjustParam(uint8_t uc_tab, uint8_t uc_item, bool b_add);


#endif //boardENG_MODE_EN

#endif //SYS_QUEUE_TASK_ENG_H_
