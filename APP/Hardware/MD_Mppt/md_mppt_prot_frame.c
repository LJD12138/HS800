#include "MD_Mppt/md_mppt_prot_frame.h"

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_queue_task.h"
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"

#include "check.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_prot_frame.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "MD_Dcac/md_dcac_rec_task.h"

#endif  //boardDCAC_EN


#define       	mpptTX_PROTO_BUFF_LEN                   128
#define       	mpptRX_PROTO_BUFF_LEN                   256

//MPPT设备地址
#define       	mpptDEV_ADRR                          	0x01
#define  		mpptWAIT_NOTIFY_OUTTIME              	1000     //任务通知超时时间 MS

//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	ModbusProtoTx_t *tpMpptProtoTx = NULL;	//发送协议
__ALIGNED(4) 	ModbusProtoRx_t *tpMpptProtoRx = NULL;	//接受协议

//****************************************************函数声明****************************************************//
static s8 c_mppt_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bMppt_SendProtInit(void)
{
	s8 c_result = 1;

	c_result = cModbus_TransProtoInit(&tpMpptProtoTx, mpptTX_PROTO_BUFF_LEN, mpptDEV_ADRR);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
			log_e("bMpptTask:tpMpptProtoTx协议对象初始化失败,代码%d",c_result);
		
		return false;
	}
	
	return true;
}

bool bMppt_RecProtInit(void)
{
	s8 c_result = cModbus_RecProtoInit(&tpMpptProtoRx, 	//协议指针
								mpptRX_PROTO_BUFF_LEN,	//协议缓存器大小
								mpptDEV_ADRR,			//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bMpptRecTask || uPrint.tFlag.bImportant)
			log_e("bMpptRecTask:tpMpptProtoRx协议对象初始化失败,代码%d",c_result);
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
s8 c_mppt_cs_get_param(void)
{
	MpptParam_T t_param;
	
	return c_mppt_data_trans(modbusREAD_MULTI_REG, 
						mpptREG_ADDR_GET_PARAM1, 
						NULL, 
						sizeof(t_param)/2);
}

/*****************************************************************************************************************
-----函数功能    指令:设置充电功率
-----说明(备注)  none
-----传入参数    pwr:功率值
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_mppt_cs_set_pwr(u16 pwr)
{
	return c_mppt_data_trans(modbusWRITE_SINGLE_REG, 
						mpptREG_ADDR_SET_PV_CHG_PWR, 
						(u8*)&pwr, 
						1);
}

/***********************************************************************************************************************
-----函数功能	MPPT数据传输（Modbus协议帧发送与接收等待）
-----说明(备注)	该函数通过Modbus协议向MPPT设备发送命令，并等待设备回复。
				使用DCAC的串口发送数据，通过互斥量共享串口资源。
				使用任务通知机制实现发送与接收的同步。
-----传入参数	cmd:Modbus命令码（如modbusREAD_MULTI_REG、modbusWRITE_SINGLE_REG等）
				reg_addr:寄存器地址
				data:指向数据的指针（写操作时为要写入的数据，读操作时为NULL）
				len:数据长度（以16位寄存器为单位）
-----输出参数	none
-----返回值		-99:获取互斥锁超时（仅在操作系统环境下）
				-1:写入的Len超出最大长度
				-2:等待回复超时
				-3:数据发送错误
				0:无操作（协议对象未初始化或互斥锁未创建）
				1:操作成功
************************************************************************************************************************/
static s8 c_mppt_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpMpptProtoTx == NULL)
		return 0;
	
	
	#if(boardUSE_OS)
	// 检查互斥锁是否已创建，并获取互斥锁保护共享资源（最多等待1秒）
	if(dcacSemaphoreMutex == NULL)
		return 0;
	if(xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
		return -99;

	// 清除任务通知，避免历史通知干扰本次通信
	while(ulTaskNotifyTake(pdTRUE, 0) > 0)
	{
	}
	#endif  //boardUSE_OS
	
	#if(boardDCAC_IFACE)
	result = cModbus_ProtoCreate(tpMpptProtoTx, cmd, reg_addr, data, len);
	if(result > 0)
	{
		//MPPT
		bDcacUseFlag = false;

		// 通过DCAC串口发送MPPT数据
		if(bDcac_DataSendStart(tpMpptProtoTx->ucaFrameData, tpMpptProtoTx->ucFrameLen) == true)
		{
			// 等待接收任务通知（超时1秒），表示收到MPPT设备的回复
			#if(boardUSE_OS)
			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(mpptWAIT_NOTIFY_OUTTIME)) <= 0)
			{
				if((uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant) && tMppt.eDevState != DS_LOST)
					log_w("bMpptTask:命令0x%x,寄存器%d等待回复超时", cmd, reg_addr);
				
				result = -2;
			}
			#endif  //boardUSE_OS
		}
		else
			result = -3;
	}
	#endif  //boardDCAC_IFACE
	
	cModbus_ResetTx(tpMpptProtoTx, mpptTX_PROTO_BUFF_LEN);
	
	vTaskDelay(5);
	
	#if(boardUSE_OS)
	if(dcacSemaphoreMutex != NULL)
		xSemaphoreGive(dcacSemaphoreMutex);
	#endif  //boardUSE_OS
	
	return result;
}

#endif  //boardMPPT_EN
