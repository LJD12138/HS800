/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\Filter
 * File    : filter_test.c
 * Date    : 2026-10-12
 * Author  : LJD(291483914@qq.com)
 * Desc    : 常用数字滤波算法中间件自动化自检与精度验证实现
 *           包含中位值去极值平均、滑动递推平均、限幅消抖、一阶滞后、卡尔曼及数据稳定性检测等 6 大核心算法单元测试
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "filter_test.h"
#include "filter.h"
#include <math.h>

#if (boardPRINT_EN && boardPRINT_IFACE)
#include "Print/print_api.h"
#endif

#if (filterSELF_TEST_ENABLE)
//****************************************************Macros********************************************************************//
#if (boardPRINT_EN && boardPRINT_IFACE)
#define			FILTER_TEST_LOG(fmt, ...)				sMyPrint(fmt, ##__VA_ARGS__)
#else
#define			FILTER_TEST_LOG(fmt, ...)				((void)0)
#endif


//****************************************************Parameter Initialization**************************************************//



//****************************************************Function Declaration******************************************************//
static bool b_test_median_average(void);
static bool b_test_recursion_average(void);
static bool b_test_limit_clear_shake(void);
static bool b_test_first_order_lag(void);
static bool b_test_kalman_filter(void);
static bool b_test_data_stability(void);
static bool b_test_weight_recur(void);
static bool b_test_limit_calc(void);

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
 * 函数功能    : 测试 2 - 滑动递推平均滤波 O(1) 增量滑动与 32位整型接口验证
 * 说明(备注)  : 验证窗口填满后最早数据滚出、最新数据滚入的 O(1) 递推准确性及负数处理
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_recursion_average(void)
{
    s32             s_buf[4];
    FilterHandler_T t_filter;
    u16             val16;
    s32             val32;

    t_filter.data = s_buf;
    t_filter.Buff_Size = 4;
    t_filter.Max_Swing = 0;
    vFilter_HandlerReset(&t_filter);

    /* 2.1 填满窗口 4 个 100 */
    val16 = 100;
    usFilter_RecursionAverage(&t_filter, &val16);
    usFilter_RecursionAverage(&t_filter, &val16);
    usFilter_RecursionAverage(&t_filter, &val16);
    if (usFilter_RecursionAverage(&t_filter, &val16) != 100)
        return false;

    /* 2.2 第 5 拍输入 104: 窗口滚动为 [104, 100, 100, 100]，Sum = 404，输出应为 101 */
    val16 = 104;
    if (usFilter_RecursionAverage(&t_filter, &val16) != 101)
        return false;

    /* 2.3 第 6 拍输入 103: 窗口滚动为 [104, 103, 100, 100]，Sum = 407，407/4=101.75 -> 四舍五入得 102 */
    val16 = 103;
    if (usFilter_RecursionAverage(&t_filter, &val16) != 102)
        return false;

    /* 2.4 32位通用负数递推测试 */
    vFilter_HandlerReset(&t_filter);
    val32 = -100;
    lFilter_RecursionAverage(&t_filter, &val32);
    lFilter_RecursionAverage(&t_filter, &val32);
    lFilter_RecursionAverage(&t_filter, &val32);
    if (lFilter_RecursionAverage(&t_filter, &val32) != -100)
        return false;

    val32 = -104;
    if (lFilter_RecursionAverage(&t_filter, &val32) != -101)
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
 * 函数功能    : 测试 4 - 一阶滞后滤波 (整型死区消除与 Q15 高精度定点滤波验证)
 * 说明(备注)  : 验证加入 +50 四舍五入后消除了微小增量整除向零截断停滞的缺陷，并验证 Q15 定点滤波收敛性
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_first_order_lag(void)
{
    FirstOrderFilter_T    t_filter = {100, 30, true};
    FirstOrderQ15Filter_T t_q15;
    uint16_t              out_val;
    uint8_t               i;

    /* 4.1 整型微弱步进爬坡测试: 输入连续微小步进 101 */
    out_val = 100;
    for (i = 0; i < 6; i++)
        out_val = usFilter_FirstOrderCalc(&t_filter, 101);

    if (out_val != 101)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] FirstOrder deadzone failed to climb to 101, got %u\r\n", out_val);
        return false;
    }

    /* 4.2 整型大阶跃平滑收敛 */
    for (i = 0; i < 25; i++)
        out_val = usFilter_FirstOrderCalc(&t_filter, 200);

    if (out_val != 200)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] FirstOrder failed to converge to 200, got %u\r\n", out_val);
        return false;
    }

    /* 4.3 Q15 定点一阶滤波测试: 系数 a = 0.3 (0.3 * 32768 = 9830) */
    vFilter_FirstOrderQ15Reset(&t_q15, 100, 9830);
    for (i = 0; i < 25; i++)
        out_val = usFilter_FirstOrderQ15Calc(&t_q15, 200);

    if (out_val != 200)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] FirstOrderQ15 failed to converge to 200, got %u\r\n", out_val);
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 5 - 卡尔曼滤波 (除零保护、协方差非负与噪声抑制)
 * 说明(备注)  : 验证除零极限保护与协方差非负性截断
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_kalman_filter(void)
{
    KFP_t t_kfp;
    float    out;
    uint8_t  i;
    float    noise_seq[10] = {1002.0f, 997.0f, 1003.0f, 998.0f, 1001.0f, 999.0f, 1002.0f, 998.0f, 1000.0f, 1001.0f};

    /* 5.1 除零保护测试: 异常传入 0 协方差与 0 噪声 */
    vFilter_KalmanInit(&t_kfp, 0.0f, 0.0f, 0.0f, 50.0f);
    out = fFilter_KalmanCalc(&t_kfp, 60.0f);
    if (isnan(out) || isinf(out))
        return false;

    /* 5.2 协方差非负性保护测试 */
    if (t_kfp.LastP < 0.0f)
        return false;

    /* 5.3 高斯噪声平滑测试: 基准 1000.0f */
    vFilter_KalmanInit(&t_kfp, 0.001f, 0.1f, 1.0f, 1000.0f);
    for (i = 0; i < 10; i++)
        out = fFilter_KalmanCalc(&t_kfp, noise_seq[i]);

    if (fabs(out - 1000.0f) > 1.0f)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] Kalman divergence, got %d/10\r\n", (int)(out * 10.0f));
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 6 - 数据稳定性检测 (平稳信号、剧烈跳变与斜坡漂移拒绝)
 * 说明(备注)  : 验证平稳信号返回 1，超标跳变与斜坡漂移返回 -1
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
    u16             drift_seq[5] = {100, 104, 108, 112, 116}; /* 单调漂移: 相邻差和 16, 极差 16 > 10 */

    t_filter.data = s_buf;
    t_filter.Buff_Size = 5;
    t_filter.Max_Swing = 10;
    vFilter_HandlerReset(&t_filter);

    /* 6.1 平稳序列: 相邻差之和 5 <= 10, 极差 2 <= 10 -> 返回 1 */
    s_res = 0;
    for (i = 0; i < 5; i++)
        s_res = cFilter_CheckDataStability(&t_filter, stable_seq[i]);

    if (s_res != 1)
        return false;

    /* 6.2 剧烈抖动序列 -> 返回 -1 */
    for (i = 0; i < 5; i++)
        s_res = cFilter_CheckDataStability(&t_filter, shock_seq[i]);

    if (s_res != -1)
        return false;

    /* 6.3 斜坡单调漂移 -> 极差超标拒绝误判为稳定 */
    for (i = 0; i < 5; i++)
        s_res = cFilter_CheckDataStability(&t_filter, drift_seq[i]);

    if (s_res != -1)
        return false;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 7 - 多实例加权递推平均滤波加权精度验证
 * 说明(备注)  : 验证越新的数据赋予更高权重的加权计算准确性
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_weight_recur(void)
{
    WeightRecurFilter_T t_filter;
    uint16_t            out;

    vFilter_WeightRecurReset(&t_filter);

    /* 7.1 单点热启动 */
    out = usFilter_WeightRecurCalc(&t_filter, 100);
    if (out != 100)
        return false;

    /* 7.2 输入恒定序列 100 */
    usFilter_WeightRecurCalc(&t_filter, 100);
    out = usFilter_WeightRecurCalc(&t_filter, 100);
    if (out != 100)
        return false;

    /* 7.3 加权阶跃响应验证:
     * 3 点序列: [100 (w=1), 100 (w=2), 160 (w=3)]
     * 总和 = 100*1 + 100*2 + 160*3 = 100 + 200 + 480 = 780
     * 权重和 = 1 + 2 + 3 = 6 -> 780 / 6 = 130
     */
    vFilter_WeightRecurReset(&t_filter);
    usFilter_WeightRecurCalc(&t_filter, 100);
    usFilter_WeightRecurCalc(&t_filter, 100);
    out = usFilter_WeightRecurCalc(&t_filter, 160);

    if (out != 130)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] WeightRecur expected 130, got %u\r\n", out);
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 测试 8 - 限幅滤波斜率限制 (Rate Limiter) 防死锁平滑跟踪验证
 * 说明(备注)  : 验证超出门限时按最大允许斜率逼近跟踪，彻底杜绝死锁卡死
 * 传入参数    : none
 * 输出参数    : 无
 * 返回值      : bool: true 验证通过, false 验证失败
 ************************************************************************************************************************/
static bool b_test_limit_calc(void)
{
    LimitFilter_T t_filter = {0, 10, false}; /* 步长门限 10 */
    uint16_t      out;

    /* 8.1 初值有效 */
    out = usFilter_LimitCalc(&t_filter, 100);
    if (out != 100)
        return false;

    /* 8.2 门限内波动正常跟随 */
    out = usFilter_LimitCalc(&t_filter, 108);
    if (out != 108)
        return false;

    /* 8.3 发生大物理阶跃至 138 (超出门限 10):
     * 斜率限制算法以每次 +10 逼近: 118 -> 128 -> 138 顺利跟踪阶跃！
     */
    out = usFilter_LimitCalc(&t_filter, 138);
    if (out != 118)
        return false;

    out = usFilter_LimitCalc(&t_filter, 138);
    if (out != 128)
        return false;

    out = usFilter_LimitCalc(&t_filter, 138);
    if (out != 138)
    {
        FILTER_TEST_LOG("[FILTER TEST FAIL] LimitCalc rate limit expected 138, got %u\r\n", out);
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 滤波算法库全面自动化自检与精度验证执行入口
 * 说明(备注)  : 运行全部 8 大核心滤波算法单元测试，输出统计报告
 * 传入参数    : p_report: 报告指针 (可为 NULL)
 * 输出参数    : p_report: 填充通过用例数与失败数
 * 返回值      : bool: true 全部测试通过, false 存在失败用例
 ************************************************************************************************************************/
bool bFilter_RunSelfTest(FilterTestReport_T *p_report)
{
    uint16_t us_total = 8;
    uint16_t us_pass = 0;
    uint16_t us_fail = 0;

    FILTER_TEST_LOG("\r\n================ [FILTER ALGORITHM SELF-TEST] ================\r\n");

    /* Test 1 */
    if (b_test_median_average())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 1/8] Median Average (S32 & Float)   : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 1/8] Median Average (S32 & Float)   : [FAIL]\r\n");
    }

    /* Test 2 */
    if (b_test_recursion_average())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 2/8] Recursion Average (O(1) Slide) : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 2/8] Recursion Average (O(1) Slide) : [FAIL]\r\n");
    }

    /* Test 3 */
    if (b_test_limit_clear_shake())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 3/8] Limit Clear Shake (Anti-Spike) : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 3/8] Limit Clear Shake (Anti-Spike) : [FAIL]\r\n");
    }

    /* Test 4 */
    if (b_test_first_order_lag())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 4/8] First-Order (Int & Q15 FixPt)  : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 4/8] First-Order (Int & Q15 FixPt)  : [FAIL]\r\n");
    }

    /* Test 5 */
    if (b_test_kalman_filter())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 5/8] Kalman Filter (Covariance Safe): [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 5/8] Kalman Filter (Covariance Safe): [FAIL]\r\n");
    }

    /* Test 6 */
    if (b_test_data_stability())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 6/8] Stability (Diff & Peak-to-Peak): [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 6/8] Stability (Diff & Peak-to-Peak): [FAIL]\r\n");
    }

    /* Test 7 */
    if (b_test_weight_recur())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 7/8] Weight Recursive Average       : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 7/8] Weight Recursive Average       : [FAIL]\r\n");
    }

    /* Test 8 */
    if (b_test_limit_calc())
    {
        us_pass++;
        FILTER_TEST_LOG("[TEST 8/8] Limit Rate Limiter (Anti-Dead) : [PASS]\r\n");
    }
    else
    {
        us_fail++;
        FILTER_TEST_LOG("[TEST 8/8] Limit Rate Limiter (Anti-Dead) : [FAIL]\r\n");
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

