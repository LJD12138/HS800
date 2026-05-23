/*****************************************************************************************************************
*                                                                                                                *
 *                                         系统总任务                                                           *
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_task.h"

DebugPrint_U   	uPrint;

#if(boardPRINT_IFACE)
#include "Print/print_prot_frame.h"
#include "Print/print_iface.h"
#include "Print/print_queue_task.h"
#include "Sys/sys_task.h"

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

//****************************************************任务初始化**************************************************//
#if(boardUSE_OS)
#define       	printTASK_PRIO                        	1        						//任务优先级 
#define       	printTASK_SIZE                        	1024     						//任务堆栈  实际字节数 *4
TaskHandle_t	tPrintTaskHandler = NULL;
void          	vPrint_Task(void *pvParameters);
#endif  //boardUSE_OS

//****************************************************参数初始化**************************************************//
//调试缓存器
#define       	printTX_BUFF_SIZE                     	512
lwrb_t tPrintTxBuff;
__ALIGNED(4) static u8 uca_print_tx_buff[printTX_BUFF_SIZE];          //用于发送数据的缓存区

static Task_T *tp_task = NULL;
														
/*
互斥信号量 必须是同一个任务申请，同一个任务释放，其他任务释放无效，中断内不可用，优先级可继承
二值信号量 可以由另一个任务释放，可以在中断中使用，没有优先级继承（存在优先级翻转的现象）
*/
#if(boardUSE_OS)
/*创建信号量句柄 */
SemaphoreHandle_t PrintSemaphoreBinary = NULL;   //二进制信号量,用于转载串口数据 
/* 互斥信号量句柄 */
//SemaphoreHandle_t PrintSemMutex = NULL;
#endif  //boardUSE_OS

/***********************************************************************************************************************
-----函数功能    复位接收参数BUFF
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void b_print_task_param_init(void)
{
	#if(boardUSE_OS)
	/* 创建二进制信号量 */
    PrintSemaphoreBinary = xSemaphoreCreateBinary(); 
	xSemaphoreGive(PrintSemaphoreBinary);
	
	/* 创建互斥信号量 */
//    PrintSemMutex = xSemaphoreCreateMutex();
	#endif  //boardUSE_OS
	
	lwrb_reset(&tPrintTxBuff);
	
	tp_task = tpPrintTask;
	
    vPrint_MyPrintParamInit();
	
	tSysInfo.uInit.tFinish.bIF_Print = 1;
}


/***********************************************************************************************************************
-----函数功能    逆变接收任务初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bPrint_TaskInit(void)
{
	//接收协议初始化
	if(bPrint_RecProtInit() == false)
		return false;
	
	//发送协议初始化
	if(bPrint_SendProtInit() == false)
		return false;
	
	//任务队列初始化
	if(bPrint_QueueInit() == false)
		return false;
	
	//调试缓存器初始化
    lwrb_init(&tPrintTxBuff, uca_print_tx_buff, printTX_BUFF_SIZE);  

	//任务参数初始化
	b_print_task_param_init();
	
	//数据解析任务
	#if(boardUSE_OS)
    xTaskCreate((TaskFunction_t )vPrint_Task,          	//任务函数
                (const char* )"PrintTask",				//任务名称
                (uint16_t ) printTASK_SIZE,             //任务堆栈大小
                (void* )NULL,                           //传递给任务函数的参数
                (UBaseType_t ) printTASK_PRIO,          //任务优先级
                (TaskHandle_t*)&tPrintTaskHandler);     //任务句柄
	#endif  //boardUSE_OS

	return true;
}




/***********************************************************************************************************************
-----函数功能    接收任务
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_Task(void *pvParameters)
{
	#if(boardUSE_OS)
	for(;;)
	#endif  //boardUSE_OS
    { 
		#if(boardWDGT_EN && boardPRINT_IFACE)
		vFwdgt_Reload();
		#endif
		
		if(tp_task == NULL)
		{
			if(tp_task == NULL)
				b_print_task_param_init();
			
			#if(boardUSE_OS)
			vTaskDelay(500);
			continue;
			#else
			return;
			#endif  //boardUSE_OS
		}

		#if(boardUSB_EN)
		if(tUsb.eDevState == DS_SHUT_DOWN)
			printIFACE_EN_ON();
		else
			printIFACE_EN_OFF();
		#else
			printIFACE_EN_ON();
		#endif  //boardUSB_EN
		
		if(tp_task->vp_func != NULL && tp_task ->bNowRun == false)
			tp_task->vp_func(tp_task);
		else if(tp_task->vp_func == NULL || tp_task ->bNowRun == true)
		{
			#if(boardUSE_OS)
			if(lwrb_get_full(&tp_task->tQueueBuff) == 0)
				ulTaskNotifyTake(pdTRUE, printTASK_CYCLE_TIME);//pdFALSE:任务通知多少次就执行多少次
			#endif  //boardUSE_OS
			
			if(tp_task->bp_task_manage_func != NULL)
				tp_task->bp_task_manage_func(tp_task);
		}
    }
}

/***********************************************************************************************************************
-----函数功能    把缓存区的数据读取出来发送
-----说明(备注)  MyPrint函数写入的数据
-----传入参数    none
-----输出参数    none
-----返回值      true:开始发送    false:还有数据未发送完成   
************************************************************************************************************************/
bool bPrint_SendDataToUsart(void)
{
	static u16 us_data_len;
	bool flag = false;
	
	if(tpPrintProtoTx == NULL)
		return false;
	
	#if(boardUSE_OS)
	if(PrintSemaphoreBinary == NULL)
		return false;
	#endif  //boardUSE_OS
	
	if(bPrint_CheckSendFinish() == false)
		return false;
	
	#if(boardUSE_OS)
	if(xSemaphoreTake(PrintSemaphoreBinary, ( TickType_t) 0 ) == pdPASS) 
	#endif  //boardUSE_OS
	{
		us_data_len = lwrb_get_full(&tPrintTxBuff);
		
		#if(boardPRINT_IFACE)
		if(us_data_len)
			flag = bPrint_DataSendStart(us_data_len);	
		#endif
		
		//释放信号量
		#if(boardUSE_OS)
        xSemaphoreGive(PrintSemaphoreBinary);
		#endif
	}
	return flag;
}

/***********************************************************************************************************************
-----函数功能    检测设备的连接状态
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_RecTickTimer(void)
{
	if(tpPrintProtoRx ==NULL)
		return;
	
	//******************************************数据帧接收超时计算***************************************************
	if(tpPrintProtoRx->usRecOverTimeCnt > 0)
	{    
		tpPrintProtoRx->usRecOverTimeCnt--;
	
		if(tpPrintProtoRx->usRecOverTimeCnt == 0)        
		{
			cBaiku_StepWaitOutTime(tpPrintProtoRx);
		}
	}
	
	//******************************************逆变模块连接超时计算*************************************************		
	if(tpPrintProtoRx->usLostOverTimeCnt > 0)
	{    
		tpPrintProtoRx->usLostOverTimeCnt--;
	
		if(tpPrintProtoRx->usLostOverTimeCnt == 0)      //丢失    
		{
			cBaiku_ResetRxBuff(tpPrintProtoRx);
		}
	}
}
#endif //boardPRINT_IFACE





