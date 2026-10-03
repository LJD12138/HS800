/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Dcac
 * File    : md_dcac_task.c
 * Date    : 2026-09-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 逆变器业务控制与状态管理任务
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Dcac/md_dcac_task.h"

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_queue_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			DCAC_TASK_PRIO							3		/* 任务优先级 */
#define			DCAC_TASK_SIZE							256		/* 任务堆栈(字) */
TaskHandle_t tDcacTaskHandler = NULL;							/* 任务句柄 */
void vDcac_Task(void *pvParameters);							/* 任务函数 */
#endif  /* boardUSE_OS */


//****************************************************Parameter Initialization**************************************************//
Dcac_T tDcac;
static Task_T *s_tpDcacTask = NULL;

/* 表驱动参数步进配置 */
typedef struct
{
	volatile void		*pVal;
	bool				bIsSigned8;
	int32_t				slMin;
	int32_t				slMax;
}DcacParamStep_T;

static const DcacParamStep_T s_tDcacParamStepTbl[] =
{
    [0]  = { (volatile void *)&tAppMemParam.tDCAC.usAutoOffTime,   false, 0,    3600 },
    [1]  = { (volatile void *)&tAppMemParam.tDCAC.usMinOpenVolt,   false, 0,    65535 },
    [2]  = { (volatile void *)&tAppMemParam.tDCAC.usVoltRating,    false, 0,    65535 },
    [3]  = { (volatile void *)&tAppMemParam.tDCAC.usMaxInVolt,     false, 0,    65535 },
    [4]  = { (volatile void *)&tAppMemParam.tDCAC.usMinInVolt,     false, 0,    65535 },
    [5]  = { (volatile void *)&tAppMemParam.tDCAC.usInPwrRating,   false, 0,    65535 },
    [6]  = { (volatile void *)&tAppMemParam.tDCAC.usMinInPwr,      false, 0,    65535 },
    [7]  = { (volatile void *)&tAppMemParam.tDCAC.usMaxInCurr,     false, 0,    65535 },
    [8]  = { (volatile void *)&tAppMemParam.tDCAC.usOutPwrRating,  false, 0,    65535 },
    [9]  = { (volatile void *)&tAppMemParam.tDCAC.usOverLoadPwr,   false, 0,    65535 },
    [10] = { (volatile void *)&tAppMemParam.tDCAC.usParaInPwr,     false, 0,    65535 },
    [11] = { (volatile void *)&tAppMemParam.tDCAC.usAcOutFreq,     false, 0,    65535 },
    [12] = { (volatile void *)&tAppMemParam.tDCAC.sMaxTemp,        true,  -127, 127 },
};

#define			DCAC_PARAM_STEP_TBL_NUM					(sizeof(s_tDcacParamStepTbl) / sizeof(s_tDcacParamStepTbl[0]))

//****************************************************Function Declaration******************************************************//
static bool b_dcac_task_param_init(void);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 逆变参数初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
static bool b_dcac_task_param_init(void)
{
    if (tpDcacTask == NULL)
        return false;

    memset(&tDcac, 0, sizeof(tDcac));
    lwrb_reset(&tpDcacTask->tQueueBuff);
    s_tpDcacTask = tpDcacTask;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 逆变任务初始化
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 发送协议初始化失败, -2: 队列初始化失败, -3: 参数初始化失败, -4: 任务创建失败
 ************************************************************************************************************************/
s8 cDcac_TaskInit(void)
{
    if (bDcac_SendProtInit() == false)
        return -1;

    if (bDcac_QueueInit() == false)
        return -2;

    if (b_dcac_task_param_init() == false)
        return -3;

    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vDcac_Task,
                    (const char*    )"DcacTask",
                    (uint16_t       )DCAC_TASK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )DCAC_TASK_PRIO,
                    (TaskHandle_t*  )&tDcacTaskHandler) != pdPASS)
        return -4;
    vQueue_BindTaskHandler(tpDcacTask, tDcacTaskHandler);
    #endif  /* boardUSE_OS */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 逆变队列任务循环
 * 说明(备注)  : 无
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_Task(void *pvParameters)
{
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        if (s_tpDcacTask == NULL
            #if (boardUPDATE)
            || (tSysInfo.eDevState == DS_UPDATE_MODE && !IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
            #endif  /* boardUPDATE */
           )
        {
            if (s_tpDcacTask == NULL)
                b_dcac_task_param_init();

            #if (boardUSE_OS)
            vTaskDelay(500);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

        vQueue_TaskPoll(s_tpDcacTask, dcacTASK_CYCLE_TIME);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 设置 AC 充放电及并网状态
 * 说明(备注)  : 无
 * 传入参数    : obj: 操作对象, state: 目标出入状态
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-失败
 ************************************************************************************************************************/
bool bDcac_SetAcState(OperaObject_E obj, InOutState_E state)
{
    switch (obj)
    {
        case OO_CHG:
        {
            if (tDcac.eChgState == IOS_PROTE)
                return false;
            tDcac.eChgState = state;
        }
        break;

        case OO_DISCHG:
        {
            tDcac.eDisChgState = state;
        }
        break;

        case OO_PARA_IN:
        {
            tDcac.eParanInState = state;
        }
        break;

        case OO_ALL:
        {
            tDcac.eDisChgState  = state;
            tDcac.eParanInState = state;
            if (tDcac.eChgState == IOS_PROTE)
                return false;
            tDcac.eChgState = state;
        }
        break;

        default:
            if (uPrint.tFlag.bDcacTask)
                log_w("bDcacTask:AC设置对象%d错误", obj);
            return false;
    }

    if (tDcac.eDisChgState == IOS_SHUT_DOWN)
    {
        if (tDcac.uErrCode.tCode.bSysOutOL == 1)
            bDcac_SetErrCode(DEC_SYS_OUT_OL, false);
    }

    if (tDcac.eDisChgState == IOS_ERR   ||
        tDcac.eParanInState == IOS_ERR  ||
        tDcac.eParanInState == IOS_PROTE||
        tDcac.eChgState == IOS_PROTE    ||
        tDcac.eChgState == IOS_ERR)
    {
        bDcac_SetDevState(DS_ERR);
    }
    else if (tDcac.eDisChgState >= IOS_STARTING || tDcac.eChgState >= IOS_STARTING || tDcac.eParanInState >= IOS_STARTING)
        bDcac_SetDevState(DS_WORK);
    else
        bDcac_SetDevState(DS_SHUT_DOWN);

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置设备状态
 * 说明(备注)  : 无
 * 传入参数    : state: 设备状态
 * 输出参数    : 无
 * 返回值      : bool: true-成功
 ************************************************************************************************************************/
bool bDcac_SetDevState(DevState_E state)
{
    if (tDcac.eDevState != state)
    {
        if (state != DS_LOST)
        {
            #if (boardSYS_DATA_UPADATA)
            STAT_SET(tSysInfo.Mod_Exist, OL_DCAC);
            #endif  /* boardSYS_DATA_UPADATA */
        }
        else
        {
            #if (boardSYS_DATA_UPADATA)
            STAT_CLR(tSysInfo.Mod_Exist, OL_DCAC);
            #endif  /* boardSYS_DATA_UPADATA */
        }
    }

    tDcac.eDevState = state;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置设备错误代码
 * 说明(备注)  : 无
 * 传入参数    : code: 错误类型, set: true置位, false清除
 * 输出参数    : 无
 * 返回值      : bool: false
 ************************************************************************************************************************/
bool bDcac_SetErrCode(DCAC_ErrCode_E code, bool set)
{
    static DCAC_ErrCode_E s_e_last_code = DEC_CLEAR_ALL;
    static bool           s_b_last_set  = false;

    if (tSysInfo.uInit.tFinish.bIF_DcacTask == 0 && set == true)
    {
        if (uPrint.tFlag.bDcacTask)
            log_w("bDcacTask:DCAC模块未初始化完成，不允许标记错误%d", code);
        return false;
    }

    if (code == DEC_SYS_DEV_LOST)
    {
        if (set == false && tDcac.uErrCode.tCode.bSysDevLost == 0)
        {
            b_dcac_task_param_init();
            return false;
        }
    }

    if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
    {
        if (s_e_last_code != code || s_b_last_set != set)
        {
            log_e("bDcacTask:任务错误 代码%d 类型%d", code, set);
            s_e_last_code = code;
            s_b_last_set  = set;
        }
    }

    if (code > DEC_CLEAR_ALL)
    {
        if (code == DEC_SYS_DEV_LOST)
        {
            tDcac.uErrCode.ulCode = 0;
            memset(&tDcacRx, 0, sizeof(tDcacRx));

            if (set)
            {
                b_dcac_task_param_init();
                ERR_SET(tDcac.uErrCode.ulCode, (code - 1));
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_e("bDcacTask:DCAC模块丢失");
                bDcac_SetPerm(DPO_ALL, false);
                bDcac_SetDevState(DS_LOST);
            }
            else
            {
                if (tpDcacTask->ucID != DTI_INIT)
                    cQueue_AddQueueTask(tpDcacTask, DTI_INIT, NULL, true);
            }
        }
        else
        {
            if (set)
                ERR_SET(tDcac.uErrCode.ulCode, (code - 1));
            else
                ERR_CLR(tDcac.uErrCode.ulCode, (code - 1));
        }
    }
    else
    {
        tDcac.uErrCode.ulCode = 0;
        memset(&tDcacRx.uErrCode.tCode, 0, sizeof(tDcacRx.uErrCode.tCode));
    }

    if (tDcac.uErrCode.ulCode)
    {
        #if (boardBUZ_EN)
        bBuz_Tweet(LONG_3);
        #endif  /* boardBUZ_EN */
    }
    else
    {
        if (tDcac.eChgState == IOS_ERR)
        {
            bDcac_SetAcState(OO_CHG, IOS_SHUT_DOWN);
            if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                log_i("bDcacTask:错误清空,清除输入保护");
        }

        if (tDcac.eDisChgState == IOS_ERR)
        {
            bDcac_SetAcState(OO_DISCHG, IOS_SHUT_DOWN);
            if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                log_i("bDcacTask:错误清空,清除输出保护");
        }
    }

    return false;
}

/***********************************************************************************************************************
 * 函数功能    : DCAC 开关控制
 * 说明(备注)  : 无
 * 传入参数    : obj: 开关对象, sw: 开关类型, buz_en: 是否允许蜂鸣
 * 输出参数    : 无
 * 返回值      : int8_t: 1-成功, 小于0-失败
 ************************************************************************************************************************/
int8_t cDCAC_Switch(DACD_SwitchObject_E obj, SwitchType_E sw, bool buz_en)
{
    if (tDcac.eDevState == DS_LOST)
    {
        #if (boardBUZ_EN)
        bBuz_Tweet(LONG_2);
        #endif  /* boardBUZ_EN */

        #if (boardSYS_DATA_UPADATA)
        STAT_CLR(tSysInfo.Mod_Exist, OL_DCAC);
        Sys_Update_Element(AT_AC_SWITCH_ADDR, NULL, false, true);
        #endif  /* boardSYS_DATA_UPADATA */

        if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
            log_w("bDcacTask:开关失败,DCAC模块丢失");
        return -1;
    }

    /* 关闭全部对象 */
    if (obj == DSO_OFF_ALL)
    {
        #if (boardBUZ_EN)
        bBuz_Tweet(LONG_1);
        #endif  /* boardBUZ_EN */

        cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_OFF, false);
        cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_PARA_IN, ST_OFF, false);

        if (uPrint.tFlag.bDcacTask)
            sMyPrint("bDcacTask:添加关闭所有任务\r\n");
        return 1;
    }

    /* ---------------- 控制输出 (DSO_AC_OUT) ---------------- */
    if (obj == DSO_AC_OUT)
    {
        bool b_turn_on = false;
        if (sw == ST_ON)
            b_turn_on = true;
        else if (sw == ST_OFF)
            b_turn_on = false;
        else
            b_turn_on = (tDcac.eDisChgState < IOS_ERR);

        if (b_turn_on)
        {
            if (tDcac.uPerm.tPerm.bDisChgPerm == false)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(SHORT_2);
                #endif  /* boardBUZ_EN */
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_w("bDcacTask:不允许开启放电");
                return -2;
            }

            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_ON, false);
            #if (boardBUZ_EN)
            bBuz_Tweet(LONG_1);
            #endif  /* boardBUZ_EN */
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----添加开启逆变任务----\r\n");
        }
        else
        {
            bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);
            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_OFF, false);
            #if (boardBUZ_EN)
            bBuz_Tweet(LONG_1);
            #endif  /* boardBUZ_EN */
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----添加关闭逆变任务----\r\n");
        }
    }
    /* ---------------- 控制充电输入 (DSO_AC_IN) ---------------- */
    else if (obj == DSO_AC_IN)
    {
        bool b_turn_on = false;
        if (sw == ST_ON)
            b_turn_on = true;
        else if (sw == ST_OFF)
            b_turn_on = false;
        else
            b_turn_on = (tDcac.eChgState <= IOS_STARTING);

        if (b_turn_on)
        {
            if (tDcac.eChgState == IOS_PROTE)
            {
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_w("bDcacTask:输入状态为保护,不允许开启充电\r\n");
                return -5;
            }

            if (tDcac.uPerm.tPerm.bChgPerm == false)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(SHORT_2);
                #endif  /* boardBUZ_EN */
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_w("bDcacTask:当前不许可充电");
                return -8;
            }

            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_IN, ST_ON, false);
            if (buz_en == true)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(LONG_1);
                #endif  /* boardBUZ_EN */
            }
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----添加开启充电任务----\r\n");
        }
        else
        {
            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_IN, ST_OFF, false);
            #if (boardBUZ_EN)
            bBuz_Tweet(LONG_1);
            #endif  /* boardBUZ_EN */
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----添加关闭充电任务----\r\n");
        }
    }
    /* ---------------- 控制并网 (DSO_PARA_IN) ---------------- */
    else if (obj == DSO_PARA_IN)
    {
        bool b_turn_on = false;
        if (sw == ST_ON)
            b_turn_on = true;
        else if (sw == ST_OFF)
            b_turn_on = false;
        else
            b_turn_on = (tDcac.eParanInState <= IOS_STARTING);

        if (b_turn_on)
        {
            if (tDcac.eParanInState == IOS_PROTE)
            {
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_w("bDcacTask:状态为保护,不允许开启并网");
                return -9;
            }

            if (tDcac.uPerm.tPerm.bParaInPerm == false)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(SHORT_2);
                #endif  /* boardBUZ_EN */
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_w("bDcacTask:当前不允许开启并网");
                return -10;
            }

            if (tDcacRx.usInVolt < tAppMemParam.tDCAC.usMinInVolt)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(SHORT_2);
                #endif  /* boardBUZ_EN */
                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    log_w("bDcacTask:当前未接电网,不允许开启并网");
                return -11;
            }

            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_PARA_IN, ST_ON, false);
            if (buz_en == true)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(LONG_1);
                #endif  /* boardBUZ_EN */
            }
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----添加开启并网任务----\r\n");
        }
        else
        {
            cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_PARA_IN, ST_OFF, false);
            #if (boardBUZ_EN)
            bBuz_Tweet(LONG_1);
            #endif  /* boardBUZ_EN */
            if (uPrint.tFlag.bDcacTask)
                sMyPrint("bDcacTask:----添加关闭并网任务----\r\n");
        }
    }

    #if (boardDISPLAY_EN)
    bDisp_Switch(ST_ON, false);
    #endif  /* boardDISPLAY_EN */

    #if (boardSYS_DATA_UPADATA)
    Sys_Update_Mod(DCAC_Mod, true);
    #endif  /* boardSYS_DATA_UPADATA */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 无输出时自动关闭倒计时
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_TickTimer(void)
{
    if (bSys_IsWorkState() == false)
        return;

    if (tDcacRx.usOutPwr)
    {
        vDcac_RefreshOffTime();
        return;
    }

    if (tDcac.eDisChgState <= IOS_STARTING)
        return;

    if (tDcac.eChgState >= IOS_STARTING)
        return;

    if (tDcac.usAutoOffTime)
    {
        if (tDcac.usAutoOffCnt)
        {
            tDcac.usAutoOffCnt--;
            if (tDcac.usAutoOffCnt == 0)
            {
                #if (boardBUZ_EN)
                bBuz_Tweet(LONG_1);
                #endif  /* boardBUZ_EN */

                cDCAC_Switch(DSO_AC_OUT, ST_OFF, true);

                if (uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
                    sMyPrint("Dcac_Task:====倒计时结束,关闭输出,时间=%dS====\r\n", tDcac.usAutoOffTime);

                if (bSys_CheckActState() == false)
                    cSys_Switch(SO_DCAC, ST_OFF, false);
            }
        }
    }
}

/***********************************************************************************************************************
 * 函数功能    : 刷新逆变器关闭倒计时计数
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_RefreshOffTime(void)
{
    if (tDcac.usAutoOffTime)
        tDcac.usAutoOffCnt = tDcac.usAutoOffTime;
}

/***********************************************************************************************************************
 * 函数功能    : 输入保护功能开关
 * 说明(备注)  : 无
 * 传入参数    : b_sw: true 开启保护, false 解除
 * 输出参数    : 无
 * 返回值      : bool: true
 ************************************************************************************************************************/
bool bDcac_InProteFuncSwitch(bool b_sw)
{
    if (b_sw)
    {
        if (tDcac.eChgState >= IOS_STARTING)
            bDcac_SetErrCode(DEC_SYS_SET_IN_PROTE, true);
    }
    else
        bDcac_SetErrCode(DEC_SYS_SET_IN_PROTE, false);

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 自动关闭功能开关
 * 说明(备注)  : 无
 * 传入参数    : us_time: 自动关机秒数 (0为关闭)
 * 输出参数    : 无
 * 返回值      : bool: true
 ************************************************************************************************************************/
bool bDcac_SetAutoOffTime(uint16_t us_time)
{
    tDcac.usAutoOffTime = us_time;
    vDcac_RefreshOffTime();
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置充放电及并网许可
 * 说明(备注)  : 无
 * 传入参数    : obj: 许可对象, en: true 允许, false 禁止
 * 输出参数    : 无
 * 返回值      : bool: true-成功, false-非法对象
 ************************************************************************************************************************/
bool bDcac_SetPerm(DcacPermObject_E obj, bool en)
{
    switch (obj)
    {
        case DPO_CHG:
        {
            if (en != tDcac.uPerm.tPerm.bChgPerm)
            {
                if (uPrint.tFlag.bDcacTask)
                    sMyPrint("bDcacTask:设置充电许可: 设置=%d 当前状态=%d \r\n", en, tDcac.uPerm.tPerm.bChgPerm);
                tDcac.uPerm.tPerm.bChgPerm = en;
            }
        }
        break;

        case DPO_DISCHG:
        {
            if (en != tDcac.uPerm.tPerm.bDisChgPerm)
            {
                if (en == false && tDcac.eDisChgState >= IOS_STARTING)
                    cDCAC_Switch(DSO_AC_OUT, ST_OFF, true);
                if (uPrint.tFlag.bDcacTask)
                    sMyPrint("bDcacTask:设置放电许可: 设置=%d 当前状态=%d \r\n", en, tDcac.uPerm.tPerm.bDisChgPerm);
                tDcac.uPerm.tPerm.bDisChgPerm = en;
            }
        }
        break;

        case DPO_PARA_IN:
        {
            if (en != tDcac.uPerm.tPerm.bParaInPerm)
            {
                if (uPrint.tFlag.bDcacTask)
                    sMyPrint("bDcacTask:设置并网许可: 设置=%d 当前状态=%d \r\n", en, tDcac.uPerm.tPerm.bParaInPerm);
                tDcac.uPerm.tPerm.bParaInPerm = en;
            }
        }
        break;

        case DPO_ALL:
        {
            if (en != tDcac.uPerm.tPerm.bChgPerm)
            {
                if (uPrint.tFlag.bDcacTask)
                    sMyPrint("bDcacTask:设置充电许可: 设置=%d 当前状态=%d \r\n", en, tDcac.uPerm.tPerm.bChgPerm);
                tDcac.uPerm.tPerm.bChgPerm = en;
            }

            if (en != tDcac.uPerm.tPerm.bDisChgPerm)
            {
                if (en == false && tDcac.eDisChgState >= IOS_STARTING)
                    cDCAC_Switch(DSO_AC_OUT, ST_OFF, true);
                if (uPrint.tFlag.bDcacTask)
                    sMyPrint("bDcacTask:设置放电许可: 设置=%d 当前状态=%d \r\n", en, tDcac.uPerm.tPerm.bDisChgPerm);
                tDcac.uPerm.tPerm.bDisChgPerm = en;
            }

            if (en != tDcac.uPerm.tPerm.bParaInPerm)
            {
                if (uPrint.tFlag.bDcacTask)
                    sMyPrint("bDcacTask:设置并网许可: 设置=%d 当前状态=%d \r\n", en, tDcac.uPerm.tPerm.bParaInPerm);
                tDcac.uPerm.tPerm.bParaInPerm = en;
            }
        }
        break;

        default:
            return false;
    }
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 DCAC 记忆参数
 * 说明(备注)  : 无
 * 传入参数    : p_dcac_mem: 记忆参数结构体指针
 * 输出参数    : 无
 * 返回值      : bool: true
 ************************************************************************************************************************/
bool bDcac_MemParamInit(DcacMemParam_T *p_dcac_mem)
{
    p_dcac_mem->usAutoOffTime  = boardDCAC_OFF_TIME;
    p_dcac_mem->usMinOpenVolt  = boardDCAC_OPEN_MIN_VOLT;
    p_dcac_mem->usVoltRating   = boardDCAC_VOLT_RATING;
    p_dcac_mem->usMaxInVolt    = boardDCAC_MAX_IN_VOLT;
    p_dcac_mem->usMinInVolt    = boardDCAC_MIN_IN_VOLT;
    p_dcac_mem->usInPwrRating  = boardDCAC_IN_PWR_RATING;
    p_dcac_mem->usMinInPwr     = boardDCAC_MIN_IN_PWR;
    p_dcac_mem->usMaxInCurr    = boardDCAC_MAX_IN_CURR;
    p_dcac_mem->usOutPwrRating = boardDCAC_OUT_PWR_RATING;
    p_dcac_mem->usOverLoadPwr  = boardDCAC_OVERLOAD_PWR;
    p_dcac_mem->usParaInPwr    = boardDCAC_PARA_IN_PWR;
    p_dcac_mem->usAcOutFreq    = boardDCAC_OUT_FREQ;
    p_dcac_mem->sMaxTemp       = boardDCAC_MAX_TEMP;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置 DCAC 记忆参数 (表驱动优化版本)
 * 说明(备注)  : 无
 * 传入参数    : uc_item: 参数索引, add: true 增加, false 减少
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDcac_MemParamSet(uint8_t uc_item, bool add)
{
    if (uc_item < DCAC_PARAM_STEP_TBL_NUM)
    {
        const DcacParamStep_T *pt = &s_tDcacParamStepTbl[uc_item];
        if (pt->bIsSigned8)
        {
            int8_t *ps8 = (int8_t *)pt->pVal;
            if (add)
            {
                if (*ps8 < (int8_t)pt->slMax)
                    (*ps8)++;
            }
            else
            {
                if (*ps8 > (int8_t)pt->slMin)
                    (*ps8)--;
            }
        }
        else
        {
            uint16_t *pu16 = (uint16_t *)pt->pVal;
            if (add)
            {
                if (*pu16 < (uint16_t)pt->slMax)
                    (*pu16)++;
            }
            else
            {
                if (*pu16 > (uint16_t)pt->slMin)
                    (*pu16)--;
            }
        }
    }
}

#endif  /* boardDCAC_EN */
