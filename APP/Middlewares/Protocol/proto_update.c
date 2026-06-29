/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\15-M50\1.software\M5004-3\APP\Middlewares\Protocol
 * File    : proto_update.c
 * Date    : 2026-03-13 15:24:10
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "proto_update.h"

#include <string.h>

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
#include "Print/print_prot_frame.h"

//****************************************************Macros*******************************************************************//
#define UPDATE_BUFF_SIZE    256     /* 需兼容Megmeet升级帧(文件头56B/数据包236B)及Baiku大数据帧 */


//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//






/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cUpdate_ProtoCheck(lwrb_t* proto_buff)
{
    if(proto_buff == NULL)
        return -1;

    //获取数据长度
	vu16 us_char_len = lwrb_get_full(proto_buff);

	if(us_char_len == 0)
		return 0;

	if(us_char_len > UPDATE_BUFF_SIZE)
		return -2;

    u8 uca_buff[UPDATE_BUFF_SIZE];
	lwrb_read(proto_buff, uca_buff, us_char_len);

    //Xmodem
	if(us_char_len == 1)
	{
		u8 index = uca_buff[0];
		switch(index)
		{
			case XMODEM_FRM_FLAG_ACK:
			{
				tUpdate.usRecFrameCnt++;
			}
			break;
			
			case XMODEM_FRM_FLAG_NAK:
			{
				tUpdate.usRecFrameCnt = 0;
			}
			break;

			//取消
			case XMODEM_FRM_FLAG_CAN:
			{
				tUpdate.usRecFrameCnt = 0;
			}
			break;
			
			default:
				break;
		}
		c_print_info_trans(uca_buff, us_char_len);
		return PT_XMODEM;
	}
    //BMS Baiku协议
	else if(tBms.eDevState == DS_UPDATE_MODE)
	{
		if(tpBmsProtoRx == NULL || (us_char_len > tpBmsProtoRx->tRxBuff.size))
			return -3;

		if(cBaiku_UpdateCheck(tpBmsProtoRx, uca_buff, us_char_len) > 0)
			return PT_BAIKU;
	}
	//DCAC Megmeet协议
	else if(tDcac.eDevState == DS_UPDATE_MODE)
	{
		if(tpDcacMegmeetProtoRx == NULL || (us_char_len > tpDcacMegmeetProtoRx->usBuffSize))
			return -4;

		tpDcacMegmeetProtoRx->usFrameLen = us_char_len;
		memcpy(tpDcacMegmeetProtoRx->ucaFrameData, uca_buff, us_char_len);

		if(cMegmeet_FrameParse(&tpDcacMegmeetProtoRx->tFrame,
								tpDcacMegmeetProtoRx->ucaFrameData,
								tpDcacMegmeetProtoRx->usFrameLen) > 0)
			return PT_MEGMEET;
	}

    return PT_NULL;
}
#endif  //boardUPDATE
