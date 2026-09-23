/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_prot_frame.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 通信协议帧封装与发送接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_BMS_PROT_FRAME_H_
#define MD_BMS_PROT_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardBMS_EN)
#include "Baiku/baiku_proto.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			bmsGET_PARAM_OBJ						0x10

//****************************************************Globals*******************************************************************//
extern BaikuProtoRx_t *tpBmsProtoRx;
extern BaikuProtoTx_t *tpBmsProtoTx;

//****************************************************Extern********************************************************************//
s8 c_bms_cs_get_param(uint8_t uc_num);
s8 c_bms_cs_switch(TaskInParam_U u_in_param);
s8 c_bms_cs_set_cali(uint8_t uc_num);
s8 c_bms_cs_get_app_info(uint16_t us_num);
s8 c_bms_cs_sys_set(tSysSetParam *p_tparam);
s8 c_bms_cs_req_chg(void);

#if (boardRUN_LOG_EN)
s8 c_bms_cs_log_unlock(uint8_t uc_key);
s8 c_bms_cs_get_run_log(uint16_t us_num);
s8 c_bms_cs_read_run_log_slot(uint16_t us_slot);
s8 c_bms_cs_reset_run_log(uint8_t uc_key);
#endif  /* boardRUN_LOG_EN */

s8 c_bms_cs_C2_set_update_proto(uint8_t *p_data, uint8_t uc_len);
s8 c_bms_cs_C5_send_file(uint8_t *p_data, uint8_t uc_len, uint8_t uc_sn);
s8 c_bms_cs_C7_update_finish(void);
s8 c_bms_cs_C8_trans_cancel(void);

bool bBms_SendProtInit(void);
bool bBms_RecProtInit(void);

#endif  /* boardBMS_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_BMS_PROT_FRAME_H_ */
