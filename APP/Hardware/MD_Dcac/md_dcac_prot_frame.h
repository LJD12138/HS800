/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_prot_frame.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器通信协议帧组帧与收发接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef MD_DCAC_PROT_FRAME_H_
#define MD_DCAC_PROT_FRAME_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardDCAC_EN)
#include "Modbus/modbus_proto.h"
#include "Sys/sys_queue_task_update.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			dcacSWITCH_REG_ON						0x0001
#define			dcacSWITCH_REG_OFF						0x0000
#define			dcacPERM_CHG							0x0000
#define			dcacIMPERM_CHG							0x0001

/* 初始化 */
#define			dcacREG_ADDR_INIT						4049

#if (boardUPDATE)
/* DCAC升级模块使用的Megmeet芯片类型，应与固件文件头及A6回复中的芯片ID保持一致 */
#define			dcacUPDATE_IC_TYPE						MEGMEET_IC_TYPE_AC
#endif  /* boardUPDATE */

/* 放电开关 */
#define			dcacREG_ADDR_DISCHG_SW					4049

/* 设置AC充电功率 */
#define			dcacREG_ADDR_SET_TOTAL_CHG_PWR			4054
#define			dcacREG_ADDR_SET_AC_CHG_PWR				4060

/* 获取基础参数 */
#define			dcacREG_ADDR_GET_PARAM1					4036
#define			dcacREG_ADDR_GET_PARAM2					4013
#define			dcacREG_ADDR_GET_PARAM3					4026

//****************************************************Globals*******************************************************************//
extern ModbusProtoTx_t                                      *tpDcacProtoTx;
extern ModbusProtoRx_t                                      *tpDcacProtoRx;

#if (boardUSE_OS)
extern SemaphoreHandle_t                                    dcacSemaphoreMutex;
#endif  /* boardUSE_OS */

//****************************************************Extern********************************************************************//
bool b_dcac_cs_ac_output_switch(uint16_t us_temp);
bool b_dcac_cs_get_param1(void);
bool b_dcac_cs_get_param2(void);
bool b_dcac_cs_get_param3(void);
bool b_dcac_cs_set_total_chg_pwr(uint16_t us_pwr);
bool b_dcac_cs_set_chg_pwr(uint16_t us_pwr);
bool b_dcac_cs_init(void);
bool b_dcac_cs_set_para_in_pwr(uint16_t us_pwr);
bool b_dcac_cs_sys_switch(uint16_t us_temp);

bool bDcac_SendProtInit(void);
bool bDcac_RecProtInit(void);

#if (boardUPDATE)
bool bDcac_MegmeetProtInit(void);

/* 协议帧发送函数 */
uint8_t ucDcac_GetUpdateSlaveAddr(ModuleObject_E e_obj);
uint8_t ucDcac_GetUpdateIcType(ModuleObject_E e_obj);
bool b_dcac_send_megmeet_frame(uint8_t uc_slave_addr, uint8_t uc_ic_type, uint8_t uc_cmd, const uint8_t *p_payload, uint16_t us_payload_len);
bool b_dcac_send_f0(uint8_t uc_payload);
bool b_dcac_send_f6(bool b_reset_timeout);
bool b_dcac_send_f2(uint32_t ul_baud, bool b_reset_timeout);
bool b_dcac_cs_send_fw_data(uint8_t uc_cmd, const uint8_t *p_payload, uint16_t us_payload_len, bool b_reset_timeout);
#endif  /* boardUPDATE */

/* 升级阶段DCAC任务回复缓存的线程安全访问接口 */
bool b_dcac_update_buf_write(Task_T *p_task, const uint8_t *p_data, uint16_t us_len);
bool b_dcac_update_buf_peek(Task_T *p_task, uint8_t *p_data, uint16_t us_len);
bool b_dcac_update_buf_reset(Task_T *p_task);

#endif  /* boardDCAC_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* MD_DCAC_PROT_FRAME_H_ */
