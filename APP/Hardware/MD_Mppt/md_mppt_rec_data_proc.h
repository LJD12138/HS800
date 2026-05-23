#ifndef MD_MPPT_REC_DATA_PROC_H
#define MD_MPPT_REC_DATA_PROC_H

#include "board_config.h"

#if(boardMPPT_EN)
#include "main.h"
#include "Modbus/modbus_proto.h"

s8 c_mppt_rec_proc_data(ModbusProtoRx_t* proto_rx, ModbusProtoTx_t* proto_tx);

#endif  //boardMPPT_EN

#endif  //MD_MPPT_REC_DATA_PROC_H
