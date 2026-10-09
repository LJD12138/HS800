/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_task.c
 * Date    : 2026-09-28
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 显示任务外壳实现 - 调度引擎驱动/页面注册/兼容控制
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_task.h"

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_iface.h"

#include "uni_disp_port.h"
#include "app_info.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */


//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			dispTASK_PRIO							2U		/* 任务优先级 */
#define			dispTASK_STK_SIZE						1024U	/* 任务堆栈 */
TaskHandle_t    tDispTaskHandler = NULL;
void            vDisp_Task(void *pvParameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
#if (boardENG_MODE_EN)
/* 显示记忆参数步进配置表 */
typedef struct
{
	void				*pParam;
	int32_t				lMin;
	int32_t				lMax;
	uint8_t				ucType;				/* 0: uint16_t, 1: int8_t, 2: uint8_t */
}DispParamStep_T;

static const DispParamStep_T s_tDispParamTable[] =
{
	{(void *)&tAppMemParam.tDISP.ucHighLightValue, 0x88, 0x8F, 2}, /* item 0: 高亮值 */
	{(void *)&tAppMemParam.tDISP.ucLowLightValue,  0x88, 0x8F, 2}, /* item 1: 低亮值 */
	{(void *)&tAppMemParam.tDISP.usAutoOffTime,       0, 3600, 0}, /* item 2: 息屏时间 */
};
#endif  /* boardENG_MODE_EN */


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

    /* 绑定唤醒钩子 (供 bDisp_PostEvent / bDisp_Switch 即时唤醒任务) */
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
 * 说明(备注)  : 统一等待原语 (通知+超时): 亮屏按当前页帧周期做耗时补偿等待(扣除本次渲染已耗时, 保证
 *               实际帧周期贴近页面声明值), 息屏 2 秒低频挂起; 事件入队/背光点亮均触发任务通知提前唤醒
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_Task(void *pvParameters)
{
    (void)pvParameters;

    #if (boardUSE_OS)
    TickType_t x_start;
    TickType_t x_period;
    TickType_t x_elapsed;

    for (;;)
    #endif  /* boardUSE_OS */
    {
        #if (boardUSE_OS)
        x_start = xTaskGetTickCount();
        #endif  /* boardUSE_OS */

        /* 执行 UniDisplay 轮询引擎 (快照抓取 -> 状态路由 -> 事件分发 -> 切页 -> 适配器渲染) */
        vDisp_EnginePoll();

        #if (boardUSE_OS)
        if (bDisp_IsBacklightOn() == false)
        {
            /* 息屏: 2 秒低频心跳挂起, CPU 占用 0% */
            ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
        }
        else
        {
            /* 亮屏: 帧周期 - 已耗渲染时间 = 剩余等待时间 (已超时则立刻进入下一帧) */
            x_period  = pdMS_TO_TICKS(usDisp_GetFramePeriod());
            x_elapsed = xTaskGetTickCount() - x_start;
            ulTaskNotifyTake(pdTRUE, (x_elapsed < x_period) ? (x_period - x_elapsed) : 1U);
        }
        #endif  /* boardUSE_OS */
    }
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
 * 函数功能    : 设置显示记忆参数 (表驱动版)
 * 说明(备注)  : 支持 uint8 与 uint16 类型，自动进行安全上下限防越界检查 (参考 sys_task.c:1070)
 * 传入参数    : item: 参数索引, add: true-增加, false-减少
 * 输出参数    : 无
 * 返回值      : void
 ***********************************************************************************************************************/
#if (boardENG_MODE_EN)
void vDisp_MemParamSet(u8 item, bool add)
{
	if ((item >= mainARRAY_SIZE(s_tDispParamTable)) || (s_tDispParamTable[item].pParam == NULL))
		return;

	const DispParamStep_T *p = &s_tDispParamTable[item];
	if (p->ucType == 2)
	{
		uint8_t *p_val = (uint8_t *)p->pParam;
		if (add && (*p_val < (uint8_t)p->lMax))
			(*p_val)++;
		else if (!add && (*p_val > (uint8_t)p->lMin))
			(*p_val)--;
	}
	else if (p->ucType == 1)
	{
		int8_t *p_val = (int8_t *)p->pParam;
		if (add && (*p_val < (int8_t)p->lMax))
			(*p_val)++;
		else if (!add && (*p_val > (int8_t)p->lMin))
			(*p_val)--;
	}
	else
	{
		uint16_t *p_val = (uint16_t *)p->pParam;
		if (add && (*p_val < (uint16_t)p->lMax))
			(*p_val)++;
		else if (!add && (*p_val > (uint16_t)p->lMin))
			(*p_val)--;
	}
}
#endif  /* boardENG_MODE_EN */

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 选择显示供电通路
 * 说明(备注)  : 根据外部输入电源状态控制显示屏电池供电开关
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_dis_power_select(void)
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
