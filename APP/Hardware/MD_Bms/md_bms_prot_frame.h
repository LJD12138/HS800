#ifndef MD_BMS_PROT_FRAME_H_
#define MD_BMS_PROT_FRAME_H_

#include "board_config.h"

#if(boardBMS_EN)
#include "main.h"
#include "Baiku/baiku_proto.h"

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

#define     	bmsGET_PARAM_OBJ					0x10

extern BaikuProtoRx_t *tpBmsProtoRx;
extern BaikuProtoTx_t *tpBmsProtoTx;

#if(boardUSE_OS)
extern SemaphoreHandle_t bmsSemaphoreMutex;
#endif  //boardUSE_OS

s8 c_bms_cs_get_param(u8 num);
s8 c_bms_cs_switch(TaskInParam_U u_in_param);
s8 c_bms_cs_set_cali(u8 num);
s8 c_bms_cs_get_app_info(u16 num);
s8 c_bms_cs_sys_set(tSysSetParam *tparam);
s8 c_bms_cs_req_chg(void);

s8 c_bms_cs_C2_set_update_proto(u8* data, u8 len);
s8 c_bms_cs_C5_send_file(u8* data, u8 len, u8 ucSN);
s8 c_bms_cs_C7_update_finish(void);
s8 c_bms_cs_C8_trans_cancel(void);

bool bBms_SendProtInit(void);
bool bBms_RecProtInit(void);

#endif  //boardBMS_EN

#endif  //MD_BMS_PROT_FRAME_H_
