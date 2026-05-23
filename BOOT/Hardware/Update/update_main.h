#ifndef UPDATE_H_
#define UPDATE_H_

#include "board_config.h"

#if(boardUPDATE)
#include "Update/xmodem_proto.h"
#include "Update/baiku_proto.h"

extern Xmodem_T tXmodem;
extern BaiKuProto_T tBaiKuProto;

#endif //#if(boardUPDATE)

#endif //UPDATE_H_


