/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_queue_task.h"

#if(boardPRINT_IFACE)
#include "Sys/sys_task.h"
#include "Sys/sys_queue_task_update.h"
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

	//处理发送的数据
	if(lwrb_get_full(&tPrintTxBuff))
		bPrint_SendDataToUsart();

	//处于升级,不处理数据
	if(tpSysTask->ucID == STI_UPDATE && 
		tUpdate.eChType == CT_PRINT) 
		return;
	
	c_cycle_relay_data();
		
	//处理接收的数据
	c_ret = cBaiku_ProtoCheck(tpPrintProtoRx);
	if(c_ret > 0)
		c_print_rec_proc_data(tpPrintProtoRx);

	// c_relay1C_afe_param();

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
        case baikuCMD_SET_PROTO:
		{
			c_relay84_set_proto(proto);
		}
		break;
		
		case baikuCMD_SYS_SET:  //88
        {
            c_relay88_sys_set(proto);
        }
        break;
		
        default:
            break;
    }
    return 0;
}

#endif  //boardPRINT_IFACE
