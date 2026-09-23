/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_update.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器升级队列任务实现(基于Megmeet协议的固件在线升级)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_queue_task_update.h"
#include "Sys/sys_queue_task_update.h"
#include <stdbool.h>

#if (boardDCAC_EN && boardUPDATE)
#include "MD_Dcac/md_dcac_queue_task.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_prot_frame.h"
#include "Megmeet/megmeet_proto.h"

//****************************************************Macros********************************************************************//
#define			dcacTASK_UPDATE_CYCLE_TIME				500		/*!< DCAC升级任务周期，单位：ms */

#define			dcacUPDATE_HS_TIMEOUT_MS				1000	/*!< 握手阶段单次等待超时时间，单位：ms */
#define			dcacUPDATE_F7_WAIT_MS					1000	/*!< 等待F7(跳转BOOT回复)超时时间，单位：ms */
#define			dcacUPDATE_BOOT_JUMP_DELAY_MS			500		/*!< BOOT跳转后等待稳定延时，单位：ms */

#define			dcacUPDATE_MAX_RETRY_COUNT				3		/*!< 最大重试次数 */

#define			dcacUPDATE_SEND_FAIL_DELAY_MS			100		/*!< 发送失败后重试间隔，单位：ms */
#define			dcacUPDATE_RESEND_DELAY_MS				200		/*!< 重发数据帧间隔，单位：ms */

//****************************************************Parameter Initialization**************************************************//
DcacPrepStage_E eDcacPrepStage;
DcacFwTransStage_E eDcacFwTransStage;

//****************************************************Function Declaration******************************************************//
static bool b_dcac_check_task_valid(Task_T *p_task);
static int8_t c_dcac_prepare_update(Task_T *p_task, uint16_t us_reply_len, uint16_t us_cycle_time);
static int8_t c_dcac_update_firmware_transfer(Task_T *p_task, uint16_t us_reply_len, uint16_t us_cycle_time);
static bool b_dcac_check_retry_limit(Task_T *p_task, UpdateErrCode_E e_err_code);
static bool b_dcac_check_wait_timeout(Task_T *p_task, uint16_t us_timeout_ms, uint16_t us_cycle_time);

/***********************************************************************************************************************
 * 函数功能    : DCAC升级队列任务主函数
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dcac_queue_task_update(Task_T *p_task)
{
	int8_t c_ret = 0;
	uint16_t us_cycle_time = dcacTASK_UPDATE_CYCLE_TIME;

	if (p_task == NULL)
		return;

	/* 动态调整升级固件传输阶段的轮询周期，以提升传输速率 */
	if (p_task->ucStep == DUS_FW_TRANS)
		us_cycle_time = 100;

	#if (boardUSE_OS)
	ulTaskNotifyTake(pdTRUE, us_cycle_time);
	#endif  /* boardUSE_OS */

	/* 升级失败,进入收尾流程 */
	if (b_dcac_check_task_valid(p_task) == false)
		cQueue_GotoStep(p_task, DUS_ERROR_CLEANUP);

	/* 获取任务缓存器的返回值长度，判断是否有数据需要处理 */
	uint16_t us_reply_len = lwrb_get_full(&p_task->tReplyBuff);

	switch (p_task->ucStep)
	{
		/*---------------- 步骤0：初始化升级环境 ----------------*/
		case DUS_INIT:
		{
			if ((tDcacMegmeetProtoTx == NULL || tpDcacMegmeetProtoRx == NULL) && bDcac_MegmeetProtInit() == false)
			{
				bUpdate_SetErrCode(UEF_DQ_PROTO_INIT_FAIL);
				break;
			}

			bDcacUseFlag = true;
			bDcac_SetDevState(DS_SHUT_DOWN);
			bUpdate_SetResult(URT_SLAVE, UTR_RUNNING);
			bDcac_SetPrepStage(p_task, DPS_IDLE); /* 复位升级准备阶段 */
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		/*---------------- 步骤1：升级准备阶段（F0/F1/F6/F7/延时/F2/F3） ----------------*/
		case DUS_PREPARE:
		{
			c_ret = c_dcac_prepare_update(p_task, us_reply_len, us_cycle_time);
			if (c_ret < 0)
			{
				cQueue_GotoStep(p_task, DUS_ERROR_CLEANUP);
				break;
			}
			else if (c_ret == 0)
				break;
			/* c_ret > 0: 准备完成，进入数据交互阶段 */
			bDcac_SetFwTransStage(p_task, DFTS_IDLE); /* 复位升级阶段 */
			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		/*---------------- 步骤2：根据升级阶段执行数据交互（A3/A4/A5/A6） ----------------*/
		case DUS_FW_TRANS:
		{
			c_ret = c_dcac_update_firmware_transfer(p_task, us_reply_len, us_cycle_time);
			if (c_ret < 0)
			{
				cQueue_GotoStep(p_task, DUS_ERROR_CLEANUP); /* 进入异常 */
				break;
			}
			else if (c_ret == 0)
				break;

			cQueue_GotoStep(p_task, DUS_FINISH_CLEANUP); /* 升级完成 */
		}
		break;

		/*---------------- 步骤3：升级错误,收尾 ----------------*/
		case DUS_ERROR_CLEANUP:
		{
			if (tUpdate.eErrCode != UEF_NONE)
				bUpdate_SetResult(URT_SLAVE, UTR_FAIL);

			/* 若上位机还在等待，主动发送取消指令防止上位机死等 */
			if (tUpdate.eHostResult == UTR_RUNNING)
				c_print_cs_C8_trans_cancel();

			cQueue_GotoStep(p_task, DUS_END);
		}
		break;

		/*---------------- 步骤4：升级完成，收尾 ----------------*/
		case DUS_FINISH_CLEANUP:
		{
			/* 上机位还没提示完成,就强制让上机位完成升级 */
			if (tUpdate.eHostResult != UTR_OK)
				bUpdate_SetResult(URT_HOST, UTR_OK);

			cQueue_GotoStep(p_task, STEP_NEXT);
		}
		break;

		/*---------------- 步骤5：结束 ---------------------------*/
		case DUS_END:
		{
			/* 波特率恢复正常 */
			bDcac_IfaceSetBaud(dcacUSART_BAUD);

			bDcac_SetDevState(DS_SHUT_DOWN);
			lwrb_reset(&p_task->tReplyBuff);
			cModbus_ResetTx(tpDcacProtoTx, tpDcacProtoTx->usFrameDataSize);
			cModbus_ResetRxBuff(tpDcacProtoRx);
			cQueue_GotoStep(p_task, STEP_END); /* 退出当前任务,等待重新载入 */
		}
		break;

		default:
		{
			cQueue_GotoStep(p_task, STEP_END);
		}
		break;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 检查DCAC升级任务的有效性
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : true: 任务有效  false: 任务无效
 ************************************************************************************************************************/
static bool b_dcac_check_task_valid(Task_T *p_task)
{
	/* 参数合法性检查：任务指针为空则直接返回 */
	if (p_task == NULL)
		return false;

	/* 升级对象无效则结束任务 */
	if (!IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
	{
		bUpdate_SetErrCode(UEF_DQ_INVALID_OBJ);
		return false;
	}

	/* 检查回复缓冲区是否有效 */
	if (p_task->tReplyBuff.buff == NULL)
	{
		bUpdate_SetErrCode(UEF_DQ_BUFF_NULL);
		return false;
	}

	/* 检查设备是否处于升级模式，且任务队列无残留数据 */
	if (tSysInfo.eDevState != DS_UPDATE_MODE || lwrb_get_full(&p_task->tQueueBuff))
		return false;

	/* 检查是否存在报错，若有错误则进入错误处理流程 */
	if (tUpdate.eErrCode != UEF_NONE && p_task->ucStep < DUS_ERROR_CLEANUP)
		return false;

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 检查重试次数是否超限
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针, e_err_code: 超限错误码
 * 输出参数    : 无
 * 返回值      : true: 超限  false: 未超限
 ************************************************************************************************************************/
static bool b_dcac_check_retry_limit(Task_T *p_task, UpdateErrCode_E e_err_code)
{
	p_task->usStepRepeatCnt++;
	if (p_task->usStepRepeatCnt > dcacUPDATE_MAX_RETRY_COUNT)
	{
		bUpdate_SetErrCode(e_err_code);
		return true;
	}

	return false;
}

/***********************************************************************************************************************
 * 函数功能    : 检查等待是否超时
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针, us_timeout_ms: 超时时间(ms), us_cycle_time: 周期(ms)
 * 输出参数    : 无
 * 返回值      : true: 超时  false: 未超时
 ************************************************************************************************************************/
static bool b_dcac_check_wait_timeout(Task_T *p_task, uint16_t us_timeout_ms, uint16_t us_cycle_time)
{
	if (us_cycle_time == 0)
		return true;

	return bQueue_IsStepTimeout(p_task, us_timeout_ms / us_cycle_time);
}

/***********************************************************************************************************************
 * 函数功能    : DCAC升级准备阶段处理
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针, us_reply_len: 回复数据长度, us_cycle_time: 任务周期
 * 输出参数    : 无
 * 返回值      : 正值:准备完成  0:继续  负值:失败
 ************************************************************************************************************************/
static int8_t c_dcac_prepare_update(Task_T *p_task, uint16_t us_reply_len, uint16_t us_cycle_time)
{
	if (tUpdate.eHostResult == UTR_CANCEL)
	{
		bUpdate_SetErrCode(UEF_DQ_CANCEL_REQ);
		return -1;
	}

	switch (eDcacPrepStage)
	{
		/* 复位初始化 */
		case DPS_IDLE:
		{
			lwrb_reset(&p_task->tReplyBuff);
			bDcac_SetPrepStage(p_task, DPS_SEND_F0);
		}
		break;

		/*---------------- 步骤1：发送F0请求升级 ----------------*/
		case DPS_SEND_F0:
		{
			if (b_dcac_check_retry_limit(p_task, UEF_DQ_F0_RETRY_OVER))
				return -1;

			if (b_dcac_send_f0(0) == false)
			{
				vTaskDelay(dcacUPDATE_SEND_FAIL_DELAY_MS);
				break;
			}

			bDcac_SetPrepStage(p_task, DPS_WAIT_F1);
		}
		break;

		/*---------------- 步骤2：等待F1回复 ----------------*/
		case DPS_WAIT_F1:
			/* 超时进入F6跳转BOOT */
			if (b_dcac_check_wait_timeout(p_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time) == false)
				break;

			/* 直接切换stage,不重置重试计数器,保留F6重试累积 */
			eDcacPrepStage = DPS_SEND_F6;
			/* fall through */

		/*---------------- 步骤3：发送F6请求跳转BOOT ----------------*/
		case DPS_SEND_F6:
		{
			if (b_dcac_check_retry_limit(p_task, UEF_DQ_F6_RETRY_OVER))
				return -4;

			if (b_dcac_send_f6(true) == false)
			{
				vTaskDelay(dcacUPDATE_SEND_FAIL_DELAY_MS);
				break;
			}

			bDcac_SetPrepStage(p_task, DPS_WAIT_F7);
		}
		break;

		/*---------------- 步骤4：等待F7回复 ----------------*/
		case DPS_WAIT_F7:
		{
			p_task->usStepRepeatCnt++;
			if (p_task->usStepRepeatCnt > 1)
			{
				p_task->usStepRepeatCnt = 0;
				b_dcac_send_f0(0);
				break;
			}

			if (b_dcac_check_wait_timeout(p_task, dcacUPDATE_F7_WAIT_MS, us_cycle_time) == true)
				b_dcac_send_f6(false);
		}
		break;

		/*---------------- 步骤5：BOOT跳转后延时等待 ----------------*/
		case DPS_BOOT_DELAY:
		{
			if (b_dcac_check_wait_timeout(p_task, dcacUPDATE_BOOT_JUMP_DELAY_MS, us_cycle_time) == true)
				bDcac_SetPrepStage(p_task, DPS_SEND_F0); /* 延时结束，再次发送F0握手 */
		}
		break;

		/*---------------- 步骤6：发送F2设置波特率 ----------------*/
		case DPS_SEND_F2:
		{
			if (b_dcac_check_retry_limit(p_task, UEF_DQ_F2_RETRY_OVER))
				return -6;

			/* 波特率已是目标值 */
			if (tUpdate.ulBaud == dcacUSART_BAUD)
			{
				bDcac_SetDevState(DS_UPDATE_MODE);
				bDcac_SetPrepStage(p_task, DPS_WAIT_PRINT_UPDATE_REQ);
				break;
			}

			if (b_dcac_send_f2(tUpdate.ulBaud, true) == false)
			{
				vTaskDelay(dcacUPDATE_SEND_FAIL_DELAY_MS);
				break;
			}

			bDcac_SetPrepStage(p_task, DPS_WAIT_F3);
		}
		break;

		/*---------------- 步骤7：等待F3回复 ------------------------*/
		case DPS_WAIT_F3:
		{
			/* 等待超时 */
			if (b_dcac_check_wait_timeout(p_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time))
				b_dcac_send_f2(tUpdate.ulBaud, false);
		}
		break;

		/*---------------- 步骤8：等待Print请求升级,等待5S ----------------*/
		case DPS_WAIT_PRINT_UPDATE_REQ:
		{
			if (b_dcac_check_wait_timeout(p_task, 5000, us_cycle_time) == true)
				bDcac_SetPrepStage(p_task, DPS_PRINT_SEND_C4);
		}
		break;

		/*---------------- 步骤9：发送C4获取头文件 ----------------*/
		case DPS_PRINT_SEND_C4:
		{
			if (c_print_cs_C4_req_start_send() <= 0)
			{
				vTaskDelay(dcacUPDATE_SEND_FAIL_DELAY_MS);
				break;
			}

			bDcac_SetPrepStage(p_task, DPS_PRINT_WAIT_REPLY_C5);
		}
		break;

		/*---------------- 步骤10：等待回复C5 ----------------*/
		case DPS_PRINT_WAIT_REPLY_C5:
		{
			if (b_dcac_check_wait_timeout(p_task, 1000, us_cycle_time) == true)
			{
				p_task->usStepWaitCnt = 0;
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_C4_RETRY_OVER))
					return -13;

				/* 等待超时,重新发送C4请求 */
				c_print_cs_C4_req_resend_curr();
			}
		}
		break;

		/*---------------- 步骤11：等待A2文件头回复 ----------------*/
		case DPS_WAIT_A2:
		{
			if (b_dcac_check_wait_timeout(p_task, 1000, us_cycle_time) == true)
			{
				p_task->usStepWaitCnt = 0;
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_A2_RETRY_OVER))
				{
					lwrb_reset(&p_task->tReplyBuff);
					return -14;
				}

				/* 重新发送: 校验数据 */
				uint8_t uca_buff[MEGMEET_FILE_HEAD_SIZE] = {0};
				if (us_reply_len != MEGMEET_FILE_HEAD_SIZE)
				{
					bUpdate_SetErrCode(UEF_DQ_A2_RESEND_LEN_ERR);
					break;
				}

				/* 发送文件头 */
				if (b_dcac_update_buf_peek(p_task, uca_buff, us_reply_len) == false)
				{
					bUpdate_SetErrCode(UEF_DQ_A2_RESEND_PEEK_FAIL);
					break;
				}

				if (b_dcac_cs_send_fw_data(MEGMEET_CMD_FILE_HEAD, uca_buff, us_reply_len, false) == false)
				{
					vTaskDelay(dcacUPDATE_RESEND_DELAY_MS);
					break;
				}
			}
		}
		break;

		case DPS_FINISH_CLEANUP:
			return 1;

		default:
		{
		}
		break;
	}

	return 0;
}

/***********************************************************************************************************************
 * 函数功能    : DCAC固件传输处理
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针, us_reply_len: 回复数据长度, us_cycle_time: 任务周期
 * 输出参数    : 无
 * 返回值      : 正值:升级完成  0:继续  负值:失败
 ************************************************************************************************************************/
static int8_t c_dcac_update_firmware_transfer(Task_T *p_task, uint16_t us_reply_len, uint16_t us_cycle_time)
{
	switch (eDcacFwTransStage)
	{
		case DFTS_IDLE:
			bDcac_SetFwTransStage(p_task, DFTS_SLAVE_READY_OK);
			/* fall through */

		case DFTS_SLAVE_READY_OK: /* 进入请求数据阶段，通知Print准备数据 */
			bDcac_SetFwTransStage(p_task, DFTS_HOST_REQ_DATA);
			/* fall through */

		case DFTS_HOST_REQ_DATA: /* C6主机请求数据 */
			if (c_print_cs_C6_req_cont_send() <= 0)
			{
				vTaskDelay(dcacUPDATE_SEND_FAIL_DELAY_MS);
				break;
			}
			bDcac_SetFwTransStage(p_task, DFTS_WAIT_HOST_REPLY);
			break;

		case DFTS_WAIT_HOST_REPLY: /* C5 等待主机数据 */
			if (b_dcac_check_wait_timeout(p_task, 1000, us_cycle_time) == true)
			{
				p_task->usStepWaitCnt = 0;
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_C5_RETRY_OVER))
					return -9;

				/* 没有回复,重复请求 */
				c_print_cs_C4_req_resend_curr();
			}
			break;

		case DFTS_SEND_FW_DATA: /* 发送A3固件包数据 */
		{
			/* 校验数据 (A3负载 = 2字节包序号 + 固件数据) */
			uint8_t uca_buff[MEGMEET_FRM_PKG_SIZE + 2] = {0};
			if (us_reply_len == 0 || us_reply_len > (MEGMEET_FRM_PKG_SIZE + 2))
			{
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_A3_LEN_RETRY_OVER))
					return -11;

				bDcac_SetFwTransStage(p_task, DFTS_HOST_REQ_DATA); /* 数据长度异常，返回上一步重新请求 */
				b_dcac_update_buf_reset(p_task);                  /* 清空异常数据 */
				break;
			}

			/* 读取数据 */
			if (b_dcac_update_buf_peek(p_task, uca_buff, us_reply_len) == false)
			{
				bDcac_SetFwTransStage(p_task, DFTS_HOST_REQ_DATA); /* 缓存异常，重新请求 */
				break;
			}

			/* 重复发送计数 */
			if (b_dcac_check_retry_limit(p_task, UEF_DQ_A3_RETRY_OVER))
				return -1;

			/* 发送失败 */
			if (b_dcac_cs_send_fw_data(MEGMEET_CMD_FIRMWARE_DATA, uca_buff, us_reply_len, false) == false)
			{
				vTaskDelay(dcacUPDATE_RESEND_DELAY_MS);
				break;
			}

			bDcac_SetFwTransStage(p_task, DFTS_WAIT_SLAVE_REPLY);
		}
		break;

		case DFTS_WAIT_SLAVE_REPLY: /* 等待A4固件包数据回复 */
			if (b_dcac_check_wait_timeout(p_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time) == true)
			{
				p_task->usStepWaitCnt = 0;
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_A4_RESEND_OVER))
					return -2;

				bDcac_SetFwTransStage(p_task, DFTS_SEND_FW_DATA); /* 返回上一步继续 */
			}
			break;

		case DFTS_QUERY_SLAVE_RESULT: /* 发送查询结果请求A5 */
			vTaskDelay(dcacUPDATE_RESEND_DELAY_MS);
			if (b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj), MEGMEET_CMD_QUERY_RESULT, NULL, 0) == false)
			{
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_A5_RETRY_OVER))
					return -6;

				vTaskDelay(dcacUPDATE_RESEND_DELAY_MS);
				break;
			}

			bDcac_SetFwTransStage(p_task, DFTS_WAIT_SLAVE_RESULT_REPLY);
			break;

		case DFTS_WAIT_SLAVE_RESULT_REPLY: /* 等待A6查询结果回复 */
			if (b_dcac_check_wait_timeout(p_task, dcacUPDATE_HS_TIMEOUT_MS, us_cycle_time) == true)
			{
				if (b_dcac_check_retry_limit(p_task, UEF_DQ_A6_WAIT_RETRY_OVER))
					return -7;

				bDcac_SetFwTransStage(p_task, DFTS_QUERY_SLAVE_RESULT); /* 返回上一步继续 */
			}
			break;

		case DFTS_FINISH_CLEANUP: /* 升级完成，收尾 */
			return 1;

		default:
		{
		}
		break;
	}

	return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 获取升级阶段
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 阶段状态码
 ************************************************************************************************************************/
int8_t cDcac_GetUpdateStage(void)
{
	if (tSysInfo.eDevState != DS_UPDATE_MODE ||
		!IS_DCAC_UPDATE_OBJ(tUpdate.eObj) ||
		tDcac.eDevState != DS_UPDATE_MODE)
		return -1;

	if (tpDcacTask == NULL)
		return -2;

	if (tpDcacTask->ucID != DTI_UPDATE)
		return -3;

	if (tpDcacTask->ucStep == DUS_ERROR_CLEANUP)
		return UPDATE_QUEUE_STAGE_ERR;

	if (tpDcacTask->ucStep == DUS_END)
		return UPDATE_QUEUE_STAGE_WAIT_RESTART;

	if (tpDcacTask->ucStep > DUS_FINISH_CLEANUP)
		return UPDATE_QUEUE_STAGE_FINISH;

	return UPDATE_QUEUE_STAGE_RUNNING;
}

/***********************************************************************************************************************
 * 函数功能    : 设置升级准备阶段
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针, stage: 升级准备阶段
 * 输出参数    : 无
 * 返回值      : true: 成功  false: 失败
 ************************************************************************************************************************/
bool bDcac_SetPrepStage(Task_T *p_task, DcacPrepStage_E stage)
{
	p_task->usStepWaitCnt = 0;
	p_task->usStepRepeatCnt = 0;

	if (stage != eDcacPrepStage)
	{
		eDcacPrepStage = stage;
		return true;
	}

	return false;
}

/***********************************************************************************************************************
 * 函数功能    : 设置DCAC固件传输阶段
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针, stage: DCAC固件传输阶段
 * 输出参数    : 无
 * 返回值      : true: 成功  false: 失败
 ************************************************************************************************************************/
bool bDcac_SetFwTransStage(Task_T *p_task, DcacFwTransStage_E stage)
{
	p_task->usStepWaitCnt = 0;
	p_task->usStepRepeatCnt = 0;

	if (stage != eDcacFwTransStage)
	{
		eDcacFwTransStage = stage;
		return true;
	}

	return false;
}

#endif  /* boardDCAC_EN && boardUPDATE */
