/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Print
 * File    : print_queue_task_main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print主轮询任务及上位机指令解析与分发
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_queue_task.h"

#if (boardPRINT_IFACE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

//****************************************************Macros********************************************************************//
#define			printTASK_PARAM_CYCLE_TIME				50

//****************************************************Function Declaration******************************************************//
static s8 c_print_rec_proc_data(BaikuProtoRx_t* proto);

/***********************************************************************************************************************
 * 函数功能    : Print主循环队列任务
 * 说明(备注)  : 定时轮询发送数据、校验协议并分发接收指令
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_queue_task_main(Task_T *p_task)
{
	s8 c_ret = 0;

	//队列里面有任务
	if (lwrb_get_full(&p_task->tQueueBuff))  
	{
		cQueue_GotoStep(p_task, STEP_END);  //结束
		return;
	}

	/* 处理发送的数据 */
	//******************************************处理发送的数据****************************************************
	if (lwrb_get_full(&tPrintTxBuff))
		bPrint_SendDataToUsart();

	/* 处于升级,不处理数据 */
	if (tpSysTask->ucID == STI_UPDATE && tUpdate.eChType == CT_PRINT) 
		return;
	
	c_cycle_relay_data();
		
	/* 处理接收的数据 */
	c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
	if (c_ret > 0)
		c_print_rec_proc_data(tpPrintProtoRx);
	
	switch (p_task->ucStep)
    {
        case 0:
        {
			
        }
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);  //结束
		}
		break;
    }
	
	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, printTASK_PARAM_CYCLE_TIME);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 处理接收到的协议数据
 * 说明(备注)  : 解析协议命令码并调用对应的回复打包函数
 * 传入参数    : proto: 协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : 0: 正常, 其它: 错误码
 ************************************************************************************************************************/
__STATIC_INLINE s8 c_print_rec_proc_data(BaikuProtoRx_t* proto)
{
	if (proto == NULL)
		return 99;
		
	uc_next_cmd = proto->ucCmd;
	switch (proto->ucCmd)
	{
		case baikuCMD_SET_PROTO:
		{
			c_relay84_set_proto(proto);
		}
		break;
		
		case baikuCMD_SYS_SET:	/* 0x88 */
		{
			c_relay88_sys_set(proto);
		}
		break;
		
        default:
            break;
    }
    return 0;
}

#endif  /* boardPRINT_IFACE */
