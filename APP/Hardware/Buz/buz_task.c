/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Buz
 * File    : buz_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 蜂鸣器驱动任务与音效分发(表驱动优化)实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Buz/buz_task.h"

#if (boardBUZ_EN)
#include "Buz/buz_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			BUZ_TASK_PRIO							2		/* 任务优先级 */
#define			BUZ_TASK_STK_SIZE						128		/* 任务堆栈 (512B，达到 configMINIMAL_STACK_SIZE 防溢出标准) */
static TaskHandle_t s_t_buz_task_handler = NULL;
void 			vBuz_Task(void *p_v_parameters);
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			BUZ_ON()								buzTIMER_PWM_SET(300)
#define			BUZ_OFF()								buzTIMER_PWM_SET(0)

//****************************************************Parameter Initialization**************************************************//
typedef struct
{
	uint16_t			usNum;				/* 响的次数 */
	uint16_t			usOnMs;				/* 单次鸣叫时长 (ms) */
	uint16_t			usOffMs;			/* 鸣叫间隔时长 (ms) */
}BuzPattern_T;

/* 蜂鸣器各类型音效参数表 */
static const BuzPattern_T s_t_buz_patterns[] =
{
	[Null]    = { 0, 0,   0   },
	[SHORT_1] = { 1, 100, 0   },	/* 操作提示音(长按触发) */
	[SHORT_2] = { 2, 100, 100 },	/* 操作失败 */
	[SHORT_3] = { 3, 100, 100 },	/* 准备打开时的错误 */
	[LONG_1]  = { 1, 200, 0   },	/* 重要操作提示: 开关提示音 */
	[LONG_2]  = { 2, 200, 200 },	/* 设备丢失: 逆变器丢失 */
	[LONG_3]  = { 3, 200, 200 },	/* 运行中的错误: 高低温报警/过压/过流 */
	[LONG_5]  = { 5, 200, 200 },	/* 特殊提示音 */
};

//****************************************************Parameter Initialization**************************************************//
static vs16   s_s_buz_num          = 0;
static vu16   s_us_buz_on_time     = 0;
static vu16   s_us_buz_off_time    = 0;
static bool   s_b_buz_tri_flag     = false;
static Buzz_E s_e_current_buz_type = Null;

//****************************************************Function Declaration******************************************************//
static uint8_t uc_buz_get_priority(Buzz_E e_type);
static void    v_buz_trigger(Buzz_E e_type, uint16_t us_num, uint16_t us_on_time, uint16_t us_off_time);


/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器任务初始化
 * 说明(备注)  : 完成硬件底层初始化并创建蜂鸣器任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 任务创建失败
 ************************************************************************************************************************/
s8 cBuz_TaskInit(void)
{
	vBuz_Init();

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vBuz_Task,
	                (const char*    )"BuzTask",
	                (uint16_t       )BUZ_TASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )BUZ_TASK_PRIO,
	                (TaskHandle_t*  )&s_t_buz_task_handler) != pdPASS)
		return -1;
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器任务
 * 说明(备注)  : 依据通知执行蜂鸣鸣叫与间歇控制
 * 传入参数    : p_v_parameters
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBuz_Task(void *p_v_parameters)
{
	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		#if (boardUSE_OS)
		if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY) == pdTRUE)
		#endif  /* boardUSE_OS */
		{
			while (s_s_buz_num > 0)
			{
				if (!tAppMemParam.tSYS.bBuzSwitchOff)
					BUZ_ON();
				vTaskDelay(s_us_buz_on_time);
				BUZ_OFF();
				vTaskDelay(s_us_buz_off_time);
				s_s_buz_num--;
			}
			if (s_s_buz_num <= 0)
			{
				s_s_buz_num          = 0;
				s_b_buz_tri_flag     = false;
				s_e_current_buz_type = Null;
			}
		}
	}
}

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 获取音效优先级等级
 * 说明(备注)  : 用于告警抢占仲裁，等级越高优先级越高
 * 传入参数    : e_type: 音效类型
 * 输出参数    : 无
 * 返回值      : 优先级数值 (0~7)
 ************************************************************************************************************************/
static uint8_t uc_buz_get_priority(Buzz_E e_type)
{
	switch (e_type)
	{
		case LONG_5:  return 7;
		case LONG_3:  return 6;                                 /* 严重错误: 高低温/过压/过流 */
		case LONG_2:  return 5;                                 /* 设备丢失 */
		case SHORT_3: return 4;                                 /* 准备打开时的错误 */
		case SHORT_2: return 3;                                 /* 操作失败 */
		case LONG_1:  return 2;                                 /* 开关提示音 */
		case SHORT_1: return 1;                                 /* 按键提示音 */
		default:      return 0;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 触发蜂鸣器操作 (支持高优先级抢占)
 * 说明(备注)  : 内部静态函数，设置鸣叫参数并发送任务通知
 * 传入参数    : e_type: 音效类型, us_num: 鸣叫次数, us_on_time: 鸣叫时长(ms), us_off_time: 间隔时长(ms)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_buz_trigger(Buzz_E e_type, uint16_t us_num, uint16_t us_on_time, uint16_t us_off_time)
{
	if (s_t_buz_task_handler == NULL)
		return;

	/* 当空闲 或 传入音效的优先级高于当前正在鸣叫的音效时，允许抢占触发 */
	if (!s_b_buz_tri_flag || (uc_buz_get_priority(e_type) > uc_buz_get_priority(s_e_current_buz_type)))
	{
		s_e_current_buz_type = e_type;
		s_s_buz_num          = (int16_t)us_num;
		s_us_buz_on_time     = us_on_time;
		s_us_buz_off_time    = us_off_time;
		s_b_buz_tri_flag     = true;

		#if (boardUSE_OS)
		xTaskNotifyGive(s_t_buz_task_handler);
		#endif  /* boardUSE_OS */
	}

	#if (boardDISPLAY_EN)
	if (s_s_buz_num >= 3 && tSysInfo.eDevState != DS_INIT)
		bDisp_Switch(ST_ON, false);
	#endif  /* boardDISPLAY_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 发送蜂鸣器音效
 * 说明(备注)  : 基于静态常量表匹配音效参数，查表驱动
 * 传入参数    : e_type: 音效类型
 * 输出参数    : 无
 * 返回值      : true: 执行成功, false: 参数非法
 ************************************************************************************************************************/
bool bBuz_Tweet(Buzz_E e_type)
{
	if (e_type > Null && e_type <= LONG_5)
	{
		const BuzPattern_T *p_pat = &s_t_buz_patterns[e_type];
		if (p_pat->usNum > 0)
			v_buz_trigger(e_type, p_pat->usNum, p_pat->usOnMs, p_pat->usOffMs);
	}
	return true;
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器进入低功耗
 * 说明(备注)  : 挂起任务并配置 GPIO 为低功耗模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBuz_EnterLowPower(void)
{
	vBuz_IoEnterLowPower();
	if (s_t_buz_task_handler != NULL)
		vTaskSuspend(s_t_buz_task_handler);
}

/***********************************************************************************************************************
 * 函数功能    : 蜂鸣器退出低功耗
 * 说明(备注)  : 恢复硬件与定时器并恢复任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vBuz_ExitLowPower(void)
{
	vBuz_Init();
	if (s_t_buz_task_handler != NULL)
		vTaskResume(s_t_buz_task_handler);
}
#endif  /* boardLOW_POWER */

#endif  /* boardBUZ_EN */

