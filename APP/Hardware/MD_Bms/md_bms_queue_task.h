#ifndef MD_BMS_QUEUE_TASK_H_
#define MD_BMS_QUEUE_TASK_H_

#include "board_config.h"

#if(boardBMS_EN)
#include "queue_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

extern Task_T *tpBmsTask;  //队列任务指针

bool bBms_QueueInit(void);

//队列函数
void v_bms_queue_task_init(Task_T *tp_task);
void v_bms_queue_task_clt_switch(Task_T *tp_task);
void v_bms_queue_task_main(Task_T *tp_task);
void v_bms_queue_task_cali(Task_T *tp_task);
void v_bms_queue_task_err(Task_T *tp_task);
void v_bms_queue_task_get_app_info(Task_T *tp_task);
void v_bms_queue_task_req_set_cmd(Task_T *tp_task);

#if(boardUPDATE)

/**
 * @brief BMS升级队列任务步骤枚举
 */
typedef enum
{
    BMS_UPDATE_STEP_INIT = 0,           /*!< 步骤0：初始化升级环境 */
    BMS_UPDATE_STEP_FORWARD_DATA,       /*!< 步骤1：转发升级数据到BMS */
    BMS_UPDATE_STEP_ERROR_CLEANUP,      /*!< 步骤2：升级错误,收尾 */
    BMS_UPDATE_STEP_FINISH_CLEANUP,     /*!< 步骤3：升级完成，收尾 */
    BMS_UPDATE_STEP_END,                /*!< 步骤4：结束 */
} BmsUpdateStep_E;

void v_bms_queue_task_update(Task_T *tp_task);
#endif  //boardUPDATE

#endif  //boardBMS_EN

#endif  //MD_BMS_QUEUE_FUNC_H_

