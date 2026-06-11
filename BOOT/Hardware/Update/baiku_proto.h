#ifndef BAIKU_PROTO_H__
#define BAIKU_PROTO_H__
#include "board_config.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
BAIKU数据包格式
---------------------------------------------------------------------------------------
|      BYTE1      |     BYTE2     |      BYTE3       | BYTE4~BYTE131 | BYTE132~BYTE133 |
---------------------------------------------------------------------------------------
| Start Of Header | Packet Number | ~(Packet Number) |  Packet Data  |    16Bit CRC    |
---------------------------------------------------------------------------------------
*/

//超时时间 10MS计时
#define BAIKU_RX_TIMEOUT_MS            		(2000/updateTASK_CYCLE_TIME)   //帧超时Ms
#define BAIKU_START_TIMEOUT_MS            	(60000/updateTASK_CYCLE_TIME)	//开始超时Ms
#define BAIKU_END_TIMEOUT_MS            	(10000/updateTASK_CYCLE_TIME)	//结束超时Ms

//XMMODE工作状态
typedef enum
{
    BAIKU_STATE_IDLE = 0,          //BAIKU空闲状态
    BAIKU_STATE_STANDBY,           //BAIKU就绪状态
    BAIKU_STATE_RECEIVING,         //BAIKU接收状态
    BAIKU_STATE_FINISH,            //BAIKU完成状态
    BAIKU_STATE_CANCEL,            //BAIKU取消状态
    BAIKU_STATE_STOP,              //BAIKU结束状态
    BAIKU_STATE_UNKNOW,				//未知
}BaikuWorkState_E;

typedef struct
{
	bool 				bStartSendFrm;		//可以发送数据
    vu8 				ucFrmCnt; 			//记录当前接收的帧数
	vu16 				usFrmOvertimeCnt;	//帧等待计时
    vu16 				usWaitStartOutTimeCnt;//开始等待计时
    vu16 				usWaitExitOutTimeCnt;//结束等待超时
    BaikuWorkState_E 	eState;				//工作状态
	s8 					(*c_xmodem_trans_data)(u8 cmd, u8 *buf, u16 len);
    s8 					(*c_xmodem_rec_data)(u8 *buf, u16 buf_len, u16 *len);
    void 				(*v_proc_check_ok_rec_data)(u8 *buf, u16 len);
	void 				(*v_rec_start)(void);
	void 				(*v_rec_end)(u8 code);
}BaiKuProto_T;
extern BaiKuProto_T tBaiKuProto;

void vBaiKuProto_Proto(BaiKuProto_T *obj, BaikuProtoRx_t *proto);
void vBaiKuProto_TickTime(BaiKuProto_T *obj);
void bBaiKuProto_Reset(BaiKuProto_T *obj, BaikuProtoRx_t *proto);

#ifdef __cplusplus
}
#endif

#endif //boardUPDATE

#endif //BAIKU_PROTO_H__
