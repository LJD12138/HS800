#ifndef MD_DCAC_REC_DATA_PROC_H
#define MD_DCAC_REC_DATA_PROC_H

#include "board_config.h"

#if(boardDCAC_EN)
#include "main.h"
#include "Modbus/modbus_proto.h"

s8 c_dcac_rec_proc_data(ModbusProtoRx_t* proto_rx, ModbusProtoTx_t* proto_tx);

#endif  //boardDCAC_EN

#endif  //MD_DCAC_REC_DATA_PROC_H
