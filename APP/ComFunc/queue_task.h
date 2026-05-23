#ifndef QUEUE_TASK_H
#define QUEUE_TASK_H


#include "main.h"
#include "lwrb.h"

// 1. 前置声明Task_T结构体
typedef struct Task_T Task_T;

typedef bool (*bpTaskManageFunc)(Task_T *tp_task);            //任务调度 
typedef void (*vpFunc)(Task_T *tp_task);  					  //执行任务的函数指针
typedef void (*vpAddTaskReturnFunc)(Task_T *tp_task, u8 num); //执行任务的函数指针

#pragma pack(1)
struct Task_T
{
	vu8   				ucID;             	//当前任务ID
	vu8              	ucStep;             //当前步骤
	vu16             	usInParam;          //函数参数
	vu16              	usStepWaitCnt;		//步骤等待次数
	vu16              	usStepRepeatCnt;	//步骤重复次数
	vu16              	usTaskWaitCnt;		//任务等待次数
	bool 				bNowRun;			//立刻执行
	bpTaskManageFunc    bp_task_manage_func;//任务调度函数
	vpFunc				vp_func;			//任务函数
	vpAddTaskReturnFunc vp_return_func;
	lwrb_t       		tQueueBuff;			//任务队列缓存器
	lwrb_t       		tReplyBuff;			//回复缓存器
	u8					uac_buff[];			//柔性数组缓冲区（包含任务队列和回复缓存器）
};
#pragma pack()


s8 cQueue_TaskInit(Task_T** task,
	u16 task_queue_size,
	u16 reply_buff_size,
	bpTaskManageFunc mfunc,
	vpAddTaskReturnFunc aFunc);
s8 cQueue_GotoStep(Task_T* task, u8 toStep);
s8 cQueue_AddQueueTask(Task_T* task, u8 task_id, u16 in_param, bool now_run);
bool bQueue_Reset(Task_T* task);
#endif

