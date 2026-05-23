#include "lv_port_timer.h"
#include "lvgl.h"

//TIM_HandleTypeDef htim14;

//void basic_timer_config(uint16_t pre,uint16_t per)
//{
//	__HAL_RCC_TIM14_CLK_ENABLE();

//    /* TIM14 interrupt Init */
//    HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 0, 0);
//    HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
//	
//	htim14.Instance = TIM14;
//	htim14.Init.Prescaler = pre;
//	htim14.Init.CounterMode = TIM_COUNTERMODE_UP;
//	htim14.Init.Period = per;
//	htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV4;
//	htim14.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
//	if (HAL_TIM_Base_Init(&htim14) != HAL_OK)
//	{
//		Error_Handler();
//	}
//}

//void TIM8_TRG_COM_TIM14_IRQHandler(void)
//{
//	/* USER CODE BEGIN TIM8_TRG_COM_TIM14_IRQn 0 */
//	lv_tick_inc(1);//lvgl的1ms中断
//	HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_10);
//	/* USER CODE END TIM8_TRG_COM_TIM14_IRQn 0 */
//	HAL_TIM_IRQHandler(&htim14);
//	/* USER CODE BEGIN TIM8_TRG_COM_TIM14_IRQn 1 */

//	/* USER CODE END TIM8_TRG_COM_TIM14_IRQn 1 */
//}



TIM_HandleTypeDef g_timx_handle;      /* 定时器x句柄 */


/**
 * @brief       通用定时器TIMX定时中断初始化函数
 * @note
 *              通用定时器的时钟来自APB1,当D2PPRE1≥2分频的时候
 *              通用定时器的时钟为APB1时钟的2倍, 而APB1为120M, 所以定时器时钟 = 240Mhz
 *              定时器溢出时间计算方法: Tout = ((arr + 1) * (psc + 1)) / Ft us.
 *              Ft=定时器工作频率,单位:Mhz
 *
 * @param       arr: 自动重装值。
 * @param       psc: 时钟预分频数
 * @retval      无
 */
void gtim_timx_int_init(uint16_t arr, uint16_t psc)
{
    GTIM_TIMX_INT_CLK_ENABLE();                                      /* 使能TIMx时钟 */
    
    g_timx_handle.Instance = GTIM_TIMX_INT;                          /* 通用定时器x */
    g_timx_handle.Init.Prescaler = psc;                              /* 预分频系数 */
    g_timx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;             /* 递增计数模式 */
    g_timx_handle.Init.Period = arr;                                 /* 自动装载值 */
    HAL_TIM_Base_Init(&g_timx_handle);

    HAL_NVIC_SetPriority(GTIM_TIMX_INT_IRQn, 2, 0);  /* 设置中断优先级，抢占优先级1，子优先级3 */
    HAL_NVIC_EnableIRQ(GTIM_TIMX_INT_IRQn);          /* 开启ITMx中断 */

    HAL_TIM_Base_Start_IT(&g_timx_handle);           /* 使能定时器x和定时器x更新中断 */
}

/**
 * @brief       定时器中断服务函数
 * @param       无
 * @retval      无
 */
void GTIM_TIMX_INT_IRQHandler(void)
{
    /* 以下代码没有使用定时器HAL库共用处理函数来处理，而是直接通过判断中断标志位的方式 */
    if(__HAL_TIM_GET_FLAG(&g_timx_handle, TIM_FLAG_UPDATE) != RESET)
    {
		lv_tick_inc(1);//lvgl的1ms中断
        __HAL_TIM_CLEAR_IT(&g_timx_handle, TIM_IT_UPDATE);  /* 清除定时器溢出中断标志位 */
    }
}

