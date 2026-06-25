/***********************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\15-M50\1.software\M5004-3\APP\Application\Sys
 * File    : sys_queue_task_update.h
 * Date    : 2026-03-13 17:00:57
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
************************************************************************************************************************/
#ifndef SYS_QUEUE_TASK_UPDATE_H
#define SYS_QUEUE_TASK_UPDATE_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"
#include "queue_task.h"

#if(boardUPDATE)
/* ==========================================macros======================================*/
//DCAC升级阶段
#define DUS_IDLE                0
#define DUS_SLAVE_READY_OK	    3 	//DCAC前置数据准备完成,如确定协议/获取文件信息
#define DUS_HOST_REQ_DATA       4	//Print请求数据
#define DUS_WAIT_HOST_REPLY     5	//等待Print回复
#define DUS_SLAVE_SEND_DATA     6	//DCAC发送数据
#define DUS_WAIT_SLAVE_REPLY    7	//等待DCAC回复
#define DUS_GET_SLAVE_RESULT  	8	//获取DCAC结果
#define DUS_WAIT_SLAVE_RESULT_REPLY 9	//等待DCAC结果回复

#define UPDATE_QUEUE_STAGE_ERR      	 ((s8)0)
#define UPDATE_QUEUE_STAGE_RUNNING       ((s8)1)
#define UPDATE_QUEUE_STAGE_FINISH        ((s8)2)
#define UPDATE_QUEUE_STAGE_WAIT_RESTART  ((s8)3)

/* ==========================================types=======================================*/
//通道选择
typedef enum
{
	CT_NULL = 0,		//未选择
    CT_CONSOLE,			//Xmodem协议
    CT_PRINT,			//上位机
	CT_INVAILD,			//超范围
}ChannelType_E;

//协议类型
typedef enum
{
	PT_NULL = 0,		//未选择
    PT_XMODEM,			//Xmodem协议
    PT_BAIKU,			//百酷协议
	PT_MEGMEET,			//麦格米特协议
	PT_INVAILD,			//超范围
}ProtoType_E;

//升级对象
typedef enum
{
    UO_DEFAULT = 0,		//当前连接设备
    UO_CONSOLE,			//主控
	UO_BMS,				//电池
	UO_MPPT,			//光伏
	UO_DCAC,			//逆变
	UO_INVAILD,			//超范围
}UpdateObj_E;

typedef enum
{
	UTR_INVALID = 0,
	UTR_RUNNING,
	UTR_OK,
	UTR_LATEST,
	UTR_CANCEL,
	UTR_FAIL,
}UpdateTaskResult_E;

// DCAC升级失败错误码（每个调用点唯一，方便定位问题）
typedef enum
{
	UEF_NONE = 0,				// 无错误

	// md_dcac_rec_data_proc.c (0x01 ~ 0x0F)
	UEF_DR_BAUD_REPLY = 0x01,	// F3波特率回复处理失败
	UEF_DR_ERR_FRAME  = 0x02,	// 收到MEGMEET错误回复帧

	// md_dcac_queue_task_update.c (0x10 ~ 0x2F)
	UEF_D_RESEND_FAIL  = 0x10,	// 重发当前帧失败
	UEF_D_BUFF_NULL    = 0x12,	// 回复缓冲区为空
	UEF_D_SEND_F0_FAIL = 0x13,	// 发送F0失败
	UEF_D_F1_CHECK_FAIL= 0x14,	// F1校验出错
	UEF_D_SEND_F6_FAIL = 0x15,	// 发送F6失败
	UEF_D_SEND_F0A_FAIL= 0x16,	// 再次发送F0失败
	UEF_D_SEND_F2_FAIL = 0x17,	// 发送F2失败
	UEF_D_F3_CHECK_FAIL= 0x18,	// F3校验出错
	UEF_D_F3_BAUD_REPLY = 0x19,	// F3波特率回复处理失败
	UEF_D_F3_SET_BAUD  = 0x1A,	// F3设置波特率失败
	UEF_D_HEAD_LEN_ERR = 0x1B,	// 文件头长度错误
	UEF_D_SEND_A1_FAIL = 0x1C,	// 发送A1(文件头)失败
	UEF_D_A2_CHECK_FAIL= 0x11,	// A2文件头校验失败
	UEF_D_A2_REPLY_ERR = 0x26,	// A2文件头回复错误
	UEF_D_DATA_LEN_ERR = 0x1D,	// 数据包长度错误
	UEF_D_SEND_A3_FAIL = 0x1E,	// 发送A3(固件数据)失败
	UEF_D_SEND_A5_FAIL = 0x1F,	// 发送A5(查询)失败
	UEF_D_A4_CHECK_FAIL= 0x20,	// A4固件数据校验失败
	UEF_D_A4_REPLY_ERR = 0x27,	// A4固件数据回复错误
	UEF_D_A6_REPLY_ERR = 0x21,	// A6查询结果回复错误
	UEF_D_INVALID_OBJ  = 0x22,	// 升级对象无效
	UEF_D_SET_INVALID_BAUD = 0x23,	// 设置的波特率无效
	UEF_D_A4_SEQ_MISMATCH = 0x24,	// A4包序号不匹配
	UEF_D_A6_CHECK_FAIL  = 0x25,	// A6查询结果校验失败
	UEF_D_F7_CHECK_FAIL= 0x14,	// F7校验出错

	// print_queue_task_update.c (0x30 ~ 0x3F)
	UEF_P_TASK_NULL    = 0x30,	// 任务指针为空
	UEF_P_PENDING_FAIL = 0x31,	// pending result为FAIL
	UEF_P_CANCEL_FAIL  = 0x32,	// 取消时处于FAIL状态(兼容保留)
	UEF_P_INVALID_OBJ  = 0x33,	// 升级对象无效
	UEF_P_CANCEL_REQ   = 0x34,	// 上位机主动取消升级
	UEF_P_BUFF_NULL    = 0x35,	// 缓冲区为空
	UEF_P_C2_DATA_ERR  = 0x36,	// C2数据长度或内容错误
	UEF_P_C3_REPLY_FAIL= 0x37,	// C3回复设置协议失败
	UEF_P_C5_DATA_ERR  = 0x38,	// C5数据长度或内容错误
	UEF_P_HEAD_PARSE_FAIL = 0x39,	// DCAC文件头解析失败
	UEF_P_FWD_DCAC_FAIL= 0x3A,	// 转发到DCAC任务失败
	UEF_P_C4_TIMEOUT   = 0x3B,	// C4/C2/C5握手超时
	UEF_P_SLAVE_RESULT_ERR = 0x3C,	// DCAC从机结果状态异常

	//v_sys_queue_task_update.c (0x40 ~ 0x4F)
	UEF_S_TASK_OVER_TIME = 0x40,	// 升级任务等待超时
	UEF_S_LOST_OVERTIME = 0x41,	// 升级任务丢失超时

}UpdateErrCode_E;

/* ==========================================globals=====================================*/
typedef struct
{
	vu16				usRecFrameCnt;		//记录当前接收的帧数
	vu16 				usTotalFrmValue; 	//总帧数
	vu16            	usRecOverTimeCnt;	//接收超时计数
	vu16            	usLostOverTimeCnt;	//丢失超时计数
	u32					ulBaud;				//波特率
	u32					ulFwSize;			//固件总大小
	u32					ulRxSize;			//已接收固件大小
	u32					ulFwCrc32;			//固件头中的CRC32
	u32					ulFwCalcCrc32;		//固件数据滚动CRC32状态
	u32					ulFwPendCrc32;		// pending packet crc state
	u16					usPendPacketLen;	// pending packet len
	u8					ucStage;			//升级阶段
	UpdateTaskResult_E  eHostResult;		//主机升级结果
	UpdateTaskResult_E  eSlaveResult;		//从机升级结果
	UpdateObj_E			eObj;				//升级对象
	ChannelType_E		eChType;			//通道类型
	ProtoType_E 		eProtoType;			//协议类型
	UpdateErrCode_E		eErrCode;			//升级失败错误码
}Update_T;
extern Update_T  tUpdate;

/* ==========================================extern======================================*/
bool bUpdate_Init(void);
void vUpdate_ResetTimeout(void);
bool bUpdate_SetErrCode(UpdateErrCode_E code);
void vUpdate_SetStage(Task_T *tp_task, u8 stage);
s8 cUpdate_ChSelect(UpdateObj_E e_obj, ChannelType_E ch_type);
s8 cUpdate_ProtoSelect(UpdateObj_E e_obj, ProtoType_E proto_type);
void vUpdate_TickTimer(void);
#endif  //boardUPDATE

#ifdef __cplusplus
}
#endif

#endif  //SYS_QUEUE_TASK_UPDATE_H
