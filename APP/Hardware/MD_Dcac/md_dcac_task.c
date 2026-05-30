/*****************************************************************************************************************
*                                                                                                                *
 *                                         逆变发送任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_queue_task.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_rec_task.h"
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
#define       	DCAC_TASK_PRIO                         	3        	//任务优先级 
#define       	DCAC_TASK_SIZE                         	192      	//任务堆栈  实际字节数 *4
TaskHandle_t    tDcacTaskHandler = NULL; 
void          	vDcac_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
//结构体
Dcac_T          tDcac;			//任务结构体
static Task_T	*tp_task = NULL;

/*****************************************************************************************************************
-----函数功能    逆变参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_task_param_init(void)
{
	if(tpDcacTask == NULL)
		return false;
	
	memset(&tDcac, 0, sizeof(tDcac));
	
	lwrb_reset(&tpDcacTask->tQueueBuff);
	
	tp_task = tpDcacTask;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    逆变任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bDcac_TaskInit(void)
{
	//协议初始化
	if(bDcac_SendProtInit() == false)
		return false;
	
	//任务队列初始化
	if(bDcac_QueueInit() == false)
		return false;
	
	//任务参数初始化
	if(b_dcac_task_param_init() == false)
		return false;
	
	//任务初始化
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vDcac_Task,            //任务函数 (1)
                (const char* )"DcacTask",               //任务名称
                (uint16_t ) DCAC_TASK_SIZE,              //任务堆栈大小
                (void* )NULL,                            //传递给任务函数的参数
                (UBaseType_t ) DCAC_TASK_PRIO,           //任务优先级
                (TaskHandle_t*)&tDcacTaskHandler);      //任务句柄
	#endif  //boardUSE_OS
				
	return true;
}


/*****************************************************************************************************************
-----函数功能    逆变任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vDcac_Task(void *pvParameters)
{
    #if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		if(tp_task == NULL
			#if(boardUPDATE)
			|| (tSysInfo.eDevState == DS_UPDATE_MODE && tUpdate.eObj != UO_DCAC)
			#endif  //boardUPDATE
		)
		{
			if(tp_task == NULL)
				b_dcac_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdTRUE, dcacTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
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
-----函数功能    设置AC状态
-----说明(备注)  none
-----传入参数    

-----输出参数    none
-----返回值      
******************************************************************************************************************/
bool bDcac_SetAcState(OperaObject_E obj, InOutState_E state)  
{
	switch(obj)
	{
		case OO_CHG:
		{
			//处于保护状态下,无法设置状态,只能关机
			if(tDcac.eChgState == IOS_PROTE)
				return false;
			
			tDcac.eChgState = state;
		}
		break;
		
		case OO_DISCHG:
		{
			tDcac.eDisChgState = state;
		}
		break;
		
		case OO_PARA_IN:
		{
			tDcac.eParanInState = state;
		}
		break;
		
		case OO_ALL:
		{
			tDcac.eDisChgState = state;
			tDcac.eParanInState = state;
			
			//处于保护状态下,无法设置状态,只能关机
			if(tDcac.eChgState == IOS_PROTE)
				return false;
			
			tDcac.eChgState = state;
		}
		break;
		
		default:
			if(uPrint.tFlag.bDcacTask)
				log_w("bDcacTask:AC设置对象%d错误", obj);
			return false;
	}
	
	if(tDcac.eDisChgState == IOS_SHUT_DOWN)
	{
		if(tDcac.uErrCode.tCode.bSysOutOL == 1)
			bDcac_SetErrCode(DEC_SYS_OUT_OL,false);
	}
	
	if(tDcac.eDisChgState == IOS_ERR   	|| 
		tDcac.eParanInState == IOS_ERR	||
		tDcac.eParanInState == IOS_PROTE||
		tDcac.eChgState == IOS_PROTE  	||
		tDcac.eChgState == IOS_ERR
	  )
	{
		bDcac_SetDevState(DS_ERR);
	}
	else if(tDcac.eDisChgState >= IOS_STARTING || tDcac.eChgState >= IOS_STARTING || tDcac.eParanInState >= IOS_STARTING)
	{
		bDcac_SetDevState(DS_WORK);
	}
	else 
	{
		bDcac_SetDevState(DS_SHUT_DOWN);
	}
	return true;
}

/***********************************************************************************************************************
-----函数功能    设置设备状态
-----说明(备注)  none
-----传入参数    DST_WAIT = 0,  //逆变关闭
				 DST_ERR,      //逆变错误
				 DST_LOST,     //逆变丢失
				 DST_WORK,       //逆变工作 
-----输出参数    none
-----返回值      true:没有错误  false:有错误
************************************************************************************************************************/
bool bDcac_SetDevState(DevState_E state)
{
	if(tDcac.eDevState != state)  //状态发生变化
	{
		if(state != DS_LOST) //连上
		{
			#if(boardSYS_DATA_UPADATA)
			STAT_SET(tSysInfo.Mod_Exist,OL_DCAC );
			#endif
		}
		else                  //丢失
		{
			#if(boardSYS_DATA_UPADATA)
			STAT_CLR(tSysInfo.Mod_Exist,OL_DCAC );
            #endif			
		}
	}

	tDcac.eDevState = state;
	return true;
}


/***********************************************************************************************************************
-----函数功能    设置设备错误代码
-----说明(备注)  none
-----传入参数    ERR_CODE
                 true:设置错误    false:清除错误
-----输出参数    none
-----返回值      true:添加了任务  false:没有添加任务
************************************************************************************************************************/
bool bDcac_SetErrCode(DCAC_ErrCode_E code, bool set)
{
	static DCAC_ErrCode_E e_next_code;
	static bool b_next_set;
	
	//没初始化完成不标记错误
	if(tSysInfo.uInit.tFinish.bIF_DcacTask == 0 && set == true)
    {
        if(uPrint.tFlag.bDcacTask)
			log_w("bDcacTask:DCAC模块未初始化完成，不允许标记错误%d",code);
        return false;
    }
	
	//第一次连接
	if(code == DEC_SYS_DEV_LOST)
	{
		if(set == false && tDcac.uErrCode.tCode.bSysDevLost == 0)
		{
			b_dcac_task_param_init();
			return false;
		}
	}
	
	//标记错误状态
	if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code || b_next_set != set)
		{
			log_e("bDcacTask:任务错误 代码%d 类型%d",code, set);
			e_next_code = code;
			b_next_set = set;
		}
	}	
	
	//有错误
	if(code > DEC_CLEAR_ALL)
	{
		//系统错误:丢失
		if(code == DEC_SYS_DEV_LOST)
		{
			tDcac.uErrCode.ulCode = 0;
			memset(&tDcacRx, 0, sizeof(tDcacRx));

			//丢失
			if(set)
			{
				b_dcac_task_param_init();
				
				ERR_SET(tDcac.uErrCode.ulCode, (code - 1));
				if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
					log_e("bDcacTask:DCAC模块丢失");
				
				bDcac_SetPerm(DPO_ALL, false);
			}
			//重连
			else 
			{
				if(tpDcacTask->ucID != DTI_INIT)
					cQueue_AddQueueTask(tpDcacTask, DTI_INIT, NULL, true);   //初始化tDcac
			}
		}
		//其他系统错误
		else 
		{
			if(set)
				ERR_SET(tDcac.uErrCode.ulCode, (code - 1));
			else 
				ERR_CLR(tDcac.uErrCode.ulCode, (code - 1));
		}
		
	}
	//清除所有错误
	else
	{
		//之前状态为异常,就进入关闭
		tDcac.uErrCode.ulCode = 0;
		memset(&tDcacRx.uErrCode.tCode, 0, sizeof(tDcacRx.uErrCode.tCode));
	}
	
	if(tDcac.uErrCode.ulCode)
	{
		#if(boardBUZ_EN)
		bBuz_Tweet(LONG_3);
		#endif  //boardBUZ_EN

		// cQueue_AddQueueTask(tp_task, DTI_ERR_PROC, code,false);//关闭逆变
		// if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
		// 		log_e("bDcacTask:添加错误处理任务 代码0x%x",tDcac.uErrCode.ulCode);
	}
	else
	{
		if(tDcac.eChgState == IOS_ERR)
		{
			bDcac_SetAcState(OO_CHG, IOS_SHUT_DOWN);
			if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
				log_i("bDcacTask:错误清空,清除输入保护");
		}
		
		if(tDcac.eDisChgState == IOS_ERR)
		{
			bDcac_SetAcState(OO_DISCHG, IOS_SHUT_DOWN);
			if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
				log_i("bDcacTask:错误清空,清除输出保护");
		}
	}
	return false;
}



/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数
        type类型:DSO_AC_OUT=0,  //开启输出
				 DSO_AC_IN,    //开启充电
				 DSO_OFF_ALL,     //关闭所有
          sw类型:ST_NULL=0, //进行取反
				 ST_ON,
				 ST_OFF,
-----输出参数    none
-----返回值      小于0:有错误   等于0:没操作    大于0:操作成功
************************************************************************************************************************/
s8 cDCAC_Switch(DACD_SwitchObject_E obj, SwitchType_E sw,bool buz_en)
{
	//要求打开时候,设备处于丢失
    if(tDcac.eDevState == DS_LOST)                            
    {   
		#if(boardBUZ_EN) 
        bBuz_Tweet(LONG_2);
		#endif  //boardBUZ_EN

		#if(boardSYS_DATA_UPADATA)
		STAT_CLR(tSysInfo.Mod_Exist,OL_DCAC );
		Sys_Update_Element(AT_AC_SWITCH_ADDR,NULL,false,true);//AC OUT S25 
		#endif
		
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_w("bDcacTask:开关失败,DCAC模块丢失");
		
        return -1;
    }
	
	
	//关闭所有
	if(obj == DSO_OFF_ALL)
	{
		#if(boardBUZ_EN)
		bBuz_Tweet(LONG_1);
		#endif  //boardBUZ_EN

		cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_OFF,false);//关闭输出
//		cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_IN, ST_OFF,false);//关闭充电;
		cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_PARA_IN, ST_OFF,false);//关闭充电;
		
		if(uPrint.tFlag.bDcacTask)
			sMyPrint("bDcacTask:添加关闭所有任务r\n");
		return 1;
	}	
	
	//单独控制
	switch(sw)
	{
		case ST_ON:
		{
			if(obj == DSO_AC_OUT)
				goto Loop1;
			else if(obj == DSO_AC_IN)
				goto LoopA1;
			else if(obj == DSO_PARA_IN)
				goto LoopB1;
		}
		
		case ST_OFF:
		{
			if(obj == DSO_AC_OUT)
				goto Loop2;
			else if(obj == DSO_AC_IN)
				goto LoopA2;
			else if(obj == DSO_PARA_IN)
				goto LoopB2;
		}
		
		default:
		{	
			if(obj == DSO_AC_OUT)
			{
				//错误,启动中,工作状态下,则执行关闭
				if(tDcac.eDisChgState >= IOS_ERR) 
				{
					Loop2:
					bSys_SetAutoOffTime(tAppMemParam.tSYS.usAutoOffTime);   //设置系统关闭时间
					cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_OFF, false);//关闭逆变

					#if(boardBUZ_EN)
					bBuz_Tweet(LONG_1);
					#endif  //boardBUZ_EN

					if(uPrint.tFlag.bDcacTask)
						sMyPrint("bDcacTask:----添加关闭逆变任务----\r\n");
				}
				else      //执行开启
				{
					Loop1:
					if(tDcac.uPerm.tPerm.bDisChgPerm == false)
					{
						#if(boardBUZ_EN)
						bBuz_Tweet(SHORT_2);
						#endif  //boardBUZ_EN

						if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
							log_w("bDcacTask:不允许开启放电");
						
						return -2;
					}
          
					cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_OUT, ST_ON,false);//打开逆变
//					vFan_ForceOpenFan();

					#if(boardBUZ_EN)
					bBuz_Tweet(LONG_1);
					#endif  //boardBUZ_EN

					if(uPrint.tFlag.bDcacTask)
						sMyPrint("bDcacTask:----添加开启逆变任务----\r\n");
				} 
			}
			else if(obj == DSO_AC_IN)
			{
				//启动中,工作,则执行关闭
				if(tDcac.eChgState > IOS_STARTING)
				{
					LoopA2:
					cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_IN, ST_OFF,false);//关闭逆变充电
					
					#if(boardBUZ_EN)
					bBuz_Tweet(LONG_1);
					#endif  //boardBUZ_EN
					
					if(uPrint.tFlag.bDcacTask)
						sMyPrint("bDcacTask:----添加关闭充电任务----\r\n");
				}
				else      //执行开启
				{
					LoopA1:
					//如果开启输入保护,则不允许开启
					if(tDcac.eChgState == IOS_PROTE)
					{
						if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
							log_w("bDcacTask:输入状态为保护,不允许开启充电\r\n");
						return -5;
					}
					
					if(tDcac.uPerm.tPerm.bChgPerm == false)
					{
						#if(boardBUZ_EN)
						bBuz_Tweet(SHORT_2);
						#endif  //boardBUZ_EN

						if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
							log_w("bDcacTask:当前不许可充电");
						return -8;
					}
					
					cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_DCAC_IN, ST_ON, false);//打开逆变充电
					
					if(buz_en == true)
					{
						#if(boardBUZ_EN)
						bBuz_Tweet(LONG_1);
						#endif  //boardBUZ_EN
					}
					
					if(uPrint.tFlag.bDcacTask)
						sMyPrint("bDcacTask:----添加开启充电任务----\r\n");
				} 
			}
			else if(obj == DSO_PARA_IN)
			{
				//启动中,工作,则执行关闭
				if(tDcac.eParanInState > IOS_STARTING)
				{
					LoopB2:
					cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_PARA_IN, ST_OFF,false);//关闭逆变充电
					
					#if(boardBUZ_EN)
					bBuz_Tweet(LONG_1);
					#endif  //boardBUZ_EN
					
					if(uPrint.tFlag.bDcacTask)
						sMyPrint("bDcacTask:----添加关闭并网任务----\r\n");
				}
				else      //执行开启
				{
					LoopB1:
					//如果开启输入保护,则不允许开启
					if(tDcac.eParanInState == IOS_PROTE)
					{
						if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
							log_w("bDcacTask:状态为保护,不允许开启并网");
						return -9;
					}
					
					if(tDcac.uPerm.tPerm.bParaInPerm == false)
					{
						#if(boardBUZ_EN)
						bBuz_Tweet(SHORT_2);
						#endif  //boardBUZ_EN
						
						if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
							log_w("bDcacTask:当前不允许开启并网");
						
						return -10;
					}
					
					if(tDcacRx.usInVolt < tAppMemParam.tDCAC.usMinInVolt)
					{
						#if(boardBUZ_EN)
						bBuz_Tweet(SHORT_2);
						#endif  //boardBUZ_EN
						
						if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
							log_w("bDcacTask:当前未接电网,不允许开启并网");
						
						return -11;
					}
					
					cQueue_AddQueueTask(tpDcacTask, DTI_CTRL_PARA_IN, ST_ON,false);//打开逆变充电
					
					if(buz_en == true)
					{
						#if(boardBUZ_EN)
						bBuz_Tweet(LONG_1);
						#endif  //boardBUZ_EN
					}
					
					if(uPrint.tFlag.bDcacTask)
						sMyPrint("bDcacTask:----添加开启并网任务----\r\n");
				} 
			}
			break;
		}
	}
    
	#if(boardDISPLAY_EN)
	bDisp_Switch(ST_ON, false);
	#endif  //boardDISPLAY_EN

	#if(boardSYS_DATA_UPADATA)
	Sys_Update_Mod(DCAC_Mod,true );
	#endif
	
    return 1;
}



/***********************************************************************************************************************
-----函数功能    无输出时,自动关闭倒计时 
-----说明(备注)  放在定时器中
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDcac_TickTimer(void)
{
	//非工作状态下退出
	if(bSys_IsWorkState() ==false) 
		return;

	//非空载更新倒计时 
	if(tDcacRx.usOutPwr)
	{
		vDcac_RefreshOffTime();
		return;
	}
	
	//逆变非工作状态下退出
	if(tDcac.eDisChgState <= IOS_STARTING)
		return;
	
	//充电状态下退出
	if(tDcac.eChgState >= IOS_STARTING)
		return;
	
	//----自动关闭逆变器输出---------------------------------------
	if(tDcac.usAutoOffTime)
	{
		if(tDcac.usAutoOffCnt) 
		{
			tDcac.usAutoOffCnt--;
			
			if(tDcac.usAutoOffCnt == 0)
			{
				#if(boardBUZ_EN)
				bBuz_Tweet(LONG_1);
				#endif  //boardBUZ_EN

				cDCAC_Switch(DSO_AC_OUT, ST_OFF, true);  //关闭逆变输出

				if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
					sMyPrint("Dcac_Task:====倒计时结束,关闭输出,时间=%dS====\r\n",tDcac.usAutoOffTime);

				//没有其他开启就关机
				if(bSys_CheckActState() == false)
					cSys_Switch(SO_DCAC, ST_OFF, false);
			}
		}
	}   
}


/***********************************************************************************************************************
-----函数功能    刷新逆变器关闭倒数计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDcac_RefreshOffTime(void)
{
	if(tDcac.usAutoOffTime)
		tDcac.usAutoOffCnt = tDcac.usAutoOffTime;  //更新时间
}

/***********************************************************************************************************************
-----函数功能    输入保护功能开关
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功  false:操作不成功
************************************************************************************************************************/
bool bDcac_InProteFuncSwitch(bool sw)
{
	if(sw)
	{
		//工作状态下,允许标记
		if(tDcac.eChgState >= IOS_STARTING)
			bDcac_SetErrCode(DEC_SYS_SET_IN_PROTE,true);
	}
	else 
		bDcac_SetErrCode(DEC_SYS_SET_IN_PROTE,false);
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    自动关闭功能开关
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功  false:操作不成功
************************************************************************************************************************/
bool bDcac_SetAutoOffTime(u16 time)
{
	tDcac.usAutoOffTime = time;
	vDcac_RefreshOffTime();
	return true;
}


/***********************************************************************************************************************
-----函数功能    获取过载状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:过载  false:不过载
************************************************************************************************************************/
bool bDcac_GetOverLoadState(void)
{
	if(tDcac.uErrCode.tCode.bDcacOL == 1)
		return true;
	else
		return false;
}

/*****************************************************************************************************************
-----函数功能    设置充放电许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:设置成功  反之失败
******************************************************************************************************************/
bool bDcac_SetPerm(DcacPermObject_E obj, bool en)
{
	switch(obj)
	{
		case DPO_CHG:
		{
			//状态变化
			if(en != tDcac.uPerm.tPerm.bChgPerm)
			{
				if(uPrint.tFlag.bDcacTask)
					sMyPrint("bDcacTask:设置充电许可: 设置=%d 当前状态=%d \r\n",en,tDcac.uPerm.tPerm.bChgPerm);
				
				tDcac.uPerm.tPerm.bChgPerm = en;
			}
		}
		break;
		
		case DPO_DISCHG:
		{
			//状态变化
			if(en != tDcac.uPerm.tPerm.bDisChgPerm)
			{
				//不允许充电
				if(en == false)
				{
					//AC充电未关闭
					if(tDcac.eDisChgState >= IOS_STARTING)
						cDCAC_Switch(DSO_AC_OUT,ST_OFF, true);
				}
				
				if(uPrint.tFlag.bDcacTask)
					sMyPrint("bDcacTask:设置放电许可: 设置=%d 当前状态=%d \r\n",en,tDcac.uPerm.tPerm.bDisChgPerm);
				
				tDcac.uPerm.tPerm.bDisChgPerm = en;
			}
		}
		break;
		
		case DPO_PARA_IN:
		{
			//状态变化
			if(en != tDcac.uPerm.tPerm.bParaInPerm)
			{
				if(uPrint.tFlag.bDcacTask)
					sMyPrint("bDcacTask:设置并网许可: 设置=%d 当前状态=%d \r\n",en,tDcac.uPerm.tPerm.bParaInPerm);
				
				tDcac.uPerm.tPerm.bParaInPerm = en;
			}
		}
		break;
		
		case DPO_ALL:
		{
			//状态变化
			if(en != tDcac.uPerm.tPerm.bChgPerm)
			{
				if(uPrint.tFlag.bDcacTask)
					sMyPrint("bDcacTask:设置充电许可: 设置=%d 当前状态=%d \r\n",en,tDcac.uPerm.tPerm.bChgPerm);
				
				tDcac.uPerm.tPerm.bChgPerm = en;
			}
			
			//状态变化
			if(en != tDcac.uPerm.tPerm.bDisChgPerm)
			{
				//不允许充电
				if(en == false)
				{
					//AC充电未关闭
					if(tDcac.eDisChgState >= IOS_STARTING)
						cDCAC_Switch(DSO_AC_OUT,ST_OFF, true);
				}
				
				if(uPrint.tFlag.bDcacTask)
					sMyPrint("bDcacTask:设置放电许可: 设置=%d 当前状态=%d \r\n",en,tDcac.uPerm.tPerm.bDisChgPerm);
				
				tDcac.uPerm.tPerm.bDisChgPerm = en;
			}
			
			//状态变化
			if(en != tDcac.uPerm.tPerm.bParaInPerm)
			{
				if(uPrint.tFlag.bDcacTask)
					sMyPrint("bDcacTask:设置并网许可: 设置=%d 当前状态=%d \r\n",en,tDcac.uPerm.tPerm.bParaInPerm);
				
				tDcac.uPerm.tPerm.bParaInPerm = en;
			}
		}
		break;
		
		default:
			return false;
	}
	return true;
}

/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_dcac_mem : 记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bDcac_MemParamInit(DcacMemParam_T* p_dcac_mem)
{
	p_dcac_mem->usAutoOffTime = boardDCAC_OFF_TIME;
	p_dcac_mem->usMinOpenVolt = boardDCAC_OPEN_MIN_VOLT;
	p_dcac_mem->usVoltRating = boardDCAC_VOLT_RATING;
	p_dcac_mem->usMaxInVolt = boardDCAC_MAX_IN_VOLT;
	p_dcac_mem->usMinInVolt = boardDCAC_MIN_IN_VOLT;
	p_dcac_mem->usInPwrRating = boardDCAC_IN_PWR_RATING;
	p_dcac_mem->usMinInPwr = boardDCAC_MIN_IN_PWR;
	p_dcac_mem->usMaxInCurr = boardDCAC_MAX_IN_CURR;
	p_dcac_mem->usOutPwrRating = boardDCAC_OUT_PWR_RATING;
	p_dcac_mem->usOverLoadPwr = boardDCAC_OVERLOAD_PWR;
	p_dcac_mem->usParaInPwr = boardDCAC_PARA_IN_PWR;
	p_dcac_mem->usAcOutFreq = boardDCAC_OUT_FREQ;
	p_dcac_mem->sMaxTemp = boardDCAC_MAX_TEMP;
	return true;
}	

/*****************************************************************************************************************
-----函数功能    设置记忆参数
-----说明(备注)  none
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vDcac_MemParamSet(u8 item, bool add)
{
	
	switch(item)
	{
		case 0:
		{
			if(add == true)
			{
				if(tAppMemParam.tDCAC.usAutoOffTime < 3600)
					tAppMemParam.tDCAC.usAutoOffTime++;
			}
			else
			{
				if(tAppMemParam.tDCAC.usAutoOffTime > 0)
					tAppMemParam.tDCAC.usAutoOffTime--;
			}
		}
		break;
		
		case 1:
		{
			if(add == true)
				tAppMemParam.tDCAC.usMinOpenVolt++;
			else
				tAppMemParam.tDCAC.usMinOpenVolt--;
		}
		break;
		
		case 2:
		{
			if(add == true)
				tAppMemParam.tDCAC.usVoltRating++;
			else
				tAppMemParam.tDCAC.usVoltRating--;
		}
		break;
		
		case 3:
		{
			if(add == true)
				tAppMemParam.tDCAC.usMaxInVolt++;
			else
				tAppMemParam.tDCAC.usMaxInVolt--;
		}
		break;
		
		case 4:
		{
			if(add == true)
				tAppMemParam.tDCAC.usMinInVolt++;
			else
				tAppMemParam.tDCAC.usMinInVolt--;
		}
		break;
		
		case 5:
		{
			if(add == true)
				tAppMemParam.tDCAC.usInPwrRating++;
			else
				tAppMemParam.tDCAC.usInPwrRating--;
		}
		break;
		
		case 6:
		{
			if(add == true)
				tAppMemParam.tDCAC.usMinInPwr++;
			else
				tAppMemParam.tDCAC.usMinInPwr--;
		}
		break;
		
		case 7:
		{
			if(add == true)
				tAppMemParam.tDCAC.usMaxInCurr++;
			else
				tAppMemParam.tDCAC.usMaxInCurr--;
		}
		break;
		
		case 8:
		{
			if(add == true)
				tAppMemParam.tDCAC.usOutPwrRating++;
			else
				tAppMemParam.tDCAC.usOutPwrRating--;
		}
		break;
		
		case 9:
		{
			if(add == true)
				tAppMemParam.tDCAC.usOverLoadPwr++;
			else
				tAppMemParam.tDCAC.usOverLoadPwr--;
		}
		break;
		
		case 10:
		{
			if(add == true)
				tAppMemParam.tDCAC.usParaInPwr++;
			else
				tAppMemParam.tDCAC.usParaInPwr--;
		}
		break;
		
		case 11:
		{
			if(add == true)
				tAppMemParam.tDCAC.usAcOutFreq++;
			else
				tAppMemParam.tDCAC.usAcOutFreq--;
		}
		break;
		
		case 12:
		{
			if(add == true)
			{
				if(tAppMemParam.tDCAC.sMaxTemp < 127)
					tAppMemParam.tDCAC.sMaxTemp++;
			}
			else
			{
				if(tAppMemParam.tDCAC.sMaxTemp > -127)
					tAppMemParam.tDCAC.sMaxTemp--;
			}
		}
		break;
	}
}

/*****************************************************************************************************************
-----函数功能    动态调整充电电流
-----说明(备注)  none
-----传入参数    curr:当前电流  0.1A
-----输出参数    none
-----返回值      true:开始调整  反之不调整
*****************************************************************************************************************/
//bool bDcac_DynAdjustChgCurr(vu16 curr)
//{
//	//电流过流
//	static vu16  us_adjust_pwr = 0;

//	if(tDcac.eChgState == IOS_WORK)
//	{
//		if(us_adjust_pwr != tSysInfo.tSetChgPwr.usDCAC || (tDcacRx.tParam1.usBatInPwr - tSysInfo.tSetChgPwr.usDCAC) > 300)
//		{
//			if(b_dcac_cs_set_chg_pwr(tSysInfo.tSetChgPwr.usDCAC) == false)
//			{
//				if(uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
//					log_w("bDcacRecTask:充电功率更新失败");
//				return false;
//			}
//			us_adjust_pwr = tSysInfo.tSetChgPwr.usDCAC;
//		}
//	}
//	return true;
//}


#endif  //boardDCAC_EN








