
#include "Usb/usb_prot_frame.h"

#if(boardUSB_EN)
#include "Usb/usb_queue_task.h"
#include "Usb/usb_task.h"
#include "Usb/usb_iface.h"
#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

#include "filtration.h"
#include "math.h"
// #include "check.h"


// #define       	usbDEV_ADRR                          	0x01
// #define  		usbWAIT_NOTIFY_OUTTIME              	1000     //任务通知超时时间 MS
// #define       	usbTX_PROTO_BUFF_LEN                   	128
// #define       	usbRX_PROTO_BUFF_LEN                   	256

//*********************************寄存器地址********************************
#define     	SW3516_SYS_STATE1_ADDR        			0x08//系统状态1
#define     	SW3516_VOUT_ADDR              			0x31//VOUT
#define     	SW3516_IOUT_C_ADDR             			0x33//IOUT1
#define     	SW3516_IOUT_A_ADDR             			0x34//IOUT2
#define     	SW3516_ADC_CFG_ADDR           			0x3A//ADC配置
#define     	SW3516_ADC_DATA_H_ADDR        			0x3B//ADC-DATA
#define     	SW3516_ADC_DATA_L_ADDR        			0x3C//ADC-DATA


//****************************************************参数初始化**************************************************//
// __ALIGNED(4) 	ModbusProtoTx_t *tpUsbProtoTx = NULL;	//发送协议
// __ALIGNED(4) 	ModbusProtoRx_t *tpUsbProtoRx = NULL;	//接受协议
#pragma pack(1)
typedef struct
{
	vu8					ucState;          	//设备状态
	vs8            		cTemp;              //温度
    vs16           		sPower;             //1W 总功率 
    vu16           		usVolt;             //mV
    vu16           		usPdCurr;       	//mA
	vu16           		usQcCurr;       	//mA
	vu16           		usPdPwr;        	//1W
    vu16           		usQcPwr;        	//1W
}USB_IC_T; 
#pragma pack()

//PD100W温度滤波器
#define 		usbPD_TEMP_FILTER_BUFF_SIZE     		10 
static s32 usa_pd_temp_buff[usbPD_TEMP_FILTER_BUFF_SIZE];
FilterHandler_T tAdc_PDTempFilterMadAvg = {usa_pd_temp_buff, usbPD_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//****************************************************函数声明****************************************************//
// static s8 c_usb_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len);



/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bUsb_SendProtInit(void)
{
	// s8 c_result = 1;

	// c_result = cModbus_TransProtoInit(&tpUsbProtoTx, usbTX_PROTO_BUFF_LEN, usbDEV_ADRR);
	// if(c_result <= 0)
	// {
	// 	if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
	// 		log_e("bUsbTask:tpUsbProtoTx协议对象初始化失败,代码%d",c_result);
		
	// 	return false;
	// }
	
	return true;
}

bool bUsb_RecProtInit(void)
{
	// s8 c_result = cModbus_RecProtoInit(&tpUsbProtoRx, 	//协议指针
	// 							usbRX_PROTO_BUFF_LEN,	//协议缓存器大小
	// 							usbDEV_ADRR,			//协议设备ID
	// 							boardREPET_TIMER_CYCLE_TMIE);			//计数器采样时间
	// if(c_result <= 0)
	// {
	// 	if(uPrint.tFlag.bUsbRecTask || uPrint.tFlag.bImportant)
	// 		log_e("bUsbRecTask:tpUsbProtoRx协议对象初始化失败,代码%d",c_result);
	// 	return false;
	// }
	
	return true;
}


USB_IC_T tUsbIc[2] = {0}; //IC参数: [0]PD100W + QC300W芯片     [1]PD100W芯片
/*****************************************************************************************************************
-----函数功能    指令:获取参数
-----说明(备注)  p_i2c_obj: 传入&tUSB_IC1_I2C或&tUSB_IC2_I2C
-----传入参数    p_i2c_obj: I2C对象指针
-----输出参数    none
-----返回值      -1:IC丢失  0:进行中  1:成功
******************************************************************************************************************/
s8 c_usb_cs_get_ic_param(const I2cObj_T *p_i2c_obj)
{
	static u8 uc_index[2] = {0};
	static u8 uc_lost_cnt[2] = {0};
	static u8 s_uca_buff[2][6] = {0};
	u8 ch = (p_i2c_obj == &tUSB_IC1_I2C) ? 0 : 1;
	u8 data[1] = {0};
	USB_IC_T *p_ic = &tUsbIc[ch];

	switch (uc_index[ch])
	{
		/* 初始化温度ADC转换 */
		case 0:
		{
			data[0] = 0x06;
			if(cI2C_WriteBytes(p_i2c_obj, SW3516_ADC_CFG_ADDR, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt[ch] < 0xff) 
					uc_lost_cnt[ch]++;
				break;
			}

			uc_lost_cnt[ch] = 0;
			uc_index[ch]++;
		}

		/* 读取温度AD */
		case 1:
		{
			memset(&data, 0, sizeof(data));
			if(cI2C_ReadBytes(p_i2c_obj, SW3516_ADC_DATA_H_ADDR, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt[ch] < 0xff) uc_lost_cnt[ch]++;
				break;
			}
			else 
				uc_lost_cnt[ch] = 0;

			s_uca_buff[ch][0] = data[0];
			
			memset(&data, 0, sizeof(data));
			if(cI2C_ReadBytes(p_i2c_obj, SW3516_ADC_DATA_L_ADDR, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt[ch] < 0xff) uc_lost_cnt[ch]++;
				break;
			}
			else 
				uc_lost_cnt[ch] = 0;

			s_uca_buff[ch][1] = data[0];
			uc_index[ch]++;
		}
		
		/* 读取电压与Type-C电流 */
		case 2:
		{
			memset(&data, 0, sizeof(data));
			if(cI2C_ReadBytes(p_i2c_obj, SW3516_VOUT_ADDR, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt[ch] < 0xff) uc_lost_cnt[ch]++;
				break;
			}
			else 
				uc_lost_cnt[ch] = 0;

			s_uca_buff[ch][2] = data[0];
			
			memset(&data, 0, sizeof(data));
			if(cI2C_ReadBytes(p_i2c_obj, SW3516_IOUT_C_ADDR, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt[ch] < 0xff) uc_lost_cnt[ch]++;
				break;
			}
			else 
				uc_lost_cnt[ch] = 0;

			s_uca_buff[ch][3] = data[0];
			uc_index[ch]++;
		}

		/* 读取Type-A (QC)电流 */
		case 3:
		{
			memset(&data, 0, sizeof(data));
			if(cI2C_ReadBytes(p_i2c_obj, SW3516_IOUT_A_ADDR, data, sizeof(data)) <= 0)
			{
				if(uc_lost_cnt[ch] < 0xff) uc_lost_cnt[ch]++;
				break;
			}
			else 
				uc_lost_cnt[ch] = 0;

			s_uca_buff[ch][4] = data[0];
			uc_index[ch]++;
		}

		/* 结算 */
		case 4:
		{
			/* ********************************************************************************* */
			s32 temp = (s_uca_buff[ch][0] << 4) | (s_uca_buff[ch][1] & 0x0f);  /* 温度AD值 */
			temp = lFilter_MadianAverage(&tAdc_PDTempFilterMadAvg, &temp);
			/* 用户通讯 */
//			if(s_uca_buff[ch][0] > 100 )  
				p_ic->cTemp = LIMIT((307 - (37 * log((float)temp))), -128, 127) / 2;
			
			p_ic->usVolt = s_uca_buff[ch][2] * 96;    /* mV */
			/* if(p_ic->usVolt >= 100) */
			/* 	p_ic->usVolt -= 100; */
			
			p_ic->usPdCurr = s_uca_buff[ch][3] * 40;    /* mA */
			/* if(p_ic->usPdCurr >= 255) */
			/* 	p_ic->usPdCurr -= 255; */
			
			p_ic->usQcCurr = s_uca_buff[ch][4] * 40;    /* mA */

			/* ********************************************************************************* */
			p_ic->usPdPwr = (p_ic->usPdCurr / 1000.0f) * (p_ic->usVolt / 1000.0f);
			p_ic->usQcPwr = (p_ic->usQcCurr / 1000.0f) * (p_ic->usVolt / 1000.0f);
			p_ic->sPower = p_ic->usPdPwr + p_ic->usQcPwr;
			us_usb_total_out_pwr += p_ic->sPower;
			
			uc_index[ch] = 0;
			
			if(uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:SW3518[%d]电压 = %dmV, 功率 = %dW \r\n", ch, p_ic->usVolt, p_ic->sPower);
		}
		break;

		default:
			uc_index[ch] = 0;
			break ;
	}

	/* 状态 */
	if(uc_lost_cnt[ch] >= 16)  /* 丢失 */
	{
		uc_lost_cnt[ch] = 0;
		if(ch == 0)
		{
			if(tUsb.uErrCode.tCode.bIc1Lost == 0)
			{
				bUsb_SetErrCode(UEC_IC1_LOST,true);
			
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_e("bUsbTask:IC1丢失");
			}
		}
		else
		{
			if(tUsb.uErrCode.tCode.bIc2Lost == 0)
			{
				bUsb_SetErrCode(UEC_IC2_LOST,true);
			
				if(uPrint.tFlag.bUsbTask || uPrint.tFlag.bImportant)
					log_e("bUsbTask:IC2丢失");
			}
		}
		return -1;
	}
	else if(!uc_lost_cnt[ch])  /* 正常 */
	{
		if(ch == 0)
		{
			if(tUsb.uErrCode.tCode.bIc1Lost == 1)
				bUsb_SetErrCode(UEC_IC1_LOST,false);
		}
		else
		{
			if(tUsb.uErrCode.tCode.bIc2Lost == 1)
				bUsb_SetErrCode(UEC_IC2_LOST,false);
		}
		return 1;
	}
	else
		return 0;
}

/*****************************************************************************************************************
-----函数功能    指令:开关MPPT
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
s8 c_usb_set_pwr_cs(u16 pwr)
{
	// if(c_usb_data_trans(modbusWRITE_SINGLE_REG, 
	// 					usbREG_ADDR_SET_CHG_PWR, 
	// 					(u8*)&pwr, 
	// 					1) <= 0)
	// 	return false;
	
	return 1;
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
// static s8 c_usb_data_trans(u8 cmd, u16 reg_addr, u8* data, u8 len)
// {
// 	s8 result = 0;
	
// 	if(dcacSemaphoreMutex == NULL)
// 		return 0;
	
// 	if(tpProtoTx == NULL)
// 		return 0;
	
// 	//开始互斥
// 	#if(boardUSE_OS)
// 	if(xSemaphoreTake(dcacSemaphoreMutex, pdMS_TO_TICKS(1000)) == pdFAIL)
// 		return -99;
// 	#endif  //boardUSE_OS
	
// 	//开始发送
// 	#if(boardDCAC_EN)
// 	result = cModbus_ProtoCreate(tpProtoTx, cmd, reg_addr, data, len);
// 	if(result > 0)
// 	{
// 		if(bDcac_DataSendStart(tpProtoTx->ucaFrameData, tpProtoTx->ucFrameLen) == true)
// 		{
// 			//等待任务通知,等待时间为1S
// 			#if(boardUSE_OS)
// 			if(ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(usbWAIT_NOTIFY_OUTTIME)) <= 0) 
// 			{
// 				if(uPrint.tFlag.bUsbTask)
// 					log_w("bUsbTask:等待指令0x%x,地址0x%x回复超时", cmd, reg_addr);
				
// 				result = -2;
// 			}
// 			#endif  //boardUSE_OS
// 		}
// 		else 
// 			result = -3;
// 	}
// 	#endif
	
// 	cModbus_ResetTx(tpUsbProtoTx, usbTX_PROTO_BUFF_LEN);
	
// 	//释放互斥量
// 	#if(boardUSE_OS)
// 	xSemaphoreGive(dcacSemaphoreMutex);
// 	#endif  //boardUSE_OS
	
// 	return result;
// }

#endif  //boardUSB_EN
