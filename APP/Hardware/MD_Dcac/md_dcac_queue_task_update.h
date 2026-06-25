/***********************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\11-G24\1.software\G2404-3\APP\Hardware\MD_Dcac
 * File    : md_dcac_queue_task_update.h
 * Date    : 2026-05-07 15:50:00
 * Author  : LJD(291483914@qq.com)
 * Desc    : DCAC升级任务队列头文件
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef MD_DCAC_QUEUE_TASK_UPDATE_H
#define MD_DCAC_QUEUE_TASK_UPDATE_H


#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================includes====================================*/
#include "board_config.h"

#if(boardDCAC_EN)
#include "Megmeet/megmeet_proto.h"
#include "Sys/sys_queue_task_update.h"

/* ==========================================macros======================================*/


/* ==========================================types=======================================*/

/**
 * @brief DCAC升级队列任务步骤枚举
 * @note  升级流程：INIT -> SEND_F0 -> WAIT_F1 -> (成功:SEND_F2, 失败:SEND_F6->WAIT_F7->BOOT_DELAY->SEND_F0)
 *                                      -> SEND_F2 -> WAIT_F3 -> SEND_A1 -> WAIT_A2 -> DATA_EXCHANGE -> FINISH_CLEANUP -> END
 */
typedef enum
{
    /* ---- 握手阶段 ---- */
    DCAC_UPDATE_STEP_INIT = 0,              /*!< 步骤0：初始化升级环境 */
    DCAC_UPDATE_STEP_SEND_F0,               /*!< 步骤1：发送F0请求升级 */
    DCAC_UPDATE_STEP_WAIT_F1,               /*!< 步骤2：等待F1回复 */
    
    /* ---- BOOT跳转阶段 ---- */
    DCAC_UPDATE_STEP_SEND_F6,               /*!< 步骤3：发送F6请求跳转BOOT */
    DCAC_UPDATE_STEP_WAIT_F7,               /*!< 步骤4：等待F7回复 */
    DCAC_UPDATE_STEP_BOOT_DELAY,            /*!< 步骤5：BOOT跳转后延时等待 */
    
    /* ---- 波特率设置阶段 ---- */
    DCAC_UPDATE_STEP_SEND_F2,               /*!< 步骤6：发送F2设置波特率 */
    DCAC_UPDATE_STEP_WAIT_F3,               /*!< 步骤7：等待F3回复 */
    
    /* ---- 文件头传输阶段 ---- */
    DCAC_UPDATE_STEP_SEND_A1,               /*!< 步骤8：发送A1文件头 */
    DCAC_UPDATE_STEP_WAIT_A2,               /*!< 步骤9：等待A2文件头回复 */
    
    /* ---- 数据传输阶段 ---- */
    DCAC_UPDATE_STEP_DATA_EXCHANGE,         /*!< 步骤10：根据升级阶段执行数据交互(A3/A4/A5/A6) */
    
    /* ---- 结束阶段 ---- */
    DCAC_UPDATE_STEP_ERROR_CLEANUP,         /*!< 步骤11：升级错误,收尾 */
    DCAC_UPDATE_STEP_FINISH_CLEANUP,        /*!< 步骤12：升级完成，收尾 */
    DCAC_UPDATE_STEP_END,                   /*!< 步骤13：结束，等待后续操作 */
} DcacUpdateStep_E;

/* ==========================================globals=====================================*/
extern MegmeetProtoTx_t*  tDcacMegmeetProtoTx;
extern MegmeetProtoRx_t*  tpDcacMegmeetProtoRx;

/* ==========================================extern======================================*/


s8 cDcac_GetUpdateStage(void);

#endif  //boardDCAC_EN

#ifdef __cplusplus
}
#endif

#endif  //MD_DCAC_QUEUE_TASK_UPDATE_H
