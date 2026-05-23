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
s8 c_relay84_set_proto(BaikuProtoRx_t* proto);
s8 c_relay84_set_print_state(BaikuProtoRx_t* proto);
s8 c_relay86_get_print_state(BaikuProtoRx_t* proto);
s8 c_relay88_sys_set(BaikuProtoRx_t* proto);


bool b_get_bms_ver_info(u8* data, u8* data_len);

bool bPrint_SendProtInit(void);
bool bPrint_RecProtInit(void);

#endif  //boardPRINT_IFACE

#endif  //MD_PRINT_PROT_FRAME_H_
