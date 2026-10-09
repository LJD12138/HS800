/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_rec_data_proc.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器接收数据解析与状态装载接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DCAC_REC_DATA_PROC_H_
#define MD_DCAC_REC_DATA_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDCAC_EN)
#include "Modbus/modbus_proto.h"
#include "Megmeet/megmeet_proto.h"

//****************************************************Extern********************************************************************//
int8_t c_dcac_rec_proc_data(ModbusProtoRx_t *p_proto_rx, ModbusProtoTx_t *p_proto_tx);
#if (boardUPDATE)
int8_t c_dcac_rec_proc_megmeet_proto(MegmeetProtoRx_t *p_proto_rx);
#endif  /* boardUPDATE */

#endif  /* boardDCAC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DCAC_REC_DATA_PROC_H_ */
