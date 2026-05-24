#ifndef MD_DISPLAY_TASK_H_
#define MD_DISPLAY_TASK_H_

#include "board_config.h"
#include "MD_Display/md_display_api.h"

#if(boardDISPLAY_EN)
#include "queue_task.h"

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif

#if(boardENG_MODE_EN)
#include "MD_Display/md_display_eng_mode.h"
#endif  //boardENG_MODE_EN

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS


//*********************************显示任务ID**********************************
//显示队列任务ID, 由队列管理函数装载对应任务函数
typedef enum
{
	DDTI_NULL = 0,      //空任务
	DDTI_INIT,          //初始化显示
	DDTI_CLOSING,       //关闭中显示
	DDTI_SHUT_DOWN,     //关闭完成显示
	DDTI_ERR,           //错误显示
	DDTI_BOOTING,       //启动中显示
	DDTI_WORK,          //工作显示
	DDTI_UPDATA,        //升级显示
	DDTI_ENG,           //工程模式显示
}DispTaskId_E;

//*********************************任务对象**********************************
//显示任务运行参数, 由显示任务、定时节拍和亮灭控制共用
typedef struct
{
    bool             	bLight;           	//1:打开   0:关闭
	bool             	bSleepShow;       	//1:打开   0:关闭
	DevState_E    		eDevState;          //设备状态
	vu16             	usAutoOffTime;     //息屏时间
	vu16             	usAutoOffCnt;      //息屏倒计时
	#if(boardENG_MODE_EN)
	DispTypeSet_E    	eLightSetType;     //亮度设置
	#endif
}Disp_T;  
extern Disp_T tDisp; 

//LVGL版本不需要OLED页面系统结构定义

extern Task_T *tpDispTask;      //队列任务对象
extern bool g_bDispPageDirty;   //页面脏标志

#if(boardUSE_OS)
extern TaskHandle_t tDispTaskHandler;
#endif

//*********************************记忆参数**********************************
//显示模块记忆参数, 存入APP参数区
#pragma pack(1)
typedef struct
{
	u8           ucHighLightValue;
	u8           ucLowLightValue;
	vu16         usAutoOffTime;      //存储息屏的时间,大于0存在有息屏,0为常亮
}DispMemParam_T;
#pragma pack()


//显示任务对外接口
bool bDisp_TaskInit(void);
bool bDisp_SetDevState(DevState_E state);
bool bDisp_Switch(SwitchType_E type, bool fore_en);
void vDisp_TickTimer(void);
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem);
u16 usDisp_ErrCodeDisplay(void);
void vDisp_RequestRefresh(void);

#if(!boardUSE_OS)
void vDisp_Task(void *pvParameters);
#endif  //boardUSE_OS

#if(boardLOW_POWER)
void vLcd_EnterLowPower(void);
void vLcd_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardDISPLAY_EN

#endif  //MD_DISPLAY_TASK_H_

