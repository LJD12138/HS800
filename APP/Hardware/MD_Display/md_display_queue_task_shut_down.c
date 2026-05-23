/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-关闭完成                                                 *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


#define     dispTASK_SHUT_DOWN_CYCLE_TIME       100 //任务时间

/***********************************************************************************************************************
-----函数功能    关闭完成显示任务
-----说明(备注)  关闭OLED显示, 如有新显示任务入队则退出当前任务
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_shut_down(Task_T *tp_task)
{
    //新的任务
    if(lwrb_get_full(&tp_task->tQueueBuff))
        cQueue_GotoStep(tp_task, STEP_END);

    switch (tp_task->ucStep)
    {
        case 0:
        {
            if(tDisp.eDevState != DS_SHUT_DOWN)
                bDisp_SetDevState(DS_SHUT_DOWN);
            
            bDisp_Switch(ST_OFF, false);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            // 保持关闭完成显示
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
        
        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }
    
    #if(boardUSE_OS)
    vTaskDelay(dispTASK_SHUT_DOWN_CYCLE_TIME);
    #endif  //boardUSE_OS
}
