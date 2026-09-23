/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Update
 * File    : update_main.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 固件升级主模块实现（Xmodem / BaiKu协议通用回调）
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Update/update_main.h"

#if (boardUPDATE)
#include "Flash/flash_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task.h"
#include "Sys/sys_queue_task_update.h"

#include "boot_info.h"

//****************************************************Function Declaration******************************************************//
static s8 c_frame_trans_cd(u8 cmd, u8 *buf, u16 len);
static s8 c_frame_rec_cd(u8 *buf, u16 buf_len, u16 *len);
static void v_proc_check_ok_rec_data_cd(u8 *buf, u16 len);
static void v_rec_start_cd(void);
static void v_rec_end_cd(u8 code);


/***********************************************************************************************************************
 * 函数功能    : Xmodem协议结构体对象初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
Xmodem_T tXmodem = {
	.bStartSendFrm              = true,                     /* 可以发送数据 */
	.frm_cnt                    = 1,                        /* 帧计数 指向下一帧 从1开始 */
	.usRecLen                   = 0,                        /* 接受长度 */
	.usFrmOvertimeCnt           = 0,                        /* 计数清零 */
	.usWaitStartOutTimeCnt      = XMODEM_START_TIMEOUT_MS,  /* 计数清零 */
	.usWaitExitOutTimeCnt       = XMODEM_END_TIMEOUT_MS,    /* 计数清零 */
	.eFrameLen                  = FRAME_LEN_128,            /* 128的长度 */
	.eCheckMode                 = CHECK_MODE_ADD,           /* 累加和 */
	.eState                     = XMODEM_STATE_IDLE,        /* 工作状态为空 */
	.eRecState                  = REC_STATE_IDLE,
	.buf                        = {0},                      /* 缓冲区 */
	.c_xmodem_trans_data        = c_frame_trans_cd,
	.c_xmodem_rec_data          = c_frame_rec_cd,
	.v_proc_check_ok_rec_data   = v_proc_check_ok_rec_data_cd,
	.v_rec_start                = v_rec_start_cd,
	.v_rec_end                  = v_rec_end_cd,
};

/***********************************************************************************************************************
 * 函数功能    : Baiku私有协议结构体对象初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
BaiKuProto_T tBaiKuProto = {
	.bStartSendFrm              = true,                     /* 可以发送数据 */
	.ucFrmCnt                   = 1,                        /* 帧计数 指向下一帧 从1开始 */
	.usFrmOvertimeCnt           = 0,                        /* 计数清零 */
	.usWaitStartOutTimeCnt      = BAIKU_START_TIMEOUT_MS,   /* 计数清零 */
	.usWaitExitOutTimeCnt       = BAIKU_END_TIMEOUT_MS,     /* 计数清零 */
	.eState                     = BAIKU_STATE_IDLE,         /* 工作状态为空 */
	.c_xmodem_trans_data        = c_frame_trans_cd,
	.c_xmodem_rec_data          = c_frame_rec_cd,
	.v_proc_check_ok_rec_data   = v_proc_check_ok_rec_data_cd,
	.v_rec_start                = v_rec_start_cd,
	.v_rec_end                  = v_rec_end_cd,
};

/***********************************************************************************************************************
 * 函数功能    : 数据帧发送回调函数
 * 说明(备注)  : 协议生成需要发送的数据，就会调用此函数把数据发送出去
 * 传入参数    : cmd: 命令字节
 *               buf: 数据缓冲区
 *               len: 数据长度
 * 输出参数    : none
 * 返回值      : >0:成功  <=0:失败
 ************************************************************************************************************************/
static s8 c_frame_trans_cd(u8 cmd, u8 *buf, u16 len)
{
	s8 c_result = 1;

	switch (tUpdate.eChType)
	{
		#if (boardCONSOLE_EN)
		case CT_CONSOLE:
		{
			switch (tUpdate.eProtoType)
			{
				case PT_XMODEM:
				{
					bConsole_DataSendStart(&cmd, 1);
				}
				break;

				case PT_BAIKU:
				{
					if (cmd == 0)
						return -1;

					c_result = cBaiku_ProtoCreate(tUpdate.tpProtoTx, cmd, buf, len);
					if (c_result > 0)
					{
						if (bConsole_DataSendStart(tUpdate.tpProtoTx->ucaFrameData, tUpdate.tpProtoTx->ucFrameLen) == true)
							return true;

						c_result = 0;
					}
				}
				break;

				default:
				{
				}
				break;
			}
		}
		break;
		#endif  /* boardCONSOLE_EN */

		#if (boardPRINT_IFACE)
		case CT_PRINT:
		{
			switch (tUpdate.eProtoType)
			{
				case PT_XMODEM:
				{
					lwrb_write(tUpdate.pTxBuff, &cmd, 1);
					if (len)
						lwrb_write(tUpdate.pTxBuff, buf, len);
					bPrint_SendDataToUsart();
				}
				break;

				case PT_BAIKU:
				{
					if (cmd == 0)
						return -1;

					c_result = cBaiku_ProtoCreate(tUpdate.tpProtoTx, cmd, buf, len);
					if (c_result > 0)
					{
						lwrb_write(tUpdate.pTxBuff, tUpdate.tpProtoTx->ucaFrameData, tUpdate.tpProtoTx->ucFrameLen);
						bPrint_SendDataToUsart();
						return true;
					}
				}
				break;

				default:
				{
				}
				break;
			}
		}
		break;
		#endif  /* boardPRINT_IFACE */

		#if (boardWIFI_USARTX)
		case CT_WIFI:
		{
			memcpy(ucaWiFiTxDmaBuffData, buf, len);
			bWIFI_DataSendStart(len);
		}
		break;
		#endif  /* boardWIFI_USARTX */

		default:
		{
			log_i("当前通道%d未开启,请重新选择通道", tUpdate.eChType);
			c_result = 0;
		}
		break;
	}

	return c_result;
}

/***********************************************************************************************************************
 * 函数功能    : 检测接收到的数据
 * 说明(备注)  : 协议通过不断检测此函数，判断是否有数据接收到，把接收到的数据读取出来
 * 传入参数    : buf:     缓冲区
 *               buf_len: 需要接收的长度
 *               len:     实际接受的长度
 * 输出参数    : none
 * 返回值      : -1:超时  0:等待  1:接受完成  2:传输完成
 ************************************************************************************************************************/
static s8 c_frame_rec_cd(u8 *buf, u16 buf_len, u16 *len)
{
	/* Xmodem */
	vu16 rx_len_temp = 0;

	/* Baiku */
	s8 c_result = 0;

	switch (tUpdate.eProtoType)
	{
		case PT_XMODEM:
		{
			/* 非阻塞接收:单次仅读取当前缓冲区可用数据,未收满一帧返回0等待下次调用继续(*len跨调用保持接收进度) */
			if (lwrb_get_full(tUpdate.pRxBuff))
			{
				rx_len_temp = lwrb_get_full(tUpdate.pRxBuff);

				if ((rx_len_temp + (*len)) > buf_len)
					rx_len_temp = buf_len - (*len);

				lwrb_read(tUpdate.pRxBuff, &buf[(*len)], rx_len_temp);
				(*len) += rx_len_temp;
			}

			/* 若收到EOT则直接返回接收完成 */
			if ((XMODEM_FRM_FLAG_EOT == buf[0]) && (1 == (*len)))
			{
				if (uPrint.tFlag.bUpdate)
					sMyPrint("bUpdate:----接收到符号EOT,接受结束----\n\r");
				return 2;
			}

			if (tXmodem.eState != XMODEM_STATE_RECEIVING)
			{
				bool b_ret = false;
				if (tUpdate.eChType == CT_PRINT)
				{
					#if (boardPRINT_IFACE)
					if (cBaiku_UpdateCheck(tUpdate.tpProtoRx, buf, (*len)) > 0)
					{
						if (tUpdate.tpProtoRx->ucCmd == baikuCMD_SET_PROTO
							&& tUpdate.tpProtoRx->ucpValidData != NULL
							&& tUpdate.tpProtoRx->ucValidLen == 3)
						{
							if (tUpdate.tpProtoRx->ucpValidData[0] == PT_BAIKU)
							{
								memcpy((u16*)&tUpdate.usTotalFrmValue, &tUpdate.tpProtoRx->ucpValidData[1], 2);
								b_ret = true;
							}
						}
					}
					#else
					log_w("当前通道%d未开启,请重新选择通道", tUpdate.eChType);
					#endif  /* boardPRINT_IFACE */
				}
				else if (tUpdate.eChType == CT_CONSOLE)
				{
					#if (boardCONSOLE_EN)
					b_ret = bConsole_DataProc(buf, (*len));
					#else
					log_w("当前通道%d未开启,请重新选择通道", tUpdate.eChType);
					#endif  /* boardCONSOLE_EN */
				}
				else
				{
					if (uPrint.tFlag.bUpdate)
						log_w("bUpdate:通道%d未定义", tUpdate.eChType);
					return -1;
				}

				/* 切换协议 */
				if (b_ret == true)
				{
					u8 temp[4] = {0};
					temp[0] = PT_BAIKU;
					if (cUpdate_ProtoSelect((ProtoType_E)temp[0]) == true)
					{
						memcpy(&temp[1], (u8*)&tUpdate.usTotalFrmValue, 2);
						c_frame_trans_cd(baikuCMD_REPLY_SET_PROTO, temp, 3);
						tBaiKuProto.bStartSendFrm = true;
						return 99;
					}
				}
			}

			/* 帧数据接收完成 */
			if ((*len) >= buf_len)
				return 1;

			/* 等待超时 */
			if (tXmodem.usFrmOvertimeCnt == 0)
			{
				if (uPrint.tFlag.bUpdate)
					log_w("bUpdate:接受等待超时");
				return -1;
			}

			/* 未收满一帧,继续等待 */
			return 0;
		}
		break;

		case PT_BAIKU:
		{
			c_result = cBaiku_ProtoCheck(tUpdate.tpProtoRx);
			if (c_result > 0)
			{
				switch (tUpdate.tpProtoRx->ucCmd)
				{
					case baikuCMD_SET_PROTO:
					{
						memcpy((u16*)&tUpdate.usTotalFrmValue, &tUpdate.tpProtoRx->ucpValidData[1], 2);

						if (tUpdate.tpProtoRx->ucpValidData[0] == PT_BAIKU)
						{
							u8 temp[4] = {0};

							temp[0] = PT_BAIKU;
							memcpy(&temp[1], (u8*)&tUpdate.usTotalFrmValue, 2);
							c_frame_trans_cd(baikuCMD_REPLY_SET_PROTO, temp, 3);

							if (uPrint.tFlag.bUpdate)
								sMyPrint("bUpdate:设置升级协议为%d \n\r", PT_BAIKU);

							return 0;
						}
					}
					break;

					default:
					{
					}
					break;
				}
			}

			if (c_result < 0)
			{
				if (uPrint.tFlag.bUpdate)
					log_w("bUpdate:数据解析错误,代码%d", c_result);
			}

			return c_result;
		}

		default:
		{
		}
		break;
	}
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 处理校验完成的接收数据
 * 说明(备注)  : XMODEM数据回调函数, XMODEM调用此函数向上层通知接收到数据
 *               buf为空  len为0    -> 表示传输故障，所有传输数据无效
 *               buf不为空 len不为0 -> 表示当前数据有效
 *               buf不为空 len为0   -> 表示当前传输完成
 * 传入参数    : buf: 接收到的有效数据缓存BUFF
 *               len: 有效数据的长度
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_proc_check_ok_rec_data_cd(u8 *buf, u16 len)
{
	if (!buf)
		return;

	tUpdate.usRecFrameCnt++;

	/* 写入APP的Flash */
	bFlash_WriteDataToFlash(buf, len);

	if (uPrint.tFlag.bUpdate)
	{
		u16 pos = 0;
		while (pos < len)
		{
			for (u8 i = 0; i < 16; i++)
			{
				if (i == 15)
					sMyPrint("%x", buf[pos++]);
				else
					sMyPrint("%x,", buf[pos++]);
				if (pos >= len)
				{
					sMyPrint("\n\r");
					return;
				}
			}
			sMyPrint("\n\r");
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 接收开始回调
 * 说明(备注)  : 第一包有效数据到来时触发，初始化Flash写入
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_rec_start_cd(void)
{
	tUpdate.usRecFrameCnt = 0;
	vFlash_WriteDataToFlashInit();
	cBoot_CtrlUpdate(true, AS_ERASE);
}

/***********************************************************************************************************************
 * 函数功能    : 接收结束回调
 * 说明(备注)  : code: 0无错误传输完成  1升级错误  2还没开始等待升级超时
 * 传入参数    : code: 结束代码
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_rec_end_cd(u8 code)
{
	/* 升级错误，停止接收 */
	if (code == 1)
	{
		tUpdate.usRecFrameCnt = 0;
		cBoot_CtrlUpdate(true, AS_ERASE);

		if (uPrint.tFlag.bUpdate)
			log_e("bUpdate:因错误导致中断升级");
		return;
	}
	else if (code == 2)
	{
		uint32_t sp = *(__IO uint32_t*)flashAPP_START;
		uint32_t jump_addr = *(__IO uint32_t*)(flashAPP_START + 4);

		/* APP栈顶指针与入口地址均合法，且原本处于有效状态时才退出并返回APP */
		if (((sp & 0x2FFE0000) == 0x20000000) &&
			(jump_addr >= flashAPP_START && jump_addr <= flashAPP_END && (jump_addr & 0x01) == 1) &&
			tBootMemParam.tParam.eAppState == AS_OK)
		{
			cBoot_CtrlUpdate(false, AS_OK);
		}

		if (uPrint.tFlag.bUpdate)
			log_w("bUpdate:升级等待超时!");
		return;
	}
	else   /* 升级成功 */
	{
		uint32_t sp = *(__IO uint32_t*)flashAPP_START;
		uint32_t jump_addr = *(__IO uint32_t*)(flashAPP_START + 4);

		if (uPrint.tFlag.bUpdate)
		{
			sMyPrint("bUpdate:----升级完成,正在校验固件合法性----\n\r");
			sMyPrint("bUpdate:栈顶地址 = %x \r\n", sp);
			sMyPrint("bUpdate:入口地址 = %x \r\n", jump_addr);
			sMyPrint("bUpdate:接收帧数 = %d \r\n", tUpdate.usRecFrameCnt);
		}

		/* 三重固件合法性安全防御校验:
		 * 1. 栈顶指针落在有效SRAM范围 (0x20000000~0x2001FFFF)
		 * 2. 复位入口地址落在APP Flash代码区且Thumb模式位为1
		 * 3. 接收帧数达到最小有效阈值 (防止空包/截断包误判)
		 */
		if (((sp & 0x2FFE0000) == 0x20000000) &&
			(jump_addr >= flashAPP_START && jump_addr <= flashAPP_END && (jump_addr & 0x01) == 1) &&
			(tUpdate.usRecFrameCnt >= 10))
		{
			if (uPrint.tFlag.bUpdate)
				sMyPrint("bUpdate:固件安全校验通过,准备进入APP\r\n");
			cBoot_CtrlUpdate(false, AS_FINISH);
		}
		else
		{
			if (uPrint.tFlag.bUpdate)
				log_e("bUpdate:固件安全校验失败(栈顶=0x%08X, 入口=0x%08X, 帧数=%d),拒绝跳转并保持升级等待模式!", sp, jump_addr, tUpdate.usRecFrameCnt);
			cBoot_CtrlUpdate(true, AS_ERASE);
		}
		return;
	}
}

#endif  /* boardUPDATE */
