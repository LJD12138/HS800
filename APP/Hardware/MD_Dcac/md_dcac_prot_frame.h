#ifndef MD_DCAC_PROT_FRAME_H_
#define MD_DCAC_PROT_FRAME_H_

#include "board_config.h"

#if(boardDCAC_EN)
#include "main.h"
#include "Modbus/modbus_proto.h"

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

#define  		dcacSWITCH_REG_ON                    	0x0001
#define  		dcacSWITCH_REG_OFF                   	0x0000
#define  		dcacPERM_CHG                    		0x0000
#define  		dcacIMPERM_CHG                   		0x0001
//初始化
#define  		dcacREG_ADDR_INIT              			4049
//放电开关
#define  		dcacREG_ADDR_DISCHG_SW               	4049
//设置AC充电功率		
#define  		dcacREG_ADDR_SET_TOTAL_CHG_PWR          4054
//设置AC充电功率		
#define  		dcacREG_ADDR_SET_AC_CHG_PWR             4060
//获取基础参数
#define  		dcacREG_ADDR_GET_PARAM1           		4036
#define  		dcacREG_ADDR_GET_PARAM2            		4013
#define  		dcacREG_ADDR_GET_PARAM3            		4026

extern			ModbusProtoTx_t 						*tpDcacProtoTx;
extern			ModbusProtoRx_t 						*tpDcacProtoRx;
extern			SemaphoreHandle_t 						dcacSemaphoreMutex;

bool b_dcac_cs_ac_output_switch(u16 temp);
bool b_dcac_cs_get_param1(void);
bool b_dcac_cs_get_param2(void);
bool b_dcac_cs_get_param3(void);
bool b_dcac_cs_set_total_chg_pwr(u16 pwr);
bool b_dcac_cs_set_chg_pwr(u16 pwr);
bool b_dcac_cs_init(void);
bool b_dcac_cs_set_para_in_pwr(u16 pwr);
bool b_dcac_cs_sys_switch(u16 temp);

bool bDcac_SendProtInit(void);
bool bDcac_RecProtInit(void);

#endif  //boardDCAC_EN

#endif  //MD_DCAC_PROT_FRAME_H_
