#ifndef MD_BMS_REC_DATA_PROC_H
#define MD_BMS_REC_DATA_PROC_H

#include "board_config.h"

#if(boardBMS_EN)
#include "main.h"
#include "Modbus1/modbus_proto1.h"

s8 c_bms_rec_proc_data(ModbusProtoRx1_t* proto_rx, ModbusProtoTx1_t* proto_tx);

#endif  //boardBMS_EN

#endif  //MD_BMS_REC_DATA_PROC_H
