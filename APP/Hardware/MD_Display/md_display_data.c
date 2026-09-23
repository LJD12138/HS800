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

    if (p_snapshot == NULL)
        return;

    mainENTER_CRITICAL();

    /*----------------系统状态与功率----------------*/
    p_snapshot->ucDevState   = (uint8_t)tSysInfo.eDevState;
    p_snapshot->usSysErrCode = tSysInfo.uErrCode.usCode;
    p_snapshot->usInPwr      = tSysInfo.usInPwr;
    p_snapshot->usOutPwr     = tSysInfo.usOutPwr;

    /*----------------端口状态 -> 位图----------------*/
    #if (boardUSB_EN)
    if (tUsb.eDevState >= DS_BOOTING)
        ul_active |= DISP_PORT_BIT_USB_OUT;
    else if (tUsb.eDevState == DS_ERR)
        ul_blink |= DISP_PORT_BIT_USB_OUT;
    #endif  /* boardUSB_EN */

    #if (boardDC_EN)
    if (tDc.eDevState >= DS_BOOTING)
        ul_active |= DISP_PORT_BIT_DC_OUT;
    else if (tDc.eDevState == DS_ERR)
        ul_blink |= DISP_PORT_BIT_DC_OUT;
    #endif  /* boardDC_EN */

    #if (boardDCAC_EN)
    if (tDcac.eDisChgState >= IOS_STARTING)
        ul_active |= DISP_PORT_BIT_AC_OUT;
    else if (tDcac.eDisChgState == IOS_ERR)
        ul_blink |= DISP_PORT_BIT_AC_OUT;

    if (tDcac.eChgState >= IOS_STARTING)
        ul_active |= DISP_PORT_BIT_AC_IN;
    else if (tDcac.eChgState == IOS_PROTE || tDcac.eChgState == IOS_ERR)
        ul_blink |= DISP_PORT_BIT_AC_IN;
    #endif  /* boardDCAC_EN */

    #if (boardMPPT_EN)
    if (tMppt.eDevState >= DS_BOOTING)
        ul_active |= DISP_PORT_BIT_DC_IN;
    else if (tMppt.eDevState == DS_ERR)
        ul_blink |= DISP_PORT_BIT_DC_IN;
    #endif  /* boardMPPT_EN */

    #if (boardLIGHT_EN)
    if (tLight.eDevState == DS_WORK)
        ul_active |= DISP_PORT_BIT_LIGHT;
    #endif  /* boardLIGHT_EN */

    p_snapshot->ulPortActiveFlags = ul_active;
    p_snapshot->ulPortBlinkFlags  = ul_blink;

    /*----------------电池相关 (BMS 64 位故障字临界区内防撕裂)----------------*/
    #if (boardBMS_EN)
    {
        uint64_t ull_bms_err;

        ull_bms_err = tBms.uErrCode.ullCode;
        p_snapshot->bBatErr              = (ull_bms_err != 0);
        p_snapshot->bBatLock             = ((tBms.uPerm.tPerm.bDisChgPerm == 0) ||
                                            (tBms.uPerm.tPerm.bChgPerm == 0));
        p_snapshot->ucSoc                = (uint8_t)tBmsRx.usSOC;
        p_snapshot->usChgFullTimeMin     = tBmsRx.usChgFullTime;
        p_snapshot->usDisChgEmptyTimeMin = tBmsRx.usDisChgEmptyTime;
    }
    #else
    p_snapshot->bBatErr              = false;
    p_snapshot->bBatLock             = false;
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

/***********************************************************************************************************************
 * 函数功能    : 设备状态 -> 页面 映射表
 * 说明(备注)  : 根据设备全局状态返回目标页面 ID (HS800 映射表)
 * 传入参数    : uc_dev_state: 当前设备状态
 * 输出参数    : 无
 * 返回值      : 目标页面 ID
 ************************************************************************************************************************/
DispPageId_E eDisp_MapDevStateToPage(uint8_t uc_dev_state)
{
    switch ((DevState_E)uc_dev_state)
    {
        case DS_INIT:        return PAGE_ID_INIT;        /* 初始化: 保持息屏 */
        case DS_BOOTING:     return PAGE_ID_BOOT;        /* 启动中: 开机步进动画页 */
        case DS_WORK:        return PAGE_ID_WORK;        /* 工作中: 主工作界面 */
        case DS_ERR:         return PAGE_ID_FAULT;       /* 故障态: 故障专用告警页 */
        case DS_CLOSING:     return PAGE_ID_CLOSING;     /* 关机中: 关机画面展示 */
        case DS_SHUT_DOWN:   return PAGE_ID_SHUT_DOWN;   /* 已关机: 关机状态页(松开电源键息屏) */
        case DS_UPDATE_MODE: return PAGE_ID_UPDATE;      /* 升级中: 固件升级进度页 */
        case DS_ENG_MODE:    return PAGE_ID_ENG_MODE;    /* 工程模式: 工程校准设置页 */
        default:             return PAGE_ID_MAX;         /* 无映射: 维持当前页 */
    }
}

#endif  /* boardDISPLAY_EN */
