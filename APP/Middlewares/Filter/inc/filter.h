/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\Filter
 * File    : filter.h
 * Date    : 2026-10-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 嵌入式常用通用数字滤波算法中间件头文件
 *           包含中位值去极值平均滤波、滑动递推平均滤波、限幅滤波、一阶滞后滤波、消抖滤波及卡尔曼滤波
 * @note   并发约束: 所有滤波句柄/上下文均不支持多任务并发调用, 同一实例仅允许在单一任务上下文中使用
 *         (多实例隔离无限制, 不同句柄可在不同任务中各自独立运行)
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef FILTER_H
#define FILTER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)
//****************************************************Macros********************************************************************//
#define			filterMEDIAN_STACK_BUF_MAX				32		/* 中位值滤波窗口参考建议深度; 当前算法已升级为零栈分配扫描 */
#define			filterWEIGHT_RECUR_BUF_MAX				12		/* 加权递推滤波缓冲区最大深度 */


//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

/**
 * @brief 32位整型通用滤波句柄结构体 (中位均值滤波 / 递推平均滤波 / 数据稳定性检测)
 * @note  为保持硬件各模块静态初始化的 100% 向后兼容，前 7 个字段成员及顺序严格保持不变
 */
typedef struct
{
	s32*				data;				//数据缓冲指针
	u8					Buff_Size;			//缓冲器容量大小
	u8					Max_Swing;			//最大波动门限
	vu8					BuffUseSize;		//缓冲器当前已填入大小
	vu8					CyclicCount;		//环形索引计数
	s64					Sum;				//累加和累积器 (64位防溢出)
	s32					DataOut;			//当前滤波输出值
}FilterHandler_T;

/**
 * @brief 单精度浮点数通用滤波句柄结构体
 */
typedef struct
{
	float*				data;				//浮点数据缓冲指针
	u8					Buff_Size;			//缓冲器容量大小
	float				Max_Swing;			//最大波动门限
	vu8					BuffUseSize;		//缓冲器当前已填入大小
	vu8					CyclicCount;		//环形索引计数
	float				Sum;				//累加和累积器
	float				DataOut;			//当前滤波输出值
}fFilterHandler_T;

/**
 * @brief 卡尔曼滤波器参数结构体
 */
typedef struct
{
	float				LastP;				//上次估算协方差，初始推荐 0.02f
	float				Now_P;				//当前估算协方差
	float				out;				//卡尔曼输出估计值
	float				Kg;					//卡尔曼增益
	float				Q;					//过程噪声协方差，初始推荐 0.001f
	float				R;					//观测噪声协方差，初始推荐 0.543f
}KFP_t;

/**
 * @brief 多实例限幅滤波上下文结构体
 */
typedef struct
{
	uint16_t			usLastValue;		//上次有效输出值
	uint16_t			usMaxSwing;			//允许的最大跳变差值
	bool				bInitialized;		//初值有效标志
}LimitFilter_T;

/**
 * @brief 多实例一阶滞后滤波上下文结构体
 */
typedef struct
{
	uint16_t			usLastValue;		//上次滤波输出值
	uint8_t				ucCoeff;			//本次采样权重 a (0~100)
	bool				bInitialized;		//初值有效标志
}FirstOrderFilter_T;

/**
 * @brief 定点数一阶滞后滤波上下文结构体 (Q15高精度无死区，零浮点开销，完美适配 ESP32-C3)
 */
typedef struct
{
	int32_t				lStateQ15;			//Q15定点状态累积器(保存小数残差)
	uint16_t			usLastValue;		//上次滤波输出值
	uint16_t			usCoeffQ15;			//权重系数 a (0~32768, 32768 对应 1.0)
	bool				bInitialized;		//初值有效标志
}FirstOrderQ15Filter_T;

/**
 * @brief 多实例消抖滤波上下文结构体
 */
typedef struct
{
	uint16_t			usLastValue;		//当前稳定输出值
	uint16_t			usTargetValue;		//跟踪候选值
	uint8_t				ucShakeCount;		//连续到达计数值
	uint8_t				ucMaxShake;			//消抖确认阈值
	bool				bInitialized;		//初值有效标志
}ClearShakeFilter_T;

/**
 * @brief 多实例限幅消抖滤波上下文结构体
 */
typedef struct
{
	uint16_t			usLastValue;		//当前稳定输出值
	uint16_t			usTargetValue;		//候选采样值
	uint16_t			usMaxSwing;			//限幅门限
	uint8_t				ucShakeCount;		//消抖计数
	uint8_t				ucMaxShake;			//消抖确认阈值
	bool				bInitialized;		//初值有效标志
}LimitClearShakeFilter_T;

/**
 * @brief 多实例加权递推平均滤波上下文结构体
 */
typedef struct
{
	uint16_t			usaBuffer[filterWEIGHT_RECUR_BUF_MAX];	//数据环形队列
	uint8_t				ucCount;			//当前有效采样点数
	uint8_t				ucIndex;			//环形入队游标
}WeightRecurFilter_T;

//****************************************************Extern********************************************************************//

/* 1. 中位值去极值平均滤波算法 (O(N) 极速扫描、零栈分配、四舍五入防截断偏置) */
s32      lFilter_MadianAverage(FilterHandler_T *p_handler, const s32 *p_datain);
float    fFilter_MadianAverage(fFilterHandler_T *p_handler, const float *p_datain);

/* 2. 递推滑动平均滤波算法 (滑动窗口 O(1) 增量更新，64位防溢出，四舍五入) */
s32      lFilter_RecursionAverage(FilterHandler_T *p_handler, const s32 *p_data);
u16      usFilter_RecursionAverage(FilterHandler_T *p_handler, const u16 *p_data);

/* 3. 数据稳定性检测 (结合相邻差与极差双重判定，杜绝斜坡漂移误判) */
s8       cFilter_CheckDataStability(FilterHandler_T *p_filter, u16 num);
s8       cFilter_CheckFloatDataStability(fFilterHandler_T *p_filter, float num);

/* 4. 句柄复位初始化接口 (彻底清零状态与底层缓冲区) */
void     vFilter_HandlerReset(FilterHandler_T *p_handler);
void     vFilter_FloatHandlerReset(fFilterHandler_T *p_handler);

/* 5. 多实例滤波算法接口 */
uint16_t usFilter_LimitCalc(LimitFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_FirstOrderCalc(FirstOrderFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_FirstOrderQ15Calc(FirstOrderQ15Filter_T *p_filter, uint16_t us_new_val);
void     vFilter_FirstOrderQ15Reset(FirstOrderQ15Filter_T *p_filter, uint16_t us_init_val, uint16_t us_coeff_q15);
uint16_t usFilter_ClearShakeCalc(ClearShakeFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_LimitClearShakeCalc(LimitClearShakeFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_WeightRecurCalc(WeightRecurFilter_T *p_filter, uint16_t us_new_val);
void     vFilter_WeightRecurReset(WeightRecurFilter_T *p_filter);
void     vFilter_KalmanInit(KFP_t *p_kfp, float q, float r, float p_init, float out_init);
float    fFilter_KalmanCalc(KFP_t *p_kfp, float input);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* FILTER_H */
