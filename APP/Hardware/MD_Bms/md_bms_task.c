/*****************************************************************************************************************
*                                                                                                                *
 *                                         电池包发送任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_task.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_queue_task.h"
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "app_info.h"

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define        	BMS_TASK_PRIO                         	2                       	//任务优先级 
#define        	BMS_TASK_SIZE                         	192                      	//任务堆栈  实际字节数 *4
TaskHandle_t    tBmsTaskHandler = NULL; 
void            vBms_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
//结构体
__ALIGNED(4)	Bms_T tBms;			//任务
static Task_T	*tp_task = NULL;


//****************************************************函数声明****************************************************//


/*****************************************************************************************************************
-----函数功能    电池包任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_bms_task_param_init(void)
{
	if(tpBmsTask == NULL)
		return false;
	
	memset(&tBms, 0, sizeof(tBms));
	
	lwrb_reset(&tpBmsTask->tQueueBuff);
	
	tp_task = tpBmsTask;
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    电池包任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bBms_TaskInit(void)
{ 
	//发送协议初始化
	if(bBms_SendProtInit() == false)
		return false;
	
	//任务队列初始化
	if(bBms_QueueInit() == false)
		return false;
	
	//任务参数初始化
	if(b_bms_task_param_init() == false)
		return false;
	
	//任务初始化
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vBms_Task,            	//任务函数
                (const char* )"BmsTask",              	//任务名称
                (uint16_t ) BMS_TASK_SIZE,              //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) BMS_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tBmsTaskHandler);       //任务句柄
	#endif  //boardUSE_OS
				
	return true;
}



/*****************************************************************************************************************
-----函数功能    电池包任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vBms_Task(void *pvParameters)
{
	#if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		if(tp_task == NULL
			#if(boardUPDATE)
			|| (tSysInfo.eDevState == DS_UPDATE_MODE && tUpdate.eObj != UO_BMS)
			#endif  //boardUPDATE
		)
		{
			if(tp_task == NULL)
				b_bms_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  //boardUSE_OS
		}
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdFALSE, bmsTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);
		}
    }
}




/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/
/*****************************************************************************************************************
-----函数功能    获取电池包的充电状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:充电   false:放电
******************************************************************************************************************/
bool bBms_GetBmsChgState(void)  
{
	if(cSys_IsChgState())
		return true;
	else 
		return false;
}


/***********************************************************************************************************************
-----函数功能    设置设备状态
-----说明(备注)  none
-----传入参数    DevState_E 
-----输出参数    none
-----返回值      true:没有错误  false:有错误
************************************************************************************************************************/
bool bBms_SetDevState(DevState_E state)
{
	if(tBms.eDevState != state)  //状态发生变化
	{
		if(state == DS_LOST) //丢失
		{   
			cQueue_AddQueueTask(tpBmsTask, BTI_NULL, 0, false);
			
			#if(boardSYS_DATA_UPADATA)
			STAT_CLR(tSysInfo.Mod_Exist,OL_BMS);
			#endif
		}
		else                 //连上
		{
			#if(boardSYS_DATA_UPADATA)
			STAT_SET(tSysInfo.Mod_Exist,OL_BMS);
			#endif
		}
	}

	tBms.eDevState = state;
	return true;
}


/***********************************************************************************************************************
-----函数功能    设置设备错误代码
-----说明(备注)  none
-----传入参数    ERR_CODE
-----输出参数    none
-----返回值      true:添加了任务,并立即执行  false:没有添加任务,或添加了任务不执行
************************************************************************************************************************/
bool bBms_SetErrCode(BmsErrCode_E code, bool set)
{
	static BmsErrCode_E e_next_code;
	static bool b_next_set;
	
	//第一次连接
	if(code == BEC_SYS_DEV_LOST)
	{
		if(set == false && tBms.uErrCode.tCode.bSysDevLost == 0)
		{
			b_bms_task_param_init();
			bBms_SetDevState(DS_SHUT_DOWN);
			return false;
		}
	}
	
	//标记错误状态
	if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code || b_next_set != set)
		{
			log_e("bBmsTask:任务错误 代码%d 类型%d",code, set);
			e_next_code = code;
			b_next_set = set;
		}
	}
	
	//有错误
	if(code > BEC_CLEAR_ALL)
	{
		//上报的错误
		if(code == BEC_BMS_ERR)
		{
			tBms.uErrCode.ullCode &=0xFFFFFFFF00000000;//去除低位
			if(set == true)
			{
				tBms.uErrCode.ullCode |= ulBmsRxErrCode;
				if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
					log_e("bBmsTask:设备上报错误,代码:0x%x",
				              ulBmsRxErrCode);
			}
		}
		//系统判断的错误
		else 
		{
			//系统错误:丢失
			if(code == BEC_SYS_DEV_LOST)
			{
				tBms.uErrCode.ullCode = 0;
				ulBmsRxErrCode = 0;
				if(set)
				{
					b_bms_task_param_init();
					
					ERR_SET(tBms.uErrCode.ullCode, (code - 2));
					
					if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
						log_e("bBmsTask:任务错误_BMS模块丢失");
					
					bBms_SetPerm(BPO_ALL, false);
				}
				else
					bBms_SetDevState(DS_SHUT_DOWN);
			}
			//其他系统错误
			else 
			{
				//设置清除错误位
				if(set)
					ERR_SET(tBms.uErrCode.ullCode, (code - 2));
				else
					ERR_CLR(tBms.uErrCode.ullCode, (code - 2));
			}
		}
	}
	else
	{
		tBms.uErrCode.ullCode = 0;
		ulBmsRxErrCode = 0;
	}
	
	if(tBms.uErrCode.ullCode)
	{
		if(set == true)
		{
			#if(boardBUZ_EN)
			bBuz_Tweet(LONG_3);
			#endif  //boardBUZ_EN
		}
		
		cQueue_AddQueueTask(tpBmsTask, BTI_ERR_PROCESS, code ,false);
	}
	else 
	{
		//清除错误,重新启动
		if(tBms.eDevState == DS_ERR)
		{
			bBms_SetDevState(DS_WORK);
			
			//****************************************警告,不可以在函数中调用自己*********************************************************//
			//cBms_Switch中通过bBms_SetErrCode调用自己cBms_Switch
			
//			cBms_Switch(SO_KEY, ST_ON);
//			if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
//				sMyPrint("bBmsTask:====清除错误,重新打开BMS====\r\n");
		}
	}
	
	return false;
}


/*****************************************************************************************************************
-----函数功能	开关BMS
-----传入参数   obj
-----传入参数   type
-----传入参数   fore_en
-----返回值     s8	小于0:操作失败   等于0:没操作    大于0:操作成功
-----作者       LJD(291483914@qq.com)
-----日期       2026-01-10
******************************************************************************************************************/
s8 cBms_Switch(SwitchObject_E obj, SwitchType_E type, bool fore_en)
{
	TaskInParam_U u_param;
	
	u_param.tTaskParam.ucObj = obj;
	
	switch(type)
	{
		case ST_ON:
		{	
			if((tBms.eDevState == DS_WORK || 
				tBms.eDevState == DS_BOOTING) && 
				fore_en == false)
			{
				if(uPrint.tFlag.bBmsTask)
				 sMyPrint("bBmsTask:当前状态为工作,不允许开机.对象:%d \r\n",obj);
				
				return 0;
			}
			
			goto LoopOn;
		}	
		case ST_OFF:
		{
			if((tBms.eDevState == DS_SHUT_DOWN || 
				tBms.eDevState == DS_CLOSING) 
				&& fore_en == false)
			{
				if(uPrint.tFlag.bBmsTask)
				 sMyPrint("bBmsTask:当前状态为关闭,不允许关机.对象:%d \r\n",obj);
				
				return 0;
			}
			
			goto LoopOff;	
		}	
		default:
		{	
			if(tBms.eDevState == DS_SHUT_DOWN || tBms.eDevState == DS_CLOSING)
			{
				LoopOn:
				u_param.tTaskParam.ucParam = ST_ON;
			}
		    else
			{
				LoopOff:
				u_param.tTaskParam.ucParam = ST_OFF;
			}
			
			cQueue_AddQueueTask(tpBmsTask, BTI_CTRL_BMS_SW, u_param.usTaskInParam, fore_en);
		}
		break;
	}
	
	#if(boardSYS_DATA_UPADATA)
	Sys_Update_Mod(BMS_Mod,true );
	#endif
	
    return 1;
}


/***********************************************************************************************************************
-----函数功能    获取SOC
-----说明(备注)  none
-----传入参数    none 
-----输出参数    none
-----返回值      SOC
************************************************************************************************************************/
u8 ucBms_GetSoc(void)
{
	return (u8)tBmsRx.usSOC;	
}


/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_bms_mem : 记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bBms_MemParamInit(BmsMemParam_T* p_bms_mem)
{
	p_bms_mem->cChgMaxTemp = boardBMS_CHG_MAX_TEMP;
	p_bms_mem->cDisChgMaxTemp = boardBMS_DISCHG_MAX_TEMP;
	p_bms_mem->cChgMinTemp = boardBMS_CHG_MIN_TEMP;
    p_bms_mem->cDisChgMinTemp = boardBMS_DISCHG_MIN_TEMP;
	p_bms_mem->usMaxVolt = boardBMS_MAX_VOLT;
	p_bms_mem->usMinVolt = boardBMS_MIN_VOLT;
	p_bms_mem->usChgVolt = boardBMS_CHG_VOLT;
	return true;
}

/*****************************************************************************************************************
-----函数功能    设置记忆参数
-----说明(备注)  none
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vBms_MemParamSet(u8 item, bool add)
{
	if(add == true)
	{
		if(item == 0)
		{
			if(tAppMemParam.tBMS.cChgMaxTemp < 127)
				tAppMemParam.tBMS.cChgMaxTemp++;
		}
		else if(item == 1)
		{
			if(tAppMemParam.tBMS.cDisChgMaxTemp < 127)
				tAppMemParam.tBMS.cDisChgMaxTemp++;
		}
		else if(item == 2)
		{
			if(tAppMemParam.tBMS.cChgMinTemp < 127)
				tAppMemParam.tBMS.cChgMinTemp++;
		}
		else if(item == 3)
		{
			if(tAppMemParam.tBMS.cDisChgMinTemp < 127)
				tAppMemParam.tBMS.cDisChgMinTemp++;
		}
		else if(item == 4)
		{
			tAppMemParam.tBMS.usMaxVolt++;
		}
		else if(item == 5)
		{
			tAppMemParam.tBMS.usMinVolt++;
		}
		else if(item == 6)
		{
			tAppMemParam.tBMS.usChgVolt++;
		}
		
	}
	else 
	{
		if(item == 0)
		{
			if(tAppMemParam.tBMS.cChgMaxTemp > -127)
				tAppMemParam.tBMS.cChgMaxTemp--;
		}
		else if(item == 1)
		{
			if(tAppMemParam.tBMS.cDisChgMaxTemp > -127)
				tAppMemParam.tBMS.cDisChgMaxTemp--;
		}
		else if(item == 2)
		{
			if(tAppMemParam.tBMS.cChgMinTemp > -127)
				tAppMemParam.tBMS.cChgMinTemp--;
		}
		else if(item == 3)
		{
			if(tAppMemParam.tBMS.cDisChgMinTemp > -127)
				tAppMemParam.tBMS.cDisChgMinTemp--;
		}
		else if(item == 4)
		{
			tAppMemParam.tBMS.usMaxVolt++;
		}
		else if(item == 5)
		{
			tAppMemParam.tBMS.usMinVolt++;
		}
		else if(item == 6)
		{
			tAppMemParam.tBMS.usChgVolt++;
		}
	}
}




/*****************************************************************************************************************
-----函数功能    设置充放电许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:设置成功  反之失败
******************************************************************************************************************/
bool bBms_SetPerm(BmsPermObject_E obj, bool en)  
{
	switch(obj)
	{
		case BPO_CHG:
		{
			//状态变化
			if(en != tBms.uPerm.tPerm.bChgPerm)
			{
				if(uPrint.tFlag.bBmsTask)
					sMyPrint("bBmsTask:充电许可 设置=%d 当前状态=%d \r\n",en,tBms.uPerm.tPerm.bChgPerm);
				
				tBms.uPerm.tPerm.bChgPerm = en;
			}
		}
		break;
		
		case BPO_DISCHG:
		{
			//状态变化
			if(en != tBms.uPerm.tPerm.bDisChgPerm)
			{
				if(uPrint.tFlag.bBmsTask)
					sMyPrint("bBmsTask:放电许可 设置=%d 当前状态=%d \r\n",en,tBms.uPerm.tPerm.bDisChgPerm);
				
				tBms.uPerm.tPerm.bDisChgPerm = en;
			}
		}
		break;
		
		case BPO_ALL:
		{
			//状态变化
			if(en != tBms.uPerm.tPerm.bChgPerm)
			{
				if(uPrint.tFlag.bBmsTask)
					sMyPrint("bBmsTask:充电许可 设置=%d 当前状态=%d \r\n",en,tBms.uPerm.tPerm.bChgPerm);
				
				tBms.uPerm.tPerm.bChgPerm = en;
			}
			
			//状态变化
			if(en != tBms.uPerm.tPerm.bDisChgPerm)
			{
				if(uPrint.tFlag.bBmsTask)
					sMyPrint("bBmsTask:放电许可 设置=%d 当前状态=%d \r\n",en,tBms.uPerm.tPerm.bDisChgPerm);
				
				tBms.uPerm.tPerm.bDisChgPerm = en;
			}
		}
		break;
		
		default:
			return false;
	}
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    获取参数处理
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 cBms_CheckPerm(void)
{
	//----------------------------------许可处理--------------------------------------
	//放电许可
	if(tBmsRx.tState.bImpermDisChg == 1		||	//不许可放电
		tBms.uErrCode.tCode.bSysDisChgUT == 1		||	//放电低温
		tBms.uErrCode.tCode.bSysDisChgOT == 1		||	//放电高温
		ucBms_GetSoc() == 0)
	{
		if(tBms.uPerm.tPerm.bDisChgPerm == true)
			bBms_SetPerm(BPO_DISCHG, false);
	}
	else 
	{
		if(tBms.uPerm.tPerm.bDisChgPerm == false)
			bBms_SetPerm(BPO_DISCHG, true);
	}
	
	//充电许可
	if(tBmsRx.tState.bPermChg == 0			||	//不许可充电
		tBms.uErrCode.tCode.bSysChgUT == 1        	|| 	//充电低温
		tBms.uErrCode.tCode.bSysChgOT == 1       	||	//充电高温
		ucBms_GetSoc() == 100)
	{
		if(tBms.uPerm.tPerm.bChgPerm == true)
			bBms_SetPerm(BPO_CHG, false);
	}
	else 
	{
		if(tBms.uPerm.tPerm.bChgPerm == false)
			bBms_SetPerm(BPO_CHG, true);
	}
	
	return 1;
}

#if(boardUPDATE)
/***********************************************************************************************************************
-----函数功能    获取BMS升级阶段
-----说明(备注)  仅返回BMS升级队列当前阶段，最终结果不由该接口承载
************************************************************************************************************************/
s8 cBms_GetUpdateStage(void)
{
	// if(tSysInfo.eDevState != DS_UPDATE_MODE ||
	//    tUpdate.eObj != UO_BMS ||
	//    tBms.eDevState != DS_UPDATE_MODE)
		return -1;

	// if(tpBmsTask == NULL)
	// 	return -2;

	// if(tpBmsTask->ucID != BTI_UPDATE)
	// 	return -3;

	// if(tpBmsTask->ucStep > 1)
	// 	return UPDATE_QUEUE_STAGE_FINISH;

	// return UPDATE_QUEUE_STAGE_RUNNING;
}
#endif  //boardUPDATE

#endif  //boardBMS_EN

