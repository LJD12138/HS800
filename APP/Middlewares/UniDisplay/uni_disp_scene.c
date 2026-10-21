/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\UniDisplay
 * File    : uni_disp_scene.c
 * Date    : 2026-10-19
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 场景流程状态机实现 - 唯一导航权威:
 *           overlay 电平评估 (激活条件经项目 ops, 项目可扩展) -> 场景边界仲裁 -> 步骤推进 (完成条件/超时兜底);
 *           overlay 激活期间主流程暂停 (步进/驻留计时冻结), 事件分发与渲染不受影响
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "uni_disp_scene.h"

#if (boardDISPLAY_EN)
#include "uni_disp_core.h"      /* pDisp_GetOps / usDisp_GetFramePeriod / vDisp_RequestPage */
#include "uni_disp_nav.h"       /* eDisp_NavGetCurrentPageId / pDisp_NavGetPage / vDisp_NavStackClear */

#if (boardDISP_SCENE_EN)


//****************************************************Parameter Initialization**************************************************//
/*----------------场景流程状态机 (Scene Manager)----------------*/
static DispScene_E       S_eScene        = dispSCENE_NONE; /* 当前场景 */
static uint8_t           S_ucStep        = 0;               /* 当前步骤索引 */
static uint32_t          S_ulStepDwellMs = 0;               /* 当前步骤驻留毫秒数 */
static uint32_t          S_ulStepStartMs = 0;               /* 当前步骤起始时间戳 (tick 有效时使用) */
static bool              S_bUseTick      = false;           /* true: 用项目 tick 计驻留; false: 按帧周期累计 */
static bool              S_bOverlayOn    = false;           /* overlay 弹层激活标志 */
static DispScene_E       S_eSavedScene   = dispSCENE_NONE; /* overlay 现场: 场景 */
static uint8_t           S_ucSavedStep   = 0;               /* overlay 现场: 步骤 */
static DispPageId_E      S_eSavedPage    = dispPAGE_NONE;   /* overlay 现场: 当前页 ID (组内记忆, 空页兜底) */
static uint32_t          S_ulSavedDwellMs = 0;              /* overlay 现场: 步骤已驻留 ms (弹出后冻结恢复) */

//****************************************************Function Declaration******************************************************//
static DispPageId_E e_disp_step_page(DispScene_E e_scene, uint8_t uc_step);
static uint32_t ul_disp_scene_tick(void);

/***********************************************************************************************************************
 * 函数功能    : 场景状态机初始化
 * 说明(备注)  : 框架核心初始化时调用; 无场景, 首帧由场景边界仲裁驱动
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_SceneInit(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    S_eScene        = dispSCENE_NONE;
    S_ucStep        = 0;
    S_ulStepDwellMs = 0;
    S_bOverlayOn    = false;
    S_eSavedScene   = dispSCENE_NONE;
    S_ucSavedStep   = 0;
    S_eSavedPage    = dispPAGE_NONE;
    S_ulSavedDwellMs = 0;

    /* 项目提供毫秒时基则按墙钟计时, 否则退回按帧周期累计 (兼容裸机/无时基项目) */
    S_bUseTick      = ((p_ops != NULL) && (p_ops->ulGetTickMs != NULL));
    S_ulStepStartMs = 0U;                       /* 首个场景边界会重新取时间戳 */
}

/***********************************************************************************************************************
 * 函数功能    : 场景流程调度 (仅显示任务上下文, 唯一导航权威)
 * 说明(备注)  : ① overlay 以电平语义每帧评估 (激活条件经项目 ops, 项目可扩展):
 *                 压入时保存 (场景/步骤/当前页ID) 现场, 弹出时优先恢复页记忆 (组内现场不丢),
 *                 无页记忆兜底回步骤表页;
 *               ② 系统状态仅在场景边界决定进入哪个场景, 场景内部状态边沿无权直接切页;
 *               ③ 步骤推进: 步骤完成条件 bDone (项目策略) 与步骤页 bOnDone (页面内在流程自完成,
 *                 如开机动画走满) 与合成 (各自 NULL 视为满足, 均 NULL 为常驻步), 驻留超时 usMaxHoldMs 兜底, 末步驻留 (常驻);
 *               ④ overlay 激活期间主流程暂停 (步进/驻留计时冻结, 弹出后按已驻留时间恢复计时),
 *                  事件分发与渲染不受影响。
 * 传入参数    : p_snapshot: [IN] 快照数据 (NULL 时直接返回)
 *               uc_dev_state: [IN] 快照设备状态 (引擎已适配提取, 避免本域重复调用项目 ops)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_SceneSchedule(const DispDataSnapshot_T *p_snapshot, uint8_t uc_dev_state)
{
    const DispOps_T       *p_ops = pDisp_GetOps();
    DispScene_E            e_target = dispSCENE_NONE;
    const DispFlowStep_T  *p_step = NULL;
    const DispPage_T      *p_step_page = NULL;
    bool                   b_advance = false;
    uint8_t                uc_num = 0;
    uint32_t               ul_tick;

    if (p_snapshot == NULL)
        return;

    ul_tick = ul_disp_scene_tick();

    /*----------a. overlay 电平评估 (激活条件由项目 ops 提供, 每帧判定天然无边沿丢失)----------*/
    if ((p_ops != NULL) && (p_ops->bOverlayActive != NULL) && p_ops->bOverlayActive(p_snapshot))
    {
        DispPageId_E e_overlay = dispPAGE_NONE;

        if (p_ops->eOverlayPage != NULL)
            e_overlay = p_ops->eOverlayPage();

        /* overlay 页无效时忽略本次 overlay, 避免主流程被永久冻结 */
        if (pDisp_NavGetPage(e_overlay) != NULL)
        {
            if (!S_bOverlayOn)                      /* 压入: 保存现场 (含已驻留时间, 弹出冻结恢复) */
            {
                S_eSavedScene    = S_eScene;
                S_ucSavedStep    = S_ucStep;
                S_eSavedPage     = eDisp_NavGetCurrentPageId();
                S_ulSavedDwellMs = S_ulStepDwellMs;
                S_bOverlayOn     = true;
                vDisp_NavStackClear();              /* 抢占清页面栈 (防跨层级残留, 与弹窗栈语义一致) */
                vDisp_RequestPage(e_overlay);
            }
            return;                                 /* overlay 期间主流程暂停 (步进/驻留计时冻结) */
        }
    }

    if (S_bOverlayOn)                           /* 弹出: 恢复现场 (有页记忆回页, 无则回步骤表页) */
    {
        S_bOverlayOn    = false;
        S_eScene        = S_eSavedScene;
        S_ucStep        = S_ucSavedStep;
        S_ulStepDwellMs = S_ulSavedDwellMs;     /* 驻留计时冻结恢复: 叠加 overlay 前已驻留时间 */
        S_ulStepStartMs = ul_tick - S_ulSavedDwellMs;
        if (S_eSavedPage < dispPAGE_REG_MAX)
            vDisp_RequestPage(S_eSavedPage);
        else
            vDisp_RequestPage(e_disp_step_page(S_eScene, S_ucStep));
        S_eSavedPage = dispPAGE_NONE;
    }

    /*----------b. 场景边界仲裁: 状态仅在场景边界决定进入哪个场景----------*/
    if ((p_ops != NULL) && (p_ops->eMapDevStateToScene != NULL))
        e_target = p_ops->eMapDevStateToScene(uc_dev_state);

    if ((e_target != dispSCENE_NONE) && (e_target != S_eScene))
    {
        /* 预取目标场景步骤表校验: 空表场景不进入 (防项目误配: 场景已切但无页可切, 导航永久卡死) */
        if ((p_ops != NULL) && (p_ops->pGetSceneSteps != NULL))
            p_step = p_ops->pGetSceneSteps(e_target, &uc_num);

        if ((p_step != NULL) && (uc_num != 0U))
        {
            S_eScene        = e_target;
            S_ucStep        = 0;
            if ((p_ops != NULL) && (p_ops->ucSceneEntryStep != NULL))
                S_ucStep = p_ops->ucSceneEntryStep(S_eScene, p_snapshot);
            if (S_ucStep >= uc_num)             /* 入口步骤越界钳位 (防项目误配导航卡死) */
                S_ucStep = 0;
            S_ulStepDwellMs = 0;
            S_ulStepStartMs = ul_tick;
            vDisp_NavStackClear();              /* 场景切换清页面栈 (栈仅场景内生效) */
            vDisp_RequestPage(e_disp_step_page(S_eScene, S_ucStep));
            return;
        }
        /* 目标场景无有效步骤表: 不进入, 原场景继续走步骤推进段 (下帧重估) */
        p_step = NULL;
        uc_num = 0U;
    }

    /*----------c. 步骤推进: (步骤完成条件 ∧ 页面自完成) 成立 或 驻留超时兜底----------*/
    if ((p_ops != NULL) && (p_ops->pGetSceneSteps != NULL))
        p_step = p_ops->pGetSceneSteps(S_eScene, &uc_num);

    if ((p_step == NULL) || (S_ucStep >= uc_num))
        return;                                  /* 无效场景/步骤越界: 维持当前页 */

    p_step_page = pDisp_NavGetPage(p_step[S_ucStep].ePage);

    /* 驻留时间: 有墙钟时基用差值, 否则按帧周期累计 (兼容无 tick 项目)。
     * 注意: 无 tick 路径为名义时间 - 事件/导航唤醒会使 EnginePoll 早于帧周期执行,
     * 驻留累计随之偏快; 对驻留精度敏感的项目应提供 ulGetTickMs */
    if (S_bUseTick)
        S_ulStepDwellMs = ul_tick - S_ulStepStartMs;
    else
        S_ulStepDwellMs += usDisp_GetFramePeriod();

    /* 步进条件与合成: bDone (项目策略: 快照/驻留) 与步骤页 bOnDone (页面内在流程自完成) 各自 NULL 视为满足;
     * 两者均 NULL 视为常驻步 (不步进); 页面未注册时仅看 bDone; 驻留超时兜底独立于完成条件 */
    if ((p_step[S_ucStep].bDone != NULL) ||
        ((p_step_page != NULL) && (p_step_page->bOnDone != NULL)))
    {
        b_advance = ((p_step[S_ucStep].bDone == NULL) ||
                     p_step[S_ucStep].bDone(p_snapshot, S_ulStepDwellMs));
        b_advance = b_advance && ((p_step_page == NULL) || (p_step_page->bOnDone == NULL) ||
                                  p_step_page->bOnDone());
    }

    if (b_advance ||
        ((p_step[S_ucStep].usMaxHoldMs != 0U) && (S_ulStepDwellMs >= p_step[S_ucStep].usMaxHoldMs)))
    {
        if (S_ucStep < (uint8_t)(uc_num - 1U))   /* 步进, 越界则驻留末步 (常驻) */
            S_ucStep++;

        S_ulStepDwellMs = 0;
        S_ulStepStartMs = ul_tick;
        vDisp_RequestPage(e_disp_step_page(S_eScene, S_ucStep));
    }
}

/***********************************************************************************************************************
 * 函数功能    : 场景步骤页查询 (框架内部)
 * 说明(备注)  : 经项目 ops 查步骤表取 ePage; 场景无效或步骤越界返回 dispPAGE_NONE (维持当前页)
 * 传入参数    : e_scene: [IN] 场景; uc_step: [IN] 步骤索引
 * 输出参数    : 无
 * 返回值      : 步骤页面 ID
 ************************************************************************************************************************/
static DispPageId_E e_disp_step_page(DispScene_E e_scene, uint8_t uc_step)
{
    const DispOps_T       *p_ops = pDisp_GetOps();
    const DispFlowStep_T  *p_step = NULL;
    uint8_t                uc_num = 0;

    if ((p_ops != NULL) && (p_ops->pGetSceneSteps != NULL))
        p_step = p_ops->pGetSceneSteps(e_scene, &uc_num);

    if ((p_step == NULL) || (uc_step >= uc_num))
        return dispPAGE_NONE;

    return p_step[uc_step].ePage;
}

/***********************************************************************************************************************
 * 函数功能    : 读取场景驻留时基
 * 说明(备注)  : 项目未提供 ulGetTickMs 时返回 0 (此时 S_bUseTick=false, 调用方不会使用该值)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 毫秒时间戳
 ************************************************************************************************************************/
static uint32_t ul_disp_scene_tick(void)
{
    const DispOps_T *p_ops = pDisp_GetOps();

    if ((p_ops != NULL) && (p_ops->ulGetTickMs != NULL))
        return p_ops->ulGetTickMs();

    return 0U;
}

#else  /* boardDISP_SCENE_EN */

/*----------------桩实现 (断码屏极简形态: 无场景/overlay 调度, 导航全由项目显式 RequestPage 驱动)----------------*/
void vDisp_SceneInit(void)
{
}

void vDisp_SceneSchedule(const DispDataSnapshot_T *p_snapshot, uint8_t uc_dev_state)
{
    (void)p_snapshot;
    (void)uc_dev_state;
}

#endif  /* boardDISP_SCENE_EN */

#endif  /* boardDISPLAY_EN */
