/*!
    \file    systick.c
    \brief   the systick configuration file

    \version 2020-09-04, V2.0.0, demo for GD32F4xx
*/

/*
    Copyright (c) 2020, GigaDevice Semiconductor Inc.

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

#include "gd32f30x.h"
#include "systick.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif

#if(boardBMS_485_IFACE_EN)
#include "MD_Bms/md_bms_iface.h"

__IO bool bSysTick_BmsSendFinish = false;
#endif

#if(boardPRINT_485_IFACE_EN)
#include "Print/print_iface.h"

__IO bool bSysTick_PrintSendFinish = false;
#endif

__IO bool bSystick_10MsFlag = false;
__IO bool bSystick_100MsFlag = false;

static vu32 delay;

/*!
    \brief      configure systick
    \param[in]  none
    \param[out] none
    \retval     none
*/
void vSys_TickConfig(void)
{
    /* setup systick timer for 1000Hz interrupts */
    if(SysTick_Config(SystemCoreClock / 1000U)) {
        /* capture error */
        while(1) {
        }
    }
    /* configure the systick handler priority */
    NVIC_SetPriority(SysTick_IRQn, 0x00U);
}

/*!
    \brief      delay a time in milliseconds
    \param[in]  count: count in milliseconds
    \param[out] none
    \retval     none
*/
void vSys_MsDelay(u32 cnt)
{
    delay = cnt;

    while(0U != delay) {
    }
}

/*!
    \brief      delay decrement
    \param[in]  none
    \param[out] none
    \retval     none
*/
void vSys_Tick(void)
{
    if(0U != delay) {
        delay--;
    }
	
	#if(boardBMS_485_IFACE_EN)
	static vu16 cnt = 0;
	//BMS485发送接口关闭倒计时
	if(bSysTick_BmsSendFinish == true)
	{
		cnt++;
		if(cnt >= 2)
		{
			cnt = 0;
			vBms_485TransEnable(false);
		}
	}
	#endif
	
	#if(boardPRINT_485_IFACE_EN)
	static vu16 cnt1 = 0;
	//BMS485发送接口关闭倒计时
	if(bSysTick_PrintSendFinish == true)
	{
		cnt1++;
		if(cnt1 >= 2)
		{
			cnt1 = 0;
			vPrint_485TransEnable(false);
		}
	}
	#endif
	
	//10MS计时
	static vu16 us_10ms_cnt = 0;
	us_10ms_cnt++;
	if(us_10ms_cnt >= 10)
	{
		us_10ms_cnt = 0;
		bSystick_10MsFlag = true;
		
		#if(boardUPDATE)
		vUpdate_TickTimer();
		#endif
		
	}
	
	//100MS计时
	static vu16 us_100ms_cnt = 0;
	us_100ms_cnt++;
	if(us_100ms_cnt >= 100)
	{
		us_100ms_cnt = 0;
		bSystick_100MsFlag = true ;
	}
	
	#if(boardDISPLAY_EN)
//	vDisp_DispTask();
	#endif
}
