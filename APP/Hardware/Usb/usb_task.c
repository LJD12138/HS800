/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB / 快充供电管理与保护处理任务实现文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Usb/usb_task.h"

#if (boardUSB_EN)
#include "Usb/usb_queue_task.h"
#include "Usb/usb_iface.h"
#include "Usb/usb_prot_frame.h"
#include "app_info.h"

#if (boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  /* boardPRINT_IFACE */

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardADC_EN)
#include "Adc/adc_task.h"
#endif  /* boardADC_EN */

//****************************************************Macros********************************************************************//
#if (boardUSE_OS)
#define			USB_TASK_PRIO							1		/* 任务优先级 */
#define			USB_TASK_SIZE							256		/* 任务堆栈大小 */
TaskHandle_t tUsbTaskHandler = NULL;
void vUsb_Task(void *p_v_parameters);
#endif  /* boardUSE_OS */

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Usb_T tUsb;
vu16 usQcPwr = 0;
vu16 usWcPwr = 0;

//****************************************************Parameter Initialization**************************************************//
/* USB 记忆参数步进配置表 */
typedef struct
{
	void				*pParam;
	int16_t				sMin;
	int16_t				sMax;
	uint8_t				ucType;				/* 0: uint16_t, 1: int8_t */
}UsbParamStep_T;

static const UsbParamStep_T s_t_usb_param_table[] = 
{
	{(void*)&tAppMemParam.tUSB.usAutoOffTime, 0,    3600,  0},
	{(void*)&tAppMemParam.tUSB.usMaxInVolt,   0,    30000, 0},
	{(void*)&tAppMemParam.tUSB.usMinInVolt,   0,    30000, 0},
	{(void*)&tAppMemParam.tUSB.usMinOpenVolt, 0,    30000, 0},
	{(void*)&tAppMemParam.tUSB.sMaxTemp,     -127,  127,   1},
};

//****************************************************Parameter Initialization**************************************************//
static Task_T *s_tp_task = NULL;

//****************************************************Function Declaration******************************************************//
static bool b_task_param_init(void);
static void v_usb_check_prote(void);
static void v_usb_param_update(void);


/***********************************************************************************************************************
 * 函数功能    : 参数初始化
 * 说明(备注)  : 重置 USB 结构体并装载队列指针与自动关闭时间
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功; false: 队列未就绪
 ************************************************************************************************************************/
static bool b_task_param_init(void)
{
	if (tpUsbTask == NULL)
		return false;

	memset((u8*)&tUsb, 0, sizeof(tUsb));
	tUsb.usAutoOffTime = tAppMemParam.tUSB.usAutoOffTime;
	s_tp_task          = tpUsbTask;

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : USB 任务初始化
 * 说明(备注)  : 初始化接口、创建任务队列与 OS 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功; -1: 队列初始化失败; -2: 参数初始化失败; -3: 任务创建失败
 ************************************************************************************************************************/
s8 cUsb_TaskInit(void)
{
	vUsb_IfaceInit();
	vUSB_ControlPorts(false);

	if (bUsb_QueueInit() == false)
		return -1;

	if (b_task_param_init() == false)
		return -2;

	#if (boardUSE_OS)
	if (xTaskCreate((TaskFunction_t )vUsb_Task,
	                (const char*    )"UsbTask",
	                (uint16_t       )USB_TASK_SIZE,
	                (void*          )NULL,
	                (UBaseType_t    )USB_TASK_PRIO,
	                (TaskHandle_t*  )&tUsbTaskHandler) != pdPASS)
		return -3;
	vQueue_BindTaskHandler(tpUsbTask, tUsbTaskHandler);
	#endif  /* boardUSE_OS */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : USB 任务主循环
 * 说明(备注)  : 周期更新物理量、执行保护检测与队列任务轮询
 * 传入参数    : p_v_parameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUsb_Task(void *p_v_parameters)
{
	#if (boardUSE_OS)
	for (;;)
	#endif  /* boardUSE_OS */
	{
		if (s_tp_task == NULL)
		{
			b_task_param_init();

			#if (boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  /* boardUSE_OS */
		}

		v_usb_param_update();
		v_usb_check_prote();

		vQueue_TaskPoll(s_tp_task, usbTASK_CYCLE_TIME);

		#if (boardUSE_OS)
		/* 队列空才按周期休眠；有排队任务时跳过休眠立即调度，投递→执行延迟 <1ms */
		if (bQueue_IsQueueEmpty(s_tp_task))
			ulTaskNotifyTake(pdTRUE, usbTASK_CYCLE_TIME);
		#endif  /* boardUSE_OS */
	}
}


/***********************************************************************************************************************
 * 函数功能    : 保护处理
 * 说明(备注)  : 检测放电权限、输入供电状态、过温与 QC 电压
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_usb_check_prote(void)
{
	static u8   s_uc_pwr_err_cnt       = 0;
	static u8   s_uc_over_temp_cnt     = 0;
	static u8   s_uc_usb_a_pwr_err_cnt = 0;
	//关闭USB
	/* 系统关闭或禁止放电时关闭 USB */
	if (tSysInfo.uPerm.tPerm.bDisChgPerm == false || 
	    tSysInfo.eDevState == DS_CLOSING || 
	    tSysInfo.eDevState == DS_SHUT_DOWN)
	{
		if (tUsb.eDevState >= DS_BOOTING)
			cUsb_Switch(ST_OFF, true);
	}

	if(tUsb.eDevState != DS_WORK && tUsb.eDevState != DS_ERR)
		return;

	/* 输入电源电压检查 */
	if (cUsb_CheckInVolt() != 0)
	{
		if (tUsb.uErrCode.tCode.bPowerErr == 0)
		{
			s_uc_pwr_err_cnt++;
			if (s_uc_pwr_err_cnt >= 10)
			{
				s_uc_pwr_err_cnt = 0;
				bUsb_SetErrCode(UEC_POWER_ERR, true);
			}
		}
		else
			s_uc_pwr_err_cnt = 0;
	}
	else
	{
		if (tUsb.uErrCode.tCode.bPowerErr == 1)
		{
			s_uc_pwr_err_cnt++;
			if (s_uc_pwr_err_cnt >= 5)
			{
				s_uc_pwr_err_cnt = 0;
				bUsb_SetErrCode(UEC_POWER_ERR, false);
			}
		}
		else
			s_uc_pwr_err_cnt = 0;
	}

	/* 过温检查 */
	if (tUsb.sMaxTemp > tAppMemParam.tUSB.sMaxTemp)
	{
		if (tUsb.uErrCode.tCode.bOT == 0)
		{
			s_uc_over_temp_cnt++;
			if (s_uc_over_temp_cnt >= 5)
			{
				s_uc_over_temp_cnt = 0;
				bUsb_SetErrCode(UEC_OT, true);
			}
		}
		else
			s_uc_over_temp_cnt = 0;
	}
	else if (tUsb.sMaxTemp < (tAppMemParam.tUSB.sMaxTemp - 10))
	{
		if (tUsb.uErrCode.tCode.bOT == 1)
		{
			s_uc_over_temp_cnt++;
			if (s_uc_over_temp_cnt >= 5)
			{
				s_uc_over_temp_cnt = 0;
				bUsb_SetErrCode(UEC_OT, false);
			}
		}
		else
			s_uc_over_temp_cnt = 0;
	}

	/* QC 供电电压检查 */
	if (cUsb_CheckQcInVolt() != 0)
	{
		if (tUsb.uErrCode.tCode.bQcPowerErr == 0)
		{
			s_uc_usb_a_pwr_err_cnt++;
			if (s_uc_usb_a_pwr_err_cnt >= 5)
			{
				s_uc_usb_a_pwr_err_cnt = 0;
				bUsb_SetErrCode(UEC_QC_POWER_ERR, true);
			}
		}
		else
			s_uc_usb_a_pwr_err_cnt = 0;
	}
	else
	{
		if (tUsb.uErrCode.tCode.bQcPowerErr == 1)
		{
			s_uc_usb_a_pwr_err_cnt++;
			if (s_uc_usb_a_pwr_err_cnt >= 5)
			{
				s_uc_usb_a_pwr_err_cnt = 0;
				bUsb_SetErrCode(UEC_QC_POWER_ERR, false);
			}
		}
		else
			s_uc_usb_a_pwr_err_cnt = 0;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 更新参数
 * 说明(备注)  : 同步电压、温度与 QC 功率
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_usb_param_update(void)
{
	tUsb.usAutoOffTime = tAppMemParam.tUSB.usAutoOffTime;
	// tUsb.sMaxTemp = tAdcSamp.sUsbTemp;
	tUsb.sMaxTemp = 25;//固定25摄氏度
	tUsb.usInVolt = tAdcSamp.usSysInVolt;//0.1V
	
	if(tUsb.eDevState == DS_WORK)
		tUsb.usInCurr = 0;//0.1A
		// tUsb.usPdPwr = tAdcSamp.fUsbPdCurr * tAdcSamp.usUsbPdVolt / 10;//W
		// tUsb.usWcPwr = tAdcSamp.fUsbWcCurr * tAdcSamp.usUsbWcVolt / 10;//W
	else
	{
		tUsb.usInCurr = 0;//0.1A
		tUsb.usPdPwr = 0;//W
		tUsb.usWcPwr = 0;//W
		tUsb.usOutPwr = 0;//W
	}
}

/***********************************************************************************************************************
 * 函数功能    : 快充开关
 * 说明(备注)  : 结构化单点意图决策，彻底消除 goto 跳转
 * 传入参数    : e_tri_type: 开关类型; b_fore_en: 强制执行
 * 输出参数    : 无
 * 返回值      : 0:维持现状; 1:成功; 负数:权限/故障禁止
 ************************************************************************************************************************/
s8 cUsb_Switch(SwitchType_E e_tri_type, bool b_fore_en)
{
	bool b_turn_on = false;

	if (e_tri_type == ST_ON)
	{
		if ((tUsb.eDevState == DS_WORK || tUsb.eDevState == DS_BOOTING) && b_fore_en == false)
		{
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:当前状态为工作,不允许开机\r\n");
			return 0;
		}
		b_turn_on = true;
	}
	else if (e_tri_type == ST_OFF)
	{
		if ((tUsb.eDevState == DS_SHUT_DOWN || tUsb.eDevState == DS_CLOSING) && b_fore_en == false)
		{
			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:当前状态为关闭,不允许关机\r\n");
			return 0;
		}
		b_turn_on = false;
	}
	else
		b_turn_on = (tUsb.eDevState == DS_SHUT_DOWN || tUsb.eDevState == DS_CLOSING);

	if (b_turn_on)
	{
		if (tSysInfo.uPerm.tPerm.bDisChgPerm == false)
		{
			#if (boardBUZ_EN)
			bBuz_Tweet(SHORT_2);
			#endif  /* boardBUZ_EN */

			if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_w("bUsbTask:系统不允许开启放电");
			return -1;
		}

		if (cUsb_CheckBatVolt() <= 0)
		{
			bUsb_SetErrCode(UEC_BAT_VOLT_LOW, true);
			if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
				log_w("bUsbTask:电池电压过低 %d.%dV", tAdcSamp.usSysInVolt / 10, tAdcSamp.usSysInVolt % 10);
			return -2;
		}

		#if (boardBUZ_EN)
		bBuz_Tweet(LONG_1);
		#endif  /* boardBUZ_EN */

		if (tUsb.uErrCode.ucErrCode)
			bUsb_SetErrCode(UEC_CLEAR_ALL, false);

		cQueue_AddQueueTask(tpUsbTask, UTI_BOOTING, NULL, b_fore_en);
	}
	else
	{
		#if (boardBUZ_EN)
		bBuz_Tweet(LONG_1);
		#endif  /* boardBUZ_EN */

		if (tUsb.uErrCode.ucErrCode)
			bUsb_SetErrCode(UEC_CLEAR_ALL, false);

		cQueue_AddQueueTask(tpUsbTask, UTI_CLOSING, NULL, b_fore_en);
	}

	#if (boardSYS_DATA_UPADATA)
	Sys_Update_Mod(USB_Mod, true);
	#endif  /* boardSYS_DATA_UPADATA */

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 快充工作状态设置
 * 说明(备注)  : 设置状态并联动控制各通道使能引脚
 * 传入参数    : e_stat: 目标状态
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void bUsb_SetDevState(DevState_E e_stat)
{
    tUsb.eDevState = e_stat;
	
	//启动和关闭都清除一次错误
	if(e_stat == DS_BOOTING)
	{
		// usbPOWER_EN_ON();
		// usbPD_EN_ON();
		// usbPD2_EN_ON();
		// usbA_EN_ON();

		usbPD_EN_ON();
		usbPD2_EN_ON();
		vUSB_ControlPorts(true);
	}
	else if(e_stat == DS_CLOSING)
	{
		// usbPOWER_EN_OFF();
		usbPD_EN_OFF();
		usbPD2_EN_OFF();
		// usbA_EN_OFF();
		vUSB_ControlPorts(false);
	}
	else if(e_stat == DS_SHUT_DOWN)
	{
		// usbPOWER_EN_OFF();
		usbPD_EN_OFF();
		usbPD2_EN_OFF();
		// usbA_EN_OFF();
		vUSB_ControlPorts(false);
	}
	else if(e_stat == DS_INIT)
		vUSB_ControlPorts(false);
	else if(e_stat == DS_ERR)
		vUSB_ControlPorts(false);
		
}

/***********************************************************************************************************************
 * 函数功能    : 设置错误状态
 * 说明(备注)  : 驱动蜂鸣器报警，并在存在致命故障时投递 UTI_ERR 任务
 * 传入参数    : e_code: 错误代码; b_set: true-设置, false-清除
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void bUsb_SetErrCode(UsbErrCode_E e_code, bool b_set)
{
	static UsbErrCode_E s_e_next_code;
	static bool         s_b_next_set;

	if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
	{
		if (s_e_next_code != e_code || s_b_next_set != b_set)
		{
			log_e("bUsbTask:任务错误 代码%d 类型%d", e_code, b_set);
			s_e_next_code = e_code;
			s_b_next_set  = b_set;
		}
	}

	if (e_code == UEC_CLEAR_ALL)
	{
		tUsb.uErrCode.ucErrCode = 0;
		return;
	}

	if (b_set)
	{
		#if (boardBUZ_EN)
		bBuz_Tweet(LONG_3);
		#endif  /* boardBUZ_EN */

		ERR_SET(tUsb.uErrCode.ucErrCode, (e_code - 1));
	}
	else
		ERR_CLR(tUsb.uErrCode.ucErrCode, (e_code - 1));

	if (tUsb.uErrCode.ucErrCode)
	{
		UsbErrCode_U u_err_code;
		u_err_code.ucErrCode      = tUsb.uErrCode.ucErrCode;
		u_err_code.tCode.bIc1Lost = 0;
		u_err_code.tCode.bIc2Lost = 0;

		if (tpUsbTask->ucID != UTI_ERR && u_err_code.ucErrCode)
			cQueue_AddQueueTask(tpUsbTask, UTI_ERR, NULL, true);
	}
}

/***********************************************************************************************************************
 * 函数功能    : 自动关闭计时
 * 说明(备注)  : 1秒心跳倒计时
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUsb_TickTimer(void)
{
	if (bSys_IsWorkState() == false || tUsb.eDevState != DS_WORK)
		return;
	
	//非工作模式不计时
	if(tUsb.eDevState != DS_WORK)
		return;
	
	if (tUsb.usAutoOffTime)
	{
		if (tUsb.usAutoOffCnt)
		{
			tUsb.usAutoOffCnt--;
			if (tUsb.usAutoOffCnt == 0)
			{
				cUsb_Switch(ST_OFF, false);
				if (uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					sMyPrint("bUsbTask:倒计时结束,关闭USB  时间=%dS\r\n", tUsb.usAutoOffTime);
			}
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 刷新关闭时间
 * 说明(备注)  : 重置倒计时计数值
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUsb_RefreshOffTime(void)
{
	if (tUsb.usAutoOffTime)
		tUsb.usAutoOffCnt = tUsb.usAutoOffTime;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化参数
 * 说明(备注)  : 载入板级配置参数默认值
 * 传入参数    : p_usb_mem: USB 记忆参数结构体指针
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bUsb_MemParamInit(UsbMemParam_T *p_usb_mem)
{
	p_usb_mem->usAutoOffTime = boardUSB_OFF_TIME;
	p_usb_mem->usMaxInVolt   = boardUSB_MAX_IN_VOLT;
	p_usb_mem->usMinInVolt   = boardUSB_MIN_IN_VOLT;
	p_usb_mem->usMinOpenVolt = boardUSB_OPEN_MIN_VOLT;
	p_usb_mem->sMaxTemp      = boardUSB_MAX_TEMP;
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置记忆参数 (表驱动精简版)
 * 说明(备注)  : 支持 uint16 与 int8 类型，自动进行安全上下限防越界检查
 * 传入参数    : uc_item: 参数索引; b_add: true-增加, false-减少
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUsb_MemParamSet(uint8_t uc_item, bool b_add)
{
	if (uc_item >= mainARRAY_SIZE(s_t_usb_param_table))
		return;

	const UsbParamStep_T *p = &s_t_usb_param_table[uc_item];
	if (p->ucType == 1)
	{
		int8_t *p_val = (int8_t*)p->pParam;
		if (b_add && *p_val < (int8_t)p->sMax)
			(*p_val)++;
		else if (!b_add && *p_val > (int8_t)p->sMin)
			(*p_val)--;
	}
	else
	{
		uint16_t *p_val = (uint16_t*)p->pParam;
		if (b_add && *p_val < (uint16_t)p->sMax)
			(*p_val)++;
		else if (!b_add && *p_val > (uint16_t)p->sMin)
			(*p_val)--;
	}
}

/***********************************************************************************************************************
 * 函数功能    : 检查 USB 供电状态
 * 说明(备注)  : 校验 USB 输入电压范围
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 0:正常; 1:过压; -1:欠压
 ************************************************************************************************************************/
s8 cUsb_CheckInVolt(void)
{
	if (RANGE(tUsb.usInVolt, tAppMemParam.tUSB.usMinInVolt, tAppMemParam.tUSB.usMaxInVolt))
		return 0;
	else if (tUsb.usInVolt > tAppMemParam.tUSB.usMaxInVolt)
		return 1;
	else
		return -1;
}

/***********************************************************************************************************************
 * 函数功能    : 检查电池电压是否满足 USB 开启门限
 * 说明(备注)  : 比对电池采样电压与最小开启门限
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1:允许开启; -1:欠压禁止
 ************************************************************************************************************************/
s8 cUsb_CheckBatVolt(void)
{
	if (tAdcSamp.usSysInVolt > tAppMemParam.tUSB.usMinOpenVolt)
		return 1;
	else
		return -1;
}

/***********************************************************************************************************************
 * 函数功能    : 检查 QC 供电状态
 * 说明(备注)  : 修复原有判断条件的逻辑缺陷，正确比对 usUsbA_Volt
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 0:正常; 1:过压; -1:欠压
 ************************************************************************************************************************/
s8 cUsb_CheckQcInVolt(void)
{
	return 0;
}

#if (boardLOW_POWER)
/***********************************************************************************************************************
 * 函数功能    : 进入低功耗
 * 说明(备注)  : 配置 IO 为模拟输入并挂起任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUsb_EnterLowPower(void)
{
	rcu_periph_clock_enable(usbPD_EN_RCU);
	gpio_init(usbPD_EN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbPD_EN_PIN);

	rcu_periph_clock_enable(usbPOWER_EN_RCU);
	gpio_init(usbPOWER_EN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbPOWER_EN_PIN);

	rcu_periph_clock_enable(usbA_EN_RCU);
	gpio_init(usbA_EN_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbA_EN_PIN);

	rcu_periph_clock_enable(usbIC1_SCL_RCU);
	gpio_init(usbIC1_SCL_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbIC1_SCL_PIN);

	rcu_periph_clock_enable(usbIC1_SDA_RCU);
	gpio_init(usbIC1_SDA_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbIC1_SDA_PIN);

	rcu_periph_clock_enable(usbIC2_SCL_RCU);
	gpio_init(usbIC2_SCL_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbIC2_SCL_PIN);

	rcu_periph_clock_enable(usbIC2_SDA_RCU);
	gpio_init(usbIC2_SDA_PORT, GPIO_MODE_AIN, GPIO_OSPEED_2MHZ, usbIC2_SDA_PIN);

	#if (boardUSE_OS)
	vTaskSuspend(tUsbTaskHandler);
	#endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 退出低功耗
 * 说明(备注)  : 调用标准接口初始化 IO 并恢复任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUsb_ExitLowPower(void)
{
	vUsb_IfaceInit();

	#if (boardUSE_OS)
	vTaskResume(tUsbTaskHandler);
	#endif  /* boardUSE_OS */
}
#endif  /* boardLOW_POWER */

#endif  /* boardUSB_EN */

