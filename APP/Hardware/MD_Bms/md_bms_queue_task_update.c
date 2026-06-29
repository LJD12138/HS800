/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统的队列函数                                                  		*
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_queue_task.h"

#if(boardBMS_EN && boardUPDATE)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Bms/md_bms_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#define       	bmsTASK_UPDATE_TIME               		50

//****************************************************函数声明****************************************************//
static bool b_send_baiku_frame(u8 cmd, u8* data, u8 len);
static bool b_bms_check_task_valid(Task_T *tp_task);

/*****************************************************************************************************************
-----函数功能    任务函数:更新
-----说明(备注)  BMS升级队列任务，负责将上位机下发的升级数据转发到BMS模块，
                并管理升级状态（初始化、数据转发、错误收尾、完成收尾）。
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_bms_queue_task_update(Task_T *tp_task)
{
	/* 统一检查任务有效性（升级对象、缓冲区、设备状态、错误） */
	if(b_bms_check_task_valid(tp_task) == false)
	{
		#if(boardUSE_OS)
		ulTaskNotifyTake(pdTRUE, bmsTASK_UPDATE_TIME);
		#endif  //boardUSE_OS
		return;
	}

	switch(tp_task->ucStep)
	{
		/* 步骤0：初始化升级环境 */
		case BMS_UPDATE_STEP_INIT:
		{
			if(tp_task->tReplyBuff.buff == NULL)
			{
				if(uPrint.tFlag.bBmsTask)
					log_w("bBmsTask:任务返回参数缓存器异常");

				bUpdate_SetErrCode(UEF_P_BUFF_NULL);
				cQueue_GotoStep(tp_task, BMS_UPDATE_STEP_END);
				break;
			}

			bUpdate_SetResult(URT_SLAVE, UTR_RUNNING);
			lwrb_reset(&tp_task->tReplyBuff);

			if(tBms.eDevState != DS_UPDATE_MODE)
				bBms_SetDevState(DS_UPDATE_MODE);

			cQueue_GotoStep(tp_task, STEP_NEXT);
		}

		/* 步骤1：等待升级完成 */
		case BMS_UPDATE_STEP_FORWARD_DATA:
		{
			/* 检查从机升级结果，完成则进入收尾 */
			if(tUpdate.eSlaveResult == UTR_OK || tUpdate.eSlaveResult == UTR_LATEST)
			{
				cQueue_GotoStep(tp_task, BMS_UPDATE_STEP_FINISH_CLEANUP);
				break;
			}

			//升级失败或取消
			if(tUpdate.eSlaveResult == UTR_FAIL || tUpdate.eSlaveResult == UTR_CANCEL)
			{
				cQueue_GotoStep(tp_task, BMS_UPDATE_STEP_ERROR_CLEANUP);
				break;
			}
		}
		break;

		/* 步骤2：升级错误,收尾 */
		case BMS_UPDATE_STEP_ERROR_CLEANUP:
		{
			if(tUpdate.eErrCode == UEF_NONE)
				bUpdate_SetErrCode(UEF_P_PENDING_FAIL);

			cQueue_GotoStep(tp_task, BMS_UPDATE_STEP_END);
		}
		break;

		/* 步骤3：升级完成，收尾 */
		case BMS_UPDATE_STEP_FINISH_CLEANUP:
		{
			cQueue_GotoStep(tp_task, STEP_NEXT);
		}
		break;

		/* 步骤4：结束 */
		case BMS_UPDATE_STEP_END:
		{
			bBms_SetDevState(DS_SHUT_DOWN);
			lwrb_reset(&tp_task->tReplyBuff);
			cBaiku_ResetRxBuff(tpBmsProtoRx);
			cQueue_GotoStep(tp_task, STEP_END);
		}
		break;

		default:
			cQueue_GotoStep(tp_task, STEP_END);
		break;
	}

	#if(boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, bmsTASK_UPDATE_TIME);
	#endif  //boardUSE_OS
}


/***********************************************************************************************************************
-----函数功能    检查BMS升级任务的有效性
-----说明(备注)  统一的任务有效性检查逻辑，包括升级对象、缓冲区、设备状态和错误检查
-----传入参数    tp_task: 任务结构体指针
-----输出参数    none
-----返回值      true: 任务有效  false: 任务无效（已设置错误码或跳转步骤）
************************************************************************************************************************/
static bool b_bms_check_task_valid(Task_T *tp_task)
{
	/* 参数合法性检查：任务指针为空则直接返回 */
	if(tp_task == NULL)
		return false;

	/* 升级对象无效则结束任务 */
	if(tUpdate.eObj != UO_BMS)
	{
		bUpdate_SetErrCode(UEF_P_INVALID_OBJ);
		cQueue_GotoStep(tp_task, STEP_END);
		return false;
	}

	/* 检查回复缓冲区是否有效 */
	if(tp_task->tReplyBuff.buff == NULL)
	{
		bUpdate_SetErrCode(UEF_P_BUFF_NULL);
		return false;
	}

	/* 检查设备是否处于升级模式，且任务队列无新的任务 */
	if(tSysInfo.eDevState != DS_UPDATE_MODE || lwrb_get_full(&tp_task->tQueueBuff))
	{
		cQueue_GotoStep(tp_task, STEP_END);
		return false;
	}

	/* 检查是否存在报错，若有错误则进入错误处理流程 */
	if(tUpdate.eErrCode != UEF_NONE)
	{
		cQueue_GotoStep(tp_task, BMS_UPDATE_STEP_ERROR_CLEANUP);
		return false;
	}

	return true;
}


/***********************************************************************************************************************
-----函数功能    BMS升级数据组帧发送
-----说明(备注)  将升级数据按照Baiku协议组帧后发送至BMS模块，不含等待回复逻辑
-----传入参数    cmd:  协议命令码
                data: 数据指针
                len:  数据长度(最大255字节)
-----输出参数    none
-----返回值      true:发送成功  false:发送失败
************************************************************************************************************************/
__STATIC_INLINE bool b_send_baiku_frame(u8 cmd, u8* data, u8 len)
{
	if(tpBmsProtoTx == NULL)
		return false;

	if(cBaiku_ProtoCreate(tpBmsProtoTx, cmd, data, len) <= 0)
		return false;

	bBmsUseFlag = true;

	return bBms_DataSendStart(tpBmsProtoTx->ucaFrameData, tpBmsProtoTx->ucFrameLen);
}

#endif  //boardBMS_EN  && boardUPDATE
