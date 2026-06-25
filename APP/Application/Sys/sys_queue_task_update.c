/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"

#if(boardUPDATE)
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Print/print_prot_frame.h"
#include "Sys/sys_queue_task_update.h"

#include "Baiku/baiku_proto.h"
#include "Modbus/modbus_proto.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

#include "gpio_init.h"
#include "app_info.h"


#define     	sysTASK_UPDATE_CYCLE_TIME				sysTASK_CYCLE_TIME //任务时间
#define       	updateREC_LOST_OVERTIME        			((2UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE) 	//ms
#define       	updateDCAC_LOST_OVERTIME      			((3UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE) 	//ms
#define       	updateLOST_OVERTIME        				((360UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE) 	//ms


//****************************************************参数初始化**************************************************//
Update_T tUpdate;


//****************************************************函数声明****************************************************//
static u16 us_update_get_lost_timeout(UpdateObj_E e_obj);
static bool b_update_start_object(Task_T *tp_task);





/***********************************************************************************************************************
-----函数功能    系统升级任务处理函数
-----说明(备注)  根据任务的当前步骤和设备状态，处理系统升级任务。
-----传入参数    tp_task: 指向任务结构体的指针，包含任务的相关信息。
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_update(Task_T *tp_task)
{
	if(tp_task == NULL)
		return;

	if(tUpdate.eErrCode != UEF_NONE)
	{
		cQueue_AddQueueTask(tpSysTask, STI_UPDATE_ERR, tUpdate.eErrCode, true);
		return;
	}

    switch (tp_task->ucStep)
    {
		case 0:
		{
			bSys_SetDevState(DS_UPDATE_MODE, true);
			cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
		}
		break;
		
		//启动升级对象
		case 1:
		{
			if(b_update_start_object(tp_task) == false)
				break;
			
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		
		//等待从机进入升级准备状态
		case 2:
		{
			//超时重新发送
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= ((10 * 1000) / sysTASK_UPDATE_CYCLE_TIME))
			{
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}

			#if(boardBMS_EN)
			tSysSetParam t_sys_set_param = {0};
			if(tUpdate.eObj == UO_BMS)
			{
				if(tBms.eDevState == DS_UPDATE_MODE)
					cQueue_GotoStep(tp_task, STEP_NEXT);
				else
					break;
			}
			else
			#endif  //boardBMS_EN
			
			#if(boardDCAC_EN)
			if(tUpdate.eObj == UO_DCAC)
			{
				if(tDcac.eDevState == DS_UPDATE_MODE)
					cQueue_GotoStep(tp_task, STEP_NEXT);
				else
					break;
			}
			else
			#endif  //boardDCAC_EN
				break;
		}
		
		//开启升级通道
		case 3:
		{
			if(tUpdate.eChType == CT_PRINT)
			{
				if(cQueue_AddQueueTask(tpPrintTask, PTI_UPDATE, tUpdate.eObj, false) > 0)
					cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
				else
					break;
			}
			else
				break;
		}
		
		//等待进入透传模式
		case 4:
		{
			//超时重新发送
			tp_task->usStepWaitCnt++;
			if(tp_task->usStepWaitCnt >= ((5 * 1000) / sysTASK_UPDATE_CYCLE_TIME))
			{
				cQueue_GotoStep(tp_task, STEP_FORWARD);
				break;
			}

			if(tUpdate.eChType == CT_PRINT)
			{
				if(tPrint.eDevState == DS_UPDATE_MODE)
					cQueue_GotoStep(tp_task, STEP_NEXT);  //下一步
				else
					break;
			}
			else
				break;
		}
		
		//等待升级完成
		case 5:
		{
			bool b_host_finish = (tUpdate.eHostResult == UTR_OK ||
								 tUpdate.eHostResult == UTR_LATEST);
			bool b_slave_finish = (tUpdate.eSlaveResult == UTR_OK ||
								 tUpdate.eSlaveResult == UTR_LATEST);

			/* 上位机和从机都结束后，进入统一收尾 */
			if(b_host_finish && b_slave_finish)
			{
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
		}
		break;

		//升级完成
		case 6:
		{
			bUpdate_Init();
			cSys_Switch(SO_KEY, ST_OFF, false);
			cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
		
        default:
				cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }

	//等待10min,超时退出
	tp_task->usTaskWaitCnt++;
	if(tp_task->usTaskWaitCnt > ((10 * 60* 1000) / sysTASK_UPDATE_CYCLE_TIME) && tp_task->ucStep != STEP_END)
	{
		if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
			log_w("bSysTask:升级任务等待超时,步骤%d", tp_task->ucStep);
		
		bUpdate_SetErrCode(UEF_S_TASK_OVER_TIME);
		cQueue_GotoStep(tp_task, STEP_END);
		return;
	}
		
	#if(boardUSE_OS)
	vTaskDelay(sysTASK_UPDATE_CYCLE_TIME);
	#endif  //boardUSE_OS
}


/***********************************************************************************************************************
-----函数功能    获取升级丢失超时
-----说明(备注)  根据升级对象获取对应的丢失超时值。
-----传入参数    e_obj: 升级对象
-----输出参数    none
-----返回值      u16: 丢失超时值
-----作者        LJD
-----日期        2026-05-09
************************************************************************************************************************/
static u16 us_update_get_lost_timeout(UpdateObj_E e_obj)
{
	#if(boardDCAC_EN)
	if(e_obj == UO_DCAC)
		return updateDCAC_LOST_OVERTIME;
	#endif  //boardDCAC_EN

	return updateLOST_OVERTIME;
}

/***********************************************************************************************************************
-----函数功能    启动升级对象
-----说明(备注)  根据升级对象类型启动相应的升级任务。
-----传入参数    tp_task: 指向任务结构体的指针
-----输出参数    none
-----返回值      true: 启动成功
                false: 启动失败
************************************************************************************************************************/
static bool b_update_start_object(Task_T *tp_task)
{
	#if(boardBMS_EN)
	tSysSetParam t_sys_set_param = {0};
	if(tUpdate.eObj == UO_BMS)
	{
		if(tpBmsTask == NULL || tpBmsTask->tReplyBuff.buff == NULL)
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return false;
		}

		t_sys_set_param.obj = UO_BMS;
		t_sys_set_param.cmd = mainUPDATE_FLAG;
		lwrb_reset(&tpBmsTask->tReplyBuff);
		lwrb_write(&tpBmsTask->tReplyBuff, &t_sys_set_param, sizeof(t_sys_set_param));

		if(cQueue_AddQueueTask(tpBmsTask, BTI_REQ_SET_CMD, 0, true) < 0)
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return false;
		}
	}
	else
	#endif  //boardBMS_EN
	
	#if(boardDCAC_EN)
	if(tUpdate.eObj == UO_DCAC)
	{
		if(tpDcacTask == NULL || tpDcacTask->tReplyBuff.buff == NULL)
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return false;
		}

		if(cQueue_AddQueueTask(tpDcacTask, DTI_UPDATE, 0, true) < 0)
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return false;
		}
	}
	else
	#endif  //boardDCAC_EN
		return false;
	
	return true;
}

















/*****************************************************************************************************************
-----函数功能    初始化升级参数
-----说明(备注)  重置升级相关的全局参数。
-----传入参数    none
-----输出参数    none
-----返回值      true: 初始化成功
******************************************************************************************************************/
bool bUpdate_Init(void)
{
	memset(&tUpdate, 0, sizeof(tUpdate));
	tUpdate.eHostResult = UTR_INVALID;
	tUpdate.eSlaveResult = UTR_INVALID;
	tUpdate.eErrCode = UEF_NONE;
	return true;
}

/*****************************************************************************************************************
-----函数功能    重置升级超时计数器
-----说明(备注)  将升级任务的丢失超时计数器重置为默认值。
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUpdate_ResetTimeout(void)
{
	tUpdate.usLostOverTimeCnt = us_update_get_lost_timeout(tUpdate.eObj);
}

/***********************************************************************************************************************
-----函数功能    设置设备错误代码
-----说明(备注)  none
-----传入参数    ERR_CODE
-----输出参数    none
-----返回值      true:标记了错误  false:没有错误
************************************************************************************************************************/
bool bUpdate_SetErrCode(UpdateErrCode_E code)
{
	bool b_ret = false;
	bool b_can_set = false;
	static UpdateErrCode_E e_next_code;

	if(uPrint.tFlag.bSysTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code && code != UEF_NONE)
			log_e("bSysTask:系统升级错误 代码%d",code);
		e_next_code = code;
	}
	
	//有错误
	if(code != UEF_NONE)
	{
		b_can_set = (tUpdate.eErrCode == UEF_NONE) ? true : false;
		if(tUpdate.eErrCode == UEF_P_CANCEL_REQ && code != UEF_P_CANCEL_REQ)
			b_can_set = true;

		if(b_can_set)
		{
			tUpdate.eErrCode = code;

			if(code != UEF_P_CANCEL_REQ)
				cQueue_AddQueueTask(tpSysTask, STI_UPDATE_ERR, code, true);
		}

		b_ret = true;
	}
	else 
	{
		tUpdate.eErrCode = code;
		b_ret = false;
	}

	return b_ret;
}

/*****************************************************************************************************************
-----函数功能    选择升级通道
-----说明(备注)  根据传入的升级对象和通道类型，选择并初始化升级通道。
-----传入参数    e_obj: 升级对象。
                ch_type: 通道类型。
-----输出参数    none
-----返回值      1: 选择成功
                0: 通道未改变
                -1: 参数无效
                -2: 添加任务失败
                -3: 未知升级对象
******************************************************************************************************************/
s8 cUpdate_ChSelect(UpdateObj_E e_obj, ChannelType_E ch_type)
{
	if(e_obj >= UO_INVAILD || ch_type >= CT_INVAILD)
		return -1;
	
	if(ch_type == tUpdate.eChType && 
		tSysInfo.eDevState == DS_UPDATE_MODE)
	{
		tUpdate.usLostOverTimeCnt = us_update_get_lost_timeout(e_obj);
		return 0;
	}
	
	switch(e_obj)
	{
		case UO_DEFAULT:
		case UO_CONSOLE:
		{
			tUpdate.ulBaud = 115200;
		}
		break;
		
		case UO_BMS:
		{
			tUpdate.ulBaud = 115200;
		}
		break;

		case UO_DCAC:
		{
			tUpdate.ulBaud = 115200;
		}
		break;
		
		default:
			return -3;
	}
	
	vUpdate_SetStage(NULL, DUS_IDLE);
	
	tUpdate.eObj = e_obj;
	tUpdate.eChType = ch_type;
	
	if(cQueue_AddQueueTask(tpSysTask, STI_UPDATE, 0, false) < 0)
		return -2;

	return 1;
}

/*****************************************************************************************************************
-----函数功能    选择升级协议
-----说明(备注)  根据传入的升级对象和协议类型，选择并初始化升级协议。
-----传入参数    e_obj: 升级对象。
                proto_type: 协议类型。
-----输出参数    none
-----返回值      1: 选择成功
                0: 协议未改变
                -1: 参数无效
******************************************************************************************************************/
s8 cUpdate_ProtoSelect(UpdateObj_E e_obj, ProtoType_E proto_type)
{
	if(e_obj >= UO_INVAILD || proto_type >= PT_INVAILD)
		return -1;
	
	if(proto_type == tUpdate.eProtoType && 
		tSysInfo.eDevState == DS_UPDATE_MODE)
	{
		tUpdate.usRecOverTimeCnt = updateREC_LOST_OVERTIME;
		return 0;
	}
	tUpdate.eProtoType = proto_type;
	return 1;
}

/*****************************************************************************************************************
-----函数功能    设置升级阶段
-----说明(备注)  更新当前升级阶段，并根据选项位标志重置相关任务的等待计数或超时计数器。
-----传入参数    stage: 升级阶段值
                tp_task: 指向任务结构体的指针
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUpdate_SetStage(Task_T *tp_task, u8 stage)
{
	switch(stage)
	{
		case DUS_IDLE:
		{
			tUpdate.eHostResult = UTR_INVALID;
			tUpdate.eSlaveResult = UTR_INVALID;
			tUpdate.ulFwSize = 0;
			tUpdate.ulRxSize = 0;
			tUpdate.ulFwCrc32 = 0;
			tUpdate.ulFwCalcCrc32 = 0xFFFFFFFFUL;
			tUpdate.ulFwPendCrc32 = 0xFFFFFFFFUL;
			tUpdate.usPendPacketLen = 0;
			tUpdate.usRecOverTimeCnt = 0;
			tUpdate.usLostOverTimeCnt = us_update_get_lost_timeout(tUpdate.eObj);

			cModbus_ResetTx(tpDcacProtoTx, tpDcacProtoTx->usFrameDataSize);
            cModbus_ResetRxBuff(tpDcacProtoRx);

			cBaiku_ResetRxBuff(tpPrintProtoRx);
		}
		break;

		case DUS_SLAVE_READY_OK:
		{
			#if(boardUSE_OS && boardPRINT_IFACE)
			if(tUpdate.eChType == CT_PRINT && tPrintTaskHandler != NULL)
				xTaskNotifyGive(tPrintTaskHandler);
			#endif
		}
		break;

		case DUS_HOST_REQ_DATA:
		{
			vUpdate_ResetTimeout();

			#if(boardUSE_OS && boardPRINT_IFACE)
			if(tUpdate.eChType == CT_PRINT && tPrintTaskHandler != NULL)
				xTaskNotifyGive(tPrintTaskHandler);
			#endif
		}
		break;

		case DUS_SLAVE_SEND_DATA:
		{
			vUpdate_ResetTimeout();
			
			#if(boardUSE_OS)
			/* 通过任务通知唤醒DCAC任务 */
			if(tDcacTaskHandler != NULL)
				xTaskNotifyGive(tDcacTaskHandler);
			#endif
		}
		break;

		case DUS_WAIT_SLAVE_REPLY:
		{
			vUpdate_ResetTimeout();
		}
		break;

		case DUS_GET_SLAVE_RESULT:
		{
			#if(boardUSE_OS)
			/* 通过任务通知唤醒DCAC任务 */
			if(tDcacTaskHandler != NULL)
				xTaskNotifyGive(tDcacTaskHandler);
			#endif
		}
		break;

		case DUS_WAIT_SLAVE_RESULT_REPLY:
		{
			vUpdate_ResetTimeout();

			#if(boardUSE_OS)
			/* 通过任务通知唤醒DCAC任务 */
			if(tDcacTaskHandler != NULL)
				xTaskNotifyGive(tDcacTaskHandler);
			#endif
		}
		break;

		default:
			break;
	}

	tUpdate.ucStage = stage;

	if(tp_task != NULL)
	{
		tp_task->usStepWaitCnt = 0;
		tp_task->usStepRepeatCnt = 0;
	}
}

/***********************************************************************************************************************
-----函数功能    升级任务Tick计时
-----说明(备注)  由定时器周期调用，递减接收超时和丢失超时计数器；丢失超时时触发错误码。
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vUpdate_TickTimer(void)
{
	if(tUpdate.usRecOverTimeCnt > 0)
	{
		tUpdate.usRecOverTimeCnt--;
		if(tUpdate.usRecOverTimeCnt == 0)
		{
			
		}
	}
	
	if(tUpdate.usLostOverTimeCnt > 0)
	{
		tUpdate.usLostOverTimeCnt--;
		if(tUpdate.usLostOverTimeCnt == 0)
		{
			bUpdate_SetErrCode(UEF_S_LOST_OVERTIME);
		}
	}
}
#endif  //boardUPDATE
