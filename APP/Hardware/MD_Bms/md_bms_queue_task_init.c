/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Bms
 * File    : md_bms_queue_task_init.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : BMS 初始化队列子任务实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Bms/md_bms_queue_task.h"

#if (boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "app_info.h"

//****************************************************Macros********************************************************************//
#define			bmsTASK_INIT_CYCLE_TIME					50

//****************************************************Function Declaration******************************************************//
static s8 c_bms_info_init(void);

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : BMS 初始化队列子任务
 * 说明(备注)  : 无
 * 传入参数    : p_task: 任务结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_bms_queue_task_init(Task_T *p_task)
{
    s8 c_ret = 0;
    
    switch (p_task->ucStep)
    {
        case 0:
        {
            /* 获取参数,用来判断是否是充电唤醒 */
            if (c_bms_cs_get_param(bmsGET_PARAM_OBJ) > 0 || G_TestMode == true)
                cQueue_GotoStep(p_task, STEP_NEXT);     /* 下一步 */
        }
        break;

        case 1:
        {
            /* 等待获取APP信息 */
            if (tSysInfo.uInit.tFinish.bIF_AppInfo == false)
                break;
            
            static bool s_b_ret = true;
            c_ret = c_bms_info_init();
            if (c_ret > 0)
            {
                if ((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && s_b_ret == false)
                    log_w("bBmsTask:BMS获取错误清除");
                
                s_b_ret = true;
            }
            else
            {
                if ((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && s_b_ret == true)
                {
                    log_w("bBmsTask:BMS初始化失败 代码%d", c_ret);
                    s_b_ret = false;
                }
                break;
            }
            cQueue_GotoStep(p_task, STEP_NEXT);
            break;
        }
        
        case 2:
        {
            tSysInfo.uInit.tFinish.bIF_BmsTask = 1;
            cBms_CheckPerm();
            if (uPrint.tFlag.bBmsTask)
                sMyPrint("bBmsTask:初始化BMS----初始化完成----\r\n");
            
            cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
        }
        break;

        default:
        {
            cQueue_GotoStep(p_task, STEP_END);          /* 结束 */
        }
        break;
    }
    
    if (bQueue_IsTaskTimeoutMs(p_task, 3000))  /* 等待超时 */
    {
        if (uPrint.tFlag.bBmsTask)
            log_w("bBmsTask:BMS初始化任务等待超时,步骤%d", p_task->ucStep);
        
        cQueue_GotoStep(p_task, STEP_END);              /* 结束 */
    }
    
    #if (boardUSE_OS)
    ulTaskNotifyTake(pdTRUE, bmsTASK_INIT_CYCLE_TIME);  /* 周期节拍；新任务投递立即唤醒抢占 */
    #endif  /* boardUSE_OS */
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 BMS 信息
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : s8: <0-失败, 0-未完成, >0-完成
 ************************************************************************************************************************/
static s8 c_bms_info_init(void)
{
    s8 ret = 0;
    const char *p_obj_str = tBmsMemParamStr;
    static bool s_b_ret = true;
    
    /* 已经初始化 */
    if (tSysInfo.uInit.tFinish.bIF_SysInit == true)
    {
        ret = cApp_GetMemParam(p_obj_str);
        if (ret > 0)                                    /* 成功 */
            return 1;

        if ((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && s_b_ret == true)
        {
            log_e("bBmsTask:当前系统已经初始化完成,但是tBMS读取依旧为空,准备重置");
            s_b_ret = false;
        }    
    }
    
    /* 重新初始化 */
    ret = cApp_MemParamInit(p_obj_str);
    if (ret <= 0)                                       /* 失败 */
        return -1;
    
    ret = cApp_UpdateMemParam(p_obj_str);
    if (ret <= 0)                                       /* 失败 */
        return -2;
    
    s_b_ret = true;
    return 2;
}

#endif  /* boardBMS_EN */
