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
#include "MD_Dcac/md_dcac_prot_frame.h"

/* ==========================================macros======================================*/


/* ==========================================types=======================================*/


typedef enum
{
    DUS_STEP_INIT = 0,              /*!< 步骤0：初始化升级环境 */
    DUS_STEP_PREPARE,               /*!< 步骤1：准备升级 */
    DUS_STEP_FW_TRANS,              /*!< 步骤2：固件数据传输(A3/A4/A5/A6) */
    DUS_STEP_ERROR_CLEANUP,         /*!< 步骤3：升级错误,收尾 */
    DUS_STEP_FINISH_CLEANUP,        /*!< 步骤4：升级完成，收尾 */
    DUS_STEP_END,                   /*!< 步骤5：结束，等待后续操作 */
} DcacUpdateStep_E;

//DCAC升级准备阶段
typedef enum
{
    DPS_IDLE = 0,
    DPS_SEND_F0,                    //准备阶段：发送F0请求升级
    DPS_WAIT_F1,                    //准备阶段：等待F1回复
    DPS_SEND_F6,                    //准备阶段：发送F6请求跳转BOOT
    DPS_WAIT_F7,                    //准备阶段：等待F7回复
    DPS_BOOT_DELAY,                 //准备阶段：BOOT跳转后延时等待
    DPS_SEND_F2,                    //准备阶段：发送F2设置波特率
    DPS_WAIT_F3,                     //准备阶段：等待F3回复
    DPS_WAIT_PRINT_UPDATE_REQ,      //准备阶段：等待Print请求升级
    DPS_PRINT_SEND_C4,              //准备阶段：发送C4请求数据
    DPS_PRINT_WAIT_REPLY_C5,        //准备阶段：等待Print回复C4
    DPS_WAIT_A2,                    //准备阶段：等待A2文件头回复
    DPS_FINISH_CLEANUP,             //准备阶段：升级完成，收尾
}DcacPrepStage_E;
extern DcacPrepStage_E eDcacPrepStage;

//DCAC固件传输阶段
typedef enum
{
    DFTS_IDLE = 0,
    DFTS_SLAVE_READY_OK, 	    //DCAC前置数据准备完成,如确定协议/获取文件信息
    DFTS_HOST_REQ_DATA,	    //Print请求数据
    DFTS_WAIT_HOST_REPLY,	    //等待Print回复
    DFTS_SEND_FW_DATA,	        //主机发送A3固件包数据到DCAC
    DFTS_WAIT_SLAVE_REPLY,	    //等待DCAC回复
    DFTS_QUERY_SLAVE_RESULT,	//主机发送A5查询DCAC结果
    DFTS_WAIT_SLAVE_RESULT_REPLY,	//等待DCAC结果回复
    DFTS_FINISH_CLEANUP,             //升级完成，收尾
}DcacFwTransStage_E;
extern DcacFwTransStage_E eDcacFwTransStage;


/* ==========================================globals=====================================*/
extern MegmeetProtoTx_t*  tDcacMegmeetProtoTx;
extern MegmeetProtoRx_t*  tpDcacMegmeetProtoRx;

/* ==========================================extern======================================*/

bool bDcac_SetPrepStage(Task_T *tp_task ,DcacPrepStage_E stage);
bool bDcac_SetFwTransStage(Task_T *tp_task, DcacFwTransStage_E stage);
s8 cDcac_GetUpdateStage(void);

#endif  //boardDCAC_EN

#ifdef __cplusplus
}
#endif

#endif  //MD_DCAC_QUEUE_TASK_UPDATE_H
