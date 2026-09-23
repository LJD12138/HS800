/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 电池管理任务、开关控制与记忆参数管理实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_queue_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			BMS_TASK_PRIO							3		/* 任务优先级(通信执行层) */
#define			BMS_TASK_SIZE							256		/* 任务堆栈大小 */
TaskHandle_t tBmsTaskHandler = NULL;
void         vBms_Task(void *pvParameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Bms_T tBms;
static Task_T *s_tpTask = NULL;

/* BMS 记忆参数步进配置表 */
typedef struct
{
	void				*pParam;
	int32_t				lMin;
	int32_t				lMax;
	uint8_t				ucType;				/* 0: uint16_t, 1: int8_t */
}BmsParamStep_T;

static const BmsParamStep_T s_tBmsParamTable[] = 
{
    {(void *)&tAppMemParam.tBMS.cChgMaxTemp,    -127,  127,   1},
    {(void *)&tAppMemParam.tBMS.cDisChgMaxTemp, -127,  127,   1},
    {(void *)&tAppMemParam.tBMS.cChgMinTemp,    -127,  127,   1},
    {(void *)&tAppMemParam.tBMS.cDisChgMinTemp, -127,  127,   1},
    {(void *)&tAppMemParam.tBMS.usMaxVolt,         0,  60000, 0},
    {(void *)&tAppMemParam.tBMS.usMinVolt,         0,  60000, 0},
    {(void *)&tAppMemParam.tBMS.usChgVolt,         0,  60000, 0},
};

//****************************************************Function Declaration******************************************************//
static bool b_bms_task_param_init(void);


/***********************************************************************************************************************
 * 函数功能    : 电池包任务参数初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
static bool b_bms_task_param_init(void)
{
    if (tpBmsTask == NULL)
        return false;

    memset(&tBms, 0, sizeof(tBms));
    lwrb_reset(&tpBmsTask->tQueueBuff);
    s_tpTask = tpBmsTask;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 电池包任务参数初始化（对外封装）
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bBms_TaskParamInit(void)
{
    return b_bms_task_param_init();
}

/***********************************************************************************************************************
 * 函数功能    : 电池包任务初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 发送协议初始化失败, -2: 队列初始化失败, -3: 参数初始化失败, -4: 任务创建失败
 ************************************************************************************************************************/
s8 cBms_TaskInit(void)
{
    if (bBms_SendProtInit() == false)
        return -1;

    if (bBms_QueueInit() == false)
        return -2;

    if (b_bms_task_param_init() == false)
        return -3;

    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vBms_Task,
                    (const char*    )"BmsTask",
                    (uint16_t       )BMS_TASK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )BMS_TASK_PRIO,
                    (TaskHandle_t*  )&tBmsTaskHandler) != pdPASS)
        return -4;
    vQueue_BindTaskHandler(tpBmsTask, tBmsTaskHandler);
    #endif  /* boardUSE_OS */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 电池包任务主循环
 * 说明(备注)  : 无
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_Task(void *pvParameters)
{
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        if (s_tpTask == NULL)
        {
            b_bms_task_param_init();

            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

        vQueue_TaskPoll(s_tpTask, bmsTASK_CYCLE_TIME);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 获取电池包的充电状态
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-充电, false-放电
 ************************************************************************************************************************/
bool bBms_GetBmsChgState(void)
{
    return cSys_IsChgState();
}

/***********************************************************************************************************************
 * 函数功能    : 设置设备状态
 * 说明(备注)  : 无
 * 传入参数    : state: 目标状态
 * 输出参数    : 无
 * 返回值      : bool: true-成功
 ************************************************************************************************************************/
bool bBms_SetDevState(DevState_E state)
{
    if (tBms.eDevState != state)
    {
        if (state == DS_LOST)
        {
            cQueue_AddQueueTask(tpBmsTask, BTI_NULL, 0, false);

            #if (boardSYS_DATA_UPADATA)
            STAT_CLR(tSysInfo.Mod_Exist, OL_BMS);
            #endif  /* boardSYS_DATA_UPADATA */
        }
        else
        {
            #if (boardSYS_DATA_UPADATA)
            STAT_SET(tSysInfo.Mod_Exist, OL_BMS);
            #endif  /* boardSYS_DATA_UPADATA */
        }
    }

    tBms.eDevState = state;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置设备错误代码
 * 说明(备注)  : 无
 * 传入参数    : code: 错误码, set: true-设置, false-清除
 * 输出参数    : 无
 * 返回值      : bool: true-存在错误, false-无错误
 ************************************************************************************************************************/
bool bBms_SetErrCode(BmsErrCode_E code, bool set)
{
    static BmsErrCode_E s_e_next_code;
    static bool         s_b_next_set;

    /* 第一次连接/退出丢失处理 */
    if (code == BEC_SYS_DEV_LOST)
    {
        if (set == false && tBms.uErrCode.tCode.bSysDevLost == 0)
        {
            b_bms_task_param_init();
            bBms_SetDevState(DS_SHUT_DOWN);
            return false;
        }
    }

    if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
    {
        if (s_e_next_code != code || s_b_next_set != set)
        {
            log_e("bBmsTask:任务错误 代码%d 类型%d", code, set);
            s_e_next_code = code;
            s_b_next_set  = set;
        }
    }

    if (code > BEC_CLEAR_ALL)
    {
        if (code == BEC_BMS_ERR)
        {
            /* 临界区保护:64位错误码读改写,多任务并发调用时防止丢失更新 */
            mainENTER_CRITICAL();
            tBms.uErrCode.ullCode &= 0xFFFFFFFF00000000ULL;
            if (set == true)
                tBms.uErrCode.ullCode |= ulBmsRxErrCode;
            mainEXIT_CRITICAL();

            if (set == true)
            {
                if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
                    log_e("bBmsTask:设备上报错误,代码:0x%x", ulBmsRxErrCode);
            }
        }
        else
        {
            if (code == BEC_SYS_DEV_LOST)
            {
                mainENTER_CRITICAL();
                tBms.uErrCode.ullCode = 0;
                ulBmsRxErrCode = 0;
                mainEXIT_CRITICAL();

                if (set)
                {
                    b_bms_task_param_init();

                    mainENTER_CRITICAL();
                    ERR_SET(tBms.uErrCode.ullCode, (code - 2));
                    mainEXIT_CRITICAL();

                    if (uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
                        log_e("bBmsTask:任务错误_BMS模块丢失");

                    bBms_SetPerm(BPO_ALL, false);
                }
                else
                    bBms_SetDevState(DS_SHUT_DOWN);
            }
            else
            {
                mainENTER_CRITICAL();
                if (set)
                    ERR_SET(tBms.uErrCode.ullCode, (code - 2));
                else
                    ERR_CLR(tBms.uErrCode.ullCode, (code - 2));
                mainEXIT_CRITICAL();
            }
        }
    }
    else
    {
        mainENTER_CRITICAL();
        tBms.uErrCode.ullCode = 0;
        ulBmsRxErrCode        = 0;
        mainEXIT_CRITICAL();
    }

    if (tBms.uErrCode.ullCode)
    {
        if (set == true)
        {
            #if (boardBUZ_EN)
            bBuz_Tweet(LONG_3);
            #endif  /* boardBUZ_EN */
        }

        cQueue_AddQueueTask(tpBmsTask, BTI_ERR_PROCESS, code, false);
    }
    else
    {
        if (tBms.eDevState == DS_ERR)
            bBms_SetDevState(DS_WORK);
    }

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 开关 BMS
 * 说明(备注)  : 无
 * 传入参数    : obj: 操作对象, type: 开关类型, fore_en: 强制执行
 * 输出参数    : 无
 * 返回值      : s8: 0-维持现状, 1-成功
 ************************************************************************************************************************/
s8 cBms_Switch(SwitchObject_E obj, SwitchType_E type, bool fore_en)
{
    TaskInParam_U u_param;
    bool b_turn_on = false;

    u_param.tTaskParam.ucObj = obj;

    if (type == ST_ON)
    {
        if ((tBms.eDevState == DS_WORK || tBms.eDevState == DS_BOOTING) && fore_en == false)
        {
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:当前状态为工作,不允许开机.对象:%d \r\n", obj);
            return 0;
        }
        b_turn_on = true;
    }
    else if (type == ST_OFF)
    {
        if ((tBms.eDevState == DS_SHUT_DOWN || tBms.eDevState == DS_CLOSING) && fore_en == false)
        {
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:当前状态为关闭,不允许关机.对象:%d \r\n", obj);
            return 0;
        }
        b_turn_on = false;
    }
    else
        b_turn_on = (tBms.eDevState == DS_SHUT_DOWN || tBms.eDevState == DS_CLOSING);

    u_param.tTaskParam.ucParam = b_turn_on ? ST_ON : ST_OFF;
    cQueue_AddQueueTask(tpBmsTask, BTI_CTRL_BMS_SW, u_param.usTaskInParam, fore_en);

    #if (boardSYS_DATA_UPADATA)
    Sys_Update_Mod(BMS_Mod, true);
    #endif  /* boardSYS_DATA_UPADATA */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 获取 SOC
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : u8: SOC (0~100)
 ************************************************************************************************************************/
u8 ucBms_GetSoc(void)
{
    return (u8)tBmsRx.usSOC;
}

/***********************************************************************************************************************
 * 函数功能    : 获取 BMS 许可的最大充电功率
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : u16: 许可的最大充电功率 (W), 0 表示不允许充电
 ************************************************************************************************************************/
u16 usBms_GetPermMaxChgPwr(void)
{
    return tBmsRx.usPermMaxChgPwr;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化参数
 * 说明(备注)  : 无
 * 传入参数    : p_bms_mem: BMS 记忆参数结构体指针
 * 输出参数    : 无
 * 返回值      : bool: true-成功
 ************************************************************************************************************************/
bool bBms_MemParamInit(BmsMemParam_T *p_bms_mem)
{
    p_bms_mem->cChgMaxTemp    = boardBMS_CHG_MAX_TEMP;
    p_bms_mem->cDisChgMaxTemp = boardBMS_DISCHG_MAX_TEMP;
    p_bms_mem->cChgMinTemp    = boardBMS_CHG_MIN_TEMP;
    p_bms_mem->cDisChgMinTemp = boardBMS_DISCHG_MIN_TEMP;
    p_bms_mem->usMaxVolt      = boardBMS_MAX_VOLT;
    p_bms_mem->usMinVolt      = boardBMS_MIN_VOLT;
    p_bms_mem->usChgVolt      = boardBMS_CHG_VOLT;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置记忆参数 (表驱动精简版)
 * 说明(备注)  : 无
 * 传入参数    : item: 参数索引, add: true-增加, false-减少
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBms_MemParamSet(u8 item, bool add)
{
    if (item >= mainARRAY_SIZE(s_tBmsParamTable))
        return;

    const BmsParamStep_T *p = &s_tBmsParamTable[item];
    if (p->ucType == 1)
    {
        int8_t *p_val = (int8_t *)p->pParam;
        if (add && *p_val < (int8_t)p->lMax)
            (*p_val)++;
        else if (!add && *p_val > (int8_t)p->lMin)
            (*p_val)--;
    }
    else
    {
        uint16_t *p_val = (uint16_t *)p->pParam;
        if (add && *p_val < (uint16_t)p->lMax)
            (*p_val)++;
        else if (!add && *p_val > (uint16_t)p->lMin)
            (*p_val)--;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 设置充放电许可
 * 说明(备注)  : 无
 * 传入参数    : obj: 许可对象 (BPO_CHG, BPO_DISCHG, BPO_ALL), en: true-允许, false-禁止
 * 输出参数    : 无
 * 返回值      : bool: true-设置成功, false-失败
 ************************************************************************************************************************/
bool bBms_SetPerm(BmsPermObject_E obj, bool en)
{
    switch (obj)
    {
        case BPO_CHG:
        {
            if (en != tBms.uPerm.tPerm.bChgPerm)
            {
                if (uPrint.tFlag.bBmsTask)
                    sMyPrint("bBmsTask:充电许可 设置=%d 当前状态=%d \r\n", en, tBms.uPerm.tPerm.bChgPerm);
                tBms.uPerm.tPerm.bChgPerm = en;
            }
        }
        break;

        case BPO_DISCHG:
        {
            if (en != tBms.uPerm.tPerm.bDisChgPerm)
            {
                if (uPrint.tFlag.bBmsTask)
                    sMyPrint("bBmsTask:放电许可 设置=%d 当前状态=%d \r\n", en, tBms.uPerm.tPerm.bDisChgPerm);
                tBms.uPerm.tPerm.bDisChgPerm = en;
            }
        }
        break;

        case BPO_ALL:
        {
            if (en != tBms.uPerm.tPerm.bChgPerm)
            {
                if (uPrint.tFlag.bBmsTask)
                    sMyPrint("bBmsTask:充电许可 设置=%d 当前状态=%d \r\n", en, tBms.uPerm.tPerm.bChgPerm);
                tBms.uPerm.tPerm.bChgPerm = en;
            }

            if (en != tBms.uPerm.tPerm.bDisChgPerm)
            {
                if (uPrint.tFlag.bBmsTask)
                    sMyPrint("bBmsTask:放电许可 设置=%d 当前状态=%d \r\n", en, tBms.uPerm.tPerm.bDisChgPerm);
                tBms.uPerm.tPerm.bDisChgPerm = en;
            }
        }
        break;

        default:
            return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 检查充放电许可
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : s8: 1
 ************************************************************************************************************************/
s8 cBms_CheckPerm(void)
{
    /* 放电许可判定 */
    if (tBmsRx.tState.bImpermDisChg == 1 ||
        tBms.uErrCode.tCode.bSysDisChgUT == 1 ||
        tBms.uErrCode.tCode.bSysDisChgOT == 1 ||
        ucBms_GetSoc() == 0)
    {
        if (tBms.uPerm.tPerm.bDisChgPerm == true)
            bBms_SetPerm(BPO_DISCHG, false);
    }
    else
    {
        if (tBms.uPerm.tPerm.bDisChgPerm == false)
            bBms_SetPerm(BPO_DISCHG, true);
    }

    /* 充电许可判定 */
    if (tBmsRx.tState.bPermChg == 0 ||
        tBms.uErrCode.tCode.bSysChgUT == 1 ||
        tBms.uErrCode.tCode.bSysChgOT == 1 ||
        ucBms_GetSoc() == 100)
    {
        if (tBms.uPerm.tPerm.bChgPerm == true)
            bBms_SetPerm(BPO_CHG, false);
    }
    else
    {
        if (tBms.uPerm.tPerm.bChgPerm == false)
            bBms_SetPerm(BPO_CHG, true);
    }

    return 1;
}

#if (boardUPDATE)
/***********************************************************************************************************************
 * 函数功能    : 获取 BMS 升级阶段
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : s8: 升级阶段状态码
 ************************************************************************************************************************/
s8 cBms_GetUpdateStage(void)
{
    if (tSysInfo.eDevState != DS_UPDATE_MODE ||
        tUpdate.eObj != MO_BMS ||
        tBms.eDevState != DS_UPDATE_MODE)
    {
        return -1;
    }

    if (tpBmsTask == NULL)
        return -2;

    if (tpBmsTask->ucID != BTI_UPDATE)
        return -3;

    if (tpBmsTask->ucStep == BMS_UPDATE_STEP_ERROR_CLEANUP)
        return UPDATE_QUEUE_STAGE_ERR;

    if (tpBmsTask->ucStep == BMS_UPDATE_STEP_END)
        return UPDATE_QUEUE_STAGE_WAIT_RESTART;

    if (tpBmsTask->ucStep > BMS_UPDATE_STEP_FINISH_CLEANUP)
        return UPDATE_QUEUE_STAGE_FINISH;

    return UPDATE_QUEUE_STAGE_RUNNING;
}
#endif  /* boardUPDATE */

#endif  /* boardBMS_EN */

