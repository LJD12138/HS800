/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_rec_data_proc.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 接收数据协议解包函数声明
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_MPPT_REC_DATA_PROC_H_
#define MD_MPPT_REC_DATA_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardMPPT_EN)
#include "Modbus/modbus_proto.h"

//****************************************************Extern********************************************************************//
s8 c_mppt_rec_proc_data(ModbusProtoRx_t *proto_rx, ModbusProtoTx_t *proto_tx);

#endif  /* boardMPPT_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_MPPT_REC_DATA_PROC_H_ */
