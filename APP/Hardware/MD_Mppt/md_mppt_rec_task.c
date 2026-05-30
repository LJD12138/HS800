/*****************************************************************************************************************
*                                                                                                                *
 *                                         MPPT接收任务                                                         *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Mppt/md_mppt_rec_task.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_rec_data_proc.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define        	mpptREC_TASK_PRIO                    	2   			//任务优先级 
#define        	mpptREC_TASK_SIZE                    	192    	 		//任务堆栈  实际字节数 *4
TaskHandle_t   	tMpptRecTaskHandle;
void           	vMppt_RecTask(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
MpptRx_T    	tMpptRx;

vs16 sMpptMaxTemp = 0;


//****************************************************函数声明****************************************************//
static void v_rec_task_param_init(void);
static s8 c_check_conn_state(void);


/***********************************************************************************************************************
-----函数功能    复位接收参数BUFF
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_rec_task_param_init(void)
{
    memset(&tMpptRx, 0 ,sizeof(tMpptRx));

	if(tpMpptTask->tReplyBuff.buff != NULL)
		lwrb_reset(&tpMpptTask->tReplyBuff);
	
	cModbus_ResetRxBuff(tpMpptProtoRx);
}

/***********************************************************************************************************************
-----函数功能    MPPT接收任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bMppt_RecTaskInit(void)
{
//	#if(boardMPPT_EN)
//	vMppt_UsartInit();
//	#endif

	//接收协议初始化
	if(bMppt_RecProtInit() == false)
		return false;
	
	//任务参数初始化
	v_rec_task_param_init();

    //MPPT数据解析任务
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vMppt_RecTask,        	//任务函数
                (const char* )"MpptRecTask",			//任务名称
                (uint16_t ) mpptREC_TASK_SIZE,          //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) mpptREC_TASK_PRIO,       //任务优先级
                (TaskHandle_t*)&tMpptRecTaskHandle);    //任务句柄
	#endif  //boardUSE_OS
				
	return true;
}

/***********************************************************************************************************************
-----函数功能    MPPT接收任务
-----说明(备注)  由于和BMS共用串口,接收数据由BMS接收任务处理,MPPT接收任务等待任务通知
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vMppt_RecTask(void *pvParameters)
{
	s8 c_result = 0;
	
	#if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    { 
		if(tpMpptProtoRx == NULL || tpMpptProtoTx == NULL)
		{
			bMppt_RecProtInit();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}

		//******************************************处理接收的数据****************************************************
        c_result = cModbus_ProtoCheck(tpMpptProtoRx);
		if(c_result > 0)
        {
			c_check_conn_state();
			c_result = c_mppt_rec_proc_data(tpMpptProtoRx, tpMpptProtoTx);
			vModbus_RecEnd(tpMpptProtoRx);
			if(c_result <= 0)
			{
				if(uPrint.tFlag.bMpptRecTask || uPrint.tFlag.bImportant)
					log_w("bMpptRecTask:装载的数据错误,代码%d",c_result);
			}
			else
			{
				//通知发送任务
				#if(boardUSE_OS)
				xTaskNotifyGive(tMpptTaskHandler);
				#endif  //boardUSE_OS
			}
        }
		else 
		{
			if(c_result == 0)
			{
				#if(boardUSE_OS)
				if(lwrb_get_full(&tpMpptProtoRx->tRxBuff) ==0)
					ulTaskNotifyTake(pdFALSE,portMAX_DELAY);//等待任务通知
				else
					vTaskDelay(10);
				#endif  //boardUSE_OS
			}
			else 
			{
				if(uPrint.tFlag.bMpptRecTask|| uPrint.tFlag.bImportant)
					log_w("bMpptRecTask:协议解析错误,代码%d",c_result);
				
				#if(boardUSE_OS)
				vTaskDelay(10);
				#endif  //boardUSE_OS
			}
		}
    }
}




	
/***********************************************************************************************************************
-----函数功能    检测设备的连接状态
-----说明(备注)  connection
-----传入参数    none
-----输出参数    none
-----返回值      小于0:有错误   等于0:没操作    大于0:操作成功
************************************************************************************************************************/
static s8 c_check_conn_state(void)	
{
	//-----------------------丢失后第一次连接-----------------------------------------------
	if(tMppt.eDevState == DS_LOST)
	{
		bMppt_SetErrCode(MEC_SYS_DEV_LOST,false);
		
		#if(boardSYS_DATA_UPADATA)
		if(!BIT_GET(tSysInfo.Mod_Exist,OL_MPPT))//第一次初始化
		{
			STAT_SET(tSysInfo.Mod_Exist,OL_MPPT);
			Sys_Update_Element(AT_SYS_MODEXIST_ADDR, NULL, tSysInfo.Mod_Exist, true);
		}
		#endif
	}
	
	return 1;
}

/***********************************************************************************************************************
-----函数功能    检测设备的连接状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
void vMppt_RecTickTimer(void)
{
	if(tpMpptProtoRx == NULL)
		return;
	
	//******************************************数据帧接收超时计算***************************************************
	if(tpMpptProtoRx->usRecOverTimeCnt > 0)
	{    
		tpMpptProtoRx->usRecOverTimeCnt--;
	
		if(tpMpptProtoRx->usRecOverTimeCnt == 0)        
		{
			cModbus_StepWaitOutTime(tpMpptProtoRx);
		}
	}
	
	//******************************************MPPT模块连接超时计算*************************************************		
	if(tpMpptProtoRx->usLostOverTimeCnt > 0 && bSys_IsWorkState() == true)
	{    
		tpMpptProtoRx->usLostOverTimeCnt--;
	
		if(tpMpptProtoRx->usLostOverTimeCnt == 0)      //MPPT丢失    
		{
			v_rec_task_param_init();
			
			bMppt_SetErrCode(MEC_SYS_DEV_LOST,true);
		}
	}
}

#endif  //boardMPPT_EN
