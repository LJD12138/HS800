#ifndef MD_DISPLAY_TASK_H_
#define MD_DISPLAY_TASK_H_

#include "board_config.h"

#if(boardDISPLAY_EN)

#if(boardENG_MODE_EN)
#include "MD_Display/md_display_eng_mode.h"
#endif

//*********************************任务对象**********************************
typedef struct
{
    bool             bLight;            //1:打开   0:关闭
	bool             bSleepShow;       	//1:打开   0:关闭
	vu16             usAutoOffTime;     //息屏时间
	vu16             usAutoOffCnt;      //息屏倒计时
	#if(boardENG_MODE_EN)
	DispTypeSet_E    eLightSetType;      //亮度设置
	#endif
}Disp_T;  
extern Disp_T   tDisp; 

//*********************************记忆参数**********************************
#pragma pack(1) //强制一个字节对齐
typedef struct
{
	u8           ucHighLightValue;
	u8           ucLowLightValue;
	vu16         usAutoOffTime;      //存储息屏的时间,大于0存在有息屏,0为常亮
}DispMemParam_T;
#pragma pack()  //取消一个字节对齐


bool bDisp_TaskInit(void);
bool bDisp_Switch(SwitchType_E type, bool fore_en);
void vDisp_TickTimer(void);
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem);
u16 usDisp_ErrCodeDisplay(void);

#if(!boardUSE_OS)
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS

#if(boardLOW_POWER)
void vLcd_EnterLowPower(void);
void vLcd_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardDISPLAY_EN

#endif  //MD_DISPLAY_TASK_H_


