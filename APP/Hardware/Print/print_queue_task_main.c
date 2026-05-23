/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"


#define       	printTASK_PARAM_CYCLE_TIME               		50


//****************************************************函数声明****************************************************//
static s8 c_print_rec_proc_data(BaikuProtoRx_t* proto);


/*****************************************************************************************************************
-----函数功能    任务函数:错误处理任务
-----说明(备注)  none
-----传入参数    PRINT_ErrState_N:
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_queue_task_main(Task_T *tp_task)
{
	s8 c_ret = 0;

	//队列里面有任务
	if(lwrb_get_full(&tp_task->tQueueBuff))  
	{
		cQueue_GotoStep( tp_task, STEP_END );  //结束
		return;
	}
	
	c_cycle_relay_data();
		
	 //******************************************处理发送的数据****************************************************
	if(lwrb_get_full(&tPrintTxBuff))
		bPrint_SendDataToUsart();
	
	//******************************************处理接收的数据****************************************************
	c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
	if(c_ret > 0)
		c_print_rec_proc_data(tpPrintProtoRx);
	
	switch (tp_task->ucStep)
    {
        case 0:
        {
			
        }break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	#if(boardUSE_OS)
	ulTaskNotifyTake(pdFALSE, printTASK_PARAM_CYCLE_TIME);
	#endif  //boardUSE_OS
}


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
__STATIC_INLINE s8 c_print_rec_proc_data(BaikuProtoRx_t* proto)
{
	if(proto == NULL)
		return 99;
		
	uc_next_cmd = proto->ucCmd;
    switch (proto->ucCmd)
    {
        case baikuCMD_SWITCH:                  //开关回复
            c_relay02_switch_result(proto->ucpValidData);
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
			c_relay12_dc_param();
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
		
        default:
            break;
    }
    return 0;
}

#endif  //boardPRINT_IFACE
