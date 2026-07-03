/*****************************************************************************************************************
*                                                                                                                *
 *                                         独立看门口                                                           *
*                       时钟源来自于内部低速时钟。所以即使主时钟失效，亦能保证产生复位。但计数精度不高。            *
*                       时钟源来自于内部低速时钟IRC40K,待机和深度睡眠模式也可工作                                  *
******************************************************************************************************************/

#include "fwdgt.h"

#if(boardWDGT_EN)
#include "gd32f30x_rcu.h"
#include "Print/print_api.h"


reset_reason_t g_reset_reason;

/*****************************************************************************************************************
-----函数功能    看门口初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFwdgt_Init(void)
{
  uint16_t timeout_t=0xFFFFU;
	/* enable IRC40K */
  rcu_osci_on(RCU_IRC40K);
	
	/* wait till IRC40K is ready */
  while(SUCCESS != rcu_osci_stab_wait(RCU_IRC40K))
  {
	if(timeout_t > 0) timeout_t--;
	else			  break;
  }
	
  /* configure FWDGT counter clock: 40KHz(IRC40K)  / 128 = 0.312 KHz */
  fwdgt_config(2*500,FWDGT_PSC_DIV128);				 //t = (1/0.312)x(2x500) = 3.2s
	
  fwdgt_write_disable();
  /* After 1.6 seconds to generate a reset */
  fwdgt_enable();
}


/*****************************************************************************************************************
-----函数功能    喂狗
-----说明(备注)  可以在任何地方喂狗,没有时间区间限制
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFwdgt_Reload(void)
{
	/* uncock fwdgt write protect*/
	fwdgt_write_enable();
	/* feed fwdgt */
	fwdgt_counter_reload();	
}


/*****************************************************************************************************************
-----函数功能    看门狗进入低功耗
-----说明(备注)  喂狗间隔改为最大26S
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFwdgt_EnterLowPower(void)
{
	vFwdgt_Reload();
  /* configure FWDGT counter clock: 40KHz(IRC40K)  / 256 = 0.156 KHz */
  fwdgt_config(0xfff,FWDGT_PSC_DIV256);				 //t = 26S
	
  fwdgt_write_disable();
  /* After 1.6 seconds to generate a reset */
  fwdgt_enable();
}


/*****************************************************************************************************************
-----函数功能    看门狗退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFwdgt_ExitLowPower(void)
{
	vFwdgt_Reload();
    vFwdgt_Init();
}

/*****************************************************************************************************************
-----函数功能    获取复位信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vResetReason_Capture(void)
{
    g_reset_reason.ext_pin   = (rcu_flag_get(RCU_FLAG_EPRST)    == SET);
    g_reset_reason.por       = (rcu_flag_get(RCU_FLAG_PORRST)   == SET);
    g_reset_reason.sw        = (rcu_flag_get(RCU_FLAG_SWRST)    == SET);
    g_reset_reason.fwdgt     = (rcu_flag_get(RCU_FLAG_FWDGTRST) == SET);
    g_reset_reason.wwdgt     = (rcu_flag_get(RCU_FLAG_WWDGTRST) == SET);
    g_reset_reason.low_power = (rcu_flag_get(RCU_FLAG_LPRST)    == SET);

    // 读取完马上清掉，防止下次误判
    rcu_all_reset_flag_clear();
}

/*****************************************************************************************************************
-----函数功能    获取看门狗复位原因
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vFwdgt_PrintResetReason(void)
{
    vResetReason_Capture();

    if(g_reset_reason.fwdgt)
		printf("[Reset] reason: FWDGT reset\r\n");
    else if(g_reset_reason.wwdgt)
		printf("[Reset] reason: WWDGT reset\r\n");
    else if(g_reset_reason.sw)
		printf("[Reset] reason: software reset\r\n");
    else if(g_reset_reason.ext_pin)
		printf("[Reset] reason: external pin reset\r\n");
    else if(g_reset_reason.por)
		printf("[Reset] reason: power on reset\r\n");
    else if(g_reset_reason.low_power)
		printf("[Reset] reason: low power reset\r\n");
}

#endif  // boardWDGT_EN

