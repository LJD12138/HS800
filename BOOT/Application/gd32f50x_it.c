/*!
    \file    gd32f50x_it.c
    \brief   interrupt service routines

    \version 2026-02-25, V1.0.4, firmware for GD32F50x
*/

/*
    Copyright (c) 2026, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/

#include "gd32f50x_it.h"
#include "main.h"
#include "systick.h"

#define			SRAM_ECC_ERROR_HANDLE(s)				do{}while(1)

bool bExti_KeyTriFlag = false;
bool bExti_SensorTriFlag = false;

/***********************************************************************************************************************
 * 函数功能    : 处理 NMI 不可屏蔽中断异常
 * 说明(备注)  : 检测到 SRAM ECC 不可纠正/可纠正错误时进入故障处理死循环; 其它 NMI 源亦进入死循环
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void NMI_Handler(void)
{
    if(SET == syscfg_sram_ecc_flag_get(SYSCFG_SRAMECCSTAT_SRAMECCMEIF)) {
        SRAM_ECC_ERROR_HANDLE("SRAM non-correction event detected\r\n");
    } else if(SET == syscfg_sram_ecc_flag_get(SYSCFG_SRAMECCSTAT_SRAMECCSEIF)) {
        SRAM_ECC_ERROR_HANDLE("SRAM single bit correction event detected\r\n");
    } else { 
        /* if NMI exception occurs, go to infinite loop */
        /* HXTAL clock monitor NMI error or NMI pin error */
        while(1) {
        }
    }
}

/***********************************************************************************************************************
 * 函数功能    : 处理 HardFault 硬件错误异常
 * 说明(备注)  : 未使能 CmBacktrace 时进入死循环; 使能后该函数不参与编译, 由 CmBacktrace 接管
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
#if(!boardCM_BACKTRACE)
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while (1){
    }
}
#endif

/***********************************************************************************************************************
 * 函数功能    : 处理 MemManage 存储器管理异常
 * 说明(备注)  : 进入死循环等待调试器定位故障
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while (1){
    }
}

/***********************************************************************************************************************
 * 函数功能    : 处理 BusFault 总线错误异常
 * 说明(备注)  : 进入死循环等待调试器定位故障
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/***********************************************************************************************************************
 * 函数功能    : 处理 UsageFault 用法错误异常
 * 说明(备注)  : 进入死循环等待调试器定位故障
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while (1){
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void)
{
}

/***********************************************************************************************************************
 * 函数功能    : 处理 DebugMon 调试监控异常
 * 说明(备注)  : 空实现, 当前未使用
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void DebugMon_Handler(void)
{
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void)
{
}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{
   vSys_Tick();
}

/***********************************************************************************************************************
 * 函数功能    : 处理外部中断线 0 中断
 * 说明(备注)  : 清除 EXTI_0 中断标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void EXTI0_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_0))
    {
		bExti_KeyTriFlag = true;
       exti_interrupt_flag_clear(EXTI_0);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 处理外部中断线 10~15 中断
 * 说明(备注)  : 清除 EXTI_13 中断标志
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void EXTI10_15_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_13))
    {
        exti_interrupt_flag_clear(EXTI_13);
    }
}

void RTC_Alarm_IRQHandler(void)
{
    if(RESET != rtc_flag_get(RTC_FLAG_ALARM)){
        /* clear the RTC alarm and EXTI_17 interrupt flags */
        rtc_flag_clear(RTC_FLAG_ALARM);
        exti_interrupt_flag_clear(EXTI_17);
		
		exti_interrupt_disable(EXTI_17);
        /* update RTC alarm time */
    }
}
