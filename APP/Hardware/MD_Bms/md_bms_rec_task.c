/*****************************************************************************************************************
*                                                                                                                *
 *                                         逆变接收任务                                                         *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Bms/md_bms_rec_task.h"
#include "main.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_data_proc.h"
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_iface.h"
#include "MD_Bms/md_bms_prot_frame.h"
#include "MD_Bms/md_bms_queue_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardUPDATE)
#include "proto_update.h"
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE


//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define			bmsREC_TASK_PRIO                    	2        //任务优先级 
#define			bmsREC_TASK_SIZE                    	192      //任务堆栈  实际字节数 *4
TaskHandle_t	tBmsRecTaskHandle;
void			vBms_RecTask(void *pvParameters);
#endif  //boardUSE_OS


//****************************************************参数初始化**************************************************//
__ALIGNED(4) BmsRx_T tBmsRx;
vu32 ulBmsRxErrCode = 0;

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
    memset(&tBmsRx, 0 ,sizeof(tBmsRx));

	if(tpBmsTask->tReplyBuff.buff != NULL)
		lwrb_reset(&tpBmsTask->tReplyBuff);
	
	tBms.sMaxTemp = 25;
	tBms.sMinTemp = 25;
	
	cBaiku_ResetRxBuff(tpBmsProtoRx);
}


/***********************************************************************************************************************
-----函数功能    逆变接收任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bBms_RecTaskInit(void)
{
	//接口初始化
	#if(boardBMS_EN)
	vBms_IfaceInit();
	#endif
	
	//接收协议初始化
	if(bBms_RecProtInit() == false)
		return false;
		
	//任务参数初始化
	v_rec_task_param_init();
	
    //数据解析任务初始化
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vBms_RecTask,			//任务函数
                (const char* )"BmsRecTask",   			//任务名称
                (uint16_t ) bmsREC_TASK_SIZE,  			//任务堆栈大小
                (void* )NULL,                 			//传递给任务函数的参数
                (UBaseType_t ) bmsREC_TASK_PRIO,		//任务优先级
                (TaskHandle_t*)&tBmsRecTaskHandle);    	//任务句柄
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
void vBms_RecTask(void *pvParameters)
{
	s8 c_result = 0;
	
    #if(boardUSE_OS)
    for(;;)
	#endif  //boardUSE_OS
    {
		if(tpBmsProtoRx == NULL)
		{
			bBms_RecProtInit();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif
		}

		//******************************************协议处理****************************************************
		//协议解析
		#if(boardUPDATE)
		if(tBms.eDevState == DS_UPDATE_MODE)
		{
			c_result = cUpdate_ProtoCheck(tpBmsProtoRx, &tpPrintTask->tReplyBuff);

			//协议适配
			if(c_result == PT_BAIKU || c_result == PT_XMODEM)
				cUpdate_ProtoSelect(UO_BMS, (ProtoType_E)c_result);
			//其他未适配或未定义
			else
				cUpdate_ProtoSelect(UO_BMS, PT_NULL);

			if(c_result != PT_BAIKU)
				c_result = 0;
		}
		else
		#endif  //boardUPDATE
        	c_result = cBaiku_ProtoCheck(tpBmsProtoRx);

		//数据处理
		if(c_result > 0)
        {
			c_check_conn_state();
			c_result = c_bms_rec_proc_data(tpBmsProtoRx);
			if(c_result <= 0)
			{
				if(uPrint.tFlag.bBmsRecTask || uPrint.tFlag.bImportant)
					log_w("bBmsRecTask:装载的数据错误,代码%d",c_result);
			}
			else
			{
				#if(boardUSE_OS)
				xTaskNotifyGive(tBmsTaskHandler);//通知发送任务
				#endif  //boardUSE_OS
			}

			#if(boardUPDATE)
			if(tBms.eDevState == DS_UPDATE_MODE)
			{
				if(tUpdate.eChType == CT_PRINT)
					xTaskNotifyGive(tPrintTaskHandler);
			}
			#endif  //boardUPDATE
        }
		else 
		{
			if(c_result == 0)
			{
				#if(boardUSE_OS)
				if(lwrb_get_full(&tpBmsProtoRx->tRxBuff) ==0)
					ulTaskNotifyTake(pdFALSE,portMAX_DELAY);//等待任务通知
				else
					vTaskDelay(10);
				#endif  //boardUSE_OS
			}
			else 
			{
				if(uPrint.tFlag.bBmsRecTask|| uPrint.tFlag.bImportant)
					log_w("bBmsRecTask:协议解析错误,代码%d",c_result);
					
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
	if(tBms.eDevState == DS_LOST)
	{
		bBms_SetErrCode(BEC_SYS_DEV_LOST, false);
		
		#if(boardSYS_DATA_UPADATA)
		if(!BIT_GET(tSysInfo.Mod_Exist,OL_BMS))//第一次初始化
		{
			STAT_SET(tSysInfo.Mod_Exist,OL_BMS);
			Sys_Update_Element(AT_SYS_MODEXIST_ADDR, NULL, tSysInfo.Mod_Exist, true );
		}
		#endif
	}
	
	return 0;
}

/***********************************************************************************************************************
-----函数功能    检测设备的连接状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vBms_RecTickTimer(void)
{
	if(tpBmsProtoRx == NULL)
		return;
	
	//******************************************数据帧接收超时计算***************************************************
	if(tpBmsProtoRx->usRecOverTimeCnt > 0)
	{    
		tpBmsProtoRx->usRecOverTimeCnt--;
	
		if(tpBmsProtoRx->usRecOverTimeCnt == 0)        
		{
			cBaiku_StepWaitOutTime(tpBmsProtoRx);
		}
	}
	
	//******************************************逆变模块连接超时计算*************************************************		
	if(tpBmsProtoRx->usLostOverTimeCnt > 0 && bSys_IsWorkState() == true )
	{    
		tpBmsProtoRx->usLostOverTimeCnt--;
	
		if(tpBmsProtoRx->usLostOverTimeCnt == 0)      //丢失    
		{
			v_rec_task_param_init();
			
			bBms_SetErrCode(BEC_SYS_DEV_LOST, true);
		}
	}
}
#endif  //boardBMS_EN
