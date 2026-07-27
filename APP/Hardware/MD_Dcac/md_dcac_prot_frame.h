#ifndef MD_DCAC_PROT_FRAME_H_
#define MD_DCAC_PROT_FRAME_H_

#include "board_config.h"

#if(boardDCAC_EN)
#include "main.h"
#include "Modbus/modbus_proto.h"
#include "Sys/sys_queue_task_update.h"

#if(boardUSE_OS)
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

#define  		dcacSWITCH_REG_ON                    	0x0001
#define  		dcacSWITCH_REG_OFF                   	0x0000
#define  		dcacPERM_CHG                    		0x0000
#define  		dcacIMPERM_CHG                   		0x0001
//初始化
#define  		dcacREG_ADDR_INIT              			4049

/* DCAC升级模块使用的Megmeet芯片类型，应与固件文件头及A6回复中的芯片ID保持一致 */
#define        dcacUPDATE_IC_TYPE                      MEGMEET_IC_TYPE_AC
//放电开关
#define  		dcacREG_ADDR_DISCHG_SW               	4049
//设置AC充电功率		
#define  		dcacREG_ADDR_SET_TOTAL_CHG_PWR          4054
//设置AC充电功率		
#define  		dcacREG_ADDR_SET_AC_CHG_PWR             4060
//获取基础参数
#define  		dcacREG_ADDR_GET_PARAM1           		4036
#define  		dcacREG_ADDR_GET_PARAM2            		4013
#define  		dcacREG_ADDR_GET_PARAM3            		4026

extern			ModbusProtoTx_t 						*tpDcacProtoTx;
extern			ModbusProtoRx_t 						*tpDcacProtoRx;
extern			SemaphoreHandle_t 						dcacSemaphoreMutex;

bool b_dcac_cs_ac_output_switch(u16 temp);
bool b_dcac_cs_get_param1(void);
bool b_dcac_cs_get_param2(void);
bool b_dcac_cs_get_param3(void);
bool b_dcac_cs_set_total_chg_pwr(u16 pwr);
bool b_dcac_cs_set_chg_pwr(u16 pwr);
bool b_dcac_cs_init(void);
bool b_dcac_cs_set_para_in_pwr(u16 pwr);
bool b_dcac_cs_sys_switch(u16 temp);

bool bDcac_SendProtInit(void);
bool bDcac_RecProtInit(void);
bool bDcac_MegmeetProtInit(void);

/* 协议帧发送函数 */
u8 ucDcac_GetUpdateSlaveAddr(ModuleObject_E e_obj);
u8 ucDcac_GetUpdateIcType(ModuleObject_E e_obj);
bool b_dcac_send_megmeet_frame(u8 slave_addr, u8 ic_type, u8 cmd, const u8* payload, u16 payload_len);
bool b_dcac_send_f0(u8 uc_payload);
bool b_dcac_send_f6(bool b_reset_timeout);
bool b_dcac_send_f2(u32 ul_baud, bool b_reset_timeout);
bool b_dcac_cs_send_fw_data(u8 cmd, const u8* payload, u16 payload_len, bool b_reset_timeout);

/* 升级阶段DCAC任务回复缓存的线程安全访问接口 */
bool b_dcac_update_buf_write(Task_T* task, const u8* data, u16 len);
bool b_dcac_update_buf_peek(Task_T* task, u8* data, u16 len);
bool b_dcac_update_buf_reset(Task_T* task);

#endif  //boardDCAC_EN

#endif  //MD_DCAC_PROT_FRAME_H_
