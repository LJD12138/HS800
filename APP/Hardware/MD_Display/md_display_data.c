/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display
 * File    : md_display_data.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : UniDisplay 数据快照池 (项目件) - 全局状态原子快照捕获与设备状态路由表
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Display/md_display_data.h"

#if (boardDISPLAY_EN)
#include "uni_disp_port.h"
#include "Sys/sys_task.h"
#include "MD_Display/md_display_task.h"     /* usDisp_ErrCodeDisplay */

#if (boardUSB_EN)
#include "Usb/usb_task.h"
#endif  /* boardUSB_EN */

#if (boardDC_EN)
#include "Dc/dc_task.h"
#endif  /* boardDC_EN */

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#endif  /* boardBMS_EN */

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  /* boardMPPT_EN */

#if (boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  /* boardDCAC_EN */

#if (boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  /* boardLIGHT_EN */

//****************************************************Parameter Initialization**************************************************//
/* 框架每帧读取的快照缓冲 (仅显示任务写/读, 无跨任务共享) */
static DispDataSnapshot_T tDispSnapshot;

/***********************************************************************************************************************
 * 函数功能    : 获取当前全局数据快照
 * 说明(备注)  : 临界区保护读取全局状态、功率与端口位图，实现单向原子输入
 * 传入参数    : 无
 * 输出参数    : p_snapshot: 快照存放指针
 * 返回值      : 无
 ************************************************************************************************************************/
void vDisp_DataCaptureSnapshot(DispDataSnapshot_T *p_snapshot)
{
    uint32_t ul_active = 0;
    uint32_t ul_blink = 0;
    bool b_dis_chg = false;

    if (p_snapshot == NULL)
        return;

    mainENTER_CRITICAL();

    /*----------------系统状态与功率----------------*/
    p_snapshot->ucDevState   = (uint8_t)tSysInfo.eDevState;
    p_snapshot->usSysErrCode = tSysInfo.uErrCode.usCode;
    p_snapshot->usInPwr      = tSysInfo.usInPwr;
    p_snapshot->usOutPwr     = tSysInfo.usOutPwr;

    /*----------------端口状态 -> 位图 (错误/保护态优先判闪烁, 与图标显隐语义一致)----------------*/
    #if (boardUSB_EN)
    if (tUsb.eDevState == DS_ERR)
        ul_blink |= DISP_PORT_BIT_USB_OUT;
    else if (tUsb.eDevState >= DS_BOOTING)
        ul_active |= DISP_PORT_BIT_USB_OUT;

    if (tUsb.eDevState >= DS_BOOTING)
        b_dis_chg = true;
    #endif  /* boardUSB_EN */

    #if (boardDC_EN)
    if (tDc.eDevState == DS_ERR)
        ul_blink |= DISP_PORT_BIT_DC_OUT;
    else if (tDc.eDevState >= DS_BOOTING)
        ul_active |= DISP_PORT_BIT_DC_OUT;

    if (tDc.eDevState >= DS_BOOTING)
        b_dis_chg = true;
    #endif  /* boardDC_EN */

    #if (boardDCAC_EN)
    if (tDcac.eDisChgState == IOS_ERR || tDcac.eDisChgState == IOS_PROTE)
        ul_blink |= DISP_PORT_BIT_AC_OUT;
    else if (tDcac.eDisChgState >= IOS_STARTING)
        ul_active |= DISP_PORT_BIT_AC_OUT;

    if (tDcac.eChgState == IOS_PROTE || tDcac.eChgState == IOS_ERR)
        ul_blink |= DISP_PORT_BIT_AC_IN;
    else if (tDcac.eChgState >= IOS_STARTING)
        ul_active |= DISP_PORT_BIT_AC_IN;

    if (tDcac.eDisChgState >= IOS_WORK)
        b_dis_chg = true;
    #endif  /* boardDCAC_EN */

    #if (boardMPPT_EN)
    if (tMppt.eDevState == DS_ERR)
        ul_blink |= DISP_PORT_BIT_DC_IN;
    else if (tMppt.eDevState >= DS_BOOTING)
        ul_active |= DISP_PORT_BIT_DC_IN;

    p_snapshot->ucPvWorkMode = (uint8_t)tMppt.eWorkMode;
    #else
    p_snapshot->ucPvWorkMode = 0;
    #endif  /* boardMPPT_EN */

    #if (boardLIGHT_EN)
    if (tLight.eDevState >= DS_WORK)
        ul_active |= DISP_PORT_BIT_LIGHT;

    if (tLight.eDevState >= DS_WORK)
        b_dis_chg = true;
    #endif  /* boardLIGHT_EN */

    p_snapshot->ulPortActiveFlags = ul_active;
    p_snapshot->ulPortBlinkFlags  = ul_blink;
    p_snapshot->bDisChgActive     = b_dis_chg;

    /*----------------电池相关 (BMS 64 位故障字临界区内防撕裂)----------------*/
    #if (boardBMS_EN)
    {
        uint64_t ull_bms_err;

        ull_bms_err = tBms.uErrCode.ullCode;
        p_snapshot->bBatErr              = (ull_bms_err != 0);
        p_snapshot->bBatLock             = ((tBms.uPerm.tPerm.bDisChgPerm == 0) ||
                                            (tBms.uPerm.tPerm.bChgPerm == 0));
        p_snapshot->bBmsChg              = (tBms.eWorkState == BWS_CHG);
        p_snapshot->ucSoc                = (uint8_t)tBmsRx.usSOC;
        p_snapshot->usChgFullTimeMin     = tBmsRx.usChgFullTime;
        p_snapshot->usDisChgEmptyTimeMin = tBmsRx.usDisChgEmptyTime;
    }
    #else
    p_snapshot->bBatErr              = false;
    p_snapshot->bBatLock             = false;
    p_snapshot->bBmsChg              = false;
    p_snapshot->ucSoc                = 0;
    p_snapshot->usChgFullTimeMin     = 0;
    p_snapshot->usDisChgEmptyTimeMin = 0;
    #endif  /* boardBMS_EN */

    mainEXIT_CRITICAL();

    /*----------------低频量 (依赖访问函数, 显示任务单读者直读安全)----------------*/
    p_snapshot->ucChgState = (uint8_t)cSys_IsChgState();
    p_snapshot->usPrimaryErrCode = usDisp_ErrCodeDisplay();
}

/***********************************************************************************************************************
 * 函数功能    : 获取框架帧快照
 * 说明(备注)  : 项目持有快照缓冲, 框架只读指针; 仅显示任务调用 (单读者, 无跨任务共享)
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 快照指针 (始终有效)
 ************************************************************************************************************************/
const DispDataSnapshot_T *pDisp_DataSnapshot(void)
{
    vDisp_DataCaptureSnapshot(&tDispSnapshot);
    return &tDispSnapshot;
}

/***********************************************************************************************************************
 * 函数功能    : 读取快照中的设备状态
 * 说明(备注)  : DispOps_T.ucGetDevState 的项目实现; 通用件不感知快照字段
 * 传入参数    : p_snapshot: [IN] 快照指针 (可为 NULL)
 * 输出参数    : 无
 * 返回值      : 设备状态; 空指针返回 dispDEV_STATE_NONE
 ************************************************************************************************************************/
uint8_t ucDisp_DataGetDevState(const DispDataSnapshot_T *p_snapshot)
{
    if (p_snapshot == NULL)
        return dispDEV_STATE_NONE;

    return p_snapshot->ucDevState;
}

/***********************************************************************************************************************
 * 函数功能    : 息屏倒计时生效判定
 * 说明(备注)  : 判断当前设备状态是否允许倒计时息屏
 * 传入参数    : uc_dev_state: 当前设备状态
 * 输出参数    : 无
 * 返回值      : true: 参与倒计时递减, false: 暂停倒计时
 ************************************************************************************************************************/
bool bDisp_DataAutoOffArmed(uint8_t uc_dev_state)
{
    return (uc_dev_state == DS_WORK);
}

/*----------------场景步骤完成条件 (约束: 仅允许读入参快照与页面自身 static 进度, 禁止直读全局)----------------*/
/* BOOT 步: 系统已工作 (动画走满由 BOOT 页 bOnDone 自报, 与本草件与合成; 5s 兜底强制步进) */
static bool b_flow_done_boot(const DispDataSnapshot_T *p_snapshot, uint32_t ul_dwell_ms)
{
    (void)ul_dwell_ms;
    return (p_snapshot->ucDevState == DS_WORK);
}

/*----------------场景步骤表----------------*/
/* 初始化场景: 单步常驻, 退出全靠场景边界 (未就绪由 eDisp_MapDevStateToScene 门控映射到本场景) */
static const DispFlowStep_T s_tSceneInit[] = { {PAGE_ID_INIT, NULL, 0} };      /* 单步: 常驻 */

static const DispFlowStep_T s_tScenePowerOn[] =
{
    {PAGE_ID_BOOT,        b_flow_done_boot,    5000},     /* 0: 动画走满(页 bOnDone 自报) ∧ DS_WORK, 5s 兜底强制步进 */
    {PAGE_ID_WORK_MAIN1,  NULL,                0},        /* 1: 常驻 (WORK 组入口页) */
};

static const DispFlowStep_T s_tScenePowerOff[] =
{
    {PAGE_ID_CLOSING,    NULL,                   1500},     /* 0: 驻留 1500ms, 超时兜底步进 */
    {PAGE_ID_SHUT_DOWN,  NULL,                   0},        /* 1: 常驻 */
};

static const DispFlowStep_T s_tSceneUpdate[] = { {PAGE_ID_UPDATE, NULL, 0} };        /* 单步: 常驻 */
static const DispFlowStep_T s_tSceneEng[]    = { {PAGE_ID_ENG_MODE, NULL, 0} };    /* 单步: 常驻 */

/***********************************************************************************************************************
 * 函数功能    : 设备状态 -> 场景 映射表
 * 说明(备注)  : ① 未就绪门控: bDisp_PortIsSystemReady() 为假时无视设备状态, 一律映射初始化场景
 *                 (只允许 INIT 页, 含 DS_ERR 也不弹故障 overlay, 故障待就绪后由电平评估补入);
 *               ② 状态仅在场景边界生效; DS_ERR 返回 DISP_SCENE_NONE, 由框架 overlay 以电平语义处理
 * 传入参数    : uc_dev_state: 当前设备状态
 * 输出参数    : 无
 * 返回值      : 目标场景
 ************************************************************************************************************************/
DispScene_E eDisp_MapDevStateToScene(uint8_t uc_dev_state)
{
    /* 未就绪门控: 初始化完成前只允许初始化页 */
    if (!bDisp_PortIsSystemReady())
        return DISP_SCENE_INIT;

    switch ((DevState_E)uc_dev_state)
    {
        case DS_INIT:       return DISP_SCENE_INIT;         /* 就绪但状态未进入开机方向: 仍驻留初始化场景 */
        case DS_BOOTING:
        case DS_WORK:       return DISP_SCENE_POWER_ON;
        case DS_CLOSING:
        case DS_SHUT_DOWN:  return DISP_SCENE_POWER_OFF;
        case DS_UPDATE_MODE:    return DISP_SCENE_UPDATE;
        case DS_ENG_MODE:   return DISP_SCENE_ENG;
        default:            return DISP_SCENE_NONE;        /* DS_ERR 由 overlay 电平处理; DS_LOST/未知保持 */
    }
}

/***********************************************************************************************************************
 * 函数功能    : 场景 -> 步骤表 查询
 * 说明(备注)  : 返回表首地址, 步数经 p_num 回填; 无效场景返回 NULL (步数 0)
 * 传入参数    : e_scene: 目标场景
 * 输出参数    : p_num: 步数回填指针 (可为 NULL)
 * 返回值      : 步骤表首地址; 无效场景返回 NULL
 ************************************************************************************************************************/
const DispFlowStep_T *pDisp_GetSceneSteps(DispScene_E e_scene, uint8_t *p_num)
{
    const DispFlowStep_T *p_table = NULL;
    uint8_t uc_num = 0;

    switch (e_scene)
    {
        case DISP_SCENE_INIT:
        {
            p_table = s_tSceneInit;
            uc_num  = (uint8_t)mainARRAY_SIZE(s_tSceneInit);
        }
        break;

        case DISP_SCENE_POWER_ON:
        {
            p_table = s_tScenePowerOn;
            uc_num  = (uint8_t)mainARRAY_SIZE(s_tScenePowerOn);
        }
        break;

        case DISP_SCENE_POWER_OFF:
        {
            p_table = s_tScenePowerOff;
            uc_num  = (uint8_t)mainARRAY_SIZE(s_tScenePowerOff);
        }
        break;

        case DISP_SCENE_UPDATE:
        {
            p_table = s_tSceneUpdate;
            uc_num  = (uint8_t)mainARRAY_SIZE(s_tSceneUpdate);
        }
        break;

        case DISP_SCENE_ENG:
        {
            p_table = s_tSceneEng;
            uc_num  = (uint8_t)mainARRAY_SIZE(s_tSceneEng);
        }
        break;

        default:
        {
        }
        break;
    }

    if (p_num != NULL)
        *p_num = uc_num;

    return p_table;
}

/***********************************************************************************************************************
 * 函数功能    : 场景入口步选择
 * 说明(备注)  : 处理"关机后再开机跳过 INIT""工程/升级退回直接进 WORK"等边界;
 *               前置: 能进入开机场景即已就绪 (未就绪由 eDisp_MapDevStateToScene 门控在初始化场景)
 * 传入参数    : e_scene: 目标场景; p_snapshot: 快照数据
 * 输出参数    : 无
 * 返回值      : 入口步骤索引
 ************************************************************************************************************************/
uint8_t ucDisp_SceneEntryStep(DispScene_E e_scene, const DispDataSnapshot_T *p_snapshot)
{
    if (p_snapshot == NULL)
        return 0U;

    if (e_scene == DISP_SCENE_POWER_ON)
        return (p_snapshot->ucDevState == DS_BOOTING) ? 0U : 1U;   /* BOOT 步 / WORK 步 */

    return 0U;        /* 默认从首步进入 */
}

#endif  /* boardDISPLAY_EN */
