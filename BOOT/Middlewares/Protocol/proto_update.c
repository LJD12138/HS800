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

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//





/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cUpdate_ProtoCheck(BaikuProtoRx_t* proto, lwrb_t* tp_reply_param)
{
    if(tp_reply_param->buff == NULL || proto == NULL)
        return -1;

    //获取数据长度
	vu16 us_char_len = lwrb_get_full(&proto->tRxBuff);

	if(us_char_len == 0)
		return 0;

    __ALIGNED(4) u8 uca_buff[256] = {0};

	if(us_char_len > sizeof(uca_buff))
		return -2;

	lwrb_read(&proto->tRxBuff, uca_buff, us_char_len);
    lwrb_reset(tp_reply_param);
    lwrb_write(tp_reply_param, uca_buff, us_char_len);

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
		return PT_XMODEM;
	}
    //其他协议
	else
	{
		if(cBaiku_UpdateCheck(proto, uca_buff, us_char_len) > 0)
            return PT_BAIKU;
	}

    return PT_NULL;
}

#endif  //boardUPDATE
