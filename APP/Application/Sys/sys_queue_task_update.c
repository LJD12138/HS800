/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "Sys/sys_queue_task.h"
#include <stdbool.h>

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

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#endif  //boardBMS_EN

#include "gpio_init.h"
#include "app_info.h"


#define     	sysTASK_UPDATE_CYCLE_TIME				sysTASK_CYCLE_TIME //任务时间
#define       	updateREC_LOST_OVERTIME        			((5UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE) 	//ms
#define       	updateLOST_OVERTIME        				((360UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE) 	//ms


//****************************************************参数初始化**************************************************//
Update_T tUpdate;


//****************************************************函数声明****************************************************//
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
			vUpdate_InitParam();
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
			//超时10S重新发送
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
			if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
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
		break;
		
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
				/* 升级完成,延时等待模块退出,再统一收尾 */
				tUpdate.usLostOverTimeCnt = ((5UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE);
				cQueue_GotoStep(tp_task, STEP_NEXT);
			}
		}
		break;

		//等待退出(延时5S,等待模块退出升级模式)
		case 6:
		{
			if(tUpdate.usLostOverTimeCnt == 0)
				cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;

		//等待队列任务退出
		case 7:
		{
			if(lwrb_get_full(&tp_task->tQueueBuff) != 0)
				cQueue_GotoStep(tp_task, STEP_END);  //结束
		}
		break;
		
        default:
				cQueue_GotoStep(tp_task, STEP_END);  //结束
			break;
    }
	
	#if(boardUSE_OS)
	vTaskDelay(sysTASK_UPDATE_CYCLE_TIME);
	#endif  //boardUSE_OS
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
		if(lwrb_get_free(&tpBmsTask->tReplyBuff) >= sizeof(t_sys_set_param))
			lwrb_write(&tpBmsTask->tReplyBuff, &t_sys_set_param, sizeof(t_sys_set_param));
		else
			return false;

		if(cQueue_AddQueueTask(tpBmsTask, BTI_REQ_SET_CMD, 0, true) < 0)
		{
			cQueue_GotoStep(tp_task, STEP_END);  //结束
			return false;
		}
	}
	else
	#endif  //boardBMS_EN
	
	#if(boardDCAC_EN)
	if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
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
	bUpdate_SetResult(URT_HOST_SLAVE, UTR_INVALID);
	tUpdate.eErrCode = UEF_NONE;
	return true;
}

/*****************************************************************************************************************
-----函数功能    初始化升级任务参数
-----说明(备注)  
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUpdate_InitParam(void)
{
	tUpdate.usRecOverTimeCnt = 0;
	vUpdate_ResetTimeout();
	bUpdate_SetResult(URT_HOST_SLAVE, UTR_INVALID);

	#if(boardDCAC_EN)
	if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
	{
		cModbus_ResetTx(tpDcacProtoTx, tpDcacProtoTx->usFrameDataSize);
		cModbus_ResetRxBuff(tpDcacProtoRx);
	}
	#endif  //boardDCAC_EN

	#if(boardBMS_EN)
	if (tUpdate.eObj == UO_BMS)
	{
		// cBaiku_ResetTx(tpBmsProtoTx, tpBmsProtoTx->usFrameDataSize);
		cBaiku_ResetRxBuff(tpBmsProtoRx);
	}
	#endif  //boardBMS_EN

	if(tUpdate.eChType == CT_PRINT)
	{
		cBaiku_ResetRxBuff(tpPrintProtoRx);
	}
}

/*****************************************************************************************************************
-----函数功能    重置升级超时计数器
-----说明(备注)  将升级任务的超时计数器重置为默认值,默认只有接收到从机回复收到固件才重置
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUpdate_ResetTimeout(void)
{
	tUpdate.usLostOverTimeCnt = updateLOST_OVERTIME;
}

/*****************************************************************************************************************
-----函数功能    重置升级接收超时计数器
-----说明(备注)  将升级任务的接收超时计数器重置为默认值,发送就会重置,接收就会重置
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vUpdate_ResetRecTimeout(bool reset)
{
	if(reset)
		tUpdate.usRecOverTimeCnt = updateREC_LOST_OVERTIME;
	else
		tUpdate.usRecOverTimeCnt = 0;
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

	//有错误
	if(code != UEF_NONE)
	{
		if(tpSysTask != NULL && tpSysTask->ucID != STI_UPDATE_ERR)
		{
			tUpdate.eErrCode = code;
			cQueue_AddQueueTask(tpSysTask, STI_UPDATE_ERR, code, true);
		}
		b_ret = true;
		vUpdate_ResetRecTimeout(false);
	}
	else 
	{
		tUpdate.eErrCode = code;
		b_ret = false;
	}

	return b_ret;
}

/***********************************************************************************************************************
-----函数功能    设置升级结果
-----说明(备注)  设置主机或从机的升级结果
-----传入参数    target: 目标-URT_HOST主机  URT_SLAVE从机
                result: 要设置的升级结果
-----输出参数    none
-----返回值      true:设置成功  false:参数无效
************************************************************************************************************************/
bool bUpdate_SetResult(UpdateResultTarget_E target, UpdateTaskResult_E result)
{
	if(result >= UTR_MAX)
		return false;

	switch(result)
	{
		case UTR_INVALID:
		{
			tUpdate.ulFwSize = 0;
			tUpdate.ulRxSize = 0;
			tUpdate.ulFwCrc32 = 0;
			tUpdate.ulFwCalcCrc32 = 0xFFFFFFFFUL;
			tUpdate.ulFwPendCrc32 = 0xFFFFFFFFUL;
			tUpdate.usPendPacketLen = 0;
			tUpdate.usRecFrameCnt = 0;
			vUpdate_ResetRecTimeout(false);
		}
		break;

		case UTR_RUNNING:
		{
			tUpdate.ulFwSize = 0;
			tUpdate.ulRxSize = 0;
			tUpdate.ulFwCrc32 = 0;
			tUpdate.ulFwCalcCrc32 = 0xFFFFFFFFUL;
			tUpdate.ulFwPendCrc32 = 0xFFFFFFFFUL;
			tUpdate.usPendPacketLen = 0;
			tUpdate.usRecFrameCnt = 0;
		}
		break;

		case UTR_OK:
		{
			vUpdate_ResetRecTimeout(false);
		}
		break;

		case UTR_LATEST:
		{
			vUpdate_ResetRecTimeout(false);
		}
		break;

		case UTR_CANCEL:
		{
			vUpdate_ResetRecTimeout(false);
		}
		break;

		case UTR_FAIL:
		{
			vUpdate_ResetRecTimeout(false);
		}
		break;

		case UTR_MAX:
		{
			vUpdate_ResetRecTimeout(false);
		}
		break;

		default:
			return false;
	}

	switch(target)
	{
		case URT_HOST:
			tUpdate.eHostResult = result;
			break;

		case URT_SLAVE:
			tUpdate.eSlaveResult = result;
			break;

		case URT_HOST_SLAVE:
			tUpdate.eHostResult = result;
			tUpdate.eSlaveResult = result;
			break;

		default:
			return false;
	}

	return true;
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
		vUpdate_ResetTimeout();
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

		case UO_MGMT_AC:
		case UO_MGMT_DC:
		{
			tUpdate.ulBaud = 115200;
		}
		break;
		
		default:
			return -3;
	}

	tUpdate.eObj = e_obj;
	tUpdate.eChType = ch_type;
	
	if(tpSysTask->ucID == STI_INIT)
	{
		if(cQueue_AddQueueTask(tpSysTask, STI_UPDATE, 0, false) < 0)
			return -2;
	}
	else
	{
		if(cQueue_AddQueueTask(tpSysTask, STI_UPDATE, 0, true) < 0)
			return -3;
	}
	

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

	/* 升级对象已确定时，协议选择必须与当前升级对象一致，防止跨对象污染 */
	if(tUpdate.eObj != UO_DEFAULT && e_obj != tUpdate.eObj)
		return -3;
	
	if(proto_type == tUpdate.eProtoType &&
		tSysInfo.eDevState == DS_UPDATE_MODE)
	{
		return 0;
	}
	tUpdate.eProtoType = proto_type;
	return 1;
}

/***********************************************************************************************************************
-----函数功能    检查升级结果是否正常
-----说明(备注)  根据升级目标判断主机/从机的升级结果是否处于正常状态(RUNNING/OK/LATEST)。
-----传入参数   target: 升级目标 URT_HOST主机  URT_SLAVE从机  URT_HOST_SLAVE主机和从机
-----返回值     bool
-----作者       LJD
-----日期       2026-07-01
************************************************************************************************************************/
bool bUpdate_ResultIsNormal(UpdateResultTarget_E target)
{
	switch(target)
	{
		case URT_HOST:
			return (tUpdate.eHostResult == UTR_RUNNING 
				|| tUpdate.eHostResult == UTR_OK 
				|| tUpdate.eHostResult == UTR_LATEST);
		
		case URT_SLAVE:
			return (tUpdate.eSlaveResult == UTR_RUNNING 
				|| tUpdate.eSlaveResult == UTR_OK 
				|| tUpdate.eSlaveResult == UTR_LATEST);

		case URT_HOST_SLAVE:
			return bUpdate_ResultIsNormal(URT_HOST) 
				&& bUpdate_ResultIsNormal(URT_SLAVE);
		
		default:
			return false;
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
	bool b_rec_timeout = false;
	bool b_lost_timeout = false;

	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif

	if(tUpdate.usRecOverTimeCnt > 0)
	{
		tUpdate.usRecOverTimeCnt--;
		if(tUpdate.usRecOverTimeCnt == 0)
			b_rec_timeout = true;
	}

	/* 升级任务超时退出,执行关机 */
	if(tUpdate.usLostOverTimeCnt > 0)
	{
		tUpdate.usLostOverTimeCnt--;
		if(tUpdate.usLostOverTimeCnt == 0)
			b_lost_timeout = true;
	}

	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif

	/* 在临界区外调用,避免临界区内嵌套调度 */
	if(b_rec_timeout)
	{
		tUpdate.usRecFrameCnt = 0;
		bUpdate_SetErrCode(UEF_S_REC_OVERTIME);
	}
		

	if(b_lost_timeout)
	{
		bUpdate_Init();
		cSys_Switch(SO_KEY, ST_OFF, true);
	}
}
#endif  //boardUPDATE
