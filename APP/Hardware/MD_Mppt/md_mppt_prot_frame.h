/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_prot_frame.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 通信协议帧封包与解析函数声明头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_MPPT_PROT_FRAME_H_
#define MD_MPPT_PROT_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardMPPT_EN)
#include "Modbus/modbus_proto.h"

//****************************************************Macros********************************************************************//
/* 设置充电功率 */
#define			mpptREG_ADDR_SET_PV_CHG_PWR				4059
/* 获取基础参数 */
#define			mpptREG_ADDR_GET_PARAM1					4017

//****************************************************Globals*******************************************************************//
extern ModbusProtoTx_t *tpMpptProtoTx;
extern ModbusProtoRx_t *tpMpptProtoRx;

//****************************************************Extern********************************************************************//
s8   c_mppt_cs_get_param(void);
s8   c_mppt_cs_set_pwr(u16 pwr);
bool bMppt_SendProtInit(void);
bool bMppt_RecProtInit(void);

#endif  /* boardMPPT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_MPPT_PROT_FRAME_H_ */
