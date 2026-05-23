#ifndef MD_BMS_PROT_FRAME_H_
#define MD_BMS_PROT_FRAME_H_

#include "board_config.h"

#if(boardBMS_EN)
#include "main.h"
#include "Modbus1/modbus_proto1.h"

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

//控制开关		
#define  		bmsREG_ADDR_CTRL_SWITCH             	3001
//获取基础参数
#define  		bmsREG_ADDR_GET_PARAM           		3000

extern ModbusProtoTx1_t *tpBmsProtoTx;
extern ModbusProtoRx1_t *tpBmsProtoRx;

#if(boardUSE_OS)
extern SemaphoreHandle_t bmsSemaphoreMutex;
#endif  //boardUSE_OS

s8 c_bms_cs_get_param(u8 num);
s8 c_bms_cs_switch(TaskInParam_U u_in_param);
s8 c_bms_cs_set_cali(u8 num);
s8 c_bms_cs_get_app_info(u16 num);
s8 c_bms_cs_sys_set(tSysSetParam *tparam);
s8 c_bms_cs_req_chg(void);

bool bBms_SendProtInit(void);
bool bBms_RecProtInit(void);


#endif  //boardBMS_EN

#endif  //MD_BMS_PROT_FRAME_H_
