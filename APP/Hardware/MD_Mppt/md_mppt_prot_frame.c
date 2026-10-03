/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_prot_frame.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 通信协议帧封包与解析函数实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_prot_frame.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_queue_task.h"
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"
#include "check.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  /* boardDCAC_EN */

//****************************************************Macros********************************************************************//
#define			mpptTX_PROTO_BUFF_LEN					128
#define			mpptRX_PROTO_BUFF_LEN					256

/* MPPT设备地址 */
#define			mpptDEV_ADRR							0x01
#define			mpptWAIT_NOTIFY_OUTTIME					500		/* 任务通知超时时间 MS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) ModbusProtoTx_t *tpMpptProtoTx = NULL;  /* 发送协议 */
__ALIGNED(4) ModbusProtoRx_t *tpMpptProtoRx = NULL;  /* 接收协议 */

//****************************************************Function Declaration******************************************************//
static s8 c_mppt_data_trans(u8 cmd, u16 reg_addr, u8 *data, u8 len);

/***********************************************************************************************************************
 * 函数功能    : MPPT发送协议初始化
 * 说明(备注)  : 初始化Modbus发送协议对象
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bMppt_SendProtInit(void)
{
    s8 c_result = 1;

    c_result = cModbus_TransProtoInit(&tpMpptProtoTx, mpptTX_PROTO_BUFF_LEN, mpptDEV_ADRR);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
            log_e("bMpptTask:tpMpptProtoTx协议对象初始化失败,代码%d", c_result);
        
        return false;
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : MPPT接收协议初始化
 * 说明(备注)  : 初始化Modbus接收协议对象
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bMppt_RecProtInit(void)
{
    s8 c_result = cModbus_RecProtoInit(&tpMpptProtoRx,
                                       mpptRX_PROTO_BUFF_LEN,
                                       mpptDEV_ADRR,
                                       boardREPET_TIMER_CYCLE_TMIE);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bMpptRecTask || uPrint.tFlag.bImportant)
            log_e("bMpptRecTask:tpMpptProtoRx协议对象初始化失败,代码%d", c_result);
        return false;
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 指令:获取参数
 * 说明(备注)  : 读取MPPT多寄存器基础参数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 通信执行结果
 ************************************************************************************************************************/
s8 c_mppt_cs_get_param(void)
{
    MpptParam_T t_param;
    
    return c_mppt_data_trans(modbusREAD_MULTI_REG, 
                             mpptREG_ADDR_GET_PARAM1, 
                             NULL, 
                             sizeof(t_param) / 2);
}

/***********************************************************************************************************************
 * 函数功能    : 指令:设置充电功率
 * 说明(备注)  : 向MPPT写入单寄存器充电功率
 * 传入参数    : pwr: 功率值 (W)
 * 输出参数    : 无
 * 返回值      : 通信执行结果
 ************************************************************************************************************************/
s8 c_mppt_cs_set_pwr(u16 pwr)
{
    return c_mppt_data_trans(modbusWRITE_SINGLE_REG, 
                             mpptREG_ADDR_SET_PV_CHG_PWR, 
                             (u8*)&pwr, 
                             1);
}

/***********************************************************************************************************************
 * 函数功能    : 发送通信数据
 * 说明(备注)  : none
 * 传入参数    : cmd: 指令, reg_addr: 寄存器地址, data: 数据, len: 长度
 * 输出参数    : none
 * 返回值      : 1: 发送成功, -1: 协议打包失败, -2: 回复超时, -3: 串口发送失败
 ************************************************************************************************************************/
static s8 c_mppt_data_trans(u8 cmd, u16 reg_addr, u8 *data, u8 len)
{
    s8 result = 0;
    
    if (tpMpptProtoTx == NULL)
        return 0;

    #if (boardUSE_OS)
    /* 检查互斥锁是否已创建，并获取互斥锁保护共享资源（最多等待1秒） */
    if (dcacSemaphoreMutex == NULL)
        return 0;
    if (xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
        return -99;

    /* 清除任务通知，避免历史通知干扰本次通信 */
    while (ulTaskNotifyTake(pdTRUE, 0) > 0)
    {
    }

    /* 排空总线可能存在的迟到残包，防止前序超时响应串扰本次通信 */
    if (tpMpptProtoRx != NULL)
        cModbus_ResetRxBuff(tpMpptProtoRx);
    #endif  /* boardUSE_OS */

    #if (boardDCAC_IFACE)
    result = cModbus_ProtoCreate(tpMpptProtoTx, cmd, reg_addr, data, len);
    if (result > 0)
    {
        /* MPPT占用DCAC接口 */
        bDcacUseFlag = false;

        /* 通过DCAC串口发送MPPT数据 */
        if (bDcac_DataSendStart(tpMpptProtoTx->ucaFrameData, tpMpptProtoTx->ucFrameLen))
        {
            /* 等待接收任务通知，表示收到MPPT设备的回复 */
            #if (boardUSE_OS)
            if (cModbus_WaitReply(tpMpptProtoTx, cmd, reg_addr, mpptWAIT_NOTIFY_OUTTIME) <= 0)
            {
                if ((uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant) && tMppt.eDevState != DS_LOST)
                    log_w("bMpptTask:命令0x%x,寄存器%d等待回复超时", cmd, reg_addr);
                
                result = -2;
            }
            #endif  /* boardUSE_OS */
        }
        else
            result = -3;
    }
    #endif  /* boardDCAC_IFACE */

    cModbus_ResetTx(tpMpptProtoTx, mpptTX_PROTO_BUFF_LEN);

    vTaskDelay(7);

    #if (boardUSE_OS)
    if (dcacSemaphoreMutex != NULL)
        xSemaphoreGive(dcacSemaphoreMutex);
    #endif  /* boardUSE_OS */

    return result;
}

#endif  /* boardMPPT_EN */
