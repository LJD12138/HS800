/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\MD_Bms
 * File    : md_bms_rec_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 接收任务调度与通信超时管理实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_rec_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_rec_data_proc.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_iface.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Bms/md_bms_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			bmsREC_TASK_PRIO						3		/* 任务优先级(通信接收层:保障数据新鲜度) */
#define			bmsREC_TASK_SIZE						256		/* 任务堆栈大小 (字) */
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
#if (boardUSE_OS)
TaskHandle_t tBmsRecTaskHandle;
void vBms_RecTask(void *pvParameters);
#endif  /* boardUSE_OS */

/* BMS 接收全局运行对象 (4 字节自然对齐) */
__ALIGNED(4) BmsRx_T tBmsRx;
vu32 ulBmsRxErrCode = 0;

//****************************************************Function Declaration******************************************************//
static uint8_t c_check_conn_state(void);
static void v_rec_task_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : 复位 BMS 接收参数缓存
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_rec_task_param_init(void)
{
    memset(&tBmsRx, 0, sizeof(tBmsRx));

    vQueue_ResetReply(tpBmsTask);
    
    tBms.sMaxTemp = 25;
    tBms.sMinTemp = 25;
    
    cBaiku_ResetRxBuff(tpBmsProtoRx);
}

/***********************************************************************************************************************
 * 函数功能    : BMS 接收任务初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 协议初始化失败, -2: 任务创建失败
 ************************************************************************************************************************/
s8 cBms_RecTaskInit(void)
{
    /* 接口初始化 */
    #if (boardBMS_EN)
    vBms_IfaceInit();
    #endif  /* boardBMS_EN */
    
    /* 接收协议初始化 */
    if (bBms_RecProtInit() == false)
        return -1;
        
    /* 任务参数初始化 */
    v_rec_task_param_init();
    
    /* 数据解析任务初始化 */
    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vBms_RecTask,       /* 任务函数 */
                    (const char*    )"BmsRecTask",       /* 任务名称 */
                    (uint16_t       )bmsREC_TASK_SIZE,   /* 任务堆栈大小 */
                    (void*          )NULL,               /* 传递给任务函数的参数 */
                    (UBaseType_t    )bmsREC_TASK_PRIO,   /* 任务优先级 */
                    (TaskHandle_t*  )&tBmsRecTaskHandle) != pdPASS)/* 任务句柄 */
        return -2;
    #endif  /* boardUSE_OS */
                
    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : BMS 接收处理任务主循环
 * 说明(备注)  : 无
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_RecTask(void *pvParameters)
{
    s8 c_result = 0;
    
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        if (tpBmsProtoRx == NULL)
        {
            bBms_RecProtInit();
            
            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

		/* 协议解析 */
		c_result = cBaiku_ProtoCheck(tpBmsProtoRx);

		/* 数据处理 */
		if (c_result > 0)
        {
			c_check_conn_state();
			c_result = c_bms_rec_proc_data(tpBmsProtoRx);
			if (c_result <= 0)
			{
				if (uPrint.tFlag.bBmsRecTask || uPrint.tFlag.bImportant)
					log_w("bBmsRecTask:装载的数据错误,代码%d", c_result);
			}
			else
			{
				#if (boardUSE_OS)
				xTaskNotifyGive(tBmsTaskHandler); /* 通知发送任务 */
				#endif  /* boardUSE_OS */
			}
        }
		else 
		{
			if (c_result == 0)
			{
				#if (boardUSE_OS)
				ulTaskNotifyTake(pdFALSE, portMAX_DELAY); /* 等待任务通知 */
				#endif  /* boardUSE_OS */
			}
			else 
			{
				if (uPrint.tFlag.bBmsRecTask || uPrint.tFlag.bImportant)
					log_w("bBmsRecTask:协议解析错误,代码%d", c_result);
			}
		}
    }
}

/***********************************************************************************************************************
 * 函数功能    : 检测 BMS 设备的连接状态
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : uint8_t: 0-正常
 ************************************************************************************************************************/
static uint8_t c_check_conn_state(void)    
{
    /* 丢失后第一次连接 */
    if (tBms.eDevState == DS_LOST || tBms.uErrCode.tCode.bSysDevLost == true)
    {
        #if (boardSYS_DATA_UPADATA)
        if (!BIT_GET(tSysInfo.Mod_Exist, OL_BMS)) /* 第一次初始化 */
        {
            STAT_SET(tSysInfo.Mod_Exist, OL_BMS);
            Sys_Update_Element(AT_SYS_MODEXIST_ADDR, NULL, tSysInfo.Mod_Exist, true);
        }
        #endif  /* boardSYS_DATA_UPADATA */
    }
    
    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : BMS 接收定时器滴答处理
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_RecTickTimer(void)
{
    if (tpBmsProtoRx == NULL)
        return;
    
    /* 数据帧接收超时计算 */
    if (tpBmsProtoRx->usRecOverTimeCnt > 0)
    {    
        tpBmsProtoRx->usRecOverTimeCnt--;
    
        if (tpBmsProtoRx->usRecOverTimeCnt == 0)        
            cBaiku_StepWaitOutTime(tpBmsProtoRx);
    }
    
    /* BMS模块连接超时计算 */        
    if (tpBmsProtoRx->usLostOverTimeCnt > 0)
    {    
        tpBmsProtoRx->usLostOverTimeCnt--;
    
        if (tpBmsProtoRx->usLostOverTimeCnt == 0) /* 丢失 */    
        {
            v_rec_task_param_init();
        }
    }
}

#endif  /* boardBMS_EN */
