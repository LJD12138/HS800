/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_prot_frame.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 通信协议帧封装与发送实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_prot_frame.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			bmsTX_PROTO_BUFF_LEN					256
#define			bmsRX_PROTO_BUFF_LEN					256

#define			bmsDEV_ADRR								0x10
#define			bmsWAIT_NOTIFY_OUTTIME					400		/* 任务通知超时时间 MS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpBmsProtoTx = NULL;                   /* 发送协议 */
__ALIGNED(4) BaikuProtoRx_t *tpBmsProtoRx = NULL;

#if (boardUSE_OS)
static SemaphoreHandle_t s_bmsSemaphoreMutex = NULL;                /* 互斥信号量 */
#endif  /* boardUSE_OS */

//****************************************************Function Declaration******************************************************//
static s8 c_bms_data_trans(uint8_t uc_cmd, uint8_t *p_data, uint8_t uc_len);


/***********************************************************************************************************************
 * 函数功能    : BMS 发送协议初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bBms_SendProtInit(void)
{
    s8 c_result = cBaiku_ProtoSendInit(&tpBmsProtoTx, 
                                bmsTX_PROTO_BUFF_LEN, 
                                bmsDEV_ADRR);
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
            log_e("bBmsTask:tpBmsProtoTx协议对象初始化失败,代码%d", c_result);
        
        return false;
    }

    /* 创建互斥信号量 */
    #if (boardUSE_OS)
    s_bmsSemaphoreMutex = xSemaphoreCreateMutex();
    #endif  /* boardUSE_OS */
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : BMS 接收协议初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bBms_RecProtInit(void)
{
    s8 c_result = cBaiku_ProtoRecInit(&tpBmsProtoRx,         /* 协议指针 */
                                bmsRX_PROTO_BUFF_LEN,        /* 协议缓存器大小 */
                                sysDEV_ADRR,                 /* 协议设备ID */
                                boardREPET_TIMER_CYCLE_TMIE);/* 计数器采样时间 */
    if (c_result <= 0)
    {
        if (uPrint.tFlag.bBmsRecTask || uPrint.tFlag.bImportant)
            log_e("bBmsRecTask:tpBmsProtoRx协议对象初始化失败,代码%d", c_result);
        return false;
    }
    
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 指令: 获取参数
 * 说明(备注)  : 无
 * 传入参数    : uc_num: 参数编号
 * 输出参数    : 无
 * 返回值      : s8: 1-成功, <=0-失败
 ************************************************************************************************************************/
s8 c_bms_cs_get_param(uint8_t uc_num)
{
    return c_bms_data_trans(baikuCMD_GET_PARAM, &uc_num, 1);
}

/***********************************************************************************************************************
 * 函数功能    : 指令: 开关BMS
 * 说明(备注)  : 无
 * 传入参数    : u_in_param: 开关参数
 * 输出参数    : 无
 * 返回值      : s8: 1-成功, <=0-失败
 ************************************************************************************************************************/
s8 c_bms_cs_switch(TaskInParam_U u_in_param)
{
    return c_bms_data_trans(baikuCMD_SWITCH, (uint8_t *)&u_in_param.usTaskInParam, 2);
}

/***********************************************************************************************************************
 * 函数功能    : 指令: 通知BMS主控在升级
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 执行结果
 ************************************************************************************************************************/
s8 c_bms_cs_send_update(void)
{
	u8 data = 0;
	return c_bms_data_trans(baikuCMD_COMSOLE_UPDATE, &data, 1);
}

/***********************************************************************************************************************
 * 函数功能    : 数据传输
 * 说明(备注)  : 无
 * 传入参数    : uc_cmd: 指令, p_data: 指向数据指针, uc_len: 数据的长度
 * 输出参数    : 无
 * 返回值      : s8: -1-写入Len超出最大长度, -2-等待回复超时, -3-数据发送错误, 0-无操作, 1-操作成功
 ************************************************************************************************************************/
static s8 c_bms_data_trans(uint8_t uc_cmd, uint8_t *p_data, uint8_t uc_len)
{
    s8 result = 0;
    
    if (tpBmsProtoTx == NULL)
        return 0;

    #if (boardUSE_OS)
    /* 检查互斥锁是否已创建，并获取互斥锁保护共享资源（最多等待1秒） */
    if (s_bmsSemaphoreMutex == NULL)
        return 0;
    if (xSemaphoreTake(s_bmsSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
        return -99;

	/* 清除任务通知，避免历史通知干扰本次通信 */
	while (ulTaskNotifyTake(pdTRUE, 0) > 0) {}

    /* 排空总线可能存在的迟到残包，防止前序超时响应串扰本次通信 */
    if (tpBmsProtoRx != NULL)
        cBaiku_ResetRxBuff(tpBmsProtoRx);
    #endif  /* boardUSE_OS */
    
    result = cBaiku_ProtoCreate(tpBmsProtoTx, uc_cmd, p_data, uc_len);
    if (result > 0)
    {
        bBmsUseFlag = true;

        if (bBms_DataSendStart(tpBmsProtoTx->ucaFrameData, tpBmsProtoTx->ucFrameLen) == true)
        {
            /* 等待任务通知,等待时间为 400ms */
            #if (boardUSE_OS)
            if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(bmsWAIT_NOTIFY_OUTTIME)) <= 0)
            {
                if (uPrint.tFlag.bBmsTask)
                    log_w("bBmsTask:等待指令0x%x回复超时", uc_cmd);
                
                result = -2;
            }
            #endif  /* boardUSE_OS */
        }
        else
            result = -3;
    }

    /* 释放互斥量 */
    #if (boardUSE_OS)
    vTaskDelay(2);
    xSemaphoreGive(s_bmsSemaphoreMutex);
    #endif  /* boardUSE_OS */

    return result;
}

#endif  /* boardBMS_EN */
