/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Flash/flash_iface.h"

#include "boot_info.h"

/**********************************************************************************************************
*	函 数 名: PrintfLogo
*	功能说明: 打印例程名称和例程发布日期, 接上串口线后，打开PC机的超级终端软件可以观察结果
*	形    参: 无
*	返 回 值: 无
**********************************************************************************************************/
static void v_print_logo(void)
{
//	uint32_t CPU_Sn0, CPU_Sn1, CPU_Sn2;
//	
//	CPU_Sn0 = *(__IO uint32_t*)(0x1FF1E800);
//	CPU_Sn1 = *(__IO uint32_t*)(0x1FF1E800 + 4);
//	CPU_Sn2 = *(__IO uint32_t*)(0x1FF1E800 + 8);
	
	sMyPrint("Boot Start!!!\r\n");
//	sMyPrint("CPU : STM32H750VB, 主频: %dMHz\r\n", SystemCoreClock / 1000000);
//	sMyPrint("UID = %x %x %x\n\r", CPU_Sn2, CPU_Sn1, CPU_Sn0);
}


/***********************************************************************************************************************
-----函数功能    系统初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/  
void v_sys_queue_task_init(Task_T *tp_task)
{
	SysTaskId_E e_task_id;
	
    switch (tp_task->ucStep)
    {
		case 0:
		{
			if(uPrint.tFlag.bSysTask)
				v_print_logo();
			tSysInfo.uInit.tFinish.bIF_BootInfo = false;
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}break;
		
		case 1:
		{
//			e_task_id = STI_DISPLAY;
//			e_task_id = STI_UPDATE;
			e_task_id = eBoot_InfoInit(false);
			if(e_task_id != STI_ERR && e_task_id >= STI_INIT)
				tSysInfo.uInit.tFinish.bIF_BootInfo = true;
			cQueue_AddQueueTask(tpSysTask, e_task_id, 0, false);
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}break;
		
		case 2:
		{
//			vFlash_ReadWriteTest();
//			cQSPI_ReadWriteTestDemo();
			tSysInfo.uInit.tFinish.bIF_SysTask = 1;
			tSysInfo.uInit.tFinish.bIF_SysInit = 1;
			tSysInfo.uInit.tFinish.bIF_AT24Cxx = 1;
			tSysInfo.uInit.tFinish.bIF_Gpio = 1;
			cQueue_GotoStep(tp_task, STEP_END);
//			cQueue_GotoStep(tp_task, STEP_NEXT);
		}break;
		
        default:
			cQueue_GotoStep(tp_task, STEP_END);
			break;
    }
	
	//初始化等待5S,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > (5000/sysTASK_CYCLE_TIME))
	{
		
	}
}

