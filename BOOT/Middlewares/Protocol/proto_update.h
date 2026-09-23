/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Middlewares\Protocol
 * File    : proto_update.h
 * Date    : 2026-03-13
 * Author  : LJD(291483914@qq.com)
 * Desc    : 升级协议帧解析接口
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef PROTO_UPDATE_H
#define PROTO_UPDATE_H


#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if(boardUPDATE)
//****************************************************Macros********************************************************************//
#include "Baiku/baiku_proto.h"


//****************************************************Types*********************************************************************//
#define			XMODEM_FRM_FLAG_ECHO					0xFE
//XMODEM协议校验和回显标志
#define			XMODEM_FRM_FLAG_ADD_ECHO				0x15
//XMODEM协议CRC16回显标志
#define			XMODEM_FRM_FLAG_CRC_ECHO				'c'
//XMODEM协议128字节头标志
#define			XMODEM_FRM_FLAG_SOH						0x01
//XMODEM协议1K字节头标志
#define			XMODEM_FRM_FLAG_STX						0x02
//XMODEM协议发送结束标志
#define			XMODEM_FRM_FLAG_EOT						0x04
//XMODEM应答标志
#define			XMODEM_FRM_FLAG_ACK						0x06
//XMODEM非应答标志
#define			XMODEM_FRM_FLAG_NAK						0x15	//错误,请求重新发送
//XMODEM取消发送标志
#define			XMODEM_FRM_FLAG_CAN						0x18
//XMODEM使用CRC16校验标志
#define			XMODEM_FRM_FLAG_CRC16					0x43
//XMODEM填充数据包标志
#define			XMODEM_FRM_FLAG_CTRLZ					0x1A


//****************************************************Globals*******************************************************************//


//****************************************************Extern********************************************************************//
s8 cUpdate_ProtoCheck(lwrb_t* proto_buff);

#endif  /* boardUPDATE */

#ifdef __cplusplus
}
#endif

#endif  /* PROTO_UPDATE_H */
