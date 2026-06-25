/*******************************************************************************************************************************
 * Module  : APP/MW-Protocol
 * File    : megmeet_proto.c
 * Date    : 2026-04-29
 * Desc    : Megmeet 协议帧工具实现（当前产品路径仅保留帧构造/解析）
 ******************************************************************************************************************************/
#include "Megmeet/megmeet_proto.h"

#if(boardUPDATE)

#include <string.h>
#include <stdlib.h>
#include "check.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

/*======================================== 局部宏 ========================================*/
#define FRAME_LEN_H_OFFSET              5U
#define FRAME_DATA_OFFSET               7U
#define FRAME_RESERVE1_OFFSET           3U
#define FRAME_RESERVE2_OFFSET           7U
#define FRAME_CMD_OFFSET                8U
#define FRAME_RESERVE3_OFFSET           9U
#define FRAME_MAX_FRAME_LEN             0xFFFFU
#define FRAME_MAX_PAYLOAD_LEN           (FRAME_MAX_FRAME_LEN - MEGMEET_FRAME_MIN_FRAME_LEN)

/*======================================== 帧构造/解析实现（与从机端通用）=========================*/
/*****************************************************************************************************************
 -----函数功能    接收协议初始化
 -----说明(备注)  none
 -----传入参数    proto:接收协议结构体
                 buff_len:协议缓存器大小
 -----输出参数    none
 -----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
 ******************************************************************************************************************/
s8 cMegmeet_ProtoRecInit(MegmeetProtoRx_t** proto, u16 buff_len)
{
    s8 result = 1;

    if(buff_len < MEGMEET_FRAME_MIN_FRAME_LEN)
        return -1;

    #if(boardUSE_OS)
    taskENTER_CRITICAL();
    #endif
    // 动态分配内存
    size_t total_size = sizeof(MegmeetProtoRx_t) + buff_len;
    #if(boardUSE_OS)
    *proto = (MegmeetProtoRx_t*)pvPortMalloc(total_size);
    #else
    *proto = (MegmeetProtoRx_t*)malloc(total_size);
    #endif

    if(*proto != NULL)
    {
        memset(*proto, 0, total_size);
        (*proto)->usBuffSize = buff_len;
        (*proto)->usFrameLen = 0;
    }
    else
        result = -2;

    #if(boardUSE_OS)
    taskEXIT_CRITICAL();
    #endif

    return result;
}

/*****************************************************************************************************************
 -----函数功能    发送协议初始化
 -----说明(备注)  none
 -----传入参数    proto:发送协议结构体
                 buff_len:协议缓存器大小
 -----输出参数    none
 -----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
 ******************************************************************************************************************/
s8 cMegmeet_ProtoSendInit(MegmeetProtoTx_t** proto, u16 buff_len)
{
    s8 result = 1;

    if(buff_len < MEGMEET_FRAME_MIN_FRAME_LEN)
        return -1;

    #if(boardUSE_OS)
    taskENTER_CRITICAL();
    #endif
    // 动态分配内存
    size_t total_size = sizeof(MegmeetProtoTx_t) + buff_len;
    #if(boardUSE_OS)
    *proto = (MegmeetProtoTx_t*)pvPortMalloc(total_size);
    #else
    *proto = (MegmeetProtoTx_t*)malloc(total_size);
    #endif

    if(*proto != NULL)
    {
        memset(*proto, 0, total_size);
        (*proto)->usBuffSize = buff_len;
        (*proto)->usFrameLen = 0;
    }
    else
        result = -2;

    #if(boardUSE_OS)
    taskEXIT_CRITICAL();
    #endif

    return result;
}

/*****************************************************************************************************************
 -----函数功能    根据升级协议文档中的指令格式构造数据帧
 -----说明(备注)  LEN字段表示RESERVE2到CRC16的总字节数，CRC16使用Modbus算法，低字节在前
 ******************************************************************************************************************/
s8 cMegmeet_FrameCreate(u8 slave_addr, u8 ic_type, u8 cmd, const u8* payload, u16 payload_len,
                        u8* out_frame, u16 frame_buff_len, u16* out_frame_len)
{
    u16 us_proto_len = 0;
    u16 us_frame_len = 0;
    u16 us_crc16 = 0;

    if(out_frame == NULL || out_frame_len == NULL)
        return -1;

    *out_frame_len = 0;

    if(payload == NULL && payload_len != 0)
        return -2;

    if(payload_len > FRAME_MAX_PAYLOAD_LEN)
        return -3;

    us_proto_len = payload_len + MEGMEET_FRAME_LEN_OVERHEAD;
    us_frame_len = payload_len + MEGMEET_FRAME_MIN_FRAME_LEN;

    if(frame_buff_len < us_frame_len)
        return -4;

    out_frame[0] = MEGMEET_FRAME_HEAD_CODE;
    out_frame[1] = slave_addr;
    out_frame[2] = ic_type;
    out_frame[3] = MEGMEET_FRAME_RESERVE_VALUE;
    out_frame[4] = MEGMEET_FRAME_RESERVE_VALUE;
    out_frame[5] = (u8)(us_proto_len >> 8);
    out_frame[6] = (u8)(us_proto_len & 0x00FF);
    out_frame[7] = MEGMEET_FRAME_RESERVE_VALUE;
    out_frame[8] = cmd;
    out_frame[9] = MEGMEET_FRAME_RESERVE_VALUE;

    if(payload_len != 0)
        memcpy(&out_frame[MEGMEET_FRAME_PAYLOAD_OFFSET], payload, payload_len);

    us_crc16 = usCheck_GetModbusCrc16(out_frame, (u32)(us_frame_len - MEGMEET_FRAME_CRC_LEN));
    out_frame[us_frame_len - 2] = (u8)(us_crc16 & 0x00FF);
    out_frame[us_frame_len - 1] = (u8)(us_crc16 >> 8);

    *out_frame_len = us_frame_len;

    return 1;
}

/*****************************************************************************************************************
-----函数功能    根据升级协议文档格式解析数据帧
-----说明(备注)  仅解析一帧，成功后tp_frame会指向当前帧中的各个字段
                支持跳过帧头前面的干扰字符
-----传入参数    tp_frame:协议的结构体
                ucp_data:指向数据指针
                len:数据的长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
 ******************************************************************************************************************/
s8 cMegmeet_FrameParse(UpdateFrame_t* tp_frame, const u8* ucp_data, u16 len)
{
    u16 us_proto_len = 0;
    u16 us_frame_len = 0;
    u16 us_crc16 = 0;
    u16 us_calc_crc16 = 0;
    u16 offset = 0;

    if(tp_frame == NULL || ucp_data == NULL)
        return -1;

    memset(tp_frame, 0, sizeof(UpdateFrame_t));

    if(len < MEGMEET_FRAME_MIN_FRAME_LEN)
        return -2;

    /* 跳过帧头前面的干扰字符 */
    while(offset < len)
    {
        if(ucp_data[offset] == MEGMEET_FRAME_HEAD_CODE)
            break;
        offset++;
    }

    /* 检查是否还有足够的数据构成完整帧 */
    if((len - offset) < MEGMEET_FRAME_MIN_FRAME_LEN)
        return -2;

    /* 帧头后面的数据结构检查 */
    if((ucp_data[offset + FRAME_RESERVE1_OFFSET] != MEGMEET_FRAME_RESERVE_VALUE) ||
       (ucp_data[offset + FRAME_RESERVE1_OFFSET + 1] != MEGMEET_FRAME_RESERVE_VALUE) ||
       (ucp_data[offset + FRAME_RESERVE2_OFFSET] != MEGMEET_FRAME_RESERVE_VALUE) ||
       (ucp_data[offset + FRAME_RESERVE3_OFFSET] != MEGMEET_FRAME_RESERVE_VALUE))
        return -3;

    us_proto_len = ((u16)ucp_data[offset + FRAME_LEN_H_OFFSET] << 8) |
                   (u16)ucp_data[offset + FRAME_LEN_H_OFFSET + 1];
    if(us_proto_len < MEGMEET_FRAME_LEN_OVERHEAD)
        return -4;

    us_frame_len = FRAME_DATA_OFFSET + us_proto_len;
    if((len - offset) < us_frame_len)
        return -4;

    us_crc16 = ((u16)ucp_data[offset + us_frame_len - 1] << 8) | 
               (u16)ucp_data[offset + us_frame_len - 2];
    us_calc_crc16 = usCheck_GetModbusCrc16((u8*)&ucp_data[offset], (u32)(us_frame_len - MEGMEET_FRAME_CRC_LEN));
    if(us_crc16 != us_calc_crc16)
        return -5;

    /* 帧解析成功 */
    tp_frame->ucpFrame      = &ucp_data[offset];
    tp_frame->usFrameLen    = us_frame_len;
    tp_frame->ucHead        = ucp_data[offset];
    tp_frame->ucSlaveAddr   = ucp_data[offset + 1];
    tp_frame->ucIcType      = ucp_data[offset + 2];
    tp_frame->usReserve1    = ((u16)ucp_data[offset + 3] << 8) | (u16)ucp_data[offset + 4];
    tp_frame->usProtoLen    = us_proto_len;
    tp_frame->ucReserve2    = ucp_data[offset + FRAME_RESERVE2_OFFSET];
    tp_frame->ucCmd         = ucp_data[offset + FRAME_CMD_OFFSET];
    tp_frame->ucReserve3    = ucp_data[offset + FRAME_RESERVE3_OFFSET];
    tp_frame->usPayloadLen  = us_proto_len - MEGMEET_FRAME_LEN_OVERHEAD;
    if(tp_frame->usPayloadLen != 0)
        tp_frame->ucpPayload = &ucp_data[offset + MEGMEET_FRAME_PAYLOAD_OFFSET];
    tp_frame->usCrc16       = us_crc16;

    return 1;
}

#endif /* boardUPDATE */
