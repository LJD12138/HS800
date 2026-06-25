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

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_queue_task_update.h"
//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//
static void v_update_copy_reply_frame(lwrb_t* tp_reply_param, const u8* ucp_data, u16 us_char_len);





/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cUpdate_ProtoCheck(lwrb_t* proto_buff, lwrb_t* tp_reply_param)
{
    if(tp_reply_param->buff == NULL || proto_buff == NULL)
        return -1;

    //获取数据长度
	vu16 us_char_len = lwrb_get_full(proto_buff);

	if(us_char_len == 0)
		return 0;

    __ALIGNED(4) u8 uca_buff[256] = {0};

	if(us_char_len > sizeof(uca_buff))
		return -2;

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
		v_update_copy_reply_frame(tp_reply_param, uca_buff, us_char_len);
		return PT_XMODEM;
	}
    //BMS Baiku协议
	else if(tBms.eDevState == DS_UPDATE_MODE)
	{
		if(tpBmsProtoRx != NULL)
		{
			if(cBaiku_UpdateCheck(tpBmsProtoRx, uca_buff, us_char_len) > 0)
			{
				v_update_copy_reply_frame(tp_reply_param, uca_buff, us_char_len);
            	return PT_BAIKU;
			}
		}
		
	}
	//DCAC Megmeet协议
	else if(tDcac.eDevState == DS_UPDATE_MODE)
	{
		if(tpDcacMegmeetProtoRx != NULL)
		{
			if(us_char_len > tpDcacMegmeetProtoRx->usBuffSize)
				return PT_NULL;

			tpDcacMegmeetProtoRx->usFrameLen = us_char_len;
			memcpy(tpDcacMegmeetProtoRx->ucaFrameData, uca_buff, us_char_len);

			if(cMegmeet_FrameParse(&tpDcacMegmeetProtoRx->tFrame,
			                       tpDcacMegmeetProtoRx->ucaFrameData,
			                       tpDcacMegmeetProtoRx->usFrameLen) > 0)
				return PT_MEGMEET;
		}
	}

    return PT_NULL;
}

/***********************************************************************************************************************
-----函数功能    复制回复帧
-----输入参数    tp_reply_param
-----输入参数    ucp_data
-----输入参数    us_char_len
-----作者        LJD
-----日期        2026-05-07
************************************************************************************************************************/
static void v_update_copy_reply_frame(lwrb_t* tp_reply_param, const u8* ucp_data, u16 us_char_len)
{
	if(tp_reply_param == NULL || tp_reply_param->buff == NULL ||
	   ucp_data == NULL || us_char_len == 0)
		return;

	lwrb_reset(tp_reply_param);
	lwrb_write(tp_reply_param, ucp_data, us_char_len);
}

#endif  //boardUPDATE
