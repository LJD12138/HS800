/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Update
 * File    : baiku_proto.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BaiKu自定义升级协议实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Update/baiku_proto.h"

#if (boardUPDATE)
#include "Print/print_task.h"

//****************************************************Function Declaration******************************************************//
static void v_set_work_state(BaiKuProto_T *obj, BaikuWorkState_E state);


/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : BAIKU接收系统协议解析处理
 * 传入参数    : obj:   BAIKU对象
 *               proto: 协议帧
 * 输出参数    : none
 * 返回值      : >0:成功  <=0:失败
 ************************************************************************************************************************/
static s8 c_rec_proc(BaiKuProto_T *obj, BaikuProtoRx_t *proto)
{
	s8 c_result = 1;

	if (obj == NULL || proto == NULL)
		c_result = -1;

	obj->bStartSendFrm = true;

	if (c_result)
	{
		obj->usWaitStartOutTimeCnt = BAIKU_START_TIMEOUT_MS;
		obj->usWaitExitOutTimeCnt = BAIKU_END_TIMEOUT_MS;

		switch (proto->ucCmd)  /* 检查帧头标志 */
		{
			case baikuCMD_REPLY_DATA:  /* 回复文件数据 */
			{
				/* 帧号合法性检测:
				 *   首帧接收的帧序号不对(ucFrmCnt为程序要求接收的序号)
				 *   接收帧号小于当前帧号-1
				 *   接收帧号大于当前帧号
				 */
				if (((1 == obj->ucFrmCnt) && (1 != proto->ucSN)) ||
					((obj->ucFrmCnt - 1) > proto->ucSN) ||
					(obj->ucFrmCnt < proto->ucSN))
				{
					c_result = -3;
					break;
				}

				if (obj->ucFrmCnt != proto->ucSN)
				{
					c_result = -4;
					break;
				}

				/* 第一包数据准备开始处理 */
				if (obj->eState == BAIKU_STATE_STANDBY &&
					proto->ucSN == 1)
				{
					(*obj->v_rec_start)();
					v_set_work_state(obj, BAIKU_STATE_RECEIVING);
				}

				/* 把接收到的数据添加到处理回调函数 */
				(*obj->v_proc_check_ok_rec_data)(proto->ucpValidData, proto->ucValidLen);
				/* 包计数增加 */
				obj->ucFrmCnt++;
			}
			break;

			case baikuCMD_REPLY_FINISH:  /* 回复结束标志 */
			{
				v_set_work_state(obj, BAIKU_STATE_FINISH);
			}
			break;

			case baikuCMD_REPLY_CANEL:  /* 回复取消标志 */
			{
				v_set_work_state(obj, BAIKU_STATE_CANCEL);
			}
			break;

			default:
			{
				if (uPrint.tFlag.bBaiKuProto)
					log_w("bBaiKuProto:未定义的指令0x%x", proto->ucCmd);
			}
			break;
		}
	}

	if (c_result <= 0)
		v_set_work_state(obj, BAIKU_STATE_CANCEL);

	return c_result;
}

/***********************************************************************************************************************
 * 函数功能    : 设置工作状态
 * 说明(备注)  : none
 * 传入参数    : obj:   BAIKU对象
 *               state: 目标工作状态
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_set_work_state(BaiKuProto_T *obj, BaikuWorkState_E state)
{
	switch (state)
	{
		case BAIKU_STATE_IDLE:
		{
		}
		break;

		case BAIKU_STATE_STANDBY:
		{
		}
		break;

		case BAIKU_STATE_RECEIVING:
		{
		}
		break;

		case BAIKU_STATE_FINISH:
		{
		}
		break;

		case BAIKU_STATE_CANCEL:
		{
		}
		break;

		case BAIKU_STATE_STOP:
		{
		}
		break;

		default:
		{
		}
		break;
	}

	obj->eState = state;
}

/***********************************************************************************************************************
 * 函数功能    : 发送命令
 * 说明(备注)  : none
 * 传入参数    : obj: BAIKU对象
 *               cmd: 命令字节
 * 输出参数    : none
 * 返回值      : true:成功  false:失败
 ************************************************************************************************************************/
static bool b_send_cmd(BaiKuProto_T *obj, u8 cmd)
{
	if (obj->bStartSendFrm == false)
		return false;

	/* 开始发送 */
	if ((*obj->c_xmodem_trans_data)(cmd, NULL, 0) <= 0)
		return false;

	obj->usFrmOvertimeCnt = BAIKU_RX_TIMEOUT_MS;
	obj->bStartSendFrm = false;
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : BaiKu协议主处理程序
 * 说明(备注)  : BAIKU接收系统主处理过程
 * 传入参数    : obj:   BAIKU对象
 *               proto: 协议帧
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vBaiKuProto_Proto(BaiKuProto_T *obj, BaikuProtoRx_t *proto)
{
	if (!obj)
		return;

	s8 c_result = 0;

	switch (obj->eState)
	{
		/* 空闲状态 */
		case BAIKU_STATE_IDLE:
		{
			v_set_work_state(obj, BAIKU_STATE_STANDBY);
		}
		break;

		/* 就绪状态：等待接收第一个有效数据帧 */
		case BAIKU_STATE_STANDBY:
		{
			/* 发送一个NAK */
			b_send_cmd(obj, baikuCMD_RRQ_START_SEND);

			/* 成功接收数据 */
			c_result = (*obj->c_xmodem_rec_data)(NULL, 0, NULL);
			if (c_result > 0)
				c_rec_proc(obj, proto);
			else
			{
				/* 等待开始超时 */
				if (obj->usWaitStartOutTimeCnt == 0)
				{
					(*obj->v_rec_end)(2);
					v_set_work_state(obj, BAIKU_STATE_STOP);
					break;
				}
			}
		}
		break;

		/* 接收状态：若当前处于接收数据状态 */
		case BAIKU_STATE_RECEIVING:
		{
			if (obj->usWaitStartOutTimeCnt)
				/* 发送一个ACK 继续 */
				b_send_cmd(obj, baikuCMD_RRQ_CONT_SEND);
			else
				/* 发送一个NAK 再次 */
				b_send_cmd(obj, baikuCMD_RRQ_START_SEND);

			/* 成功接收数据 */
			c_result = (*obj->c_xmodem_rec_data)(NULL, 0, NULL);
			if (c_result > 0)
				c_rec_proc(obj, proto);
			else
			{
				/* 等待退出超时 */
				if (obj->usWaitExitOutTimeCnt == 0)
				{
					if (uPrint.tFlag.bBaiKuProto)
						log_w("bBaiKuProto:等待超时,停止升级");

					v_set_work_state(obj, BAIKU_STATE_CANCEL);
					break;
				}
			}
		}
		break;

		/* 完成状态：BAIKU传输完成处理 */
		case BAIKU_STATE_FINISH:
		{
			/* 发送一个ACK 继续 */
			b_send_cmd(obj, baikuCMD_RRQ_CONT_SEND);

			/* 产生回调，告知上层当前传输已完成 */
			(*obj->v_rec_end)(0);

			/* 复位BAIKU */
			bBaiKuProto_Reset(obj, proto);
		}
		break;

		/* 取消状态 */
		case BAIKU_STATE_CANCEL:
		{
			if (uPrint.tFlag.bBaiKuProto)
				sMyPrint("vBaiKuProto:BAIKU_STATE_CANCEL\r\n");

			b_send_cmd(obj, baikuCMD_REPLY_CANEL);

			v_set_work_state(obj, BAIKU_STATE_STOP);
		}
		break;

		/* 结束状态 */
		case BAIKU_STATE_STOP:
		{
			if (uPrint.tFlag.bBaiKuProto)
				sMyPrint("vBaiKuProto:BAIKU_STATE_STOP \r\n");

			/* 产生回调，告知上层当前传输已停止 */
			(*obj->v_rec_end)(1);
			/* 复位BAIKU */
			bBaiKuProto_Reset(obj, proto);
		}
		break;

		default:
		{
		}
		break;
	}
}

/***********************************************************************************************************************
 * 函数功能    : Tick计时
 * 说明(备注)  : none
 * 传入参数    : obj: BAIKU对象
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vBaiKuProto_TickTime(BaiKuProto_T *obj)
{
	if (obj == NULL)
		return;

	/* 帧等待计时 */
	if (obj->usFrmOvertimeCnt > 0)
	{
		obj->usFrmOvertimeCnt--;
		if (obj->usFrmOvertimeCnt == 0)
			obj->bStartSendFrm = true;
	}

	/* 等待开始计时 */
	if (obj->usWaitStartOutTimeCnt > 0)
		obj->usWaitStartOutTimeCnt--;

	if (obj->usWaitExitOutTimeCnt > 0)
		obj->usWaitExitOutTimeCnt--;
}

/***********************************************************************************************************************
 * 函数功能    : 复位协议
 * 说明(备注)  : 复位BAIKU对象状态
 * 传入参数    : obj:   BAIKU传输对象
 *               proto: 协议帧
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void bBaiKuProto_Reset(BaiKuProto_T *obj, BaikuProtoRx_t *proto)
{
	if (obj == NULL)
		return;

	/* 将BAIKU恢复到空闲状态 */
	obj->eState = BAIKU_STATE_IDLE;
	/* 清除传输等待计数 */
	obj->usWaitStartOutTimeCnt = BAIKU_START_TIMEOUT_MS;
	/* 清除接收超时次数计数 */
	obj->usWaitExitOutTimeCnt = BAIKU_END_TIMEOUT_MS;
	/* 清除帧计数 */
	obj->ucFrmCnt = 1;

	cBaiku_ResetRxBuff(proto);
}

#endif  /* boardUPDATE */
