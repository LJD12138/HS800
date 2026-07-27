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
#define UPDATE_QUEUE_STAGE_ERR      	 ((s8)0)
#define UPDATE_QUEUE_STAGE_RUNNING       ((s8)1)
#define UPDATE_QUEUE_STAGE_FINISH        ((s8)2)
#define UPDATE_QUEUE_STAGE_WAIT_RESTART  ((s8)3)

/*!< 判断升级对象是否为DCAC系列（逆变器/AC管理/DC管理） */
#define IS_DCAC_UPDATE_OBJ(obj)  ((obj) == MO_DCAC || (obj) == MO_MGMT_AC || (obj) == MO_MGMT_DC)

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

typedef enum
{
	UTR_INVALID = 0,
	UTR_RUNNING,
	UTR_OK,
	UTR_LATEST,
	UTR_CANCEL,
	UTR_FAIL,
	UTR_MAX,			//超范围(与UTR_INVALID区分:INVALID=初始值,MAX=上限标记)
}UpdateTaskResult_E;

// 升级结果设置目标
typedef enum
{
	URT_HOST = 0,	//主机
	URT_SLAVE,		//从机
	URT_HOST_SLAVE = 2,	//主从机
	URT_INVAILD,	//超范围
}UpdateResultTarget_E;

// 升级失败错误码（每个调用点唯一，方便精准定位故障点）
// 编码规则：使用十进制数值，确保数码管显示值与源码值一一对应（Display_ShowErrCode 仅显示2位十进制）。
//           按模块分组：01~12 DCAC接收处理, 13 DCAC协议帧, 14~30 DCAC升级队列,
//                       31~34 BMS升级队列, 35~36 Print升级队列, 37~50 Print-DCAC转发,
//                       51~61 Print-BMS转发, 62 系统升级。
// 命名前缀：DR=dcac_rec  DP=dcac_prot  DQ=dcac_queue  BQ=bms_queue
//           PQ=print_queue  PD=print_dcac  PB=print_bms  S=sys
typedef enum
{
	UEF_NONE = 0,               // 无错误

	// ==================== md_dcac_rec_data_proc.c (01~12) DCAC接收数据处理 ====================
	UEF_DR_F1_CHECK_FAIL     = 1,   // F1回复参数非0x01
	UEF_DR_F3_BAUD_INVALID   = 2,   // F3回复波特率无效(0xFF)
	UEF_DR_F3_CHECK_FAIL     = 3,   // F3回复切换失败(非0x00)
	UEF_DR_F3_SET_BAUD_FAIL  = 4,   // F3本地设置波特率失败
	UEF_DR_F7_CHECK_FAIL     = 5,   // F7回复payload长度非0
	UEF_DR_A2_REPLY_ERR      = 6,   // A2回复状态非OK/LATEST
	UEF_DR_A4_SEQ_MISMATCH   = 7,   // A4包序号不匹配
	UEF_DR_A4_REPLY_ERR      = 8,   // A4回复状态非OK/ALL_OK
	UEF_DR_A6_CHECK_FAIL     = 9,   // A6从机地址/芯片ID不匹配
	UEF_DR_A6_REPLY_ERR      = 10,  // A6回复状态>100且非LATEST
	UEF_DR_A6_NOT_COMPLETE   = 11,  // A6回复状态<100未完成
	UEF_DR_ERR_FRAME         = 12,  // 收到0xFF错误回复帧

	// ==================== md_dcac_prot_frame.c (13) DCAC协议帧 ====================
	UEF_DP_F2_INVALID_BAUD   = 13,  // F2波特率非9600/115200

	// ==================== md_dcac_queue_task_update.c (14~30) DCAC升级队列 ====================
	UEF_DQ_PROTO_INIT_FAIL   = 14,  // Megmeet协议初始化失败
	UEF_DQ_INVALID_OBJ       = 15,  // 升级对象非DCAC
	UEF_DQ_BUFF_NULL         = 16,  // 回复缓冲区为空
	UEF_DQ_CANCEL_REQ        = 17,  // 上位机取消DCAC升级
	UEF_DQ_F0_RETRY_OVER     = 18,  // F0发送重试超限
	UEF_DQ_F6_RETRY_OVER     = 19,  // F6发送重试超限
	UEF_DQ_F2_RETRY_OVER     = 20,  // F2发送重试超限
	UEF_DQ_C4_RETRY_OVER     = 21,  // C4/C5握手重试超限
	UEF_DQ_A2_RETRY_OVER     = 22,  // A2回复重试超限
	UEF_DQ_A2_RESEND_LEN_ERR = 23,  // A2重发时回复长度不符
	UEF_DQ_A2_RESEND_PEEK_FAIL = 24,// A2重发时peek缓存失败
	UEF_DQ_C5_RETRY_OVER     = 25,  // C5主机数据等待重试超限
	UEF_DQ_A3_LEN_RETRY_OVER = 26,  // A3数据长度异常重试超限
	UEF_DQ_A3_RETRY_OVER     = 27,  // A3发送重试超限
	UEF_DQ_A4_RESEND_OVER    = 28,  // A4回复等待重发重试超限
	UEF_DQ_A5_RETRY_OVER     = 29,  // A5查询发送重试超限
	UEF_DQ_A6_WAIT_RETRY_OVER= 30,  // A6回复等待重试超限

	// ==================== md_bms_queue_task_update.c (31~34) BMS升级队列 ====================
	UEF_BQ_INIT_BUFF_NULL    = 31,  // BMS初始化回复缓存为空
	UEF_BQ_PENDING_FAIL      = 32,  // BMS错误收尾时无具体错误码
	UEF_BQ_INVALID_OBJ       = 33,  // BMS升级对象非MO_BMS
	UEF_BQ_BUFF_NULL         = 34,  // BMS回复缓冲区为空

	// ==================== print_queue_task_update.c (35~36) Print升级队列 ====================
	UEF_PQ_INVALID_OBJ       = 35,  // Print升级对象无效
	UEF_PQ_BUFF_NULL         = 36,  // Print回复缓存为空

	// ==================== print_update_dcac.c (37~50) Print-DCAC转发 ====================
	UEF_PD_PREP_TASK_NULL    = 37,  // prepare_update任务指针空
	UEF_PD_TRANS_TASK_NULL   = 38,  // firmware_transfer tpDcacTask空
	UEF_PD_TRANS_BUFF_NULL   = 39,  // tpDcacTask回复缓存空
	UEF_PD_SLAVE_RESULT_ERR  = 40,  // 从机结果状态异常
	UEF_PD_C2_LEN_ERR        = 41,  // C2长度非3或数据空
	UEF_PD_C2_PROTO_ERR      = 42,  // C2协议类型非BAIKU
	UEF_PD_C2_REPLY_FAIL     = 43,  // C3回复失败
	UEF_PD_C5_HEAD_LEN_ERR   = 44,  // C5文件头长度非56或空
	UEF_PD_HEAD_PARSE_FAIL   = 45,  // DCAC文件头解析失败
	UEF_PD_CACHE_FULL        = 46,  // DCAC缓存空间不足
	UEF_PD_CACHE_WRITE_FAIL  = 47,  // DCAC缓存写入失败
	UEF_PD_HEAD_SEND_FAIL    = 48,  // DCAC文件头下发失败
	UEF_PD_FW_SEND_FAIL      = 49,  // DCAC固件数据下发失败
	UEF_PD_FW_CACHE_FAIL     = 50,  // DCAC固件数据缓存写入失败

	// ==================== Print_update_bms.c (51~61) Print-BMS转发 ====================
	UEF_PB_PREP_TASK_NULL    = 51,  // prepare_update任务指针空
	UEF_PB_C2_LEN_ERR        = 52,  // C2长度非3或数据空
	UEF_PB_C2_PROTO_ERR      = 53,  // C2协议类型非BAIKU
	UEF_PB_C2_PROTO_SELECT_FAIL = 54,// C2 ProtoSelect失败
	UEF_PB_C2_FWD_FAIL       = 55,  // C2转发BMS失败
	UEF_PB_TRANS_TASK_NULL   = 56,  // firmware_transfer tpBmsTask空
	UEF_PB_TRANS_BUFF_NULL   = 57,  // tpBmsTask回复缓存空
	UEF_PB_SLAVE_RESULT_ERR  = 58,  // 从机结果状态异常
	UEF_PB_FINISH_MISMATCH   = 59,  // C7总帧数/已收帧数不匹配
	UEF_PB_C5_DATA_ERR       = 60,  // C5数据空或长度0
	UEF_PB_C5_FWD_FAIL       = 61,  // C5转发BMS失败

	// ==================== sys_queue_task_update.c (62) 系统升级 ====================
	UEF_S_REC_OVERTIME       = 62,  // 升级接收超时

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
	UpdateTaskResult_E  eHostResult;		//主机升级结果
	UpdateTaskResult_E  eSlaveResult;		//从机升级结果
	ModuleObject_E		eObj;				//升级对象
	ChannelType_E		eChType;			//通道类型
	ProtoType_E 		eProtoType;			//协议类型
	UpdateErrCode_E		eErrCode;			//升级失败错误码
}Update_T;
extern Update_T  tUpdate;

/* ==========================================extern======================================*/
bool bUpdate_Init(void);
void vUpdate_InitParam(void);
void vUpdate_ResetTimeout(void);
void vUpdate_ResetRecTimeout(bool reset);
bool bUpdate_SetErrCode(UpdateErrCode_E code);
bool bUpdate_SetResult(UpdateResultTarget_E target, UpdateTaskResult_E result);
s8 cUpdate_ChSelect(ModuleObject_E e_obj, ChannelType_E ch_type);
s8 cUpdate_ProtoSelect(ModuleObject_E e_obj, ProtoType_E proto_type);
bool bUpdate_ResultIsNormal(UpdateResultTarget_E target);
void vUpdate_TickTimer(void);
#endif  //boardUPDATE

#ifdef __cplusplus
}
#endif

#endif  //SYS_QUEUE_TASK_UPDATE_H
