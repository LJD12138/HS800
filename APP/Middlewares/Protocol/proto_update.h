/***********************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\15-M50\1.software\M5004-3\APP\Middlewares\Protocol
 * File    : proto_update.h
 * Date    : 2026-03-13 15:24:41
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
************************************************************************************************************************/
#ifndef PROTO_UPDATE_H
#define PROTO_UPDATE_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(boardUPDATE)
/* ==========================================macros======================================*/
#include "Baiku/baiku_proto.h"


/* ==========================================types=======================================*/
#define XMODEM_FRM_FLAG_ECHO        	 	0xFE
//XMODEM协议校验和回显标志
#define XMODEM_FRM_FLAG_ADD_ECHO        	0x15
//XMODEM协议CRC16回显标志
#define XMODEM_FRM_FLAG_CRC_ECHO        	'c'
//XMODEM协议128字节头标志
#define XMODEM_FRM_FLAG_SOH             	0x01
//XMODEM协议1K字节头标志
#define XMODEM_FRM_FLAG_STX             	0x02
//XMODEM协议发送结束标志
#define XMODEM_FRM_FLAG_EOT             	0x04
//XMODEM应答标志
#define XMODEM_FRM_FLAG_ACK             	0x06
//XMODEM非应答标志
#define XMODEM_FRM_FLAG_NAK             	0x15   //错误,请求重新发送
//XMODEM取消发送标志
#define XMODEM_FRM_FLAG_CAN             	0x18
//XMODEM使用CRC16校验标志
#define XMODEM_FRM_FLAG_CRC16           	0x43
//XMODEM填充数据包标志
#define XMODEM_FRM_FLAG_CTRLZ           	0x1A


/* ==========================================globals=====================================*/


/* ==========================================extern======================================*/
s8 cUpdate_ProtoCheck(BaikuProtoRx_t* proto, lwrb_t* tp_reply_param);

#endif  //boardUPDATE

#ifdef __cplusplus
}
#endif

#endif  //PROTO_UPDATE_H
