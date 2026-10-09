/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Key
 * File    : key_task.c
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键任务及中间件集成胶水层实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Key/key_task.h"

#if (boardKEY_EN)
#include "Key/key_iface.h"
#include "Key/key_func.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "mf_key.h"

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			KEY_TASK_PRIO							2		/* 任务优先级 */
#define			KEY_TASK_STK_SIZE						256		/* 任务堆栈(字) */
static TaskHandle_t s_t_key_task_handler = NULL;				/* 任务句柄 */
void        	vKey_Task(void *p_v_parameters);				/* 任务函数 */
#endif  /* boardUSE_OS */


//****************************************************Parameter Initialization**************************************************//
static bool s_b_key_lock = false;   /* 开机长按锁定标志 */

//****************************************************Function Declaration******************************************************//
static bool b_key_event_pre_proc(u8 uc_idx, bool b_long);
static void v_key_on_super_long(u8 uc_idx);
static void v_key_on_any_press(void);

//****************************************************Parameter Initialization**************************************************//
/* 中间件按键配置表: 事件码映射 + 触发方式配置(表序与 KeyId_E 严格一致) */
static const MfKeyItemCfg_T s_t_key_mw_key_cfg[] =
{
	/* 短按事件           长按事件          多功能  长按累加 */
	[keyPOWER] = {KTE_POWER_SHORT,  KTE_POWER_LONG,  false,  false},

	#if (boardDCAC_EN)
	[keyAC]    = {KTE_AC_SHORT,     KTE_AC_LONG,     true,   false},
	#endif  /* boardDCAC_EN */

	#if (boardLIGHT_EN)
	[keyLIGHT] = {KTE_LIGHT_SHORT,  KTE_LIGHT_LONG,  true,   false},
	#endif  /* boardLIGHT_EN */

	#if (boardUSB_EN)
	[keyUSB]   = {KTE_USB_SHORT,    KTE_USB_LONG,    true,   false},
	#endif  /* boardUSB_EN */

	#if (boardDC_EN)
	[keyDC]    = {KTE_DC_SHORT,     KTE_DC_LONG,     true,   false},
	#endif  /* boardDC_EN */
};

#define			KEY_MW_KEY_TBL_NUM						(sizeof(s_t_key_mw_key_cfg) / sizeof(s_t_key_mw_key_cfg[0]))

/* 编译期校验: KeyId_E 枚举序必须与 s_t_key_mw_key_cfg[] 表序严格一致 */
typedef char __key_mw_tbl_order_assert[(KEY_MW_KEY_TBL_NUM == keyNUM) ? 1 : -1];

/* 事件序列缓冲与中间件全局配置 */
static u8 s_uc_key_seq_buff[keyGROUP_NUM];

static const MfKeyCfg_T s_t_key_mw_cfg =
{
	/* 硬件接口回调 */
	(bool (*)(u8))bKey_IsPressById,    /* 查表读取按键按下电平(极性已归一化) */
	/* 扫描时序参数(单位: keyTASK_CYCLE_TIME 周期数) */
	keySHORT_PRESS_TIME,               /* 短按最小时间 */
	keyLONG_PRESS_TIME,                /* 长按最小时间 */
	keySUPER_LONG_PRESS_TIME,          /* 超长按最小时间 */
	keyNUPRESS_MAX_TIME,               /* 组合键最大等待时间 */
	keyADD_SPACE_TIME,                 /* 长按累加间隔 */
	/* 事件序列缓冲 */
	s_uc_key_seq_buff,                 /* 缓冲地址 */
	keyGROUP_NUM,                      /* 缓冲长度 */
	KTE_FUN_NULL,                      /* 序列空闲填充码 */
	/* 业务回调 */
	vKey_ProcKeyFunc,                  /* 序列就绪 -> 业务动作分发 */
	b_key_event_pre_proc,              /* 事件录入前预处理(息屏唤醒/调试日志) */
	v_key_on_super_long,               /* 超长按提示(打印+蜂鸣) */
	v_key_on_any_press                 /* 任意键按下(清休眠计数) */
};


/***********************************************************************************************************************
 * 函数功能    : 按键任务初始化
 * 说明(备注)  : 底层 GPIO 初始化、中间件配置初始化并创建 OS 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 任务创建失败
 ************************************************************************************************************************/
s8 cKey_TaskInit(void)
{
	/* 底层硬件接口初始化 (RCU + GPIO) */
	vKey_IfaceInit();

	/* 多功能按键中间件初始化 (时序 + 回调 + 按键表) */
	vMfKey_Init(&s_t_key_mw_cfg, s_t_key_mw_key_cfg, KEY_MW_KEY_TBL_NUM);

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vKey_Task,
	                (const char*    )"bKeyTask",
	                (uint16_t       )KEY_TASK_STK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )KEY_TASK_PRIO,
	                (TaskHandle_t*  )&s_t_key_task_handler) != pdPASS)
		return -1;
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 按键循环扫描任务
 * 说明(备注)  : 胶水层: 任务调度 + 电源键触发方式策略 + 中间件状态机推进
 * 传入参数    : p_v_parameters: 任务创建参数指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_Task(void *p_v_parameters)
{
	(void)p_v_parameters;

	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		/* GPIO初始化未完成或系统升级状态 (长按开机锁定) */
		if (tSysInfo.uInit.tFinish.bIF_Gpio == 0 
			#if (boardUPDATE)
			|| tpSysTask->ucID == STI_UPDATE
			#endif  /* boardUPDATE */
		)
		{
			s_b_key_lock = bKey_IsPressById(keyPOWER);

			#if (boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  /* boardUSE_OS */
		}

		/* 长按开启不松开 */
		if (s_b_key_lock == true && bKey_IsPressById(keyPOWER) == true)
		{
			#if (boardUSE_OS)
			vTaskDelay(keyTASK_CYCLE_TIME);
			continue;
			#else
			return;
			#endif  /* boardUSE_OS */
		}

		s_b_key_lock = false;

		/* 按键扫描(中间件: 去抖/长短按/超长按/组合键时序状态机) */
		vMfKey_Scan();

		#if (boardUSE_OS)
		vTaskDelay(keyTASK_CYCLE_TIME);
		#endif  /* boardUSE_OS */
	}
}

/***********************************************************************************************************************
 * 函数功能    : 电源按键已经被外部处理
 * 说明(备注)  : 标记电源按键已处理，防止全局按键重复触发
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_PowerIsTri(void)
{
	vMfKey_MarkProcessed(keyPOWER);
}

/***********************************************************************************************************************
 * 函数功能    : 按键参数初始化/清空事件缓冲区
 * 说明(备注)  : 重置中间件事件序列
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vKey_ParamInit(void)
{
	vMfKey_ClearSeq();
}


/***********************************************************************************************************************
 * 函数功能    : 事件录入前预处理回调
 * 说明(备注)  : 息屏状态下唤醒屏幕并吞掉首个事件；正常事件打印调试日志
 * 传入参数    : uc_idx: 按键索引, b_long: 是否为长按
 * 输出参数    : 无
 * 返回值      : bool: false 吞掉该事件, true 正常录入
 ************************************************************************************************************************/
static bool b_key_event_pre_proc(u8 uc_idx, bool b_long)
{
	#if (boardDISPLAY_EN)
	if (!tDisp.bLight) /* 息屏第一个功能不执行 */
	{
		bDisp_Switch(ST_ON, false);
		if (uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:当前息屏,按键功能退出\r\n");
		return false;
	}
	#else
	(void)uc_idx;
	(void)b_long;
	#endif  /* boardDISPLAY_EN */

	if (uPrint.tFlag.bKeyTask)
	{
		static const char * const s_p_key_names[] =
		{
			"Power",
			#if (boardDCAC_EN)
			"AC",
			#endif  /* boardDCAC_EN */
			#if (boardLIGHT_EN)
			"Light",
			#endif  /* boardLIGHT_EN */
			#if (boardUSB_EN)
			"USB",
			#endif  /* boardUSB_EN */
			#if (boardDC_EN)
			"DC",
			#endif  /* boardDC_EN */
		};
		const char *p_name = (uc_idx < KEY_MW_KEY_TBL_NUM) ? s_p_key_names[uc_idx] : "Unknown";
		sMyPrint("Key_Task:%s%s\r\n", p_name, b_long ? "长按" : "短按");
	}

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 超长按提示回调
 * 说明(备注)  : 超长按触发时的提示蜂鸣与调试打印
 * 传入参数    : uc_idx: 按键索引
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_key_on_super_long(u8 uc_idx)
{
	(void)uc_idx;

	if (uPrint.tFlag.bKeyTask)
		sMyPrint("Key_Task:触发长按事件\r\n");

	#if (boardBUZ_EN)
	bBuz_Tweet(SHORT_1);
	#endif  /* boardBUZ_EN */
}

/***********************************************************************************************************************
 * 函数功能    : 任意按键按下回调
 * 说明(备注)  : 任意按键按下时清空系统休眠倒计时
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_key_on_any_press(void)
{
	tSysInfo.usNeedSleepCnt = 0;
}

#endif  /* boardKEY_EN */
