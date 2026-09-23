/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : board_config.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 板级硬件外设配置、系统参数初始化与任务创建
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "main.h"
#include "gpio_init.h"

#include "Sys/sys_task.h"
#include "Led/led_task.h"
#include "Flash/flash_iface.h"

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  /* boardADC_EN */

#if (boardKEY_EN)
#include "Key/key_task.h"
#endif  /* boardKEY_EN */

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

#if (boardCM_BACKTRACE)
#include <cm_backtrace.h>
#endif  /* boardCM_BACKTRACE */

#if (boardEASY_LOGGER)
#include "elog_init.h"
#endif  /* boardEASY_LOGGER */

#if (boardEASY_FLASH)
#include "easyflash.h"
#endif  /* boardEASY_FLASH */

#if (boardWDGT_EN)
#include "fwdgt.h"
#endif  /* boardWDGT_EN */

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#include "Print/print_iface.h"
#endif  /* boardPRINT_IFACE */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  /* boardDISPLAY_EN */

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  /* boardBMS_EN */

#if (boardSEGGER)
#include "SEGGER_RTT.h"
#endif  /* boardSEGGER */

//****************************************************Types*********************************************************************//
/* 任务创建函数项类型定义 */
typedef struct
{
	const char			*p_name;			/* 任务名称字符串 */
	s8					(*p_fn_init)(void);	/* 任务初始化函数指针 (>0: 成功, <=0: 失败) */
	s16					s_base_code;		/* 任务基准错误码偏移 (0, -10, -20, ...) */
	bool				b_critical;			/* 关键任务标志 (true: 失败中断并断言, false: 容错继续) */
	void				(*p_fn_on_fail)(s16 s_err_code);	/* 专属失败回调 (可为 NULL) */
}TaskInitEntry_T;

/* 任务创建故障诊断监控变量 (供调试器 Watch 窗口实时排查) */
s8                 c_ret                = 0;    /* 任务初始化返回值 (保持向下兼容) */
static const char *s_p_failed_task_name = NULL; /* 首个故障任务名称 */
static s16         s_s_failed_err_code  = 0;    /* 首个故障综合错误码 (Base + 内部步骤码) */
static uint32_t    s_ul_free_heap_bytes = 0;    /* 故障发生时 FreeRTOS 剩余堆大小 (字节) */

//****************************************************Function Declaration******************************************************//
static void SysParamInit(void);

#if (boardPRINT_IFACE)
static void v_print_init_fail_cb(s16 s_err_code);
#endif  /* boardPRINT_IFACE */

/* 表驱动式任务注册表 (统一配置任务函数、基准错误码与关键级别) */
static const TaskInitEntry_T c_task_init_table[] = {
	/* 任务名称       初始化函数        基准错误码  是否关键任务  失败回调 */
	{"SysTask",      cSys_TaskInit,          0,         true,        NULL},
	#if (boardPRINT_IFACE)
	{"PrintTask",    cPrint_TaskInit,        -10,       false,       v_print_init_fail_cb},
	#endif  /* boardPRINT_IFACE */
	#if (boardADC_EN)
	{"AdcTask",      cAdc_TaskInit,          -20,       true,        NULL},
	#endif  /* boardADC_EN */
	#if (boardKEY_EN)
	{"KeyTask",      cKey_TaskInit,          -30,       true,        NULL},
	#endif  /* boardKEY_EN */
	#if (boardLED_EN)
	{"LedTask",      cLed_TaskInit,          -40,       false,       NULL},
	#endif  /* boardLED_EN */
	#if (boardBMS_EN)
	{"BmsTask",      cBms_TaskInit,          -50,       true,        NULL},
	{"BmsRecTask",   cBms_RecTaskInit,       -60,       true,        NULL},
	#endif  /* boardBMS_EN */
	#if (boardDISPLAY_EN)
	{"DispTask",     cDisp_TaskInit,         -70,       true,        NULL},
	#endif  /* boardDISPLAY_EN */
};

/***********************************************************************************************************************
 * 函数功能    : 系统参数与调试打印标志初始化
 * 说明(备注)  : 配置看门狗并使能调试打印类型标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void SysParamInit(void)
{
	#if (boardWDGT_EN)
	vFwdgt_Init();
	#endif  /* boardWDGT_EN */
	
	#if (boardPRINT_IFACE)
	uPrint.tFlag.bImportant = 1;
	uPrint.tFlag.bBootInfo  = 1;
	uPrint.tFlag.bSysTask   = 1;
	uPrint.tFlag.bKeyTask   = 1;
	uPrint.tFlag.bAdcTask   = 0;
	uPrint.tFlag.bOperFlash = 0;
	#endif  /* boardPRINT_IFACE */
}

/***********************************************************************************************************************
 * 函数功能    : 板级系统外设底层初始化
 * 说明(备注)  : 依序初始化GPIO、Flash、串口驱动及第三方中间件
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vBoard_SysInit(void)
{
	vGPIO_Init();
	
	#if (boardDISPLAY_EN)
	#endif  /* boardDISPLAY_EN */
	
	bFlash_IfaceInit();
	
	#if (boardPRINT_IFACE)
	vPrint_Init();
	#endif  /* boardPRINT_IFACE */
	
	#if (boardSEGGER)
	SEGGER_RTT_Init();      //初始化Seeger RTT 输出 
	SEGGER_SYSVIEW_Conf();  //SystemView 初始化
	#endif  /* boardSEGGER */
	
	#if (boardCM_BACKTRACE)
	cm_backtrace_init("APP", boardHARDWARE_VERSION, boardSOFTWARE_VERSION);
	#endif  /* boardCM_BACKTRACE */
	
	#if (boardEASY_LOGGER)
	vElog_Init();
	#endif  /* boardEASY_LOGGER */
	
	#if (boardEASY_FLASH)
	EfErrCode ret = easyflash_init();
	if (ret != EF_NO_ERR)
	{
		#if (boardPRINT_IFACE)
		sMyPrint("EasyFlash init fail, EfErrCode = %d\r\n", ret);
		#endif  /* boardPRINT_IFACE */
	}
	#endif  /* boardEASY_FLASH */
	
	#if (boardUPDATE)
	bUpdate_Init();
	#endif  /* boardUPDATE */
	
	SysParamInit();        //初始化系统参数
}

/***********************************************************************************************************************
 * 函数功能    : 系统任务初始化与启动
 * 说明(备注)  : 表驱动统一遍历初始化各任务，自动执行故障诊断、现场保存、断言与容错分发
 * 传入参数    : pvParameters: 任务入参指针
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vBoard_StartTask(void *pvParameters)
{
	mainENTER_CRITICAL();

	for (size_t i = 0; i < sizeof(c_task_init_table) / sizeof(c_task_init_table[0]); i++)
	{
		const TaskInitEntry_T *p_entry = &c_task_init_table[i];
		s8 ret = p_entry->p_fn_init();

		if (ret <= 0)
		{
			s16 s_err_code = p_entry->s_base_code + ret;

			/* 记录首个故障任务信息至监控变量 (便于仿真器 Watch 窗口排查) */
			if (s_p_failed_task_name == NULL)
			{
				s_p_failed_task_name = p_entry->p_name;
				s_s_failed_err_code  = s_err_code;
				#if (boardUSE_OS)
				s_ul_free_heap_bytes = (uint32_t)xPortGetFreeHeapSize();
				#endif  /* boardUSE_OS */
			}
			c_ret = (s8)s_err_code;

			/* 标记系统任务故障错误 */
			tSysInfo.uErrCode.tCode.bTaskFault = 1;

			#if (boardPRINT_IFACE)
			printf("\r\n[ERROR] Task init failed! Task: %s, Err: %d (ret: %d), FreeHeap: %u B\r\n",
			       p_entry->p_name, (int)s_err_code, (int)ret, (unsigned int)s_ul_free_heap_bytes);
			#endif  /* boardPRINT_IFACE */

			#if (boardSEGGER)
			SEGGER_RTT_printf(0, "\r\n[ERROR] Task init failed! Task: %s, Err: %d (ret: %d), FreeHeap: %u B\r\n",
			                  p_entry->p_name, (int)s_err_code, (int)ret, (unsigned int)s_ul_free_heap_bytes);
			#endif  /* boardSEGGER */

			/* 执行任务专属失败回调 (如关闭打印标志位等) */
			if (p_entry->p_fn_on_fail != NULL)
				p_entry->p_fn_on_fail(s_err_code);

			/* 关键任务创建失败: 触发系统断言并挂起，等待看门狗复位 */
			if (p_entry->b_critical)
			{
				#if (boardPRINT_IFACE)
				printf("[FATAL] Critical task failed! System halt.\r\n");
				#endif  /* boardPRINT_IFACE */
				#if (boardSEGGER)
				SEGGER_RTT_printf(0, "[FATAL] Critical task failed! System halt.\r\n");
				#endif  /* boardSEGGER */
				#if (boardUSE_OS)
				configASSERT(0);
				taskDISABLE_INTERRUPTS();
				#else
				__disable_irq();
				#endif  /* boardUSE_OS */
				for (;;)
				{}
			}
		}
	}

	mainEXIT_CRITICAL();

	#if (boardUSE_OS)
	vTaskDelete(NULL);
	#endif  /* boardUSE_OS */
}

#if (boardPRINT_IFACE)
/***********************************************************************************************************************
 * 函数功能    : 打印任务初始化专属失败回调
 * 说明(备注)  : 打印任务初始化失败时关闭所有调试打印输出，避免野指针或未就绪硬件访问
 * 传入参数    : s_err_code: 综合错误码
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void v_print_init_fail_cb(s16 s_err_code)
{
	(void)s_err_code;
	uPrint.ulFlag = 0;	/* 关闭全部调试输出 */
}
#endif  /* boardPRINT_IFACE */

