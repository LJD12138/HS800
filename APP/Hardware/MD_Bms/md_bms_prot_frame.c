

#include "MD_Bms/md_bms_prot_frame.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#include "MD_Bms/md_bms_iface.h"
#include "MD_Bms/md_bms_rec_task.h"
#include "Print/print_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#define       	bmsTX_PROTO_BUFF_LEN                   	128
#define       	bmsRX_PROTO_BUFF_LEN                   	256

#define     	bmsDEV_ADRR								0x01
#define    		bmsWAIT_NOTIFY_OUTTIME     				1000     //任务通知超时时间 MS


//****************************************************参数初始化**************************************************//
__ALIGNED(4) ModbusProtoTx1_t *tpBmsProtoTx = NULL;	//发送协议
__ALIGNED(4) ModbusProtoRx1_t *tpBmsProtoRx = NULL;

/*创建互斥量*/
#if(boardUSE_OS)
SemaphoreHandle_t bmsSemaphoreMutex = NULL;
#endif  //boardUSE_OS


//****************************************************函数声明****************************************************//
static s8 c_bms_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bBms_SendProtInit(void)
{
	s8 c_result = cModbus1_TransProtoInit(&tpBmsProtoTx, 
								bmsTX_PROTO_BUFF_LEN, 
								bmsDEV_ADRR);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant)
			log_e("bBmsTask:tpBmsProtoTx协议对象初始化失败,代码%d",c_result);
		
		return false;
	}

	/* 创建互斥信号量 */
	#if(boardUSE_OS)
    bmsSemaphoreMutex = xSemaphoreCreateMutex();
	#endif  //boardUSE_OS
	
	return true;
}

bool bBms_RecProtInit(void)
{
	s8 c_result = cModbus1_RecProtoInit(&tpBmsProtoRx, 	//协议指针
								bmsRX_PROTO_BUFF_LEN,	//协议缓存器大小
								bmsDEV_ADRR,			//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);//计数器采样时间
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bBmsRecTask || uPrint.tFlag.bImportant)
			log_e("bBmsRecTask:tpBmsProtoRx协议对象初始化失败,代码%d",c_result);
		return false;
	}
	
	return true;
}







/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_get_param(u8 num)
{
	BmsParam_T t_param;
	num = sizeof(t_param) / 2;  //寄存器数量 = 数据长度/寄存器长度(2字节)
	return c_bms_data_trans(modbusREAD_MULTI_REG, bmsREG_ADDR_GET_PARAM, 0, num);
}

/*****************************************************************************************************************
-----函数功能    指令:开关BMS
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_switch(TaskInParam_U u_in_param)
{
	u16 temp = 0x0000;
	
	if(u_in_param.tTaskParam.ucParam == ST_ON)
		temp = 0x0001;

	return c_bms_data_trans(modbusWRITE_MULTI_REG, bmsREG_ADDR_CTRL_SWITCH, (u8*)&temp, 1);
}

/*****************************************************************************************************************
-----函数功能    指令:校准
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_set_cali(u8 num)
{
	// return c_bms_data_trans(baikuCMD_CALI, &num, 1);
	return 0;
}

/*****************************************************************************************************************
-----函数功能    指令:获取APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_get_app_info(u16 num)
{
	// u8 buff[3] = {0};
	
	// buff[0] = MO_BMS;
	// memcpy(&buff[1], (u8*)&num, 2);
	
	// return c_bms_data_trans(baikuCMD_GET_MEM_PARAM, buff, sizeof(buff));
	return 0;
}

/*****************************************************************************************************************
-----函数功能    指令:系统设置
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_sys_set(tSysSetParam *tparam)
{	
	// tSysSetParam tparam1 = *tparam;
	// return c_bms_data_trans(baikuCMD_SYS_SET, (u8*)&tparam1, sizeof(tparam1));
	return 0;
}

/*****************************************************************************************************************
-----函数功能    指令:请求充电
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_bms_cs_req_chg(void)
{
	// u8 obj = 0;
	// return c_bms_data_trans(baikuCMD_REQ_CHG, &obj, 1);
	return 0;
}

/***********************************************************************************************************************
-----函数功能	数据传输
-----说明(备注) 
-----传入参数	cmd:指令
				data:指向数据指针
				len:数据的长度
-----输出参数	none
-----返回值		-1:写入的Len超出最大长度
				-2:等会回复超时
				-3:数据发送错误
				0:无操作
				1:操作成功
************************************************************************************************************************/
static s8 c_bms_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpBmsProtoTx == NULL)
		return 0;

	#if(boardUSE_OS)
	// 检查互斥锁是否已创建，并获取互斥锁保护共享资源（最多等待1秒）
	if(bmsSemaphoreMutex == NULL)
		return 0;
	if(xSemaphoreTake(bmsSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
		return -99;

	// 清除任务通知，避免历史通知干扰本次通信
	while(ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
	}
	#endif  //boardUSE_OS
	
	result = cModbus1_ProtoCreate(tpBmsProtoTx, cmd, reg_addr, data, len);
	if(result > 0)
	{
		//BMS
		bBmsUseFlag = true;

		if(bBms_DataSendStart(tpBmsProtoTx->ucaFrameData, tpBmsProtoTx->ucFrameLen) == true)
		{
			//等待任务通知,等待时间为1S
			#if(boardUSE_OS)
			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(bmsWAIT_NOTIFY_OUTTIME)) <= 0)
			{
				if((uPrint.tFlag.bBmsTask || uPrint.tFlag.bImportant) && tBms.eDevState != DS_LOST)
					log_w("bBmsTask:等待指令0x%x,地址0x%x回复超时", cmd, reg_addr);
				
				result = -2;
			}
			#endif  //boardUSE_OS
		}
		else 
			result = -3;
	}

	cModbus1_ResetTx(tpBmsProtoTx, bmsTX_PROTO_BUFF_LEN);

	//释放互斥量
	#if(boardUSE_OS)
	vTaskDelay(2);
	
	xSemaphoreGive(bmsSemaphoreMutex);
	#endif  //boardUSE_OS

	return result;
}

#endif  //boardBMS_EN
