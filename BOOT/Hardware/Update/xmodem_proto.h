#ifndef XMODEM_PROTO_H__
#define XMODEM_PROTO_H__
#include "board_config.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
XMODEM数据包格式
---------------------------------------------------------------------------------------
|      BYTE1      |     BYTE2     |      BYTE3       | BYTE4~BYTE131 | BYTE132~BYTE133 |
---------------------------------------------------------------------------------------
| Start Of Header | Packet Number | ~(Packet Number) |  Packet Data  |    16Bit CRC    |
---------------------------------------------------------------------------------------
*/
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


//XMODEM数据长度
#define FRAME_LEN_LEN_128             		128
#define FRAME_LEN_LEN_1K              		1024

//XMODEM字段关键字长度
#define XMODEM_FRM_FLAG_LEN_ADD         	4
#define XMODEM_FRM_FLAG_LEN_CRC         	5

//XMODEM缓冲区长度配置
#define XMODEM_BUF_LEN_128_ADD          	(FRAME_LEN_LEN_128+XMODEM_FRM_FLAG_LEN_ADD)
#define XMODEM_BUF_LEN_128_CRC          	(FRAME_LEN_LEN_128+XMODEM_FRM_FLAG_LEN_CRC)
#define XMODEM_BUF_LEN_1K_ADD           	(FRAME_LEN_LEN_1K+XMODEM_FRM_FLAG_LEN_ADD)
#define XMODEM_BUF_LEN_1K_CRC           	(FRAME_LEN_LEN_1K+XMODEM_FRM_FLAG_LEN_CRC)
#define XMODEM_BUF_LEN_MAX              	XMODEM_BUF_LEN_1K_CRC

//超时时间 10MS计时
#define XMODEM_RX_TIMEOUT_MS            	(100/updateTASK_CYCLE_TIME)   //帧超时Ms
#define XMODEM_START_TIMEOUT_MS            	(60000/updateTASK_CYCLE_TIME)	//开始超时Ms
#define XMODEM_END_TIMEOUT_MS            	(10000/updateTASK_CYCLE_TIME)	//结束超时Ms


//数据接收状态
typedef enum
{
    REC_STATE_IDLE = 0,          	//接收空闲
    REC_STATE_WAIT,          		//接收等待
    REC_STATE_TIMEOUT,       		//接收超时
    REC_STATE_OK,            		//接收完成
}RecState_E;

//数据帧长度
typedef enum
{
    FRAME_LEN_128 = 0,            	//128字节数据
    FRAME_LEN_1024            		//1024字节数据
}FrameLen_E;

//XMODEM校验模式配置
typedef enum
{
    CHECK_MODE_ADD = 0,          	//累加和校验
    CHECK_MODE_CRC,          		//CRC校验
    CHECK_MODE_MAX
}CheckMode_E;

//XMMODE工作状态
typedef enum
{
    XMODEM_STATE_IDLE = 0,          //XMODEM空闲状态
    XMODEM_STATE_STANDBY,           //XMODEM就绪状态
    XMODEM_STATE_RECEIVING,         //XMODEM接收状态
    XMODEM_STATE_FINISH,            //XMODEM完成状态
    XMODEM_STATE_CANCEL,            //XMODEM取消状态
    XMODEM_STATE_STOP,              //XMODEM结束状态
    XMODEM_STATE_UNKNOW,			//未知
}WorkState_E;

typedef struct
{
	bool 				bStartSendFrm;		//可以发送数据
    vu8 				frm_cnt; 			//记录当前接收的帧数
	vu16 				usRecLen;			//接收数据的长度
	vu16 				usFrmOvertimeCnt;	//帧等待计时
    vu16 				usWaitStartOutTimeCnt;//开始等待计时
    vu16 				usWaitExitOutTimeCnt;//结束等待超时
	
    FrameLen_E 			eFrameLen;			//协议帧长度类型 128/1024
    CheckMode_E 		eCheckMode;			//校验模式
    WorkState_E 		eState;				//工作状态
	RecState_E 			eRecState;			//接受状态
	u8 					buf[XMODEM_BUF_LEN_MAX];//XMODEM接收数据缓冲区
	s8 					(*c_xmodem_trans_data)(u8 cmd, u8 *buf, u16 len);
    s8 					(*c_xmodem_rec_data)(u8 *buf, u16 buf_len, u16 *len);
    void 				(*v_proc_check_ok_rec_data)(u8 *buf, u16 len);
	void 				(*v_rec_start)(void);
	void 				(*v_rec_end)(u8 code);
}Xmodem_T;
extern Xmodem_T tXmodem;

void vXmodem_Proto(Xmodem_T *obj);
void vXmodem_TickTime(Xmodem_T *obj);
void bXmodem_Reset(Xmodem_T *obj);

#ifdef __cplusplus
}
#endif

#endif //boardUPDATE

#endif //XMODEM_PROTO_H__
