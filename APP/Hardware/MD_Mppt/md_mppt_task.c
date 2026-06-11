/*****************************************************************************************************************
*                                                                                                                *
 *                                         电池包发送任务                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_task.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_queue_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Adc/adc_task.h"

#include "app_info.h"

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

//****************************************************任务参数初始化**********************************************//
#if(boardUSE_OS)
#define			MPPT_TASK_PRIO                         	2        					//任务优先级 
#define        	MPPT_TASK_SIZE                         	192      					//任务堆栈  实际字节数 *4
TaskHandle_t    tMpptTaskHandler = NULL; 
void           	vMppt_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
//结构体
__ALIGNED(4)	Mppt_T tMppt;
static Task_T	*tp_task = NULL;

vu16 usDcInVolt = 0;//0.1V
vu16 usXT60InVolt = 0;//0.1V

//****************************************************函数声明****************************************************//
static bool b_mppt_update_dev_state(void);
static void v_mppt_param_update(void);
static void v_mppt_auto_switch_chg_source(void);

/*****************************************************************************************************************
-----函数功能    任务参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_mppt_task_param_init(void)
{
	if(tpMpptTask == NULL)
		return false;
	
	memset(&tMppt, 0, sizeof(tMppt));
	
	lwrb_reset(&tpMpptTask->tQueueBuff);
	
	tp_task = tpMpptTask;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    电池包任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool bMppt_TaskInit(void)
{
	//接口初始化
	vMppt_IfaceInit();

	//协议初始化
	if(bMppt_SendProtInit() == false)
		return false;

	//任务队列初始化
	if(bMppt_QueueInit() == false)
		return false;
	
	//任务参数初始化
	if(b_mppt_task_param_init() == false)
		return false;
	
	//任务初始化
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vMppt_Task,		//任务函数
                (const char* )"MpptTask",			//任务名称
                (uint16_t ) MPPT_TASK_SIZE,			//任务堆栈大小
                (void* )NULL,						//传递给任务函数的参数
                (UBaseType_t ) MPPT_TASK_PRIO,		//任务优先级
                (TaskHandle_t*)&tMpptTaskHandler);	//任务句柄
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
void vMppt_Task(void *pvParameters)
{
    #if(boardUSE_OS)
    for(;;)
	#endif
    {
		if(tp_task == NULL
			#if(boardUPDATE)
			|| (tSysInfo.eDevState == DS_UPDATE_MODE && tUpdate.eObj != UO_MPPT)
			#endif  //boardUPDATE
		)
		{
			if(tp_task == NULL)
				b_mppt_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(mpptTASK_CYCLE_TIME);
			continue;
			#else
			return;
			#endif
		}

		v_mppt_param_update();
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdFALSE, mpptTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);
		}		
    }
}


/***********************************************************************************************************************
-----函数功能    更新设备状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功  反之失败
************************************************************************************************************************/
static bool b_mppt_update_dev_state(void)
{
	//没有错误
	if(tMppt.uErrCode.ulCode == 0)
	{
		if(tMppt.eDevState == DS_LOST || tMppt.eDevState == DS_ERR)
		{
			bMppt_SetDevState(DS_SHUT_DOWN);
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:错误清除,设置MPPT为关闭状态\r\n");
		}
	}
	else  //有错误
	{
		//不为丢失
		if(tMppt.uErrCode.tCode.bDevLost)
		{
			if(tMppt.eDevState != DS_LOST)
				bMppt_SetDevState(DS_LOST);
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:存在错误,设置MPPT为丢失状态\r\n");
		}
		//不为错误
		else if(tMppt.eDevState != DS_ERR)
		{
			bMppt_SetDevState(DS_ERR);
			if(uPrint.tFlag.bMpptTask)
				sMyPrint("bMpptTask:存在错误,设置MPPT为错误状态\r\n");
		}
	}
	return true;
}

/***********************************************************************************************************************
-----函数功能	更新参数
-----作者       LJD
-----日期       2026-04-10
************************************************************************************************************************/
static void v_mppt_param_update(void)
{
	usDcInVolt = tAdcSamp.usDcIn1Volt;//0.1V
	usXT60InVolt = tAdcSamp.usDcIn2Volt;//0.1V

	v_mppt_auto_switch_chg_source();
}

/***********************************************************************************************************************
-----函数功能	自动切换充电源
-----作者       LJD
-----日期       2026-05-29
************************************************************************************************************************/
static void v_mppt_auto_switch_chg_source(void)
{
    // 如果两个输入任意一个都高于 110V → 全部关闭
    if(usDcInVolt > 650 || usXT60InVolt > 650){
        mpptGPIO_DC_EN_OFF();
        mpptGPIO_XT60_EN_OFF();
        tMppt.eWorkMode = MWM_NULL;
        bMppt_SetErrCode( MEC_MPPT_IN_OV, true );
        return;
    }
    
    bMppt_SetErrCode( MEC_MPPT_IN_OV, false );
    
    // 如果两个输入都低于 110V → 全部关闭
    if(usDcInVolt < 50 && usXT60InVolt < 50)
    {
        mpptGPIO_DC_EN_OFF();
        mpptGPIO_XT60_EN_OFF();
        tMppt.eWorkMode = MWM_NULL;
        return;
    }

    // PV 优先逻辑
    if(tMppt.eWorkMode == MWM_NULL) {
        if(usXT60InVolt > 115)
        {
            tMppt.eWorkMode = MWM_PV;
            mpptGPIO_DC_EN_OFF();
            mpptGPIO_XT60_EN_ON();
        }
        else if(usDcInVolt > 115)
        {
            tMppt.eWorkMode = MWM_DC;
            mpptGPIO_DC_EN_ON();
            mpptGPIO_XT60_EN_OFF();
        }
    } else if(tMppt.eWorkMode == MWM_PV) {
        if(usXT60InVolt < 50) {
            tMppt.eWorkMode = MWM_NULL;
            mpptGPIO_DC_EN_OFF();
            mpptGPIO_XT60_EN_OFF();
        }
    } else if(tMppt.eWorkMode == MWM_DC) {
        if(usDcInVolt < 50) {
            tMppt.eWorkMode = MWM_NULL;
            mpptGPIO_DC_EN_OFF();
            mpptGPIO_XT60_EN_OFF();
        }
    }
}





























/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/
/***********************************************************************************************************************
-----函数功能    设置设备状态
-----说明(备注)  none
-----传入参数    DevState_E
-----输出参数    none
-----返回值      true:没有错误  false:有错误
************************************************************************************************************************/
bool bMppt_SetDevState(DevState_E state)
{
	if(tMppt.eDevState != state)  //状态发生变化
	{
		if(state == DS_LOST) //丢失
		{   
//			bMppt_AddQueueTask(MTI_NULL,NULL,false);
			#if(boardSYS_DATA_UPADATA)
			STAT_CLR(tSysInfo.Mod_Exist,OL_MPPT);
			#endif
		}
		else                 //连上
		{
			#if(boardSYS_DATA_UPADATA)
			STAT_SET(tSysInfo.Mod_Exist,OL_MPPT);
			#endif
		}
	}

	tMppt.eDevState = state;
	return true;
}

/***********************************************************************************************************************
-----函数功能    设置设备错误代码
-----说明(备注)  none
-----传入参数    ERR_CODE
                 true:设置错误    false:清除错误
-----输出参数    none
-----返回值      true:添加了任务,并立即执行  false:没有添加任务,或添加了任务不执行
************************************************************************************************************************/
bool bMppt_SetErrCode(MpptErrCode_E code, bool set)
{
	static MpptErrCode_E e_next_code;
	static bool b_next_set;
	
	//没初始化完成不标记错误
	if(tSysInfo.uInit.tFinish.bIF_MpptTask == 0 && set == true)
    {
        if(uPrint.tFlag.bMpptTask)
			log_w("bMpptTask:MPPT模块未初始化完成，不允许标记错误%d",code);
        return false;
    }
	
	//第一次连接
	if(code == MEC_SYS_DEV_LOST)
	{
		if(set == false && tMppt.uErrCode.tCode.bDevLost == 0)
		{
			b_mppt_task_param_init();
			return false;
		}
	}
	
	//标记错误状态
	if(uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
	{
		if(e_next_code != code || b_next_set != set)
		{
			log_e("bMpptTask:任务错误 代码%d 类型%d",code, set);
			e_next_code = code;
			b_next_set = set;
		}
	}
	
	//有错误
	if(code > MEC_CLEAR_ALL)
	{
		//系统错误:丢失
		if(code == MEC_SYS_DEV_LOST)
		{
			tMppt.uErrCode.ulCode = 0;
			tMpptRx.uErrCode.usCode = 0;
			if(set)
			{
				bMppt_SetChgPerm(false);
				b_mppt_task_param_init();
				ERR_SET(tMppt.uErrCode.ulCode, (code - 1));
			}
		}
		//其他系统错误
		else 
		{
			if(set)
				ERR_SET(tMppt.uErrCode.ulCode, (code - 1));
			else 
				ERR_CLR(tMppt.uErrCode.ulCode, (code - 1));
		}
	}
	else
	{
		tMppt.uErrCode.ulCode = 0;
		tMpptRx.uErrCode.usCode = 0;
	}
	
	//有错误
	if(tMppt.uErrCode.ulCode)
	{
		if(tMppt.eDevState != DS_ERR && tMppt.eDevState != DS_LOST)  //处于非错误非丢失状态
		{
			#if(boardBUZ_EN)
			bBuz_Tweet(LONG_3);
			#endif  //boardBUZ_EN
//			cQueue_AddQueueTask(tpMpptTask, MTI_ERR_PROCESS,code,false);
			return true;
		}
	}
	b_mppt_update_dev_state();
	
	return false;
}



/***********************************************************************************************************************
-----函数功能    设置充电功率
-----说明(备注)  none
-----传入参数    SwitchType_E
-----输出参数    none
-----返回值      小于0:有错误   等于0:已经存在相同任务    大于0:操作成功 
************************************************************************************************************************/
s8 cMppt_SetChgPwr(u16 pwr)
{
	s8 result = 1;
	// //要求打开时候,设备处于丢失
    // if(tMppt.eDevState == DS_LOST && pwr > 0)                            
    // {    
    //     bBuz_Tweet(LONG_2);
    //     return -2;
    // }
	
	// if(uPrint.tFlag.bMpptTask)
	// 	sMyPrint("bMpptTask:添加设置充电功率%dW任务\r\n",pwr);
	
	// result = cQueue_AddQueueTask(tpMpptTask, MTI_SET_CHG_PWR,pwr,false);//打开
		
	// if((pwr > 0 && tMpptRx.usMaxInPwr == 0) ||
	// 	(pwr == 0 && tMpptRx.usMaxInPwr > 0))
	// {
	// 	bBuz_Tweet(LONG_1);
		
	// 	bDisp_Switch(ST_ON, false);
	// }
	
	// if(pwr == 0 && tMppt.uErrCode.ulCode)
	// 	bMppt_SetErrCode(MEC_CLEAR_ALL,true);  //清除所有错误
    
	// #if(boardSYS_DATA_UPADATA)
	// Sys_Update_Mod(MPPT_Mod,true );
	// #endif
	
    return result;
}


/*****************************************************************************************************************
-----函数功能    设置充电许可
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      
******************************************************************************************************************/
bool bMppt_SetChgPerm(bool en)  
{
	if(tMppt.bChgPerm != en)
	{
		if(en == false) //不允许充电
		{
			//关闭
			tSysInfo.tSetChgPwr.usMPPT = 0;
			cMppt_SetChgPwr(tSysInfo.tSetChgPwr.usMPPT);
		}
		
		tMppt.bChgPerm = en;
	}
	return true;
}

/*****************************************************************************************************************
-----函数功能    初始化参数
-----说明(备注)  none
-----传入参数    p_bms_mem : 记忆参数结构体
-----输出参数    none
-----返回值      true:设置成功  反之失败
*****************************************************************************************************************/
bool bMppt_MemParamInit(MpptMemParam_T* p_mppt_mem)
{
	p_mppt_mem->cAllowMaxTemp = boardMPPT_MAX_TEMP;
	p_mppt_mem->usAutoOffTime = boardMPPT_OFF_TIME;
	p_mppt_mem->usMaxInVolt = boardMPPT_MAX_IN_VOLT;
	p_mppt_mem->usMinInVolt = boardMPPT_MIN_IN_VOLT;
	p_mppt_mem->usMaxInCurr = boardMPPT_MAX_IN_CURR;
	p_mppt_mem->usInPwrRating = boardMPPT_IN_PWR_RATING;
	return true;
}

/*****************************************************************************************************************
-----函数功能    设置记忆参数
-----说明(备注)  none
-----传入参数    add:true 增加   false:减少
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vMppt_MemParamSet(u8 item, bool add)
{
	if(add == true)
	{
		if(item == 0)
		{
			if(tAppMemParam.tMPPT.cAllowMaxTemp < 127)
				tAppMemParam.tMPPT.cAllowMaxTemp++;
		}
		else if(item == 1)
		{
			if(tAppMemParam.tMPPT.usAutoOffTime < 3600)
				tAppMemParam.tMPPT.usAutoOffTime++;
		}
		else if(item == 2)
		{
			tAppMemParam.tMPPT.usMaxInVolt++;
		}
		else if(item == 3)
		{
			tAppMemParam.tMPPT.usMinInVolt++;
		}
		else if(item == 4)
		{
			tAppMemParam.tMPPT.usMaxInCurr++;
		}
		else if(item == 5)
		{
			tAppMemParam.tMPPT.usInPwrRating++;
		}
	}
	else 
	{
		if(item == 0)
		{
			if(tAppMemParam.tMPPT.cAllowMaxTemp > -127)
				tAppMemParam.tMPPT.cAllowMaxTemp--;
		}
		else if(item == 1)
		{
			if(tAppMemParam.tMPPT.usAutoOffTime > 0)
				tAppMemParam.tMPPT.usAutoOffTime--;
		}
		else if(item == 2)
		{
			tAppMemParam.tMPPT.usMaxInVolt--;
		}
		else if(item == 3)
		{
			tAppMemParam.tMPPT.usMinInVolt--;
		}
		else if(item == 4)
		{
			tAppMemParam.tMPPT.usMaxInCurr--;
		}
		else if(item == 5)
		{
			tAppMemParam.tMPPT.usInPwrRating--;
		}
	}
}

#endif  //boardMPPT_EN


