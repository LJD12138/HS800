

#include "MD_Dcac/md_dcac_prot_frame.h"

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#include "MD_Dcac/md_dcac_iface.h"
#include "Print/print_task.h"

#include "check.h"
#include "function.h"
#include "app_info.h"



#define       	dcacDEV_ADRR                          	0x01
#define  		dcacWAIT_NOTIFY_OUTTIME              	1000     //任务通知超时时间 MS
#define       	dcacTX_PROTO_BUFF_LEN                   128
#define       	dcacRX_PROTO_BUFF_LEN                   128

//****************************************************参数初始化**************************************************//
__ALIGNED(4) 	ModbusProtoTx_t *tpDcacProtoTx = NULL;	//发送协议
__ALIGNED(4) 	ModbusProtoRx_t *tpDcacProtoRx = NULL;	//发送协议

#pragma pack (1)   //强制进行1字节对齐
struct
{
	vu16 usAcOutSwitch;
	vu16 usBatOV;//0.1V
	vu16 usBatUV;//0.1V
	vu16 usOutFreq;//50/60
	vu16 usOutVolt;//100/110/120/220/230/240
	vu16 usChgPwr;//1W
	vu16 usDisChgPwr;
	vu16 usChgVolt;//0.001V
	vu16 usPvOV;//0.1V
	vs16 ucFan;
	vu16 usPvChgPwr;//1W
	vu16 usAcChgPwr;//1W
	vu16 temp2;
	vu16 usMaxInCurr;//0.1A
}tDcacInit;
#pragma pack()   //取消进行1字节对齐

/*创建互斥量*/
#if(boardUSE_OS)
SemaphoreHandle_t dcacSemaphoreMutex = NULL;
#endif  //boardUSE_OS


//****************************************************函数声明****************************************************//
static s8 c_dcac_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bDcac_SendProtInit(void)
{
	s8 c_result = 1;

	c_result = cModbus_TransProtoInit(&tpDcacProtoTx, dcacTX_PROTO_BUFF_LEN, dcacDEV_ADRR);
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant)
			log_e("bDcacTask:tpDcacProtoTx协议对象初始化失败,代码%d",c_result);
		
		return false;
	}
	
	/* 创建互斥信号量 */
	#if(boardUSE_OS)
    dcacSemaphoreMutex = xSemaphoreCreateMutex();
	#endif  //boardUSE_OS
	
	return true;
}

bool bDcac_RecProtInit(void)
{
	s8 c_result = cModbus_RecProtoInit(&tpDcacProtoRx, 	//协议指针
								256,			//协议缓存器大小
								dcacDEV_ADRR,	//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	if(c_result <= 0)
	{
		if(uPrint.tFlag.bDcacRecTask || uPrint.tFlag.bImportant)
			log_e("bDcacRecTask:tpDcacProtoRx协议对象初始化失败,代码%d",c_result);
		return false;
	}
	
	return true;
}



/*****************************************************************************************************************
-----函数功能    指令:开关BMS
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_ac_output_switch(u16 temp)
{
	if(c_dcac_data_trans(modbusWRITE_SINGLE_REG, 
						dcacREG_ADDR_DISCHG_SW, 
						(u8*)&temp, 
						1) <= 0)
		return false;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_get_param1(void)
{
	DCAC_Param1_t tParam1;
	
	if(c_dcac_data_trans(modbusREAD_MULTI_REG, 
						dcacREG_ADDR_GET_PARAM1, 
						NULL, 
						sizeof(tParam1)/2) <= 0)
		return false;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_get_param2(void)
{
	DCAC_Param2_t tParam2;
	
	if(c_dcac_data_trans(modbusREAD_MULTI_REG, 
						dcacREG_ADDR_GET_PARAM2, 
						NULL, 
						sizeof(tParam2)/2) <= 0)
		return false;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_get_param3(void)
{
	DCAC_Param3_t tParam3;
	
	if(c_dcac_data_trans(modbusREAD_MULTI_REG, 
						dcacREG_ADDR_GET_PARAM3, 
						NULL, 
						sizeof(tParam3)/2) <= 0)
		return false;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:设置充电功率
-----说明(备注)  none
-----传入参数    num设置的充电功率
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_set_total_chg_pwr(u16 pwr)
{
	// tDcacInit.usChgVolt = tAppMemParam.tBMS.usChgVolt * 100; //0.1V
	tDcacInit.usChgPwr = pwr;	//充电功率W
	// tDcacInit.usDisChgPwr = tAppMemParam.tDCAC.usOutPwrRating;
	if(c_dcac_data_trans(modbusWRITE_MULTI_REG, 
						dcacREG_ADDR_SET_TOTAL_CHG_PWR, 
						(u8*)&tDcacInit.usChgPwr, 
						1) <= 0)
		return false;
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    指令:设置充电功率
-----说明(备注)  none
-----传入参数    num设置的充电功率
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_set_chg_pwr(u16 pwr)
{
	if(c_dcac_data_trans(modbusWRITE_SINGLE_REG,
						dcacREG_ADDR_SET_AC_CHG_PWR,
						(u8*)&pwr,
						1) <= 0)
		return false;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:开关BMS
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_init(void)
{
	tDcacInit.usAcOutSwitch = dcacSWITCH_REG_OFF;
	tDcacInit.usBatOV = tAppMemParam.tBMS.usMaxVolt;	//0.1V
	tDcacInit.usBatUV = tAppMemParam.tBMS.usMinVolt;	//0.1V
	tDcacInit.usOutFreq = (tAppMemParam.tDCAC.usAcOutFreq == 0)? 50:60;	//0:50HZ 1:60HZ
	
	#if(boardDCAC_VOLT_TYPE==0)	//110V
	tDcacInit.usOutVolt = 110;	
	#elif(boardDCAC_VOLT_TYPE==3) //230V
	tDcacInit.usOutVolt = 230;
	#else
    #error "DCAC类型定义有误"
	#endif
	
	tDcacInit.usChgVolt = tAppMemParam.tBMS.usChgVolt * 100; //0.1V
	// tDcacInit.usChgPwr = tAppMemParam.tDCAC.usInPwrRating;	//充电功率W
	tDcacInit.usChgPwr = 0;	//充电功率W
	tDcacInit.usDisChgPwr = tAppMemParam.tDCAC.usOutPwrRating;
	
	tDcacInit.usPvOV = tAppMemParam.tMPPT.usMaxInVolt; //0.1V


	// if(strstr(boardSOFTWARE_VERSION, "G3604") != NULL)
	// 	tDcacInit.ucFan = 0;
	// else
		tDcacInit.ucFan = 0;
	
	tDcacInit.usPvChgPwr = 0;//1W
	tDcacInit.usAcChgPwr = 0;//1W
	tDcacInit.usMaxInCurr = tAppMemParam.tDCAC.usMaxInCurr;
	
	if(c_dcac_data_trans(modbusWRITE_MULTI_REG, 
						dcacREG_ADDR_INIT, 
						(u8*)&tDcacInit, 
						sizeof(tDcacInit)/2) <= 0)
		return false;
	
	return true;
}

/*****************************************************************************************************************
-----函数功能    指令:并网功率设置
-----说明(备注)  none
-----传入参数    num:并网功率
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_set_para_in_pwr(u16 pwr)
{
//	if(num)
//		tDcacParaInInit.usParaOutEn = dcacSWITCH_REG_ON;
//	else
//		tDcacParaInInit.usParaOutEn = dcacSWITCH_REG_OFF;
//	
//	tDcacParaInInit.usParaPwrSet = num;	//1W
//	
//	return b_dcac_write_multi_reg(dcacREG_ADDR_SET_PARA_IN,sizeof(tDcacParaInInit)/2,(u16*)&tDcacParaInInit);
	return false;
}

/*****************************************************************************************************************
-----函数功能    指令:并网功率设置
-----说明(备注)  none
-----传入参数    num:并网功率
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
bool b_dcac_cs_sys_switch(u16 temp)
{
	// if(c_dcac_data_trans(modbusWRITE_SINGLE_REG, 
	// 					dcacREG_ADDR_DISCHG_SW, 
	// 					(u8*)&temp, 
	// 					1) <= 0)
	// 	return false;
	
	return true;
}


/***********************************************************************************************************************
-----函数功能	DCAC数据传输（Modbus协议帧发送与接收等待）
-----说明(备注)	该函数通过Modbus协议向DCAC设备发送命令，并等待设备回复。
				使用互斥锁保护共享资源，使用任务通知机制实现发送与接收的同步。
-----传入参数	cmd:Modbus命令码（如modbusREAD_MULTI_REG、modbusWRITE_SINGLE_REG等）
				reg_addr:寄存器地址
				data:指向数据的指针（写操作时为要写入的数据，读操作时为NULL）
				len:数据长度（以16位寄存器为单位）
-----输出参数	none
-----返回值		-99:获取互斥锁超时（仅在操作系统环境下）
				-1:写入的Len超出最大长度
				-2:等待回复超时
				-3:数据发送错误
				0:无操作（互斥锁或协议对象未初始化）
				1:操作成功
************************************************************************************************************************/
static s8 c_dcac_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpDcacProtoTx == NULL)
		return 0;
	
	#if(boardUSE_OS)
	// 获取互斥锁，保护共享资源（最多等待1秒)
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
	result = cModbus_ProtoCreate(tpDcacProtoTx, cmd, reg_addr, data, len);
	if(result > 0)
	{
		//DCAC
		bDcacUseFlag = true;

		if(bDcac_DataSendStart(tpDcacProtoTx->ucaFrameData, tpDcacProtoTx->ucFrameLen) == true)
		{
			// 等待接收任务通知（超时1秒），表示收到DCAC设备的回复
			#if(boardUSE_OS)
			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(dcacWAIT_NOTIFY_OUTTIME)) <= 0)
			{
				if((uPrint.tFlag.bDcacTask || uPrint.tFlag.bImportant) && tDcac.eDevState != DS_LOST)
					log_w("bDcacTask:命令0x%x,寄存器%d等待回复超时", cmd, reg_addr);
				
				result = -2;
			}
			#endif  //boardUSE_OS
		}
		else
			result = -3;
	}
	#endif
	
	cModbus_ResetTx(tpDcacProtoTx, dcacTX_PROTO_BUFF_LEN);
	
	vTaskDelay(5);
	
	#if(boardUSE_OS)
	if(dcacSemaphoreMutex != NULL)
		xSemaphoreGive(dcacSemaphoreMutex);
	#endif  //boardUSE_OS
	
	return result;
}

#endif  //boardDCAC_EN
