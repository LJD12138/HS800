#ifndef MD_PRINT_PROT_FRAME_H_
#define MD_PRINT_PROT_FRAME_H_

#include "board_config.h"

#if(boardPRINT_IFACE)
#include "main.h"
#include "Baiku/baiku_proto.h"

extern 			vu8 										uc_next_cmd;
extern 			BaikuProtoRx_t 								*tpPrintProtoRx;
extern 			BaikuProtoTx_t 								*tpPrintProtoTx;

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

bool bPrint_SendProtInit(void);
bool bPrint_RecProtInit(void);

#endif  //boardPRINT_IFACE

#endif  //MD_PRINT_PROT_FRAME_H_
