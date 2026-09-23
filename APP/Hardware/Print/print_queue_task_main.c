/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
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
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"

//****************************************************Macros********************************************************************//
#define			printTASK_PARAM_CYCLE_TIME				50

//****************************************************Function Declaration******************************************************//
static s8 c_print_rec_proc_data(BaikuProtoRx_t* proto);

/***********************************************************************************************************************
 * 函数功能    : Print主循环队列任务
 * 说明(备注)  : 定时轮询发送数据、校验协议并分发接收指令
 * 传入参数    : tp_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_print_queue_task_main(Task_T *tp_task)
{
	s8 c_ret = 0;

	//队列里面有任务
	if (lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep(tp_task, STEP_END);  //结束
		return;
	}
	
	c_cycle_relay_data();
		
	//******************************************处理发送的数据****************************************************
	if (lwrb_get_full(&tPrintTxBuff))
		bPrint_SendDataToUsart();
	
	//******************************************处理接收的数据****************************************************
	c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
	if (c_ret > 0)
		c_print_rec_proc_data(tpPrintProtoRx);
	
	switch (tp_task->ucStep)
    {
        case 0:
        {
			
        }
		break;

		default:
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
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
        case baikuCMD_SWITCH:                  //开关回复
        {
            //开关对象+开关动作共2字节,防止空帧或短帧越界访问
            if (proto->ucpValidData != NULL && proto->ucValidLen >= 2)
                c_relay02_switch_result(proto->ucpValidData);
        }
		break;

        case baikuCMD_GET_PARAM:
            c_relay08_param();
			break;

        case 0x09:
            c_relay0A_bat_param();
			break;

        case 0x0B:  //上报记忆参数信息
			c_relay0C_dcac_param();
			break;

        case 0x0D:  //上报MPPT参数信息
			c_relay0E_mppt_param();
			break;

        case 0x0F:
            c_relay10_usb_param();
			break;
		
		case 0x11:
		{
			c_relay12_dc_param();
		}
		break;

        case 0x13:
            c_relay14_sysinfo_param();
			break;
		
		case baikuCMD_SET_CHG_PWR://40
			c_relay40_set_chg_pwr(proto);
			break;
		
		case baikuCMD_CALI://44
			c_relay44_cali(proto);
			break;

		case baikuCMD_GET_MEM_PARAM://80
			c_relay80_get_mem_param(proto);
			break;
		
		case baikuCMD_WRITE_MEM_PARAM://82
			c_relay82_write_mem_info(proto);
			break;

        case baikuCMD_SET_PRINT_STATE://84
            c_relay84_set_print_state(proto);
			break;

        case baikuCMD_GET_PRINT_STATE://86
            c_relay86_get_print_state(proto);
			break;

		case baikuCMD_SYS_SET://88
			c_relay88_sys_set(proto);
			break;

		#if (boardBMS_EN && boardRUN_LOG_EN)
		case baikuCMD_LOG_UNLOCK://8A
			c_relay8A_log_unlock(proto);
			break;

		case baikuCMD_GET_RUN_LOG://B4
			c_relayB4_get_run_log(proto);
			break;

		case baikuCMD_READ_RUN_LOG_SLOT://B6
			c_relayB6_read_run_log_slot(proto);
			break;

		case baikuCMD_RESET_RUN_LOG://B8
			c_relayB8_reset_run_log(proto);
			break;
		#endif  /* boardBMS_EN && boardRUN_LOG_EN */
		
        default:
            break;
    }
    return 0;
}

#endif  /* boardPRINT_IFACE */
