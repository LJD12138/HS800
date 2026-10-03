/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\Protocol\Modbus
 * File    : modbus_proto.h
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : Modbus 协议收发接口（命令码/收发结构体/API）
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MODBUS_PROTO_H
#define MODBUS_PROTO_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"
#include "lwrb.h"

#if (boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#endif  /* boardUSE_OS */

#if (1)

//****************************************************Macros********************************************************************//
//Modbus指令
#define			modbusWRITE_MULTI_REG					0x10	//写多个寄存器
#define			modbusWRITE_SINGLE_REG					0x06	//写单个寄存器
#define			modbusREAD_MULTI_REG					0x03	//读多个寄存器
#define			modbusREAD_MULTI_BIT					0x02	//读多个状态位
#define			modbusWRITE_SINGLE_BIT					0x05	//读单个输入状态

//****************************************************Types*********************************************************************//
typedef enum
{
	MRS_ADDR=0,
	MRS_LEN,
	MRS_END,
	MRS_ERR,
}ModbusRxStep_E;

/* #pragma pack(1) removed to fix alignment-related UsageFault */
typedef struct
{
	//数据帧开始
	u8*					ucpFrameData;
	//数据帧结束
	u16					usFrameDataSize;
        
	vu8					ucAddr;
	vu8					ucCmd;
	vu8					ucCharLen;
	vu8					ucWaitRecLen;
	vu16				usRegSize;
	vu16				usRegAddr;
	vu8					ucValidLen;			//有效数据的长度
	u8*					ucpValidData;		//指向有效数据的首地址
	vu16				usRecOverTimeCnt;
	vu16				usLostOverTimeCnt;
	u16					usTaskCycleTime;
	ModbusRxStep_E		eStep;
	lwrb_t				tRxBuff;
	u8					ucaData[];
}ModbusProtoRx_t;
/* #pragma pack() removed */

/* #pragma pack(1) removed to fix alignment-related UsageFault */
typedef struct
{
	/* 同步信息 */
	#if (boardUSE_OS)
	TaskHandle_t		xTaskToNotify;		/* 当前等待回包的任务句柄 */
	volatile bool		bWaitAck;       	/* 是否正在等待从机回包 */
	vu8 				ucWaitCmd;      	/* 期望等待的应答命令字 */
	vu16				usWaitRegAddr;		/* 期望等待的寄存器地址 */
	vs8 				cAckResult;     	/* 应答结果: 1-成功, <0-失败 */
	#endif  /* boardUSE_OS */

	/* 数据帧头信息 */
	vu8					ucAddr;
	vu8					ucCharLen;
	vu8					ucFrameLen;			/* 数据帧总长度 */
	u16					usFrameDataSize;
	vu16				usRegSize;
	vu16				usRegAddr;
	vu16				usRegData;
	u8					ucaFrameData[];		/* 数据帧 */
}ModbusProtoTx_t;
/* #pragma pack() removed */

//****************************************************Extern********************************************************************//
s8 cModbus_RecProtoInit(ModbusProtoRx_t** proto, u16 buff_len, u8 dev_addr, u16 cycle_time);
s8 cModbus_TransProtoInit(ModbusProtoTx_t** proto, u16 buff_len, u8 dev_addr);
s8 cModbus_ProtoCreate(ModbusProtoTx_t* proto,u8 cmd, u16 addr, u8* data, u16 len);
s8 cModbus_ProtoCheck(ModbusProtoRx_t* proto);
s8 cModbus_StepWaitOutTime(ModbusProtoRx_t* proto);
s8 cModbus_ResetRxBuff(ModbusProtoRx_t* proto);
s8 cModbus_ResetTx(ModbusProtoTx_t* proto, u16 len);
void vModbus_RecEnd(ModbusProtoRx_t* proto);
s8 cModbus_CheckReply(const ModbusProtoTx_t* proto_tx, const ModbusProtoRx_t* proto_rx);
#if (boardUSE_OS)
s8 cModbus_WaitReply(ModbusProtoTx_t* proto, u8 uc_cmd, u16 us_reg_addr, u16 timeout_ms);
s8 cModbus_NotifyAck(ModbusProtoTx_t* proto, s8 c_result);
#endif  /* boardUSE_OS */

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* MODBUS_PROTO_H */


