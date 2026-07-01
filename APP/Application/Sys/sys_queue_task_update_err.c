/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\11-G24\1.software\G2404-3\APP\Application\Sys
 * File    : sys_queue_task_update_err.c
 * Date    : 2026-05-09 12:05:30
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "Sys/sys_queue_task.h"

#if(boardUPDATE)
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task_update.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //boardDCAC_EN

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

//****************************************************Macros*******************************************************************//
#define     	sysTASK_UPDATE_ERR_CYCLE_TIME				sysTASK_CYCLE_TIME //任务时间
#define     	sysTASK_UPDATE_ERR_EXIT_OVERTIME			((5UL * 1000UL) / sysTASK_UPDATE_ERR_CYCLE_TIME) //等待模块退出升级模式超时5S

#if(boardKEY_EN)
#define     	sysTASK_UPDATE_ERR_KEY_LONG_TIME			(keyLONG_PRESS_TIME * keyTASK_CYCLE_TIME / sysTASK_UPDATE_ERR_CYCLE_TIME) //长按阈值(任务周期数)
#endif  //boardKEY_EN


//****************************************************Parameter Initialization************************************************//



//****************************************************Function Declaration****************************************************//



/***********************************************************************************************************************
-----函数功能    系统升级错误任务处理函数
-----说明(备注)  进入错误状态后,让相关升级队列退出,延时等待60S,如果没有用户操作,则进入关机状态。
-----传入参数    tp_task: 指向任务结构体的指针，包含任务的相关信息。
-----输出参数    none
-----返回值      none
************************************************************************************************************************/ 
void v_sys_queue_task_update_err(Task_T *tp_task)
{
    if(tp_task == NULL)
        return;

    //有任务退出
    if(lwrb_get_full(&tp_task->tQueueBuff)) //队列里面有任务
    {
        tUpdate.eErrCode = UEF_NONE;
        cQueue_GotoStep( tp_task, STEP_END ); //结束
    }

    switch(tp_task->ucStep)
    {
        //让Host(Print)退出升级队列
        case 0:
        {
            //等待Host退出升级模式(由各任务自身的有效性检查触发错误收尾)
            if(tPrint.eDevState != DS_UPDATE_MODE && tpPrintTask->ucID != PTI_UPDATE)
            {
                cQueue_GotoStep(tp_task, STEP_NEXT);
            }
            else
            {
                //等待Host退出升级模式,超时强制退出
                tp_task->usStepWaitCnt++;
                if(tp_task->usStepWaitCnt >= sysTASK_UPDATE_ERR_EXIT_OVERTIME)
                {
                    //超时强制退出
                    bQueue_Reset(tpPrintTask);
                    tPrint.eDevState = DS_SHUT_DOWN;
                    cQueue_GotoStep(tp_task, STEP_NEXT);
                }
            }
        }
        break;

        //让Slave(BMS/DCAC)退出升级队列
        case 1:
        {
            bool b_slave_exit = true;

            #if(boardBMS_EN)
            if(tUpdate.eObj == UO_BMS && tBms.eDevState == DS_UPDATE_MODE && tpBmsTask->ucID != BTI_UPDATE)
                b_slave_exit = false;
            #endif
            #if(boardDCAC_EN)
            if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj) && tDcac.eDevState == DS_UPDATE_MODE && tpDcacTask->ucID != DTI_UPDATE)
                b_slave_exit = false;
            #endif

            if(b_slave_exit)
            {
                //等待用户操作,超时由vUpdate_TickTimer自动进入关机
                tUpdate.usLostOverTimeCnt = ((60UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE); //延时等待60S
                cQueue_GotoStep(tp_task, STEP_NEXT);
            }
            else
            {
                tp_task->usStepWaitCnt++;
                if(tp_task->usStepWaitCnt >= sysTASK_UPDATE_ERR_EXIT_OVERTIME)
                {
                    //超时强制退出
                    #if(boardBMS_EN)
                    if(tUpdate.eObj == UO_BMS)
                    {
                        bQueue_Reset(tpBmsTask);
                        bBms_SetDevState(DS_SHUT_DOWN);
                    }
                    #endif
                    #if(boardDCAC_EN)
                    if(IS_DCAC_UPDATE_OBJ(tUpdate.eObj))
                    {
                        bQueue_Reset(tpDcacTask);
                        bDcac_SetDevState(DS_SHUT_DOWN);
                    }
                    #endif
                    //等待用户操作,超时由vUpdate_TickTimer自动进入关机
                    tUpdate.usLostOverTimeCnt = ((60UL * 1000UL) / boardREPET_TIMER_CYCLE_TMIE); //延时等待60S
                    cQueue_GotoStep(tp_task, STEP_NEXT);
                }
            }
        }
        break;

        //等待用户操作,点按power按键重新开始升级,长按power按键进入关机
        case 2:
        {
            #if(boardKEY_EN)
            //短按power重新开始升级,长按power进入关机
            if(bKey_PowerIsPress() == true)
            {
                tp_task->usStepWaitCnt++;
                if(tp_task->usStepWaitCnt >= sysTASK_UPDATE_ERR_KEY_LONG_TIME)
                {
                    //长按power,进入关机
                    vKey_PowerIsTri();  //标记已处理,防止全局按键重复触发
                    cQueue_GotoStep(tp_task, STEP_NEXT);  //跳转到case 3
                }
            }
            else
            {
                //按键释放,且按压时间小于长按阈值,判定为短按
                if(tp_task->usStepWaitCnt > 0 &&
                   tp_task->usStepWaitCnt < sysTASK_UPDATE_ERR_KEY_LONG_TIME)
                {
                    //短按power,重新开始升级
                    vKey_PowerIsTri();  //标记已处理,防止全局按键重复触发
                    bUpdate_Init();
                    cQueue_AddQueueTask(tpSysTask, STI_UPDATE, 0, true);
                    cQueue_GotoStep(tp_task, STEP_END);
                }
                tp_task->usStepWaitCnt = 0;
            }
            #endif
        }
        break;

        //开始关机
        case 3:
        {
            bUpdate_Init();
            cSys_Switch(SO_KEY, ST_OFF, true);
            cQueue_GotoStep(tp_task, STEP_END);

        }
        break;

        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }

	#if(boardUSE_OS)
	vTaskDelay(sysTASK_UPDATE_ERR_CYCLE_TIME);
	#endif  //boardUSE_OS
}


#endif  //boardUPDATE
