/*******************************************************************************************************************************
 * Module  : APP/MW-Protocol
 * File    : megmeet_proto.h
 * Date    : 2026-04-29
 * Desc    : 在线升级协议状态机接口（主机端，用于APP升级DCAC等从机模块）
 *           协议帧格式：HEADER(1)+SlaveAddr(1)+ICType(1)+RESERVE1(2)+LEN(2)+RESERVE2(1)+CMD(1)+RESERVE3(1)+PAYLOAD(N)+CRC16(2)
 ******************************************************************************************************************************/
#ifndef MEGMEET_PROTO_H__
#define MEGMEET_PROTO_H__

#include "board_config.h"

#if(boardUPDATE)

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/*======================================== 帧格式常量 ========================================*/
#define MEGMEET_FRAME_HEAD_CODE             0xAAU
#define MEGMEET_FRAME_RESERVE_VALUE         0x00U
#define MEGMEET_FRAME_LEN_OVERHEAD          5U
#define MEGMEET_FRAME_FIXED_HEAD_LEN        10U
#define MEGMEET_FRAME_CRC_LEN               2U
#define MEGMEET_FRAME_PAYLOAD_OFFSET        10U
#define MEGMEET_FRAME_MIN_FRAME_LEN         (MEGMEET_FRAME_FIXED_HEAD_LEN + MEGMEET_FRAME_CRC_LEN)

/*======================================== 命令码 ========================================*/
#define MEGMEET_CMD_ERR_REPLY           0xFF
#define MEGMEET_CMD_REQ_UPDATE          0xF0
#define MEGMEET_CMD_REQ_UPDATE_REPLY    0xF1
#define MEGMEET_CMD_SET_BAUD            0xF2
#define MEGMEET_CMD_SET_BAUD_REPLY      0xF3
#define MEGMEET_CMD_JUMP_BOOT           0xF6
#define MEGMEET_CMD_JUMP_BOOT_REPLY     0xF7
#define MEGMEET_CMD_QUERY_INFO          0xF8
#define MEGMEET_CMD_QUERY_INFO_REPLY    0xF9
#define MEGMEET_CMD_FILE_HEAD           0xA1
#define MEGMEET_CMD_FILE_HEAD_REPLY     0xA2
#define MEGMEET_CMD_FIRMWARE_DATA       0xA3
#define MEGMEET_CMD_FIRMWARE_DATA_REPLY 0xA4
#define MEGMEET_CMD_QUERY_RESULT        0xA5
#define MEGMEET_CMD_QUERY_RESULT_REPLY  0xA6

/*======================================== 芯片ID ========================================*/
#define MEGMEET_IC_TYPE_ARM             0x10
#define MEGMEET_IC_TYPE_DC              0x20
#define MEGMEET_IC_TYPE_AC              0x30
#define MEGMEET_IC_TYPE_BMS             0x40
#define MEGMEET_IC_TYPE_MPPT            0x50

/*======================================== 错误码(0xFF回复) ========================================*/
#define MEGMEET_ERR_NO_REQ              0x01
#define MEGMEET_ERR_CRC16_FAIL          0x02
#define MEGMEET_ERR_CMD_INVALID         0x03
#define MEGMEET_ERR_FRAME_LEN           0x04

/*======================================== 波特率回复(0xF3) ========================================*/
#define MEGMEET_BAUD_OK                 0x00
#define MEGMEET_BAUD_INVALID            0xFF

/*======================================== 文件头回复(0xA2) ========================================*/
#define MEGMEET_A2_OK                   0x00
#define MEGMEET_A2_VER_LATEST           0xF7
#define MEGMEET_A2_FLASH_ERR            0xF8
#define MEGMEET_A2_SIZE_ERR             0xF9
#define MEGMEET_A2_SW_VER_ERR           0xFA
#define MEGMEET_A2_HW_VER3_ERR          0xFB
#define MEGMEET_A2_HW_VER2_ERR          0xFC
#define MEGMEET_A2_HW_VER1_ERR          0xFD
#define MEGMEET_A2_REGION_ERR           0xFE
#define MEGMEET_A2_PROJECT_ERR          0xFF

/*======================================== 固件包回复(0xA4) ========================================*/
#define MEGMEET_A4_OK                   0x00
#define MEGMEET_A4_ALL_OK               0x01
#define MEGMEET_A4_FLASH_ERR            0xFD
#define MEGMEET_A4_NO_A1                0xFE
#define MEGMEET_A4_CRC_FAIL             0xFF

/*======================================== 查询结果回复(0xA6) ========================================*/
#define MEGMEET_A6_VER_LATEST           0xF7
#define MEGMEET_A6_REGION_ERR           0xF8
#define MEGMEET_A6_PROJECT_ERR          0xF9
#define MEGMEET_A6_CRC32_ERR            0xFA
#define MEGMEET_A6_FLASH_ERR            0xFB
#define MEGMEET_A6_SIZE_ERR             0xFC
#define MEGMEET_A6_SW_VER_ERR           0xFD
#define MEGMEET_A6_HW_VER_ERR           0xFE
#define MEGMEET_A6_TIMEOUT              0xFF

/*======================================== 常量 ========================================*/
#define MEGMEET_FILE_HEAD_SIZE          56
#define MEGMEET_FILE_MAGIC              0x5053474D  /* 文档定义文件标识（小端：'P''S''G''M'） */
#define MEGMEET_SN_TOTAL_LEN            128

#define MEGMEET_TIMEOUT_5S_MS           5000
#define MEGMEET_TIMEOUT_1S_MS           1000
#define MEGMEET_TIMEOUT_500MS_MS        500
#define MEGMEET_TIMEOUT_100MS_MS        100

#define MEGMEET_MAX_RETRY               3
#define MEGMEET_FRM_PKG_SIZE            224         /* 每包固件数据长度（32的倍数：192或224） */
#define MEGMEET_HOST_TX_BUF_SIZE        512
#define MEGMEET_HOST_RX_BUF_SIZE        256

/*======================================== 帧结构体 ========================================*/
#pragma pack(1)
typedef struct
{
    const u8*          ucpFrame;          /* 指向原始完整帧首地址 */
    u16                usFrameLen;        /* 当前完整帧总长度 */
    u8                 ucHead;            /* 协议头 */
    u8                 ucSlaveAddr;       /* 从机地址 */
    u8                 ucIcType;          /* 芯片ID */
    u16                usReserve1;        /* 保留字段1 */
    u16                usProtoLen;        /* LEN字段原始值 */
    u8                 ucReserve2;        /* 保留字段2 */
    u8                 ucCmd;             /* 指令码 */
    u8                 ucReserve3;        /* 保留字段3 */
    u16                usPayloadLen;      /* 有效载荷长度 */
    const u8*          ucpPayload;        /* 指向有效载荷首地址 */
    u16                usCrc16;           /* 原始帧中的CRC16值 */
} UpdateFrame_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    u16                usBuffSize;        /* 发送缓存总长度 */
    u16                usFrameLen;        /* 当前待发送帧长度 */
    u8                 ucaFrameData[];    /* 发送帧缓存 */
} MegmeetProtoTx_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
    u16                usBuffSize;        /* 接收缓存总长度 */
    u16                usFrameLen;        /* 当前接收帧长度 */
    UpdateFrame_t      tFrame;            /* 最近一次解析成功的帧 */
    u8                 ucaFrameData[];    /* 接收帧缓存 */
} MegmeetProtoRx_t;
#pragma pack()

/*======================================== 帧构造/解析接口 ========================================*/
s8 cMegmeet_ProtoRecInit(MegmeetProtoRx_t** proto, u16 buff_len);
s8 cMegmeet_ProtoSendInit(MegmeetProtoTx_t** proto, u16 buff_len);
s8 cMegmeet_FrameCreate(u8 slave_addr, u8 ic_type, u8 cmd, const u8* payload, u16 payload_len,
                        u8* out_frame, u16 frame_buff_len, u16* out_frame_len);
s8 cMegmeet_FrameParse(UpdateFrame_t* tp_frame, const u8* ucp_data, u16 len);

#ifdef __cplusplus
}
#endif

#endif /* boardUPDATE */
#endif /* MEGMEET_PROTO_H__ */
