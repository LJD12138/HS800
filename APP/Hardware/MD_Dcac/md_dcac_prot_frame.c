/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_prot_frame.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器通信协议帧组帧与收发实现(Modbus与Megmeet升级协议)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_prot_frame.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "Print/print_task.h"
#include "Megmeet/megmeet_proto.h"
#include "Sys/sys_queue_task_update.h"
#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			dcacDEV_ADRR							0x01
#define			dcacWAIT_NOTIFY_OUTTIME					500		/* 任务通知超时时间 (ms) */
#define			dcacTX_PROTO_BUFF_LEN					64
#define			dcacRX_PROTO_BUFF_LEN					64

#define			dcTASK_UPDATE_TX_FRAME_SIZE				256		/* DCAC升级发送帧缓存大小 (字节) */
#define			dcTASK_UPDATE_RX_FRAME_SIZE				64		/* DCAC升级接收帧缓存大小 (字节) */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) ModbusProtoTx_t *tpDcacProtoTx = NULL;
__ALIGNED(4) ModbusProtoRx_t *tpDcacProtoRx = NULL;

#pragma pack(1)
typedef struct
{
	vu16				usAcOutSwitch;
	vu16				usBatOV;			//0.1V
	vu16				usBatUV;			//0.1V
	vu16				usOutFreq;			//50/60
	vu16				usOutVolt;			//100/110/120/220/230/240
	vu16				usChgPwr;			//1W
	vu16				usDisChgPwr;
	vu16				usChgVolt;			//0.001V
	vu16				usPvOV;				//0.1V
	vs16				ucFan;
	vu16				usPvChgPwr;			//1W
	vu16				usAcChgPwr;			//1W
	vu16				temp2;
	vu16				usMaxInCurr;		//0.1A
}DcacInitPkt_T;
#pragma pack()

static DcacInitPkt_T s_tDcacInit;

#if (boardUSE_OS)
SemaphoreHandle_t dcacSemaphoreMutex = NULL;
#endif  /* boardUSE_OS */

MegmeetProtoTx_t *tDcacMegmeetProtoTx  = NULL;
MegmeetProtoRx_t *tpDcacMegmeetProtoRx = NULL;

//****************************************************Function Declaration******************************************************//
static int8_t c_dcac_data_trans(uint8_t uc_cmd, uint16_t us_reg_addr, uint8_t *p_data, uint8_t uc_len);


/***********************************************************************************************************************
 * 函数功能    : 通讯发送协议初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bDcac_SendProtInit(void)
{
    int8_t c_result = cModbus_TransProtoInit(&tpDcacProtoTx, dcacTX_PROTO_BUFF_LEN, dcacDEV_ADRR);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
            log_e("bDcacTask:tpDcacProtoTx协议对象初始化失败,代码%d", c_result);
        return false;
    }

    #if (boardUSE_OS)
    dcacSemaphoreMutex = xSemaphoreCreateMutex();
    #endif  /* boardUSE_OS */

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 通讯接收协议初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bDcac_RecProtInit(void)
{
    int8_t c_result = cModbus_RecProtoInit(&tpDcacProtoRx, dcacRX_PROTO_BUFF_LEN,
                                           dcacDEV_ADRR, boardREPET_TIMER_CYCLE_TMIE);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
            log_e("bDcacRecTask:tpDcacProtoRx协议对象初始化失败,代码%d", c_result);
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 写入 DCAC 升级回复缓存
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务指针, p_data: 待写入数据, us_len: 长度
 * 输出参数    : 无
 * 返回值      : bool: true-写入成功
 ************************************************************************************************************************/
bool b_dcac_update_buf_write(Task_T *p_task, const uint8_t *p_data, uint16_t us_len)
{
    return bQueue_WriteReply(p_task, p_data, us_len);
}

/***********************************************************************************************************************
 * 函数功能    : 偷窥读取 DCAC 升级回复缓存
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务指针, p_data: 存储指针, us_len: 长度
 * 输出参数    : 无
 * 返回值      : bool: true-成功
 ************************************************************************************************************************/
bool b_dcac_update_buf_peek(Task_T *p_task, uint8_t *p_data, uint16_t us_len)
{
    return (usQueue_PeekReply(p_task, 0, p_data, us_len) == us_len);
}

/***********************************************************************************************************************
 * 函数功能    : 复位 DCAC 升级回复缓存
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务指针
 * 输出参数    : 无
 * 返回值      : bool: true-成功
 ************************************************************************************************************************/
bool b_dcac_update_buf_reset(Task_T *p_task)
{
    vQueue_ResetReply(p_task);
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 控制 AC 输出开关
 * 说明(备注)  : 无
 * 传入参数    : us_temp: dcacSWITCH_REG_ON / dcacSWITCH_REG_OFF
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_ac_output_switch(uint16_t us_temp)
{
    if (c_dcac_data_trans(modbusWRITE_SINGLE_REG, dcacREG_ADDR_DISCHG_SW, (uint8_t *)&us_temp, 1) <= 0)
        return false;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 获取 Param1 参数
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_get_param1(void)
{
    if (c_dcac_data_trans(modbusREAD_MULTI_REG, dcacREG_ADDR_GET_PARAM1, NULL, sizeof(DCAC_Param1_t) / 2) <= 0)
        return false;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 获取 Param2 参数
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_get_param2(void)
{
    if (c_dcac_data_trans(modbusREAD_MULTI_REG, dcacREG_ADDR_GET_PARAM2, NULL, sizeof(DCAC_Param2_t) / 2) <= 0)
        return false;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 获取 Param3 参数
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_get_param3(void)
{
    if (c_dcac_data_trans(modbusREAD_MULTI_REG, dcacREG_ADDR_GET_PARAM3, NULL, sizeof(DCAC_Param3_t) / 2) <= 0)
        return false;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置总充电功率
 * 说明(备注)  : 无
 * 传入参数    : us_pwr: 充电功率 (W)
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_set_total_chg_pwr(uint16_t us_pwr)
{
	// s_tDcacInit.usChgVolt = tAppMemParam.tBMS.usChgVolt * 100; //0.1V
	s_tDcacInit.usChgPwr = us_pwr;	//充电功率W
	// s_tDcacInit.usDisChgPwr = tAppMemParam.tDCAC.usOutPwrRating;
	if(c_dcac_data_trans(modbusWRITE_MULTI_REG, 
						dcacREG_ADDR_SET_TOTAL_CHG_PWR, 
						(u8*)&s_tDcacInit.usChgPwr, 
						1) <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置 AC 充电功率
 * 说明(备注)  : 无
 * 传入参数    : us_pwr: 功率 (W)
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_set_chg_pwr(uint16_t us_pwr)
{
    if (c_dcac_data_trans(modbusWRITE_SINGLE_REG, dcacREG_ADDR_SET_AC_CHG_PWR, (uint8_t *)&us_pwr, 1) <= 0)
        return false;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 DCAC 硬件寄存器
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_init(void)
{
	s_tDcacInit.usAcOutSwitch = dcacSWITCH_REG_OFF;
	s_tDcacInit.usBatOV = tAppMemParam.tBMS.usMaxVolt;	//0.1V
	s_tDcacInit.usBatUV = tAppMemParam.tBMS.usMinVolt;	//0.1V
	s_tDcacInit.usOutFreq = (tAppMemParam.tDCAC.usAcOutFreq == 0)? 50:60;	//0:50HZ 1:60HZ
	
	#if(boardDCAC_VOLT_TYPE==0)	//110V
	s_tDcacInit.usOutVolt = 110;	
	#elif(boardDCAC_VOLT_TYPE==3) //230V
	s_tDcacInit.usOutVolt = 230;
	#else
    #error "DCAC类型定义有误"
	#endif  /* boardDCAC_VOLT_TYPE */
	
	s_tDcacInit.usChgVolt = tAppMemParam.tBMS.usChgVolt * 100; //0.1V
	// tDcacInit.usChgPwr = tAppMemParam.tDCAC.usInPwrRating;	//充电功率W
	s_tDcacInit.usChgPwr = 0;	//充电功率W
	s_tDcacInit.usDisChgPwr = tAppMemParam.tDCAC.usOutPwrRating;
	s_tDcacInit.usPvOV = tAppMemParam.tMPPT.usMaxInVolt; //0.1V


	// if(strstr(boardSOFTWARE_VERSION, "G3604") != NULL)
	// 	tDcacInit.ucFan = 0;
	// else
		s_tDcacInit.ucFan = 0;
	
	s_tDcacInit.usPvChgPwr = 0;//1W
	s_tDcacInit.usAcChgPwr = 0;//1W
	s_tDcacInit.usMaxInCurr = tAppMemParam.tDCAC.usMaxInCurr;
	
    if (c_dcac_data_trans(modbusWRITE_MULTI_REG, dcacREG_ADDR_INIT, (uint8_t *)&s_tDcacInit, sizeof(s_tDcacInit) / 2) <= 0)
        return false;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 指令:并网功率设置
 * 说明(备注)  : none
 * 传入参数    : num:并网功率
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
bool b_dcac_cs_set_para_in_pwr(uint16_t us_pwr)
{
//	if(num)
//		tDcacParaInInit.usParaOutEn = dcacSWITCH_REG_ON;
//	else
//		tDcacParaInInit.usParaOutEn = dcacSWITCH_REG_OFF;
//	
//	tDcacParaInInit.usParaPwrSet = num;	//1W
//	
//	return b_dcac_write_multi_reg(dcacREG_ADDR_SET_PARA_IN,sizeof(tDcacParaInInit)/2,(u16*)&tDcacParaInInit);
	return false;
}

/***********************************************************************************************************************
 * 函数功能    : 指令:并网功率设置
 * 说明(备注)  : none
 * 传入参数    : num:并网功率
 * 输出参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
bool b_dcac_cs_sys_switch(uint16_t us_temp)
{
	// if(c_dcac_data_trans(modbusWRITE_SINGLE_REG, 
	// 					dcacREG_ADDR_DISCHG_SW, 
	// 					(u8*)&temp, 
	// 					1) <= 0)
	// 	return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : DCAC 数据传输底层实现 (Modbus)
 * 说明(备注)  : 无
 * 传入参数    : uc_cmd: 命令码, us_reg_addr: 寄存器地址, p_data: 发送数据, uc_len: 寄存器长度(字)
 * 输出参数    : 无
 * 返回值      : int8_t: 1-成功, 小于0-错误
 ************************************************************************************************************************/
static int8_t c_dcac_data_trans(uint8_t uc_cmd, uint16_t us_reg_addr, uint8_t *p_data, uint8_t uc_len)
{
    int8_t result = 0;

    if (tpDcacProtoTx == NULL)
        return 0;

    #if (boardUSE_OS)
    if (dcacSemaphoreMutex == NULL)
        return 0;
    if (xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
        return -99;

    while (ulTaskNotifyTake(pdTRUE, 0) > 0)
    {
    }

    /* 排空总线可能存在的迟到残包，防止前序超时响应串扰本次通信 */
    if (tpDcacProtoRx != NULL)
        cModbus_ResetRxBuff(tpDcacProtoRx);
    #endif  /* boardUSE_OS */

    #if (boardDCAC_IFACE)
    result = cModbus_ProtoCreate(tpDcacProtoTx, uc_cmd, us_reg_addr, p_data, uc_len);
    if (result > 0)
    {
        bDcacUseFlag = true;

        if (bDcac_DataSendStart(tpDcacProtoTx->ucaFrameData, tpDcacProtoTx->ucFrameLen) == true)
        {
            #if (boardUSE_OS)
            if (cModbus_WaitReply(tpDcacProtoTx, uc_cmd, us_reg_addr, dcacWAIT_NOTIFY_OUTTIME) <= 0)
            {
                if ((uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant) && tDcac.eDevState != DS_LOST)
                    log_w("bDcacTask:命令0x%x,寄存器%d等待回复超时", uc_cmd, us_reg_addr);
                result = -2;
            }
            #endif  /* boardUSE_OS */
        }
        else
            result = -3;
    }
    #endif  /* boardDCAC_IFACE */

    cModbus_ResetTx(tpDcacProtoTx, dcacTX_PROTO_BUFF_LEN);
    vTaskDelay(7);

    #if (boardUSE_OS)
    if (dcacSemaphoreMutex != NULL)
        xSemaphoreGive(dcacSemaphoreMutex);
    #endif  /* boardUSE_OS */

    return result;
}

/***********************************************************************************************************************
 * 函数功能    : DCAC Megmeet 升级协议初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bDcac_MegmeetProtInit(void)
{
    if (cMegmeet_ProtoSendInit(&tDcacMegmeetProtoTx, dcTASK_UPDATE_TX_FRAME_SIZE) < 0)
        return false;
    if (cMegmeet_ProtoRecInit(&tpDcacMegmeetProtoRx, dcTASK_UPDATE_RX_FRAME_SIZE) < 0)
        return false;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 根据升级对象获取从机地址
 * 说明(备注)  : 无
 * 传入参数    : e_obj: 升级对象
 * 输出参数    : 无
 * 返回值      : uint8_t: 从机地址
 ************************************************************************************************************************/
uint8_t ucDcac_GetUpdateSlaveAddr(ModuleObject_E e_obj)
{
    switch (e_obj)
    {
        case MO_MGMT_AC:
            return MEGMEET_IC_TYPE_AC;
        case MO_MGMT_DC:
            return MEGMEET_IC_TYPE_DC;
        case MO_DCAC:
            return dcacDEV_ADRR;
        default:
            return 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 根据升级对象获取芯片类型
 * 说明(备注)  : 无
 * 传入参数    : e_obj: 升级对象
 * 输出参数    : 无
 * 返回值      : uint8_t: IC 类型
 ************************************************************************************************************************/
uint8_t ucDcac_GetUpdateIcType(ModuleObject_E e_obj)
{
    switch (e_obj)
    {
        case MO_MGMT_AC:
            return MEGMEET_IC_TYPE_AC;
        case MO_MGMT_DC:
            return MEGMEET_IC_TYPE_DC;
        case MO_DCAC:
        default:
            return dcacUPDATE_IC_TYPE;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 构造并发送 Megmeet 协议帧
 * 说明(备注)  : 无
 * 传入参数    : uc_slave_addr: 从机地址, uc_ic_type: 芯片类型, uc_cmd: 命令码, p_payload: 数据, us_payload_len: 长度
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_send_megmeet_frame(uint8_t uc_slave_addr, uint8_t uc_ic_type, uint8_t uc_cmd, const uint8_t *p_payload, uint16_t us_payload_len)
{
    MegmeetProtoTx_t *tp_proto_tx = tDcacMegmeetProtoTx;

    if (tp_proto_tx == NULL)
        return false;

    if (cMegmeet_FrameCreate(uc_slave_addr, uc_ic_type, uc_cmd, p_payload, us_payload_len,
                             tp_proto_tx->ucaFrameData, tp_proto_tx->usBuffSize,
                             &tp_proto_tx->usFrameLen) <= 0)
    {
        return false;
    }

    bDcacUseFlag = true;
    return bDcac_DataSendStart(tp_proto_tx->ucaFrameData, tp_proto_tx->usFrameLen);
}

/***********************************************************************************************************************
 * 函数功能    : 发送 F0 请求升级帧
 * 说明(备注)  : 无
 * 传入参数    : uc_payload: 载荷
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_send_f0(uint8_t uc_payload)
{
    bool b_send_ok = b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj),
                                               MEGMEET_CMD_REQ_UPDATE, &uc_payload, 1);
    if (b_send_ok)
        vUpdate_ResetRecTimeout(true);
    return b_send_ok;
}

/***********************************************************************************************************************
 * 函数功能    : 发送 F6 跳转 Boot 帧
 * 说明(备注)  : 无
 * 传入参数    : b_reset_timeout: 是否重置超时
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_send_f6(bool b_reset_timeout)
{
    bool b_send_ok = b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj),
                                               MEGMEET_CMD_JUMP_BOOT, NULL, 0);
    if (b_send_ok && b_reset_timeout)
        vUpdate_ResetRecTimeout(true);
    return b_send_ok;
}

/***********************************************************************************************************************
 * 函数功能    : 发送 F2 设置波特率帧
 * 说明(备注)  : 无
 * 传入参数    : ul_baud: 波特率, b_reset_timeout: 是否重置超时
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_send_f2(uint32_t ul_baud, bool b_reset_timeout)
{
    uint8_t uc_payload = 0;

    if (ul_baud == 9600)
        uc_payload = 0x00;
    else if (ul_baud == 115200)
        uc_payload = 0x01;
    else
    {
        bUpdate_SetErrCode(UEF_DP_F2_INVALID_BAUD);
        return false;
    }

    bool b_send_ok = b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj),
                                               MEGMEET_CMD_SET_BAUD, &uc_payload, 1);
    if (b_send_ok && b_reset_timeout)
        vUpdate_ResetRecTimeout(true);
    return b_send_ok;
}

/***********************************************************************************************************************
 * 函数功能    : 发送升级固件数据帧
 * 说明(备注)  : 无
 * 传入参数    : uc_cmd: 命令码, p_payload: 数据, us_payload_len: 长度, b_reset_timeout: 是否重置超时
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool b_dcac_cs_send_fw_data(uint8_t uc_cmd, const uint8_t *p_payload, uint16_t us_payload_len, bool b_reset_timeout)
{
    bool b_send_ok = b_dcac_send_megmeet_frame(0, ucDcac_GetUpdateIcType(tUpdate.eObj),
                                               uc_cmd, p_payload, us_payload_len);
    if (b_send_ok && b_reset_timeout)
        vUpdate_ResetRecTimeout(true);
    return b_send_ok;
}

#endif  /* boardDCAC_EN */

