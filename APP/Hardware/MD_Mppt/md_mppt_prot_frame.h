#ifndef MD_MPPT_PROT_FRAME_H_
#define MD_MPPT_PROT_FRAME_H_

#include "board_config.h"

#if(boardMPPT_EN)
#include "main.h"
#include "Modbus/modbus_proto.h"

//设置充电功率		
#define  		mpptREG_ADDR_SET_PV_CHG_PWR             4059
//获取基础参数
#define  		mpptREG_ADDR_GET_PARAM1           		4017

extern			ModbusProtoTx_t 						*tpMpptProtoTx;
extern 			ModbusProtoRx_t 						*tpMpptProtoRx;

s8 c_mppt_cs_get_param(void);
s8 c_mppt_cs_set_pwr(u16 pwr);

bool bMppt_SendProtInit(void);
bool bMppt_RecProtInit(void);

#endif  //boardMPPT_EN

#endif  //MD_MPPT_PROT_FRAME_H_
