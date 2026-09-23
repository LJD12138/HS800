/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_task.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 显示任务外壳实现 (TFT+LVGL) - 调度引擎驱动/页面注册/兼容控制
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_task.h"

#if (boardDISPLAY_EN)
#include <string.h>
#include "uni_disp_port.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_iface.h"
#include "MD_Display/eez_ui/ui.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

//****************************************************Parameter Initialization**************************************************//
#if (boardUSE_OS)
#define			dispTASK_PRIO							2U		/* 任务优先级 */
#define			dispTASK_STK_SIZE						2048U	/* 任务堆栈(字数, LVGL深调用栈需保证) */
TaskHandle_t tDispTaskHandler = NULL;
void vDisp_Task(void *pvParameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
static bool G_bUiInitialized = false;

//****************************************************Function Declaration******************************************************//
static bool b_task_param_init(void);

/***********************************************************************************************************************
 * 函数功能    : Disp 显示任务初始化
 * 说明(备注)  : 
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 页面注册失败, -2: 任务创建失败
 ************************************************************************************************************************/
s8 cDisp_TaskInit(void)
{
    /* 1. 初始化 UniDisplay 调度核心 */
    vDisp_CoreInit();

    /* 2. 初始化显示接口引脚与时钟 */
    vDisp_IfaceInit();

    /* 3. 注册业务页面 */
    if (!b_task_param_init())
        return -1;

    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vDisp_Task,
                    (const char*    )"DispTask",
                    (uint16_t       )dispTASK_STK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )dispTASK_PRIO,
                    (TaskHandle_t*  )&tDispTaskHandler) != pdPASS)
    {
        tDispTaskHandler = NULL;
        return -2;
    }

    /* 绑定唤醒钩子 (供 bDisp_PostEvent / bDisp_SwitchBacklight 即时唤醒任务) */
    vDisp_PortSetTaskHandle(tDispTaskHandler);
    #endif  /* boardUSE_OS */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 任务参数与页面注册
 * 说明(备注)  : 静态注册显示页面
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
static bool b_task_param_init(void)
{
    /* 页面静态注册 (项目件) */
    vDisp_PageRegister(&G_tPageInit);
    vDisp_PageRegister(&G_tPageBoot);
    vDisp_PageRegister(&G_tPageWork);
    vDisp_PageRegister(&G_tPageClosing);
    vDisp_PageRegister(&G_tPageShutDown);
    vDisp_PageRegister(&G_tPageFault);

    #if (boardUPDATE)
    vDisp_PageRegister(&G_tPageUpdate);
    #endif  /* boardUPDATE */

    #if (boardENG_MODE_EN)
    vDisp_PageRegister(&G_tPageEng);
    #endif  /* boardENG_MODE_EN */

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : Disp 显示任务主体
 * 说明(备注)  : 统一等待原语 (通知+超时): 亮屏按当前页 33ms 帧节拍等待, 息屏 2 秒低频挂起;
 *               事件入队/背光点亮均触发任务通知提前唤醒, 响应无需等到下一周期
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_Task(void *pvParameters)
{
    (void)pvParameters;

    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        /* 执行 UniDisplay 轮询引擎 (快照抓取 -> 状态路由 -> 事件分发 -> 切页 -> 适配器渲染) */
        vDisp_EnginePoll();

        #if (boardUSE_OS)
        if (bDisp_IsBacklightOn() == false)
            /* 息屏: 2 秒低频心跳挂起, CPU 占用 0% */
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
        else
            /* 亮屏: 按当前页面声明的帧周期节拍等待 (33ms) */
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(usDisp_GetFramePeriod()));
        #endif  /* boardUSE_OS */
    }
}


/***********************************************************************************************************************
 * 函数功能    : 显示开关 (向后兼容旧接口，内部对接框架 Power Manager)
 * 说明(备注)  : none
 * 传入参数    : type: 类型 (ST_ON / ST_OFF / ST_NULL), fore_en: 强制打开 (常亮)
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bDisp_Switch(SwitchType_E type, bool fore_en)
{
    DispBacklight_E e_bkl;

    switch (type)
    {
        case ST_ON:
        {
            e_bkl = DISP_BKL_ON;
        }
        break;
        case ST_OFF:
        {
            e_bkl = DISP_BKL_OFF;
        }
        break;
        case ST_NULL:
        default:
        {
            e_bkl = DISP_BKL_TOGGLE;
        }
        break;
    }

    bool b_ret = bDisp_SwitchBacklight(e_bkl, fore_en);

    #if (boardBUZ_EN)
    if (fore_en && bDisp_IsBacklightOn())
        bBuz_Tweet(LONG_1);
    #endif  /* boardBUZ_EN */

    return b_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化显示记忆参数
 * 说明(备注)  : 写入显示模块默认亮度和自动息屏时间
 * 传入参数    : p_disp_mem: 显示记忆参数结构体指针
 * 输出参数    : p_disp_mem: 回填高亮亮度、低亮亮度与自动息屏时间
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bDisp_MemParamInit(DispMemParam_T* p_disp_mem)
{
    if (p_disp_mem == NULL)
        return false;

    p_disp_mem->ucHighLightValue = boardDISP_HIGH_LIGHT_VALUE;
    p_disp_mem->ucLowLightValue  = boardDISP_LOW_LIGHT_VALUE;
    p_disp_mem->usAutoOffTime    = boardDISP_OFF_TIME;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 安全的 UI 及所有屏幕初始化
 * 说明(备注)  : 防重入保护
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_UiInit(void)
{
    if (G_bUiInitialized == false)
    {
        G_bUiInitialized = true;
        ui_init();
    }
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 选择显示供电通路
 * 说明(备注)  : 根据外部输入电源状态控制显示屏电池供电开关
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_dis_power_select(void)
{
    if (bDisp_IsBacklightOn())
    {
        if (tAdcSamp.usSysInVolt >= boardBMS_MIN_VOLT)
            Disp_EN_OFF();
        else
            Disp_EN_ON();
    }
    else
        Disp_EN_OFF();
}

/***********************************************************************************************************************
 * 函数功能    : 进入显示低功耗
 * 说明(备注)  : 挂起显示任务以降低低功耗模式下的运行消耗
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLcd_EnterLowPower(void)
{
    #if (boardUSE_OS)
    if (tDispTaskHandler != NULL)
        vTaskSuspend(tDispTaskHandler);
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 退出显示低功耗
 * 说明(备注)  : 恢复显示任务运行
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vLcd_ExitLowPower(void)
{
    #if (boardUSE_OS)
    if (tDispTaskHandler != NULL)
        vTaskResume(tDispTaskHandler);
    #endif  /* boardUSE_OS */
}
#endif  /* boardLOW_POWER */

#endif  /* boardDISPLAY_EN */
