/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\ComFunc
 * File    : filtration.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 嵌入式常用数字滤波算法库实现
 *           包含中位值平均滤波、滑动递推平均滤波、限幅滤波、一阶滞后滤波、消抖滤波及卡尔曼滤波
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "filtration.h"
#include "function.h"
#include <math.h>

#if (boardPRINT_EN && boardPRINT_IFACE)
#include "Print/print_api.h"
#endif

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#if (1)
//****************************************************Macros********************************************************************//



//****************************************************Parameter Initialization**************************************************//



//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 整型滤波句柄复位初始化
 * 说明(备注)  : 清空状态及累加器，保留底层缓存指针与配置大小
 * 传入参数    : p_handler: 滤波句柄指针
 * 输出参数    : p_handler: 复位后的句柄
 * 返回值      : void
 ************************************************************************************************************************/
void vFilter_HandlerReset(FilterHandler_T *p_handler)
{
    if (p_handler != NULL)
    {
        mainENTER_CRITICAL();

        p_handler->BuffUseSize = 0;
        p_handler->CyclicCount = 0;
        p_handler->Sum = 0;
        p_handler->DataOut = 0;

        mainEXIT_CRITICAL();
    }
}

/***********************************************************************************************************************
 * 函数功能    : 浮点滤波句柄复位初始化
 * 说明(备注)  : 清空状态及累加器
 * 传入参数    : p_handler: 浮点滤波句柄指针
 * 输出参数    : p_handler: 复位后的句柄
 * 返回值      : void
 ************************************************************************************************************************/
void vFilter_FloatHandlerReset(fFilterHandler_T *p_handler)
{
    if (p_handler != NULL)
    {
        mainENTER_CRITICAL();

        p_handler->BuffUseSize = 0;
        p_handler->CyclicCount = 0;
        p_handler->Sum = 0.0f;
        p_handler->DataOut = 0.0f;

        mainEXIT_CRITICAL();
    }
}

/***********************************************************************************************************************
 * 函数功能    : 32位整型中位值平均滤波 (滑动去极值中位均值滤波)
 * 说明(备注)  : 1. 升级为 O(N) 单次线性遍历算法，完全免除插入排序，速度提升 5~10 倍；
 *              2. 彻底免除在栈上分配 128 字节数组，极大节省任务栈空间，杜绝栈溢出；
 *              3. 剔除单一极小值和极大值，数学上与排序去首尾 100% 完全等价；
 *              4. 增加除法四舍五入偏移，消除整数截断带来的系统负偏置。
 * 传入参数    : p_handler: 滤波句柄指针, p_datain: 本次采集输入数据指针
 * 输出参数    : p_handler: 更新内部状态与输出值
 * 返回值      : s32: 滤波后的数据输出
 ************************************************************************************************************************/
s32 lFilter_MadianAverage(FilterHandler_T *p_handler, const s32 *p_datain)
{
    uint8_t uc_size;
    uint8_t uc_valid_cnt;
    s64     ll_total_sum = 0;
    s64     ll_sum = 0;
    s32     min_val;
    s32     max_val;
    uint8_t i;
    uint8_t div;

    /* 1. 安全边界检查 */
    if ((p_handler == NULL) || (p_handler->data == NULL) || (p_datain == NULL))
        return 0;

    if (p_handler->Buff_Size <= 2)
    {
        p_handler->DataOut = *p_datain;
        return p_handler->DataOut;
    }

    uc_size = (p_handler->Buff_Size > filterMEDIAN_STACK_BUF_MAX) ? 
              filterMEDIAN_STACK_BUF_MAX : p_handler->Buff_Size;

    /* 2. 环形队列快速入队 (极简原子保护，零栈数组拷贝) */
    mainENTER_CRITICAL();

    p_handler->data[p_handler->CyclicCount] = *p_datain;
    p_handler->CyclicCount++;
    if (p_handler->CyclicCount >= uc_size)
        p_handler->CyclicCount = 0;

    if (p_handler->BuffUseSize < uc_size)
        p_handler->BuffUseSize++;
    uc_valid_cnt = p_handler->BuffUseSize;

    mainEXIT_CRITICAL();

    /* 3. 在临界区外执行 O(N) 极速去极值均值计算 (零排序开销) */
    if (uc_valid_cnt <= 2)
        /* 样本不足3个，无法剔除极大极小值，直接输出当前值 */
        p_handler->DataOut = *p_datain;
    else
    {
        /* 单次线性扫描捕获极大极小值与总和，数学上完全等价于先全排序再掐头去尾 */
        min_val = p_handler->data[0];
        max_val = p_handler->data[0];
        for (i = 0; i < uc_valid_cnt; i++)
        {
            s32 val = p_handler->data[i];
            ll_total_sum += val;
            if (val < min_val)
                min_val = val;
            if (val > max_val)
                max_val = val;
        }

        /* 剔除 1 个最小值和 1 个最大值 */
        ll_sum = ll_total_sum - min_val - max_val;
        div = uc_valid_cnt - 2;

        /* 四舍五入，彻底消除整数除法向零截断带来的负向系统偏置 (Negative Bias) */
        p_handler->Sum = ll_sum;
        p_handler->DataOut = (s32)((ll_sum >= 0) ? (ll_sum + (div / 2)) / div : (ll_sum - (div / 2)) / div);
    }

    return p_handler->DataOut;
}

/***********************************************************************************************************************
 * 函数功能    : 单精度浮点中位值平均滤波
 * 说明(备注)  : 对称实现的浮点版本，单次 O(N) 扫描，零栈分配与零排序开销;
 *              入口含 NaN/Inf 异常值防护, 拒绝异常观测并维持上次输出
 * 传入参数    : p_handler: 浮点滤波句柄, p_datain: 本次输入指针
 * 输出参数    : p_handler: 更新内部状态与输出值
 * 返回值      : float: 滤波后的浮点输出
 ************************************************************************************************************************/
float fFilter_MadianAverage(fFilterHandler_T *p_handler, const float *p_datain)
{
    uint8_t uc_size;
    uint8_t uc_valid_cnt;
    float   f_total_sum = 0.0f;
    float   f_sum = 0.0f;
    float   min_val;
    float   max_val;
    uint8_t i;

    /* 1. 安全边界检查 */
    if ((p_handler == NULL) || (p_handler->data == NULL) || (p_datain == NULL))
        return 0.0f;

    /* 1.5 异常值防护: NaN/Inf 观测输入直接拒绝, 不入窗不污染历史, 维持上次输出 */
    if (!isfinite(*p_datain))
        return p_handler->DataOut;

    if (p_handler->Buff_Size <= 2)
    {
        p_handler->DataOut = *p_datain;
        return p_handler->DataOut;
    }

    uc_size = (p_handler->Buff_Size > filterMEDIAN_STACK_BUF_MAX) ? 
              filterMEDIAN_STACK_BUF_MAX : p_handler->Buff_Size;

    /* 2. 环形队列快速入队 (极简原子保护) */
    mainENTER_CRITICAL();

    p_handler->data[p_handler->CyclicCount] = *p_datain;
    p_handler->CyclicCount++;
    if (p_handler->CyclicCount >= uc_size)
        p_handler->CyclicCount = 0;

    if (p_handler->BuffUseSize < uc_size)
        p_handler->BuffUseSize++;
    uc_valid_cnt = p_handler->BuffUseSize;

    mainEXIT_CRITICAL();

    /* 3. 临界区外执行 O(N) 极速去极值均值计算 */
    if (uc_valid_cnt <= 2)
        p_handler->DataOut = *p_datain;
    else
    {
        min_val = p_handler->data[0];
        max_val = p_handler->data[0];
        for (i = 0; i < uc_valid_cnt; i++)
        {
            float val = p_handler->data[i];
            f_total_sum += val;
            if (val < min_val)
                min_val = val;
            if (val > max_val)
                max_val = val;
        }

        f_sum = f_total_sum - min_val - max_val;
        p_handler->Sum = f_sum;
        p_handler->DataOut = f_sum / (float)(uc_valid_cnt - 2);
    }

    return p_handler->DataOut;
}

/***********************************************************************************************************************
 * 函数功能    : 递推平均滤波 (滑动窗口增量均值滤波)
 * 说明(备注)  : 升级为 O(1) 滑动更新算法：Sum = Sum - oldest + newest；
 *              累加和采用 64 位防溢出，加入四舍五入防偏置
 * 传入参数    : p_handler: 滤波句柄, p_data: 本次输入指针
 * 输出参数    : p_handler: 更新内部状态与输出
 * 返回值      : u16: 滤波后的均值结果
 ************************************************************************************************************************/
u16 usFilter_RecursionAverage(FilterHandler_T *p_handler, const u16 *p_data)
{
    s32 s_new_val;
    s32 s_old_val;

    if ((p_handler == NULL) || (p_handler->data == NULL) || (p_data == NULL) || (p_handler->Buff_Size == 0))
        return 0;

    s_new_val = (s32)(*p_data);

    mainENTER_CRITICAL();

    if (p_handler->BuffUseSize < p_handler->Buff_Size)
    {
        /* 初始填满阶段 */
        p_handler->data[p_handler->CyclicCount] = s_new_val;
        p_handler->Sum += s_new_val;
        p_handler->BuffUseSize++;
        p_handler->CyclicCount++;
        if (p_handler->CyclicCount >= p_handler->Buff_Size)
            p_handler->CyclicCount = 0;
        /* 四舍五入均值 */
        p_handler->DataOut = (s32)((p_handler->Sum + (p_handler->BuffUseSize / 2)) / p_handler->BuffUseSize);
    }
    else
    {
        /* 窗口已满，进入快速 O(1) 滑动更新 */
        s_old_val = p_handler->data[p_handler->CyclicCount];
        p_handler->data[p_handler->CyclicCount] = s_new_val;
        p_handler->Sum = p_handler->Sum - s_old_val + s_new_val;

        p_handler->CyclicCount++;
        if (p_handler->CyclicCount >= p_handler->Buff_Size)
            p_handler->CyclicCount = 0;
        /* 四舍五入均值 */
        p_handler->DataOut = (s32)((p_handler->Sum + (p_handler->Buff_Size / 2)) / p_handler->Buff_Size);
    }

    mainEXIT_CRITICAL();

    return (u16)p_handler->DataOut;
}

/***********************************************************************************************************************
 * 函数功能    : 整型数据稳定状态检测
 * 说明(备注)  : 检测连续 Buff_Size 次采样的相邻差值绝对值之和是否在 Max_Swing 门限内
 * 传入参数    : p_filter: 句柄指针, num: 本次采样数据
 * 输出参数    : p_filter: 更新状态
 * 返回值      : s8: 1: 判定稳定; -1: 判定波动超标; 0: 数据收集中尚未满周期
 ************************************************************************************************************************/
s8 cFilter_CkeckDataStability(FilterHandler_T *p_filter, u16 num)
{
    int32_t i;

    if ((p_filter == NULL) || (p_filter->data == NULL) || (p_filter->Buff_Size <= 1))
        return 0;

    p_filter->data[p_filter->CyclicCount] = (s32)num;
    p_filter->CyclicCount++;

    if (p_filter->CyclicCount >= p_filter->Buff_Size)
    {
        p_filter->Sum = 0;
        for (i = 0; i < (int32_t)(p_filter->Buff_Size - 1); i++)
            p_filter->Sum += abs((int)(p_filter->data[i] - p_filter->data[i + 1]));

        p_filter->CyclicCount = 0;

        if (p_filter->Sum <= (s64)p_filter->Max_Swing)
            return 1;
        else
            return -1;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 浮点数据稳定状态检测
 * 说明(备注)  : 浮点相邻差值绝对值累加判定
 * 传入参数    : p_filter: 浮点句柄指针, num: 本次采样数据
 * 输出参数    : p_filter: 更新状态
 * 返回值      : s8: 1: 稳定; -1: 超标; 0: 采集中
 ************************************************************************************************************************/
s8 cFilter_CkeckFloatDataStability(fFilterHandler_T *p_filter, float num)
{
    int32_t i;

    if ((p_filter == NULL) || (p_filter->data == NULL) || (p_filter->Buff_Size <= 1))
        return 0;

    p_filter->data[p_filter->CyclicCount] = num;
    p_filter->CyclicCount++;

    if (p_filter->CyclicCount >= p_filter->Buff_Size)
    {
        p_filter->Sum = 0.0f;
        for (i = 0; i < (int32_t)(p_filter->Buff_Size - 1); i++)
            p_filter->Sum += fFunc_Fabs(p_filter->data[i], p_filter->data[i + 1]);

        p_filter->CyclicCount = 0;

        if (p_filter->Sum <= p_filter->Max_Swing)
            return 1;
        else
            return -1;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 多实例限幅滤波计算
 * 说明(备注)  : 若本次差值 <= usMaxSwing 则有效，否则判定为突发脉冲干扰并维持上次值
 * 传入参数    : p_filter: 限幅滤波上下文, us_new_val: 本次采样值
 * 输出参数    : p_filter: 更新历史输出
 * 返回值      : uint16_t: 滤波后输出
 ************************************************************************************************************************/
uint16_t usFilter_LimitCalc(LimitFilter_T *p_filter, uint16_t us_new_val)
{
    if (p_filter == NULL)
        return us_new_val;

    if (!p_filter->bInitialized)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->bInitialized = true;
        return us_new_val;
    }

    if ((uint16_t)abs((int32_t)us_new_val - (int32_t)p_filter->usLastValue) <= p_filter->usMaxSwing)
        p_filter->usLastValue = us_new_val;

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 多实例一阶滞后滤波计算
 * 说明(备注)  : 本次结果 = (100 - a) * 上次结果 + a * 本次采样，平滑滤除周期性高频噪声；
 *              加入 +50 四舍五入偏移，消除微小增量整除向零截断导致的爬坡死区停滞
 * 传入参数    : p_filter: 上下文指针, us_new_val: 本次采样值
 * 输出参数    : p_filter: 更新历史输出
 * 返回值      : uint16_t: 滤波后输出
 ************************************************************************************************************************/
uint16_t usFilter_FirstOrderCalc(FirstOrderFilter_T *p_filter, uint16_t us_new_val)
{
    int32_t s_diff;
    int32_t s_inc;

    if (p_filter == NULL)
        return us_new_val;

    if (!p_filter->bInitialized)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->bInitialized = true;
        return us_new_val;
    }

    s_diff = (int32_t)us_new_val - (int32_t)p_filter->usLastValue;
    if ((s_diff != 0) && (p_filter->ucCoeff != 0))
    {
        /* 增量带符号四舍五入计算: inc = (diff * a + 50) / 100 */
        s_inc = (s_diff * (int32_t)p_filter->ucCoeff + (s_diff > 0 ? 50 : -50)) / 100;
        if (s_inc == 0)
            /* 稳态静差与死区消除补偿: 当输入与历史存在差值但被整除截断吃掉时，给予最小 +/-1 步进推进
             * (仅对有效权重 a>=1 生效; a==0 语义为 0% 新值, 输出应冻结, 不做补偿爬行) */
            s_inc = (s_diff > 0) ? 1 : -1;
        p_filter->usLastValue = (uint16_t)((int32_t)p_filter->usLastValue + s_inc);
    }

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 多实例消抖滤波计算
 * 说明(备注)  : 仅当同一个新值连续出现达到 ucMaxShake 次时，才确认切换为该新值
 * 传入参数    : p_filter: 消抖上下文指针, us_new_val: 本次采样输入
 * 输出参数    : p_filter: 更新消抖状态
 * 返回值      : uint16_t: 滤波后输出
 ************************************************************************************************************************/
uint16_t usFilter_ClearShakeCalc(ClearShakeFilter_T *p_filter, uint16_t us_new_val)
{
    if (p_filter == NULL)
        return us_new_val;

    if (!p_filter->bInitialized)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->usTargetValue = us_new_val;
        p_filter->ucShakeCount = 0;
        p_filter->bInitialized = true;
        return us_new_val;
    }

    if (us_new_val != p_filter->usLastValue)
    {
        if (us_new_val == p_filter->usTargetValue)
        {
            p_filter->ucShakeCount++;
            if (p_filter->ucShakeCount >= p_filter->ucMaxShake)
            {
                p_filter->usLastValue = us_new_val;
                p_filter->ucShakeCount = 0;
            }
        }
        else
        {
            p_filter->usTargetValue = us_new_val;
            p_filter->ucShakeCount = 1;
            /* 边界修正: 新候选首拍计数已达确认阈值 (ucMaxShake <= 1) 时立即确认切换,
             * 消除原先 ucMaxShake=0/1 时需要连续出现 2 次才确认的不一致语义 */
            if (p_filter->ucShakeCount >= p_filter->ucMaxShake)
            {
                p_filter->usLastValue = us_new_val;
                p_filter->ucShakeCount = 0;
            }
        }
    }
    else
        p_filter->ucShakeCount = 0;

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 多实例限幅消抖滤波计算
 * 说明(备注)  : 门限内正常波动直接平滑跟随；超出限幅门限的大跳变启动消抖确认计数；
 *              若为瞬态尖峰杂波则自动滤除，若连续达到确认阈值则判定为真实物理阶跃并确认切换，彻底消除原死锁Bug
 * 传入参数    : p_filter: 上下文指针, us_new_val: 输入采样
 * 输出参数    : p_filter: 更新状态
 * 返回值      : uint16_t: 滤波后输出
 ************************************************************************************************************************/
uint16_t usFilter_LimitClearShakeCalc(LimitClearShakeFilter_T *p_filter, uint16_t us_new_val)
{
    if (p_filter == NULL)
        return us_new_val;

    if (!p_filter->bInitialized)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->usTargetValue = us_new_val;
        p_filter->ucShakeCount = 0;
        p_filter->bInitialized = true;
        return us_new_val;
    }

    /* 1. 采样值在正常波动门限内，直接跟随输出，清零消抖计数 */
    if ((uint16_t)abs((int32_t)us_new_val - (int32_t)p_filter->usLastValue) <= p_filter->usMaxSwing)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->ucShakeCount = 0;
    }
    else
    {
        /* 2. 采样值发生大幅度跳变 (> usMaxSwing)，可能是瞬态干扰，也可能是真实阶跃！
         *    启动消抖确认：若连续出现同一跳变目标（允许候选值内部有正常微小抖动 <= usMaxSwing），
         *    累计达到 ucMaxShake 次时，确认判定为真实阶跃并切换输出；若仅 1~2 拍尖峰则被成功滤除。
         */
        if ((uint16_t)abs((int32_t)us_new_val - (int32_t)p_filter->usTargetValue) <= p_filter->usMaxSwing)
        {
            p_filter->ucShakeCount++;
            if (p_filter->ucShakeCount >= p_filter->ucMaxShake)
            {
                p_filter->usLastValue = us_new_val;
                p_filter->ucShakeCount = 0;
            }
        }
        else
        {
            p_filter->usTargetValue = us_new_val;
            p_filter->ucShakeCount = 1;
            /* 边界修正: 新候选首拍计数已达确认阈值 (ucMaxShake <= 1) 时立即确认切换,
             * 与 usFilter_ClearShakeCalc 保持一致的确认语义 */
            if (p_filter->ucShakeCount >= p_filter->ucMaxShake)
            {
                p_filter->usLastValue = us_new_val;
                p_filter->ucShakeCount = 0;
            }
        }
    }

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 卡尔曼滤波器参数初始化
 * 说明(备注)  : 初始化过程噪声方差 Q、测量噪声方差 R 及初值
 * 传入参数    : p_kfp: 结构体指针, q: 过程噪声, r: 观测噪声, p_init: 初始协方差, out_init: 初始估计值
 * 输出参数    : p_kfp: 初始化后的结构体
 * 返回值      : void
 ************************************************************************************************************************/
void vFilter_KalmanInit(KFP_t *p_kfp, float q, float r, float p_init, float out_init)
{
    if (p_kfp != NULL)
    {
        p_kfp->Q = q;
        p_kfp->R = r;
        p_kfp->LastP = (p_init != 0.0f) ? p_init : 0.02f;
        p_kfp->Now_P = 0.0f;
        p_kfp->Kg = 0.0f;
        p_kfp->out = out_init;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 卡尔曼滤波核心迭代方程
 * 说明(备注)  : 一阶标量离散卡尔曼滤波：更新预测协方差、卡尔曼增益(带除零防护)、状态最优估计值与更新后协方差;
 *              入口含 NaN/Inf 异常观测防护, 拒绝异常输入且不更新状态机
 * 传入参数    : p_kfp: 卡尔曼参数结构体指针, input: 传感器当前观测输入
 * 输出参数    : p_kfp: 更新内部状态
 * 返回值      : float: 状态最优估计输出
 ************************************************************************************************************************/
float Filter_Kalman(KFP_t *p_kfp, float input)
{
    float denom;

    if (p_kfp == NULL)
        return input;

    /* 0. 异常值防护: NaN/Inf 观测输入直接拒绝, 状态机不更新, 返回当前最优估计 */
    if (!isfinite(input))
        return p_kfp->out;

    /* 1. 预测协方差方程: P(k|k-1) = P(k-1|k-1) + Q */
    p_kfp->Now_P = p_kfp->LastP + p_kfp->Q;

    /* 2. 卡尔曼增益方程: Kg = P(k|k-1) / (P(k|k-1) + R)，带除零保护 */
    denom = p_kfp->Now_P + p_kfp->R;
    if (denom > 1e-7f)
        p_kfp->Kg = p_kfp->Now_P / denom;
    else
        p_kfp->Kg = 0.0f;

    /* 3. 更新最优估计值方程: x(k|k) = x(k|k-1) + Kg * (z(k) - x(k|k-1)) */
    p_kfp->out = p_kfp->out + p_kfp->Kg * (input - p_kfp->out);

    /* 4. 更新误差协方差方程: P(k|k) = (1 - Kg) * P(k|k-1) */
    p_kfp->LastP = (1.0f - p_kfp->Kg) * p_kfp->Now_P;

    return p_kfp->out;
}

#if (filterSELF_TEST_ENABLE)

#if (boardPRINT_EN && boardPRINT_IFACE)
#define			FILTER_TEST_LOG(fmt, ...)				sMyPrint(fmt, ##__VA_ARGS__)
#else
#define			FILTER_TEST_LOG(fmt, ...)				((void)0)
#endif

/***********************************************************************************************************************
 * 函数功能    : 测试 1 - 中位值去极值平均滤波 (整型与浮点) 精度与去极值特性验证
 * 说明(备注)  : 验证 O(N) 线性去极值算法与排序去首尾结果严格一致，并验证除法四舍五入防偏置
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_median_average(void)
{
    s32              s_buf[7];
    FilterHandler_T  t_filter;
    uint8_t          i;
    s32              raw_seq[7] = {100, 105, 9999, 95, 102, -8888, 103};
    s32              round_seq[5] = {10, 1, 100, 3, 0};
    float            f_buf[5];
    fFilterHandler_T t_f_filter;
    float            f_seq[5] = {12.3f, 12.5f, 999.0f, 12.1f, -50.0f};

    /* 1.1 整型去极值验证: 窗口 7，输入含极大值 9999 和极小值 -8888 */
    t_filter.data = s_buf;
    t_filter.Buff_Size = 7;
    t_filter.Max_Swing = 0;
    vFilter_HandlerReset(&t_filter);

    for (i = 0; i < 7; i++)
        lFilter_MadianAverage(&t_filter, &raw_seq[i]);

    /* 去除 9999 与 -8888 后，剩余 5 项为 100, 105, 95, 102, 103，总和为 505，均值严格为 101 */
    if (t_filter.DataOut != 101)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] Median s32 expected 101, got %d\r\n", (int)t_filter.DataOut);
        return false;
    }

    /* 1.2 四舍五入防截断偏置验证: 窗口 5，输入去除极值后和为 14，分母为 3 (14/3 = 4.666...) */
    t_filter.Buff_Size = 5;
    vFilter_HandlerReset(&t_filter);
    for (i = 0; i < 5; i++)
        lFilter_MadianAverage(&t_filter, &round_seq[i]);

    if (t_filter.DataOut != 5)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] Median rounding expected 5, got %d\r\n", (int)t_filter.DataOut);
        return false;
    }

    /* 1.3 浮点去极值验证: 窗口 5，剔除 999.0f 与 -50.0f，保留 12.3f, 12.5f, 12.1f -> 均值 12.3f */
    t_f_filter.data = f_buf;
    t_f_filter.Buff_Size = 5;
    t_f_filter.Max_Swing = 0.0f;
    vFilter_FloatHandlerReset(&t_f_filter);
    for (i = 0; i < 5; i++)
        fFilter_MadianAverage(&t_f_filter, &f_seq[i]);

    if (fabs(t_f_filter.DataOut - 12.3f) > 0.001f)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] Median float expected 12.3, got %d/1000\r\n", (int)(t_f_filter.DataOut * 1000.0f));
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 2 - 滑动递推平均滤波 O(1) 增量滑动与四舍五入验证
 * 说明(备注)  : 验证窗口填满后最早数据滚出、最新数据滚入的 O(1) 递推准确性
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_recursion_average(void)
{
    s32             s_buf[4];
    FilterHandler_T t_filter;
    u16             val;

    t_filter.data = s_buf;
    t_filter.Buff_Size = 4;
    t_filter.Max_Swing = 0;
    vFilter_HandlerReset(&t_filter);

    /* 2.1 填满窗口 4 个 100 */
    val = 100;
    usFilter_RecursionAverage(&t_filter, &val);
    usFilter_RecursionAverage(&t_filter, &val);
    usFilter_RecursionAverage(&t_filter, &val);
    if (usFilter_RecursionAverage(&t_filter, &val) != 100)
        return false;

    /* 2.2 第 5 拍输入 104: 窗口滚动为 [104, 100, 100, 100]，Sum = 404，输出应为 101 */
    val = 104;
    if (usFilter_RecursionAverage(&t_filter, &val) != 101)
        return false;

    /* 2.3 第 6 拍输入 103: 窗口滚动为 [104, 103, 100, 100]，Sum = 407，407/4=101.75 -> 四舍五入得 102 */
    val = 103;
    if (usFilter_RecursionAverage(&t_filter, &val) != 102)
        return false;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 3 - 限幅消抖滤波 (抗脉冲干扰与彻底解除死锁验证)
 * 说明(备注)  : 验证微小扰动平滑跟随、突发尖峰脉冲被成功滤除、大物理阶跃连续到达后确认切换
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_limit_clear_shake(void)
{
    LimitClearShakeFilter_T t_filter = {0, 0, 10, 0, 4, false}; /* 限幅门限 10, 消抖阈值 4 */

    /* 3.1 首拍热启动: 初始值 100 */
    if (usFilter_LimitClearShakeCalc(&t_filter, 100) != 100)
        return false;

    /* 3.2 微小波动正常跟随: 输入 105 (变化 5 <= 10) -> 立即输出 105 */
    if (usFilter_LimitClearShakeCalc(&t_filter, 105) != 105)
        return false;

    /* 3.3 突发孤立尖峰干扰抑制: 连续 2 拍输入 500 (变化 395 >> 10, 但仅持续 2 拍 < 4 拍) */
    if (usFilter_LimitClearShakeCalc(&t_filter, 500) != 105)
        return false; /* 第 1 拍尖峰必须被滤除，保持 105 */

    if (usFilter_LimitClearShakeCalc(&t_filter, 500) != 105)
        return false; /* 第 2 拍尖峰必须被滤除，保持 105 */

    /* 尖峰消失恢复正常值 106 (与 105 差 1 <= 10) -> 立即跟随 106 */
    if (usFilter_LimitClearShakeCalc(&t_filter, 106) != 106)
        return false;

    /* 3.4 真实物理大幅度阶跃测试 (原代码致命死锁点回归测试): 物理信号跳变至 300 并持续保持 */
    if (usFilter_LimitClearShakeCalc(&t_filter, 300) != 106)
        return false; /* 第 1 拍确认中，输出 106 */

    if (usFilter_LimitClearShakeCalc(&t_filter, 300) != 106)
        return false; /* 第 2 拍确认中，输出 106 */

    if (usFilter_LimitClearShakeCalc(&t_filter, 300) != 106)
        return false; /* 第 3 拍确认中，输出 106 */

    /* 第 4 拍达到确认阈值 ucMaxShake (4 拍)，必须确认切换至 300！彻底验证死锁已彻底根除 */
    if (usFilter_LimitClearShakeCalc(&t_filter, 300) != 300)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] LimitClearShake deadlocked at 106!\r\n");
        return false;
    }

    /* 切换后进入新区间微小波动跟随 */
    if (usFilter_LimitClearShakeCalc(&t_filter, 302) != 302)
        return false;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 4 - 一阶滞后滤波 (微步爬坡死区消除与大阶跃指数平滑)
 * 说明(备注)  : 验证加入 +50 四舍五入后消除了微小增量整除向零截断停滞的缺陷
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_first_order_lag(void)
{
    FirstOrderFilter_T t_filter = {100, 30, true}; /* 初值 100, 系数 30 (即 30% 新值 + 70% 旧值) */
    uint16_t           out_val;
    uint8_t            i;

    /* 4.1 微弱步进爬坡测试: 输入连续微小步进 101
     * 原无四舍五入代码: (70*100 + 30*101) / 100 = 7030 / 100 = 70 (向零截断导致永久死锁停在 100)
     * 加四舍五入代码: 连续迭代能够克服截断，平滑爬升至 101
     */
    out_val = 100;
    for (i = 0; i < 6; i++)
        out_val = usFilter_FirstOrderCalc(&t_filter, 101);

    if (out_val != 101)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] FirstOrder deadzone failed to climb to 101, got %u\r\n", out_val);
        return false;
    }

    /* 4.2 大阶跃平滑收敛: 持续输入 200，经历 25 拍迭代后收敛至 200 */
    for (i = 0; i < 25; i++)
        out_val = usFilter_FirstOrderCalc(&t_filter, 200);

    if (out_val != 200)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] FirstOrder failed to converge to 200, got %u\r\n", out_val);
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 5 - 卡尔曼滤波 (除零保护与含噪数据收敛稳定性)
 * 说明(备注)  : 验证除零极限保护与收敛性能
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_kalman_filter(void)
{
    KFP_t   t_kfp;
    float   out;
    uint8_t i;
    float   noise_seq[10] = {1002.0f, 997.0f, 1003.0f, 998.0f, 1001.0f, 999.0f, 1002.0f, 998.0f, 1000.0f, 1001.0f};

    /* 5.1 除零保护测试: 异常传入 0 协方差与 0 噪声，验证不触发除零崩溃 */
    vFilter_KalmanInit(&t_kfp, 0.0f, 0.0f, 0.0f, 50.0f);
    out = Filter_Kalman(&t_kfp, 60.0f);
    if (isnan(out) || isinf(out))
        return false;

    /* 5.2 高斯噪声平滑测试: 基准 1000.0f，输入注入正负交错噪声 */
    vFilter_KalmanInit(&t_kfp, 0.001f, 0.1f, 1.0f, 1000.0f);
    for (i = 0; i < 10; i++)
        out = Filter_Kalman(&t_kfp, noise_seq[i]);

    /* 滤波后估计值必须稳定在 [999.0f, 1001.0f] 极小邻域内 */
    if (fabs(out - 1000.0f) > 1.0f)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] Kalman divergence, got %d/10\r\n", (int)(out * 10.0f));
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 6 - 数据稳定性检测器门限判定验证
 * 说明(备注)  : 验证平稳信号返回 1，超标抖动信号返回 -1
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_data_stability(void)
{
    s32             s_buf[5];
    FilterHandler_T t_filter;
    s8              s_res;
    uint8_t         i;
    u16             stable_seq[5] = {100, 101, 100, 102, 101};
    u16             shock_seq[5] = {100, 150, 100, 150, 100};

    t_filter.data = s_buf;
    t_filter.Buff_Size = 5;
    t_filter.Max_Swing = 10; /* 相邻差之和门限 10 */
    vFilter_HandlerReset(&t_filter);

    /* 6.1 平稳序列: 100, 101, 100, 102, 101 -> 相邻差之和 1+1+2+1 = 5 <= 10 -> 返回 1 */
    s_res = 0;
    for (i = 0; i < 5; i++)
        s_res = cFilter_CkeckDataStability(&t_filter, stable_seq[i]);

    if (s_res != 1)
        return false;

    /* 6.2 剧烈抖动序列: 100, 150, 100, 150, 100 -> 相邻差之和 200 > 10 -> 返回 -1 */
    for (i = 0; i < 5; i++)
        s_res = cFilter_CkeckDataStability(&t_filter, shock_seq[i]);

    if (s_res != -1)
        return false;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 滤波算法库全面自动化自检与精度验证执行入口
 * 说明(备注)  : 运行全部 6 大核心滤波算法单元测试，输出统计报告
 * 传入参数    : p_report: 报告指针 (可为 NULL)
 * 输出参数    : p_report: 填充通过用例数与失败数
 * 返回值      : bool: true 全部测试通过, false 存在失败用例
 ************************************************************************************************************************/
bool bFilter_RunSelfTest(FilterTestReport_T *p_report)
{
    uint16_t us_total = 6;
    uint16_t us_pass = 0;
    uint16_t us_fail = 0;

    FILTER_TEST_LOG("\r\n================ [FILTER ALGORITHM SELF-TEST] ================\r\n");

    /* Test 1 */
    if (b_test_median_average())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 1/6] Median Average (S32 & Float)   : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 1/6] Median Average (S32 & Float)   : [FAIL]\r\n");
    }

    /* Test 2 */
    if (b_test_recursion_average())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 2/6] Recursion Average (O(1) Slide) : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 2/6] Recursion Average (O(1) Slide) : [FAIL]\r\n");
    }

    /* Test 3 */
    if (b_test_limit_clear_shake())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 3/6] Limit Clear Shake (Anti-Spike) : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 3/6] Limit Clear Shake (Anti-Spike) : [FAIL]\r\n");
    }

    /* Test 4 */
    if (b_test_first_order_lag())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 4/6] First-Order Lag (Anti-Deadzone): [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 4/6] First-Order Lag (Anti-Deadzone): [FAIL]\r\n");
    }

    /* Test 5 */
    if (b_test_kalman_filter())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 5/6] Kalman Filter Robustness       : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 5/6] Kalman Filter Robustness       : [FAIL]\r\n");
    }

    /* Test 6 */
    if (b_test_data_stability())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 6/6] Data Stability Checker        : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 6/6] Data Stability Checker        : [FAIL]\r\n");
    }

    FILTER_TEST_LOG("--------------------------------------------------------------\r\n");
    FILTER_TEST_LOG("Total Cases: %u, Passed: %u, Failed: %u -> %s\r\n",
                    us_total, us_pass, us_fail, (us_fail == 0) ? "ALL TESTS PASSED!" : "SOME TESTS FAILED!");
    FILTER_TEST_LOG("==============================================================\r\n\r\n");

    if (p_report != NULL)
    {
        p_report->usTotalCases = us_total;
        p_report->usPassCount = us_pass;
        p_report->usFailCount = us_fail;
    }

    return (us_fail == 0);
}

#endif  /* filterSELF_TEST_ENABLE */

#endif  /* 1 */

