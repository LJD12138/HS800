#include "rtc_wakeup.h"


#if(boardLOW_POWER)

void rtc_configuration( u8 num )
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
    exti_init(EXTI_17,EXTI_INTERRUPT,EXTI_TRIG_RISING);
    rtc_flag_clear(RTC_FLAG_ALARM);
    exti_interrupt_flag_clear(EXTI_17);
    exti_interrupt_enable(EXTI_17);
}


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
	
#endif


