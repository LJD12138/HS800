#ifndef BAIKU_PROTO_H
#define BAIKU_PROTO_H

#include "main.h"
#include "lwrb.h"

#define     	baikuCMD_SWITCH							0x01	//开关机指令
#define     	baikuCMD_REPLY_SWITCH					0x02	//回复开关状态

#define     	baikuCMD_SET_TIME						0x05    //设置时间
#define     	baikuCMD_REPLY_SET_TIME					0x06    //回复设置结果

#define     	baikuCMD_GET_PARAM						0x07    //主获取数据
#define     	baikuCMD_REPLY_PARAM					0x08    //回复获取数据

#define     	baikuCMD_SET_CHG_PWR             		0x40	//设置充电功率
#define     	baikuCMD_REPLY_SET_CHG_PWR           	0x41	//回复充电功率
#define     	baikuCMD_CALI             				0x44	//校准
#define     	baikuCMD_REPLY_CALI           			0x45	//回复校准

#define     	baikuCMD_GET_MEM_PARAM            		0x80	//获取记忆参数
#define     	baikuCMD_REPLY_MEM_PARAM           		0x81	//回复记忆参数
#define     	baikuCMD_WRITE_MEM_PARAM            	0x82	//写入记忆参数
#define     	baikuCMD_REPLY_WRITE_MEM_PARAM          0x83	//回复记忆参数
#define     	baikuCMD_SET_PRINT_STATE            	0x84	//设置Print状态
#define     	baikuCMD_REPLY_SET_PRINT_STATE          0x85	//回复设置结果
#define     	baikuCMD_GET_PRINT_STATE            	0x86	//获取Print状态
#define     	baikuCMD_REPLY_PRINT_STATE          	0x87	//回复Print状态
#define     	baikuCMD_SYS_SET            			0x88	//系统设置
#define     	baikuCMD_REPLY_SYS_SET           		0x89	//回复系统设置

#define     	baikuCMD_REQ_CHG            			0x90	//请求充电
#define     	baikuCMD_REPLY_REQ_CHG           		0x91	//回复请求充电

#define     	baikuCMD_GET_TEST_PARAM            		0xB0	//获取测试参数
#define     	baikuCMD_REPLY_TEST_PARAM           	0xB1	//回复测试参数
#define     	baikuCMD_GET_ERR_LOG           			0xB2	//获取错误日志

#define     	baikuCMD_COMSOLE_UPDATE					0xC0    //上位机主控正在升级
#define     	baikuCMD_REPLY_COMSOLE_UPDATE			0xC1    //回复应答
#define     	baikuCMD_SET_PROTO						0xC2    //设置升级协议
#define     	baikuCMD_REPLY_SET_PROTO				0xC3    //回复应答
#define     	baikuCMD_RRQ_START_SEND					0xC4    //请求开始发送 NAK
#define     	baikuCMD_REPLY_DATA						0xC5    //回复数据
#define     	baikuCMD_RRQ_CONT_SEND					0xC6    //请求继续发送 ACK
#define     	baikuCMD_REPLY_FINISH					0xC7    //回复数据完成发送 EOT
#define     	baikuCMD_REPLY_CANEL					0xC8    //回复数据取消发送 CAN
#define     	baikuCMD_BMS_UPDATE						0xC9    //下位机BMS正在升级
#define     	baikuCMD_REPLY_BMS_UPDATE				0xCA    //回复应答
#define     	baikuCMD_MPPT_UPDATE					0xCB    //下位机MPPT正在升级
#define     	baikuCMD_REPLY_MPPT_UPDATE				0xCC    //回复应答
typedef enum
{
	RS_HEAD=0,
	RS_LEN,
	RS_END,
}BaikuRxStep_E; 

#pragma pack(1)
typedef struct
{
	//数据帧开始
	vu8             	ucHead;
	vu8             	ucAddr;
	vu8             	ucRemainLen;
	u8*             	ucpRemainData;
	//数据帧结束
						
    vu8             	ucCmd;    
    vu8             	ucSN;
	vu8             	ucWaitRecLen;
	vu8             	ucValidLen;    //有效数据的长度
	u8*             	ucpValidData;  //指向有效数据的首地址
	vu16            	usRecOverTimeCnt;
	vu16            	usLostOverTimeCnt;
	u16            		usTaskCycleTime;
	BaikuRxStep_E    	eStep;
	lwrb_t				tRxBuff;
	u8             		ucaData[];
}BaikuProtoRx_t;
#pragma pack()

#pragma pack(1)
typedef struct
{
	vu8             	ucHead;
	vu8             	ucAddr;
	vu8             	ucFrameLen;    //数据帧总长度
	u8             		ucaFrameData[];  //数据帧
}BaikuProtoTx_t;
#pragma pack()

s8 cBaiku_ProtoRecInit(BaikuProtoRx_t** proto, u16 buff_len, u8 dev_addr, u16 cycle_time);
s8 cBaiku_ProtoSendInit(BaikuProtoTx_t** proto, u16 buff_len, u8 dev_addr);
s8 cBaiku_ProtoCreate(BaikuProtoTx_t* proto,u8 cmd, u8* data, u8 len);
s8 cBaiku_ProtoCheck(BaikuProtoRx_t* proto);
s8 cBaiku_UpdateCheck(BaikuProtoRx_t* proto, u8* ucp_data, u16 len);
s8 cBaiku_StepWaitOutTime(BaikuProtoRx_t* proto);
s8 cBaiku_ResetRxBuff(BaikuProtoRx_t* proto);

#endif  //BAIKU_PROTO_H
