/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\ComFunc
 * File    : filtration.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 嵌入式常用数字滤波算法库头文件
 *           包含中位值平均滤波、递推滑动平均滤波、限幅滤波、一阶滞后滤波、消抖滤波及卡尔曼滤波
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef FILTRATION_H_
#define FILTRATION_H_

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)
//****************************************************Macros********************************************************************//
#define			filterMEDIAN_STACK_BUF_MAX				32		/* 中位值滤波窗口深度上限; 句柄 Buff_Size 超过此值时窗口静默截断为 32 点 */
#define			filterWEIGHT_RECUR_BUF_MAX				12		/* 加权递推滤波缓冲区深度 */


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

typedef KFP_t Kalman_T;

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
	uint8_t				ucCount;			//采样计数
}WeightRecurFilter_T;

/* 滤波算法自动化自检与精度验证接口 (方便验证算法准确性与抗扰性) */
#define			filterSELF_TEST_ENABLE					1		/* 0: 关闭自测代码节省代码空间, 1: 开启自测试 */

#if (filterSELF_TEST_ENABLE)
typedef struct
{
	uint16_t			usTotalCases;		//总测试用例数
	uint16_t			usPassCount;		//通过用例数
	uint16_t			usFailCount;		//失败用例数
}FilterTestReport_T;
#endif

//****************************************************Extern********************************************************************//

/* 1. 中位值平均滤波算法 (时序保护、小数组插入排序、防溢出)
 * 注意: lFilter_MadianAverage / fFilter_MadianAverage 的窗口深度上限为 filterMEDIAN_STACK_BUF_MAX(32),
 *       当句柄 Buff_Size > 32 时窗口将被静默截断为 32 点 (不报错、不额外分配) */
s32   lFilter_MadianAverage(FilterHandler_T *p_handler, const s32 *p_datain);
float fFilter_MadianAverage(fFilterHandler_T *p_handler, const float *p_datain); /* 入口含 NaN/Inf 异常值防护 */

/* 2. 递推平均滤波算法 (滑动窗口 O(1) 复杂度升级，彻底防溢出) */
u16   usFilter_RecursionAverage(FilterHandler_T *p_handler, const u16 *p_data);

/* 3. 数据稳定性检测 */
s8    cFilter_CkeckDataStability(FilterHandler_T *p_filter, u16 num);
s8    cFilter_CkeckFloatDataStability(fFilterHandler_T *p_filter, float num);

/* 4. 句柄复位初始化接口 */
void  vFilter_HandlerReset(FilterHandler_T *p_handler);
void  vFilter_FloatHandlerReset(fFilterHandler_T *p_handler);

/* 5. 多实例新接口 (推荐新功能模块使用) */
uint16_t usFilter_LimitCalc(LimitFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_FirstOrderCalc(FirstOrderFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_ClearShakeCalc(ClearShakeFilter_T *p_filter, uint16_t us_new_val);
uint16_t usFilter_LimitClearShakeCalc(LimitClearShakeFilter_T *p_filter, uint16_t us_new_val);
void     vFilter_KalmanInit(KFP_t *p_kfp, float q, float r, float p_init, float out_init);
float    Filter_Kalman(KFP_t *p_kfp, float input);

#if (filterSELF_TEST_ENABLE)
bool bFilter_RunSelfTest(FilterTestReport_T *p_report);
#endif

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* FILTRATION_H_ */

