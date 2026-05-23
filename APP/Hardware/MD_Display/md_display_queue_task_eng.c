/*****************************************************************************************************************
*                                                                                                                *
 *                                         显示队列任务-工程模式                                                 *
*                                                                                                                *
 ******************************************************************************************************************/
#include "MD_Display/md_display_queue_task.h"

#if(boardENG_MODE_EN && boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/md_display_api.h"
#include "MD_Display/md_display_eng_mode.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


#define     dispTASK_ENG_CYCLE_TIME             200 //任务时间

/***********************************************************************************************************************
-----函数功能    工程模式显示任务
-----说明(备注)  调用工程模式显示函数刷新工程界面
-----传入参数    tp_task:任务对象指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void v_disp_queue_task_eng(Task_T *tp_task)
{
    switch (tp_task->ucStep)
    {
        case 0:
        {
            if(tDisp.eDevState != DS_ENG_MODE)
                bDisp_SetDevState(DS_ENG_MODE);

            bDisp_Switch(ST_ON, true);
            cQueue_GotoStep(tp_task, STEP_NEXT);
        }
        break;

        case 1:
        {
            // 工程模式显示
            vDisp_EnginModeDis();
            cQueue_GotoStep(tp_task, STEP_END);
        }
        break;
        
        default:
            cQueue_GotoStep(tp_task, STEP_END);
            break;
    }
    
    #if(boardUSE_OS)
    vTaskDelay(dispTASK_ENG_CYCLE_TIME);
    #endif  //boardUSE_OS
}
#endif  //boardDISPLAY_EN  && boardENG_MODE_EN
