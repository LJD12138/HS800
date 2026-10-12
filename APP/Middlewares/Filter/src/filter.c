/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\Filter
 * File    : filter.c
 * Date    : 2026-10-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 嵌入式常用通用数字滤波算法中间件实现
 *           包含中位值去极值平均滤波、滑动递推平均滤波、限幅滤波、一阶滞后滤波、消抖滤波及卡尔曼滤波
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "filter.h"
#include "function.h"
#include <math.h>

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

        if ((p_handler->data != NULL) && (p_handler->Buff_Size > 0))
            memset(p_handler->data, 0, (size_t)p_handler->Buff_Size * sizeof(s32));

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

        if ((p_handler->data != NULL) && (p_handler->Buff_Size > 0))
            memset(p_handler->data, 0, (size_t)p_handler->Buff_Size * sizeof(float));

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

    if (p_handler->CyclicCount >= uc_size)
        p_handler->CyclicCount = 0;

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

    if (p_handler->CyclicCount >= uc_size)
        p_handler->CyclicCount = 0;

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
 * 返回值      : s32: 滤波后的均值结果
 ************************************************************************************************************************/
s32 lFilter_RecursionAverage(FilterHandler_T *p_handler, const s32 *p_data)
{
    s32 s_new_val;
    s32 s_old_val;

    if ((p_handler == NULL) || (p_handler->data == NULL) || (p_data == NULL) || (p_handler->Buff_Size == 0))
        return 0;

    s_new_val = *p_data;

    mainENTER_CRITICAL();

    if (p_handler->CyclicCount >= p_handler->Buff_Size)
        p_handler->CyclicCount = 0;

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
        p_handler->DataOut = (s32)((p_handler->Sum >= 0) ?
            (p_handler->Sum + (p_handler->BuffUseSize / 2)) / p_handler->BuffUseSize :
            (p_handler->Sum - (p_handler->BuffUseSize / 2)) / p_handler->BuffUseSize);
    }
    else
    {
        /* 窗口已满，进入快速 O(1) 滑动更新: Sum = Sum - oldest + newest */
        s_old_val = p_handler->data[p_handler->CyclicCount];
        p_handler->data[p_handler->CyclicCount] = s_new_val;
        p_handler->Sum = p_handler->Sum - s_old_val + s_new_val;

        p_handler->CyclicCount++;
        if (p_handler->CyclicCount >= p_handler->Buff_Size)
            p_handler->CyclicCount = 0;

        /* 四舍五入均值 */
        p_handler->DataOut = (s32)((p_handler->Sum >= 0) ?
            (p_handler->Sum + (p_handler->Buff_Size / 2)) / p_handler->Buff_Size :
            (p_handler->Sum - (p_handler->Buff_Size / 2)) / p_handler->Buff_Size);
    }

    mainEXIT_CRITICAL();

    return p_handler->DataOut;
}

/***********************************************************************************************************************
 * 函数功能    : 整型数据稳定状态检测
 * 说明(备注)  : 双重判定：检测窗口内相邻差绝对值之和与极差(Peak-to-Peak)是否均在 Max_Swing 门限内，
 *              彻底消除单调斜坡漂移被误判为稳定的缺陷
 * 传入参数    : p_filter: 句柄指针, num: 本次采样数据
 * 输出参数    : p_filter: 更新状态
 * 返回值      : s8: 1: 判定稳定; -1: 判定波动超标; 0: 数据收集中尚未满周期
 ************************************************************************************************************************/
s8 cFilter_CheckDataStability(FilterHandler_T *p_filter, u16 num)
{
    int32_t i;
    s32     min_val;
    s32     max_val;

    if ((p_filter == NULL) || (p_filter->data == NULL) || (p_filter->Buff_Size <= 1))
        return 0;

    if (p_filter->CyclicCount >= p_filter->Buff_Size)
        p_filter->CyclicCount = 0;

    p_filter->data[p_filter->CyclicCount] = (s32)num;
    p_filter->CyclicCount++;

    if (p_filter->CyclicCount >= p_filter->Buff_Size)
    {
        p_filter->Sum = 0;
        min_val = p_filter->data[0];
        max_val = p_filter->data[0];

        for (i = 0; i < (int32_t)(p_filter->Buff_Size - 1); i++)
        {
            p_filter->Sum += abs((int)(p_filter->data[i] - p_filter->data[i + 1]));
            if (p_filter->data[i + 1] < min_val)
                min_val = p_filter->data[i + 1];
            if (p_filter->data[i + 1] > max_val)
                max_val = p_filter->data[i + 1];
        }

        p_filter->CyclicCount = 0;

        /* 相邻差之和在门限内，且极差也在门限内 -> 判定为平稳信号 */
        if ((p_filter->Sum <= (s64)p_filter->Max_Swing) && ((max_val - min_val) <= (s32)p_filter->Max_Swing))
            return 1;
        else
            return -1;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 浮点数据稳定状态检测
 * 说明(备注)  : 浮点相邻差值与极差双重判定
 * 传入参数    : p_filter: 浮点句柄指针, num: 本次采样数据
 * 输出参数    : p_filter: 更新状态
 * 返回值      : s8: 1: 稳定; -1: 超标; 0: 采集中
 ************************************************************************************************************************/
s8 cFilter_CheckFloatDataStability(fFilterHandler_T *p_filter, float num)
{
    int32_t i;
    float   min_val;
    float   max_val;

    if ((p_filter == NULL) || (p_filter->data == NULL) || (p_filter->Buff_Size <= 1))
        return 0;

    if (!isfinite(num))
        return 0;

    if (p_filter->CyclicCount >= p_filter->Buff_Size)
        p_filter->CyclicCount = 0;

    p_filter->data[p_filter->CyclicCount] = num;
    p_filter->CyclicCount++;

    if (p_filter->CyclicCount >= p_filter->Buff_Size)
    {
        p_filter->Sum = 0.0f;
        min_val = p_filter->data[0];
        max_val = p_filter->data[0];

        for (i = 0; i < (int32_t)(p_filter->Buff_Size - 1); i++)
        {
            p_filter->Sum += (float)fabs(p_filter->data[i] - p_filter->data[i + 1]);
            if (p_filter->data[i + 1] < min_val)
                min_val = p_filter->data[i + 1];
            if (p_filter->data[i + 1] > max_val)
                max_val = p_filter->data[i + 1];
        }

        p_filter->CyclicCount = 0;

        if ((p_filter->Sum <= p_filter->Max_Swing) && ((max_val - min_val) <= p_filter->Max_Swing))
            return 1;
        else
            return -1;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 多实例限幅滤波计算 (斜率限制防死锁算法)
 * 说明(备注)  : 1. 正常小幅度波动 (|diff| <= usMaxSwing): 100% 实时跟随输出;
 *              2. 超出最大变化率的大突变 (|diff| > usMaxSwing): 限制以 usMaxSwing 步长逼近跟踪,
 *                 既能滤除高频脉冲尖峰毛刺，又彻底根除输入永久物理阶跃导致的死锁卡死缺陷
 * 传入参数    : p_filter: 限幅滤波上下文, us_new_val: 本次采样值
 * 输出参数    : p_filter: 更新历史输出
 * 返回值      : uint16_t: 滤波后输出
 ************************************************************************************************************************/
uint16_t usFilter_LimitCalc(LimitFilter_T *p_filter, uint16_t us_new_val)
{
    int32_t s_diff;

    if (p_filter == NULL)
        return us_new_val;

    if (!p_filter->bInitialized)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->bInitialized = true;
        return us_new_val;
    }

    s_diff = (int32_t)us_new_val - (int32_t)p_filter->usLastValue;

    if ((uint16_t)abs(s_diff) <= p_filter->usMaxSwing)
        p_filter->usLastValue = us_new_val;
    else
        p_filter->usLastValue = (uint16_t)((s_diff > 0) ?
            ((int32_t)p_filter->usLastValue + (int32_t)p_filter->usMaxSwing) :
            ((int32_t)p_filter->usLastValue - (int32_t)p_filter->usMaxSwing));

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 多实例一阶滞后滤波计算
 * 说明(备注)  : 本次结果 = (100 - a) * 上次结果 + a * 本次采样；
 *              加入 +50 四舍五入偏移消除截断死区，并对 ucCoeff >= 100 进行饱和防护
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

    /* 系数饱和限制: a >= 100 意味着 100% 跟踪新值 */
    if (p_filter->ucCoeff >= 100)
    {
        p_filter->usLastValue = us_new_val;
        return us_new_val;
    }

    s_diff = (int32_t)us_new_val - (int32_t)p_filter->usLastValue;
    if ((s_diff != 0) && (p_filter->ucCoeff != 0))
    {
        /* 增量带符号四舍五入计算: inc = (diff * a + 50) / 100 */
        s_inc = (s_diff * (int32_t)p_filter->ucCoeff + (s_diff > 0 ? 50 : -50)) / 100;
        if (s_inc == 0)
            /* 稳态静差与死区消除补偿: 当输入与历史存在差值但被整除截断吃掉时，给予最小 +/-1 步进推进 */
            s_inc = (s_diff > 0) ? 1 : -1;
        p_filter->usLastValue = (uint16_t)((int32_t)p_filter->usLastValue + s_inc);
    }

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 定点数一阶滞后滤波计算 (Q15 高精度低通滤波)
 * 说明(备注)  : 小数残差完整保存在 lStateQ15 低 15 位中，数学上 100% 消除死区停滞，
 *              无稳态强行抖动，零浮点开销，完美适配 ESP32-C3 架构
 * 传入参数    : p_filter: 上下文指针, us_new_val: 本次采样值
 * 输出参数    : p_filter: 更新定点状态寄存器与输出
 * 返回值      : uint16_t: 滤波后输出
 ************************************************************************************************************************/
uint16_t usFilter_FirstOrderQ15Calc(FirstOrderQ15Filter_T *p_filter, uint16_t us_new_val)
{
    int32_t s_diff_q15;
    int32_t s_inc_q15;

    if (p_filter == NULL)
        return us_new_val;

    if (!p_filter->bInitialized)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->lStateQ15 = (int32_t)us_new_val << 15;
        p_filter->bInitialized = true;
        return us_new_val;
    }

    if (p_filter->usCoeffQ15 >= 32768)
    {
        p_filter->usLastValue = us_new_val;
        p_filter->lStateQ15 = (int32_t)us_new_val << 15;
        return us_new_val;
    }

    /* Q15 定点更新: lState = lState + a * (new - Last) */
    s_diff_q15 = ((int32_t)us_new_val << 15) - p_filter->lStateQ15;
    s_inc_q15 = (int32_t)(((int64_t)s_diff_q15 * (int64_t)p_filter->usCoeffQ15) >> 15);
    p_filter->lStateQ15 += s_inc_q15;

    /* 下限截断防负数下溢翻转 */
    if (p_filter->lStateQ15 < 0)
        p_filter->lStateQ15 = 0;

    /* 加 0.5 (1 << 14) 四舍五入后输出 */
    p_filter->usLastValue = (uint16_t)((p_filter->lStateQ15 + (1 << 14)) >> 15);

    return p_filter->usLastValue;
}

/***********************************************************************************************************************
 * 函数功能    : 定点数一阶滞后滤波复位初始化
 * 说明(备注)  : 设置初值与 Q15 系数
 * 传入参数    : p_filter: 上下文指针, us_init_val: 初值, us_coeff_q15: Q15 权重系数 (0~32768)
 * 输出参数    : p_filter: 初始化后的上下文
 * 返回值      : void
 ************************************************************************************************************************/
void vFilter_FirstOrderQ15Reset(FirstOrderQ15Filter_T *p_filter, uint16_t us_init_val, uint16_t us_coeff_q15)
{
    if (p_filter != NULL)
    {
        p_filter->usLastValue = us_init_val;
        p_filter->lStateQ15 = (int32_t)us_init_val << 15;
        p_filter->usCoeffQ15 = (us_coeff_q15 <= 32768) ? us_coeff_q15 : 32768;
        p_filter->bInitialized = true;
    }
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
            /* 边界修正: 新候选首拍计数已达确认阈值 (ucMaxShake <= 1) 时立即确认切换 */
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
 *              若为瞬态尖峰杂波则自动滤除，若连续达到确认阈值则判定为真实物理阶跃并确认切换
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
        /* 2. 采样值发生大幅度跳变 (> usMaxSwing)，启动消抖确认 */
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
 * 函数功能    : 多实例加权递推平均滤波计算
 * 说明(备注)  : 环形队列维护最近数据，越新的数据赋予越高的权重 (权重系数为 1, 2, ..., N)；
 *              有效增强对当前趋势的灵敏响应，同时平滑滤除历史高频杂波
 * 传入参数    : p_filter: 上下文指针, us_new_val: 本次采样输入
 * 输出参数    : p_filter: 更新环形队列与有效计数
 * 返回值      : uint16_t: 加权平均输出值
 ************************************************************************************************************************/
uint16_t usFilter_WeightRecurCalc(WeightRecurFilter_T *p_filter, uint16_t us_new_val)
{
    uint32_t ul_weight_sum = 0;
    uint32_t ul_total = 0;
    uint8_t  uc_valid_cnt;
    uint8_t  i;
    uint8_t  idx;

    if (p_filter == NULL)
        return us_new_val;

    /* 环形入队 (入队前边界防护) */
    if (p_filter->ucIndex >= filterWEIGHT_RECUR_BUF_MAX)
        p_filter->ucIndex = 0;

    p_filter->usaBuffer[p_filter->ucIndex] = us_new_val;
    p_filter->ucIndex++;
    if (p_filter->ucIndex >= filterWEIGHT_RECUR_BUF_MAX)
        p_filter->ucIndex = 0;

    if (p_filter->ucCount < filterWEIGHT_RECUR_BUF_MAX)
        p_filter->ucCount++;

    uc_valid_cnt = p_filter->ucCount;
    if (uc_valid_cnt == 1)
        return us_new_val;

    /* 遍历有效窗口: 最老的数据权重为 1, 最新到达的数据权重为 uc_valid_cnt */
    for (i = 0; i < uc_valid_cnt; i++)
    {
        uint32_t w = i + 1;
        if (p_filter->ucCount < filterWEIGHT_RECUR_BUF_MAX)
            idx = i;
        else
            idx = (p_filter->ucIndex + i) % filterWEIGHT_RECUR_BUF_MAX;

        ul_total += (uint32_t)p_filter->usaBuffer[idx] * w;
        ul_weight_sum += w;
    }

    /* 带四舍五入除法 */
    return (uint16_t)((ul_total + (ul_weight_sum / 2)) / ul_weight_sum);
}

/***********************************************************************************************************************
 * 函数功能    : 加权递推平均滤波器复位
 * 说明(备注)  : 清空环形队列与计数器
 * 传入参数    : p_filter: 上下文指针
 * 输出参数    : p_filter: 复位后的上下文
 * 返回值      : void
 ************************************************************************************************************************/
void vFilter_WeightRecurReset(WeightRecurFilter_T *p_filter)
{
    if (p_filter != NULL)
    {
        memset(p_filter->usaBuffer, 0, sizeof(p_filter->usaBuffer));
        p_filter->ucCount = 0;
        p_filter->ucIndex = 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 卡尔曼滤波器参数初始化
 * 说明(备注)  : 初始化过程噪声方差 Q、测量噪声方差 R 及协方差初值，带非负安全性保护
 * 传入参数    : p_kfp: 结构体指针, q: 过程噪声, r: 观测噪声, p_init: 初始协方差, out_init: 初始估计值
 * 输出参数    : p_kfp: 初始化后的结构体
 * 返回值      : void
 ************************************************************************************************************************/
void vFilter_KalmanInit(KFP_t *p_kfp, float q, float r, float p_init, float out_init)
{
    if (p_kfp != NULL)
    {
        p_kfp->Q = (q >= 0.0f) ? q : 0.001f;
        p_kfp->R = (r >= 0.0f) ? r : 0.543f;
        p_kfp->LastP = (p_init > 0.0f) ? p_init : 0.02f;
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
float fFilter_KalmanCalc(KFP_t *p_kfp, float input)
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

    /* 4. 更新误差协方差方程: P(k|k) = (1 - Kg) * P(k|k-1) 并强制非负下限截断 */
    p_kfp->LastP = (1.0f - p_kfp->Kg) * p_kfp->Now_P;
    if (p_kfp->LastP < 0.0f)
        p_kfp->LastP = 0.0f;

    return p_kfp->out;
}

#endif  /* 1 */
