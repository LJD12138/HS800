/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_prot_frame.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print通信协议帧组包与解析接口声明
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef __PRINT_PROT_FRAME_H
#define __PRINT_PROT_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardPRINT_IFACE)
#include "Baiku/baiku_proto.h"

//****************************************************Macros********************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Globals*******************************************************************//
extern vu8              uc_next_cmd;
extern BaikuProtoRx_t   *tpPrintProtoRx;
extern BaikuProtoTx_t   *tpPrintProtoTx;

//****************************************************Extern********************************************************************//
s8 c_cycle_relay_data(void);
s8 c_relay02_switch_result(uint8_t* data);
s8 c_relay08_param(void);
s8 c_relay0A_bat_param(void);
s8 c_relay0C_dcac_param(void);
s8 c_relay0E_mppt_param(void);
s8 c_relay10_usb_param(void);
s8 c_relay12_dc_param(void);
s8 c_relay14_sysinfo_param(void);
s8 c_relay40_set_chg_pwr(BaikuProtoRx_t* proto);
s8 c_relay44_cali(BaikuProtoRx_t* proto);
s8 c_relay45_cali(u16 temp);
s8 c_relay80_get_mem_param(BaikuProtoRx_t* proto);
s8 c_relay82_write_mem_info(BaikuProtoRx_t* proto);
s8 c_relay84_set_print_state(BaikuProtoRx_t* proto);
s8 c_relay86_get_print_state(BaikuProtoRx_t* proto);
s8 c_relay88_sys_set(BaikuProtoRx_t* proto);
s8 c_relay_bms_app_info(u8* data, u16 len);

#if (boardBMS_EN && boardRUN_LOG_EN)
s8 c_relay8A_log_unlock(BaikuProtoRx_t* proto);
s8 c_relayB4_get_run_log(BaikuProtoRx_t* proto);
s8 c_relayB6_read_run_log_slot(BaikuProtoRx_t* proto);
s8 c_relayB8_reset_run_log(BaikuProtoRx_t* proto);
s8 c_print_cs_reply_log_unlock(u8 res);
s8 c_print_cs_reply_run_log_slot(u8* data, u8 len);
s8 c_print_cs_reply_reset_run_log(u8 res);
s8 c_print_cs_send_run_log_rec(u8* rec_data, u8 len);
s8 c_print_cs_send_run_log_end(void);
#endif  /* boardBMS_EN && boardRUN_LOG_EN */

#if (boardUPDATE)
s8 c_print_cs_C3_reply_set_proto(u8* data, u8 len);
s8 c_print_cs_C4_req_start_send(void);
s8 c_print_cs_C4_req_resend_curr(void);
s8 c_print_cs_C6_req_cont_send(void);
s8 c_print_cs_C8_trans_cancel(void);
#endif  /* boardUPDATE */

s8 c_print_info_trans(u8* data, u8 len);

bool bPrint_SendProtInit(void);
bool bPrint_RecProtInit(void);

#endif  /* boardPRINT_IFACE */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* __PRINT_PROT_FRAME_H */
