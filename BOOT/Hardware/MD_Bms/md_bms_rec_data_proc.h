/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_rec_data_proc.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 接收数据解析与状态装载接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_BMS_REC_DATA_PROC_H_
#define MD_BMS_REC_DATA_PROC_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardBMS_EN)
#include "Baiku/baiku_proto.h"

//****************************************************Extern********************************************************************//
s8 c_bms_rec_proc_data(BaikuProtoRx_t *p_proto);

#endif  /* boardBMS_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_BMS_REC_DATA_PROC_H_ */
