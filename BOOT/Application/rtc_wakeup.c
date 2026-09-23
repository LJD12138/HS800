/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Application
 * File    : rtc_wakeup.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : RTC闹钟定时唤醒配置与控制实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "rtc_wakeup.h"

#if (boardLOW_POWER)

//****************************************************Macros********************************************************************//

//****************************************************Parameter Initialization**************************************************//

//****************************************************Function Declaration******************************************************//

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : RTC闹钟中断与低功耗定时唤醒配置
 * 说明(备注)  : 配置IRC40K时钟源作为RTC时钟并设定闹钟秒数
 * 传入参数    : num: 闹钟超时秒数
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void rtc_configuration(u8 num)
{
	nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable(RTC_Alarm_IRQn, 2U, 0U);
	
    /* enable PMU and BKPI clocks */
    rcu_periph_clock_enable(RCU_BKPI);
    rcu_periph_clock_enable(RCU_PMU);
    /* allow access to backup domain */
    pmu_backup_write_enable();
    /* reset backup domain */
    bkp_deinit();

    /* enable IRC40K */
    rcu_osci_on(RCU_IRC40K);
    /* wait till IRC40K is ready */
    rcu_osci_stab_wait(RCU_IRC40K);
    /* select RCU_IRC40K as RTC clock source */
    rcu_rtc_clock_config(RCU_RTCSRC_IRC40K);
    /* enable RTC Clock */
    rcu_periph_clock_enable(RCU_RTC);

    /* wait for RTC registers synchronization */
    rtc_register_sync_wait();
    /* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();
    /* enable the RTC alarm interrupt */
    rtc_interrupt_enable(RTC_INT_ALARM);
    /* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();
    /* set RTC prescaler: set RTC period to 1s */
    rtc_prescaler_set(40000);
    /* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();
    rtc_counter_set(0U);
    /* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();
    rtc_alarm_config(num);
    /* wait until last write operation on RTC registers has finished */
    rtc_lwoff_wait();

    /* EXTI configuration */
    exti_deinit();
    exti_init(EXTI_17, EXTI_INTERRUPT, EXTI_TRIG_RISING);
    rtc_flag_clear(RTC_FLAG_ALARM);
    exti_interrupt_flag_clear(EXTI_17);
    exti_interrupt_enable(EXTI_17);
}

/***********************************************************************************************************************
 * 函数功能    : 关闭RTC唤醒与时钟
 * 说明(备注)  : 禁用相关中断并关闭外设时钟
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void rtc_close(void)
{
	nvic_irq_disable(RTC_Alarm_IRQn);
	rcu_periph_clock_disable(RCU_BKPI);
    rcu_periph_clock_disable(RCU_PMU);
	
	rcu_osci_off(RCU_IRC40K);
	
	rcu_periph_clock_disable(RCU_RTC);
	
	rtc_interrupt_disable(RTC_INT_ALARM);
	
	rtc_flag_clear(RTC_FLAG_ALARM);
    exti_interrupt_flag_clear(EXTI_17);
    exti_interrupt_disable(EXTI_17);
}

#endif  /* boardLOW_POWER */
