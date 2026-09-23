/***********************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
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

#if (boardUSE_OS)
extern SemaphoreHandle_t bmsSemaphoreMutex;
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
s8 c_bms_cs_get_param(uint8_t uc_num);
s8 c_bms_cs_switch(TaskInParam_U u_in_param);
s8 c_bms_cs_send_update(void);

bool bBms_SendProtInit(void);
bool bBms_RecProtInit(void);

#endif  /* boardBMS_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_BMS_PROT_FRAME_H_ */
