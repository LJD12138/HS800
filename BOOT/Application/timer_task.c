/*****************************************************************************************************************
*                                                                                                                *
 *                                         软件定时器                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "timer_task.h"
#include "gpio_init.h"
#include "board_config.h"

#include "Sys/sys_task.h"
//#include "eng_mode.h"

#if(boardCONSOLE_EN)
#include "MD_Console/md_console_rec_task.h"
#endif

/***********************************************************************************************************************
-----函数功能    定时器函数
-----说明(备注)  none
-----传入参数    xTimer:调用函数的句柄
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static vu8 timer_cnt = 0;
void vTimer_Task(void)
{
	timer_cnt++;
	if(timer_cnt >= (1000/boardREPET_TIMER_CYCLE_TMIE)) //1S计时 
	{
		timer_cnt = 0;
		vSys_TickTimer();
		
		#if(boardWDGT_EN && boardPRINT_IFACE == 0)
		vFwdgt_Reload();
		#endif
		
		#if(boardENG_MODE_EN)
		vEng_ExitEngModeCnt();
		#endif
	}
	
	#if(boardCONSOLE_EN)
//	vConsole_RecTickTimer();
	#endif
	
//	#if(boardMPPT_EN)
//	vMPPT_RecTickTimer();
//	#endif
}

#if(boardLOW_POWER)
//进入低功耗
void vCount_EnterLowPower(void)
{
	xTimerDelete(tSignalTimer,100);
	xTimerDelete(tRepetTimer,100);
}

//退出低功耗
void vCount_ExitLowPower(void)
{
	vTimer_TaskInit();
}
#endif

