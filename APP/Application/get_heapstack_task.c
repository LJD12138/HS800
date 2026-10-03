/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : get_heapstack_task.c
 * Date    : 2026-09-23
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统健康巡检与 FreeRTOS 堆栈水位监控实现 (低频心跳 + 异常驱动 + 智能余量判据)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "get_heapstack_task.h"

#if (boardUSE_OS && boardHEALTH_MONITOR_EN)
#include <string.h>
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  //boardMPPT_EN

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			HEALTH_TASK_PRIO						1		/* 任务优先级(后台级) */
#define			HEALTH_TASK_STK_SIZE					320		/* 任务堆栈(字) 1280B: 容纳 uxTaskGetSystemState 与格式化 */
TaskHandle_t	tHealthTaskHandler = NULL;							/* 任务句柄 */
static void v_health_task(void *pvParameters);					/* 任务函数 */
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			HEALTH_POLL_CYCLE_MS					2000	/* 后台静默采样巡检周期(ms) */
#define			HEALTH_HEARTBEAT_CYCLES					30		/* 心跳输出周期(30 * 2s = 60s) */
#define			HEALTH_ALARM_COOLDOWN_CYCLES			5		/* 报警冷却限频周期(5 * 2s = 10s) */
#define			HEALTH_STABLE_CYCLES_FOR_WASTE			60		/* 稳定评估周期(60 * 2s = 120s) */
#define			HEALTH_WASTE_REPORT_CYCLES				150		/* 余量过多优化建议周期(150 * 2s = 300s) */

#define			HEALTH_STACK_CRIT_WORDS					32		/* 栈严重危险阈值(字, <128B) */
#define			HEALTH_STACK_WARN_WORDS					64		/* 栈警戒阈值(字, <256B) */
#define			HEALTH_STACK_WASTE_WORDS				256		/* 栈余量过大阈值(字, >1024B 可优化裁剪) */

#define			HEALTH_HEAP_CRIT_BYTES					2048	/* 堆严重危险阈值(B, <2KB) */
#define			HEALTH_HEAP_WARN_BYTES					4096	/* 堆警戒阈值(B, <4KB) */
#define			HEALTH_HEAP_WASTE_RATIO					60		/* 堆余量过大比例(%, 历史最小仍>60%总堆) */

#define			HEALTH_CPU_WARN_PCT						85		/* CPU 高负载警告阈值(%) */

//****************************************************Parameter Initialization**************************************************//
volatile uint32_t ulHealthIdleLoopCnt = 0;

/* 任务状态静态数组: 共享单一缓冲区，避免多处分配吃任务栈与 BSS (节省 1KB SRAM) */
static TaskStatus_t s_task_status[24];

#if (boardUSE_OS && boardPRINT_IFACE && configGENERATE_RUN_TIME_STATS && configUSE_STATS_FORMATTING_FUNCTIONS)
static char s_ca_rt_stats[1024]; /* vTaskGetRunTimeStats 输出缓冲: 仅用于全量体检报表按需调试 */
#endif  //boardUSE_OS && boardPRINT_IFACE && configGENERATE_RUN_TIME_STATS && configUSE_STATS_FORMATTING_FUNCTIONS

//****************************************************Function Declaration******************************************************//
static void                   v_health_report_summary(uint32_t ul_window_cpu, uint32_t ul_cum_cpu, bool b_alarm_active);
static void                   v_health_check_single_queue(const Task_T *p_task, const char *pc_name, uint16_t *pus_last_full, uint16_t *pus_last_corrupt);
static void                   v_health_check_queue_alarms(void);
static bool                   b_health_check_alarms(bool b_force_alert);
static void                   v_health_check_waste_advisory(void);
static uint32_t               ul_health_cpu_by_run_time(uint32_t *pul_window_cpu, uint32_t *pul_cum_cpu);
static uint32_t               ul_health_cpu_by_idle_cnt(void);
static configSTACK_DEPTH_TYPE ul_health_worst_stack(const char **ppc_name);
void 						  v_print_heapstack_task_watermark (void);

/***********************************************************************************************************************
 * 函数功能    : 健康巡检任务初始化
 * 说明(备注)  : 创建后台健康巡检任务(由 vBoard_StartTask 装配调用)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 任务创建失败
 ************************************************************************************************************************/
s8 cHealth_TaskInit(void)
{
	BaseType_t x_ret = xTaskCreate((TaskFunction_t)v_health_task,
	                               (const char *)"HealthTask",
	                               (uint16_t)HEALTH_TASK_STK_SIZE,
	                               (void *)NULL,
	                               (UBaseType_t)HEALTH_TASK_PRIO,
	                               (TaskHandle_t *)&tHealthTaskHandler);

	return (x_ret == pdPASS) ? 1 : -1;
}

/***********************************************************************************************************************
 * 函数功能    : 健康巡检任务主循环
 * 说明(备注)  : 2s 周期静默采样巡检; 仅在有异常/优化建议或低频心跳时输出
 * 传入参数    : pvParameters: 任务入参(未使用)
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_health_task(void *pvParameters)
{
	(void)pvParameters;
	static uint32_t s_ul_cycle_cnt = 0;
	static uint32_t s_ul_waste_cycle_cnt = 0;

	for (;;)
	{
		vTaskDelay(HEALTH_POLL_CYCLE_MS);

		/* 空闲计数采样必须在唤醒后立即进行:
		 * 该增量覆盖的是刚过去的整个睡眠窗口(IDLE 真实运行期) */
		(void)ul_health_cpu_by_idle_cnt();

		#if (boardPRINT_IFACE)
		if (uPrint.tFlag.bHealthTask)
		{
			s_ul_cycle_cnt++;

			/* 1. 队列满队/异常自愈边沿巡检 (有异常才输出, 正常完全静默) */
			v_health_check_queue_alarms();

			/* 2. 栈与堆余量太少告警巡检 (低于警戒线才输出, 带冷却防抖) */
			bool b_has_alarm = b_health_check_alarms(false);

			/* 3. CPU 占用率计算与高负载告警巡检 */
			uint32_t ul_window_cpu = 0;
			uint32_t ul_cum_cpu = 0;
			(void)ul_health_cpu_by_run_time(&ul_window_cpu, &ul_cum_cpu);
			if (ul_window_cpu >= HEALTH_CPU_WARN_PCT)
			{
				static uint16_t s_us_cpu_alarm_cooldown = 0;
				if (s_us_cpu_alarm_cooldown == 0)
				{
					sMyPrint("[HEALTH-ALERT][CPU HIGH] CPU window load high: %u%%!\r\n", (unsigned int)ul_window_cpu);
					s_us_cpu_alarm_cooldown = HEALTH_ALARM_COOLDOWN_CYCLES;
				}
				else
					s_us_cpu_alarm_cooldown--;
			}

			/* 4. 余量过多评估 (开机稳定 120s 后的首个周期输出, 之后每 300s 输出一次) */
			if (s_ul_cycle_cnt >= HEALTH_STABLE_CYCLES_FOR_WASTE)
			{
				if (s_ul_waste_cycle_cnt == 0)
				{
					v_health_check_waste_advisory();
					s_ul_waste_cycle_cnt = HEALTH_WASTE_REPORT_CYCLES;
				}
				else
					s_ul_waste_cycle_cnt--;
			}

			/* 5. 常规心跳输出 (每 60s 输出一行极简心跳, 避免刷屏) */
			if ((s_ul_cycle_cnt % HEALTH_HEARTBEAT_CYCLES) == 0)
				v_health_report_summary(ul_window_cpu, ul_cum_cpu, b_has_alarm);
		}
		#endif  //boardPRINT_IFACE
	}
}

#if (boardPRINT_IFACE)
/***********************************************************************************************************************
 * 函数功能    : 输出极简健康心跳行
 * 说明(备注)  : 低频周期输出(默认 60s), 仅一行紧凑摘要
 * 传入参数    : ul_window_cpu: 窗口 CPU 占用率(%)
 *               ul_cum_cpu: 自启动累计 CPU 占用率(%)
 *               b_alarm_active: 当前是否存在活跃告警
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_health_report_summary(uint32_t ul_window_cpu, uint32_t ul_cum_cpu, bool b_alarm_active)
{
	const char *pc_worst_name = "?";
	configSTACK_DEPTH_TYPE ul_worst_hwm = 0;

	/* 堆内存水位 */
	size_t st_free_heap = xPortGetFreeHeapSize();
	size_t st_min_heap = xPortGetMinimumEverFreeHeapSize();

	/* 最差任务栈水位 */
	ul_worst_hwm = ul_health_worst_stack(&pc_worst_name);

	sMyPrint("[HEALTH] %s | CPU:%u%%(win) %u%%(cum) | Heap:%uB/%uKB(min:%uB) | WorstStk:%s(%uw)\r\n",
	         b_alarm_active ? "WARN" : "OK",
	         (unsigned int)ul_window_cpu,
	         (unsigned int)ul_cum_cpu,
	         (unsigned int)st_free_heap,
	         (unsigned int)(configTOTAL_HEAP_SIZE / 1024),
	         (unsigned int)st_min_heap,
	         pc_worst_name,
	         (unsigned int)ul_worst_hwm);
}

/***********************************************************************************************************************
 * 函数功能    : 检查单个任务队列的异常事件
 * 说明(备注)  : 仅在满队丢包或自愈损坏事件计数增加时触发打印，正常完全静默
 * 传入参数    : p_task: 队列任务指针
 *               pc_name: 队列名称
 *               pus_last_full: 上次记录的满队计数
 *               pus_last_corrupt: 上次记录的损坏计数
 * 输出参数    : pus_last_full, pus_last_corrupt 更新为最新计数值
 * 返回值      : void
 ************************************************************************************************************************/
static void v_health_check_single_queue(const Task_T *p_task, const char *pc_name, uint16_t *pus_last_full, uint16_t *pus_last_corrupt)
{
	if (p_task == NULL || pus_last_full == NULL || pus_last_corrupt == NULL)
		return;

	TaskTelemetry_T *tp_tel = tpQueue_GetTelemetry(p_task);
	if (tp_tel == NULL)
		return;

	if (tp_tel->usFullEvtCnt > *pus_last_full)
	{
		sMyPrint("[HEALTH-ALERT][QUEUE FULL] %s: full events +%u (total %u, peak %u)!\r\n",
		         pc_name,
		         (unsigned int)(tp_tel->usFullEvtCnt - *pus_last_full),
		         (unsigned int)tp_tel->usFullEvtCnt,
		         (unsigned int)tp_tel->usPeakItems);
		*pus_last_full = tp_tel->usFullEvtCnt;
	}

	if (tp_tel->usCorruptEvtCnt > *pus_last_corrupt)
	{
		sMyPrint("[HEALTH-ALERT][QUEUE CORRUPT] %s: corrupt events +%u (total %u)!\r\n",
		         pc_name,
		         (unsigned int)(tp_tel->usCorruptEvtCnt - *pus_last_corrupt),
		         (unsigned int)tp_tel->usCorruptEvtCnt);
		*pus_last_corrupt = tp_tel->usCorruptEvtCnt;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 巡检所有任务队列异常事件
 * 说明(备注)  : 遍历各模块任务队列，边沿检测丢包/损坏事件
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_health_check_queue_alarms(void)
{
	static uint16_t s_us_sys_full = 0;
	static uint16_t s_us_sys_corrupt = 0;
	#if (boardBMS_EN)
	static uint16_t s_us_bms_full = 0;
	static uint16_t s_us_bms_corrupt = 0;
	#endif  //boardBMS_EN
	#if (boardDCAC_EN)
	static uint16_t s_us_dcac_full = 0;
	static uint16_t s_us_dcac_corrupt = 0;
	#endif  //boardDCAC_EN
	#if (boardMPPT_EN)
	static uint16_t s_us_mppt_full = 0;
	static uint16_t s_us_mppt_corrupt = 0;
	#endif  //boardMPPT_EN
	#if (boardUSB_EN)
	static uint16_t s_us_usb_full = 0;
	static uint16_t s_us_usb_corrupt = 0;
	#endif  //boardUSB_EN
	static uint16_t s_us_prt_full = 0;
	static uint16_t s_us_prt_corrupt = 0;

	v_health_check_single_queue(tpSysTask, "Sys", &s_us_sys_full, &s_us_sys_corrupt);

	#if (boardBMS_EN)
	v_health_check_single_queue(tpBmsTask, "Bms", &s_us_bms_full, &s_us_bms_corrupt);
	#endif  //boardBMS_EN

	#if (boardDCAC_EN)
	v_health_check_single_queue(tpDcacTask, "Dcac", &s_us_dcac_full, &s_us_dcac_corrupt);
	#endif  //boardDCAC_EN

	#if (boardMPPT_EN)
	v_health_check_single_queue(tpMpptTask, "Mppt", &s_us_mppt_full, &s_us_mppt_corrupt);
	#endif  //boardMPPT_EN

	#if (boardUSB_EN)
	v_health_check_single_queue(tpUsbTask, "Usb", &s_us_usb_full, &s_us_usb_corrupt);
	#endif  //boardUSB_EN

	v_health_check_single_queue(tpPrintTask, "Prt", &s_us_prt_full, &s_us_prt_corrupt);
}

/***********************************************************************************************************************
 * 函数功能    : 任务栈与堆内存余量太少告警巡检
 * 说明(备注)  : 仅当任务剩余栈低于警戒线或堆内存危急时输出告警，带防抖冷却周期
 * 传入参数    : b_force_alert: true=忽略冷却强制检测输出; false=遵循冷却周期
 * 输出参数    : 无
 * 返回值      : bool: true 存在异常; false 正常
 ************************************************************************************************************************/
static bool b_health_check_alarms(bool b_force_alert)
{
	static uint16_t s_us_alarm_cooldown = 0;
	bool b_has_alarm = false;
	uint32_t ul_total = 0;

	if (s_us_alarm_cooldown > 0 && !b_force_alert)
	{
		s_us_alarm_cooldown--;
		return false;
	}

	UBaseType_t ux_num = uxTaskGetSystemState(s_task_status,
	                                          sizeof(s_task_status) / sizeof(s_task_status[0]),
	                                          &ul_total);

	for (UBaseType_t i = 0; i < ux_num; i++)
	{
		if (s_task_status[i].eCurrentState == eDeleted)
			continue;

		if (s_task_status[i].usStackHighWaterMark < HEALTH_STACK_CRIT_WORDS)
		{
			b_has_alarm = true;
			sMyPrint("[HEALTH-ALERT][STACK CRIT] Task '%s': min free only %u words (%u B)! Risk of overflow!\r\n",
			         s_task_status[i].pcTaskName,
			         (unsigned int)s_task_status[i].usStackHighWaterMark,
			         (unsigned int)(s_task_status[i].usStackHighWaterMark * sizeof(StackType_t)));
			vTaskDelay(pdMS_TO_TICKS(10));
		}
		else if (s_task_status[i].usStackHighWaterMark < HEALTH_STACK_WARN_WORDS)
		{
			b_has_alarm = true;
			sMyPrint("[HEALTH-ALERT][STACK WARN] Task '%s': min free only %u words (%u B)!\r\n",
			         s_task_status[i].pcTaskName,
			         (unsigned int)s_task_status[i].usStackHighWaterMark,
			         (unsigned int)(s_task_status[i].usStackHighWaterMark * sizeof(StackType_t)));
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}

	/* 堆内存水位检查 */
	size_t st_min_heap = xPortGetMinimumEverFreeHeapSize();
	if (st_min_heap < HEALTH_HEAP_CRIT_BYTES)
	{
		b_has_alarm = true;
		sMyPrint("[HEALTH-ALERT][HEAP CRIT] Min ever free heap only %u B (< %u B)! Memory near exhaustion!\r\n",
		         (unsigned int)st_min_heap,
		         (unsigned int)HEALTH_HEAP_CRIT_BYTES);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
	else if (st_min_heap < HEALTH_HEAP_WARN_BYTES)
	{
		b_has_alarm = true;
		sMyPrint("[HEALTH-ALERT][HEAP WARN] Min ever free heap only %u B (< %u B)!\r\n",
		         (unsigned int)st_min_heap,
		         (unsigned int)HEALTH_HEAP_WARN_BYTES);
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	if (b_has_alarm)
		s_us_alarm_cooldown = HEALTH_ALARM_COOLDOWN_CYCLES;

	return b_has_alarm;
}

/***********************************************************************************************************************
 * 函数功能    : 任务栈与堆内存余量过多评估 (优化裁剪建议)
 * 说明(备注)  : 系统稳定后低频触发，扫描分配过剩的任务栈及总堆，输出建议削减量
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
static void v_health_check_waste_advisory(void)
{
	uint32_t ul_total = 0;
	bool b_found_stack_waste = false;

	UBaseType_t ux_num = uxTaskGetSystemState(s_task_status,
	                                          sizeof(s_task_status) / sizeof(s_task_status[0]),
	                                          &ul_total);

	for (UBaseType_t i = 0; i < ux_num; i++)
	{
		if (s_task_status[i].eCurrentState == eDeleted)
			continue;

		/* 若历史最小剩余栈超过余量过多阈值，说明分配过剩 */
		if (s_task_status[i].usStackHighWaterMark >= HEALTH_STACK_WASTE_WORDS)
		{
			if (!b_found_stack_waste)
			{
				b_found_stack_waste = true;
				sMyPrint("---- [HEALTH-ADVISORY: STACK REDUCTION CANDIDATES] ----\r\n");
				vTaskDelay(pdMS_TO_TICKS(10));
			}

			/* 建议可缩减量: 保留 64 Words 警戒水位, 其余均可裁剪回收 */
			uint32_t ul_suggest_reduce = (s_task_status[i].usStackHighWaterMark > 64) ?
			                             (s_task_status[i].usStackHighWaterMark - 64) : 0;

			sMyPrint("Task '%-16s': min free %u words (%u B), suggest reduce ~%u words\r\n",
			         s_task_status[i].pcTaskName,
			         (unsigned int)s_task_status[i].usStackHighWaterMark,
			         (unsigned int)(s_task_status[i].usStackHighWaterMark * sizeof(StackType_t)),
			         (unsigned int)ul_suggest_reduce);
			vTaskDelay(pdMS_TO_TICKS(10));
		}
	}

	if (b_found_stack_waste)
	{
		sMyPrint("-------------------------------------------------------\r\n");
		vTaskDelay(pdMS_TO_TICKS(10));
	}

	/* 评估总堆是否分配过大 */
	size_t st_min_heap = xPortGetMinimumEverFreeHeapSize();
	if (st_min_heap >= ((size_t)configTOTAL_HEAP_SIZE * HEALTH_HEAP_WASTE_RATIO / 100))
	{
		sMyPrint("[HEALTH-ADVISORY][HEAP WASTE] Ever min free heap is %u B / %u KB (%u%% unused), consider reducing configTOTAL_HEAP_SIZE!\r\n",
		         (unsigned int)st_min_heap,
		         (unsigned int)(configTOTAL_HEAP_SIZE / 1024),
		         (unsigned int)(st_min_heap * 100 / configTOTAL_HEAP_SIZE));
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

/***********************************************************************************************************************
 * 函数功能    : 打印全系统 FreeRTOS 任务栈高水位线与堆内存完整体检报告
 * 说明(备注)  : 量化获取每个任务自启动以来的最小剩余栈深 (High Water Mark)，支持按需调试调用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : void
 ************************************************************************************************************************/
void v_print_heapstack_task_watermark (void)
{
	UBaseType_t ux_task_num;
	uint32_t    ul_total_run_time;
	UBaseType_t i;
	size_t      free_heap;
	size_t      min_free_heap;

	ux_task_num = uxTaskGetSystemState(s_task_status,
	                                   sizeof(s_task_status) / sizeof(s_task_status[0]),
	                                   &ul_total_run_time);

	free_heap = xPortGetFreeHeapSize();
	min_free_heap = xPortGetMinimumEverFreeHeapSize();

	sMyPrint("\r\n================ [FREERTOS TASKS STACK & HEAP AUDIT] ================\r\n");
	vTaskDelay(pdMS_TO_TICKS(10));
	sMyPrint("%-16s | %-5s | %-12s | %-15s | %-11s\r\n", "Task Name", "Prio", "Task State", "Min Free(Words)", "Min Free(B)");
	vTaskDelay(pdMS_TO_TICKS(10));
	sMyPrint("-----------------+-------+--------------+-----------------+------------\r\n");
	vTaskDelay(pdMS_TO_TICKS(10));

	for (i = 0; i < ux_task_num; i++)
	{
		const char *state_str = "Unknown";
		switch (s_task_status[i].eCurrentState)
		{
			case eRunning:
			{
				state_str = "Running";
			}
			break;

			case eReady:
			{
				state_str = "Ready";
			}
			break;

			case eBlocked:
			{
				state_str = "Blocked";
			}
			break;

			case eSuspended:
			{
				state_str = "Suspended";
			}
			break;

			case eDeleted:
			{
				state_str = "Deleted";
			}
			break;

			default:
			{
			}
			break;
		}

		sMyPrint("%-16s | %-5u | %-12s | %-15u | %-11u\r\n",
		         s_task_status[i].pcTaskName,
		         (unsigned int)s_task_status[i].uxCurrentPriority,
		         state_str,
		         (unsigned int)s_task_status[i].usStackHighWaterMark,
		         (unsigned int)(s_task_status[i].usStackHighWaterMark * sizeof(StackType_t)));

		vTaskDelay(pdMS_TO_TICKS(10));
	}

	sMyPrint("----------------------------------------------------------------------\r\n");
	vTaskDelay(pdMS_TO_TICKS(10));
	sMyPrint("Total Active Tasks     : %u\r\n", (unsigned int)ux_task_num);
	vTaskDelay(pdMS_TO_TICKS(10));
	sMyPrint("Current Free Heap      : %u Bytes (%.2f KB / %u KB)\r\n",
	         (unsigned int)free_heap, (float)free_heap / 1024.0f, (unsigned int)(configTOTAL_HEAP_SIZE / 1024));
	vTaskDelay(pdMS_TO_TICKS(10));
	sMyPrint("Minimum Ever Free Heap : %u Bytes (%.2f KB)\r\n",
	         (unsigned int)min_free_heap, (float)min_free_heap / 1024.0f);
	vTaskDelay(pdMS_TO_TICKS(10));
	sMyPrint("======================================================================\r\n\r\n");
	vTaskDelay(pdMS_TO_TICKS(10));

	#if (configGENERATE_RUN_TIME_STATS && configUSE_STATS_FORMATTING_FUNCTIONS)
	vTaskGetRunTimeStats(s_ca_rt_stats);

	sMyPrint("=========== [RUN TIME STATS] ============\r\n");
	{
		char *pc_line = s_ca_rt_stats;
		char *pc_nl;

		while ((pc_nl = strchr(pc_line, '\n')) != NULL)
		{
			if (pc_nl > pc_line && *(pc_nl - 1) == '\r')
				*(pc_nl - 1) = '\0';
			else
				*pc_nl = '\0';

			sMyPrint("%s\r\n", pc_line);
			vTaskDelay(pdMS_TO_TICKS(10));
			pc_line = pc_nl + 1;
		}
		if (*pc_line != '\0')
			sMyPrint("%s\r\n", pc_line);
	}
	sMyPrint("=========================================\r\n");
	#endif  //configGENERATE_RUN_TIME_STATS && configUSE_STATS_FORMATTING_FUNCTIONS
}

#endif  //boardPRINT_IFACE

/***********************************************************************************************************************
 * 函数功能    : RUN_TIME_STATS 法 CPU 占用率计算
 * 说明(备注)  : 基于 uxTaskGetSystemState 的 IDLE 任务运行时间占比, 输出窗口值与累计值
 * 传入参数    : pul_window_cpu: 输出窗口 CPU 占用率(%); pul_cum_cpu: 输出自启动累计 CPU 占用率(%)
 * 输出参数    : pul_window_cpu, pul_cum_cpu
 * 返回值      : uint32_t: 空闲率(%)
 ************************************************************************************************************************/
static uint32_t ul_health_cpu_by_run_time(uint32_t *pul_window_cpu, uint32_t *pul_cum_cpu)
{
	static uint32_t s_ul_idle_rt_last = 0;
	static uint32_t s_ul_total_rt_last = 0;

	uint32_t ul_total = 0;
	uint32_t ul_idle_rt = 0;
	uint32_t ul_idle_pct = 0;

	UBaseType_t ux_num = uxTaskGetSystemState(s_task_status,
	                                          sizeof(s_task_status) / sizeof(s_task_status[0]),
	                                          &ul_total);
	for (UBaseType_t i = 0; i < ux_num; i++)
	{
		if (strcmp(s_task_status[i].pcTaskName, "IDLE") == 0)
		{
			ul_idle_rt = s_task_status[i].ulRunTimeCounter;
			break;
		}
	}

	if (ul_total > 0)
	{
		uint64_t ull_idle_pct = ((uint64_t)ul_idle_rt * 100) / ul_total;
		ul_idle_pct = (uint32_t)ull_idle_pct;

		if (pul_cum_cpu != NULL)
			*pul_cum_cpu = 100 - (uint32_t)ull_idle_pct;

		/* 窗口差分(自启动首个周期退化为累计值) */
		uint32_t ul_idle_d = ul_idle_rt - s_ul_idle_rt_last;
		uint32_t ul_total_d = ul_total - s_ul_total_rt_last;
		if (pul_window_cpu != NULL)
		{
			if (ul_total_d > 0)
				*pul_window_cpu = 100 - (uint32_t)(((uint64_t)ul_idle_d * 100) / ul_total_d);
			else
				*pul_window_cpu = 0;
		}
	}

	s_ul_idle_rt_last = ul_idle_rt;
	s_ul_total_rt_last = ul_total;

	return ul_idle_pct;
}

/***********************************************************************************************************************
 * 函数功能    : Idle 空闲计数器法 CPU 占用率计算
 * 说明(备注)  : 采样 vApplicationIdleHook 计数器增量, 以历史最大增量自学习 100% 空闲基准
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : uint32_t: 窗口 CPU 占用率(%)
 ************************************************************************************************************************/
static uint32_t ul_health_cpu_by_idle_cnt(void)
{
	static uint32_t s_ul_idle_last = 0;
	static uint32_t s_ul_idle_max_delta = 0;

	uint32_t ul_now = ulHealthIdleLoopCnt;
	uint32_t ul_delta = ul_now - s_ul_idle_last;
	s_ul_idle_last = ul_now;

	if (ul_delta > s_ul_idle_max_delta)
		s_ul_idle_max_delta = ul_delta;

	return (s_ul_idle_max_delta > 0) ? (100 - ul_delta * 100 / s_ul_idle_max_delta) : 0;
}

/***********************************************************************************************************************
 * 函数功能    : 查找栈水位最差的任务
 * 说明(备注)  : 遍历任务状态数组, 返回最小剩余栈(高水位线, 单位: 字)
 * 传入参数    : ppc_name: 输出最差任务名指针
 * 输出参数    : ppc_name: 任务名称
 * 返回值      : configSTACK_DEPTH_TYPE: 最小剩余栈字数
 ************************************************************************************************************************/
static configSTACK_DEPTH_TYPE ul_health_worst_stack(const char **ppc_name)
{
	uint32_t ul_total = 0;
	configSTACK_DEPTH_TYPE ul_worst = 0xFFFFFFFF;
	const char *pc_worst = "?";

	UBaseType_t ux_num = uxTaskGetSystemState(s_task_status,
	                                          sizeof(s_task_status) / sizeof(s_task_status[0]),
	                                          &ul_total);
	for (UBaseType_t i = 0; i < ux_num; i++)
	{
		if (s_task_status[i].eCurrentState == eDeleted)
			continue;

		if (s_task_status[i].usStackHighWaterMark < ul_worst)
		{
			ul_worst = s_task_status[i].usStackHighWaterMark;
			pc_worst = s_task_status[i].pcTaskName;
		}
	}

	if (ppc_name != NULL)
		*ppc_name = pc_worst;

	return ul_worst;
}

#endif  //boardUSE_OS && boardHEALTH_MONITOR_EN
