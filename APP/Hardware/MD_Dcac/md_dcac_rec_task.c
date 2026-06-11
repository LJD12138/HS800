/*****************************************************************************************************************
*                                                                                                                *
 *                                         逆变接收任务                                                         *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Dcac/md_dcac_rec_task.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_rec_data_proc.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"



//#include "app_info.h"

//****************************************************局部宏定义**************************************************//                                


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define    		dcacREC_TASK_PRIO                		2       		//任务优先级receive 
#define        	dcacREC_TASK_SIZE                		192     		//任务堆栈  实际字节数 *4
TaskHandle_t    tDcacRecTaskHandle;
void           	vDcac_RecTask(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
DcacRx_T    	tDcacRx; 


//****************************************************函数声明****************************************************//
static u8 c_check_conn_state(void);


/***********************************************************************************************************************
-----函数功能    复位接收参数BUFF
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_rec_task_param_init(void)
{
	memset(&tDcacRx, 0 ,sizeof(tDcacRx));
	
	//重置响应缓存器
	if(tpDcacTask != NULL && tpDcacTask->tReplyBuff.buff != NULL)
		lwrb_reset(&tpDcacTask->tReplyBuff);
	
	//重置接收缓冲区
	if(tpDcacProtoRx != NULL)
	{
		cModbus_ResetRxBuff(tpDcacProtoRx);
		tpDcacProtoRx->usRecOverTimeCnt = 0;
		tpDcacProtoRx->usLostOverTimeCnt = 0;
	}
}


/***********************************************************************************************************************
-----函数功能    逆变接收任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDcac_RecTaskInit(void)
{
	//接口初始化
	#if(boardDCAC_EN)
	vDcac_IfaceInit();
	#endif
	
	//接收协议初始化
	if(bDcac_RecProtInit() == false)
		return false;
	
	//任务参数初始化
	v_rec_task_param_init();
	
    //数据解析任务初始化
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vDcac_RecTask,        	//任务函数
                (const char* )"DcacRecTask",			//任务名称
                (uint16_t ) dcacREC_TASK_SIZE,          //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) dcacREC_TASK_PRIO,       //任务优先级
                (TaskHandle_t*)&tDcacRecTaskHandle);    //任务句柄
	#endif  //boardUSE_OS
				
	return true;
}


/***********************************************************************************************************************
-----函数功能    逆变接收任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDcac_RecTask(void *pvParameters)
{
	s8 c_result = 0;
	
	#if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		
		if(tpDcacProtoRx == NULL || tpDcacProtoTx == NULL)
		{
			bDcac_RecProtInit();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}
		
		//******************************************处理接收的数据****************************************************
        c_result = cModbus_ProtoCheck(tpDcacProtoRx);
		if(c_result > 0)
        {
			c_check_conn_state();
			c_result = c_dcac_rec_proc_data(tpDcacProtoRx, tpDcacProtoTx);
			vModbus_RecEnd(tpDcacProtoRx);
			if(c_result <= 0)
			{
				if(uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
					log_w("bDcacRecTask:装载的数据错误,代码%d",c_result);
			}
			else
			{
				//通知发送任务
				#if(boardUSE_OS)
				xTaskNotifyGive(tDcacTaskHandler);
				#endif  //boardUSE_OS
			}
			
        }
		else 
		{
			if(c_result == 0)
			{
				#if(boardUSE_OS)
				if(lwrb_get_full(&tpDcacProtoRx->tRxBuff) ==0)
					ulTaskNotifyTake(pdFALSE,portMAX_DELAY);//等待任务通知
				else
					vTaskDelay(10);
				#endif  //boardUSE_OS
			}
			else 
			{
				if(uPrint.tFlag.bDcacRecTask|| uPrint.tFlag.bImportant)
					log_w("bDcacRecTask:协议解析错误,代码%d",c_result);

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
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
static u8 c_check_conn_state(void)	
{
	//-----------------------丢失后第一次连接-----------------------------------------------
	if(tDcac.eDevState == DS_LOST)
	{
		bDcac_SetErrCode(DEC_SYS_DEV_LOST,false);
		
		#if(boardSYS_DATA_UPADATA)
		if(!BIT_GET(tSysInfo.Mod_Exist,OL_DCAC))//第一次初始化
		{
			STAT_SET(tSysInfo.Mod_Exist,OL_DCAC);
			Sys_Update_Element(AT_SYS_MODEXIST_ADDR, NULL, tSysInfo.Mod_Exist, true);
		}
		#endif
	}
	
	return 0;
}




/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/

/***********************************************************************************************************************
-----函数功能    检测设备的连接状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
void vDcac_RecTickTimer(void)
{
	if(tpDcacProtoRx == NULL)
		return;
	
	//******************************************数据帧接收超时计算***************************************************
	if(tpDcacProtoRx->usRecOverTimeCnt > 0)
	{    
		tpDcacProtoRx->usRecOverTimeCnt--;
	
		if(tpDcacProtoRx->usRecOverTimeCnt == 0)        
		{
			cModbus_StepWaitOutTime(tpDcacProtoRx);
		}
	}
	
	//******************************************逆变模块连接超时计算*************************************************		
	if(tpDcacProtoRx->usLostOverTimeCnt > 0 && bSys_IsWorkState() == true)
	{    
		tpDcacProtoRx->usLostOverTimeCnt--;
	
		if(tpDcacProtoRx->usLostOverTimeCnt == 0)      //逆变器丢失    
		{
			v_rec_task_param_init();
			
			bDcac_SetErrCode(DEC_SYS_DEV_LOST,true);
		}
	}
}






#if(boardLOW_POWER)
/***********************************************************************************************************************
-----函数功能    进入低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDcac_EnterLowPower(void)
{
	vTaskSuspend(tDcacRecTaskHandle);
	vTaskSuspend(tDcacTaskHandler);
	vDcac_IoEnterLowPower();
}


/***********************************************************************************************************************
-----函数功能    退出低功耗
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vDcac_ExitLowPower(void)
{
	vDcac_IfaceInit();
	vTaskResume(tDcacRecTaskHandle);
	vTaskResume(tDcacTaskHandler);
}
#endif	//boardLOW_POWER

#endif  //boardDCAC_EN
