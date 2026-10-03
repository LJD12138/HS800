/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Usb
 * File    : usb_prot_frame.c
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : USB 协议帧收发与设备参数读写
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "Usb/usb_prot_frame.h"
#include <stdbool.h>

#if (boardUSB_EN)
#include "Usb/usb_queue_task.h"
#include "Usb/usb_task.h"
#include "Usb/usb_iface.h"
#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  /* boardPRINT_IFACE */

#include "filtration.h"
#include "ntc.h"

//****************************************************Macros********************************************************************//
#define			SW3516_SYS_STATE1_ADDR					0x08	//系统状态1
#define			SW3516_VOUT_ADDR						0x31	//VOUT
#define			SW3516_IOUT_C_ADDR						0x33	//IOUT1
#define			SW3516_IOUT_A_ADDR						0x34	//IOUT2
#define			SW3516_ADC_CFG_ADDR						0x3A	//ADC配置
#define			SW3516_ADC_DATA_H_ADDR					0x3B	//ADC-DATA
#define			SW3516_ADC_DATA_L_ADDR					0x3C	//ADC-DATA


//****************************************************Parameter Initialization**************************************************//
#pragma pack(1)
typedef struct
{
	vu8					ucState;			/* 设备状态 */
	vs8					cTemp;				/* 温度 (摄氏度) */
	vs16				sPower;				/* 总功率 (W) */
	vu16				usVolt;				/* 输出电压 (mV) */
	vu16				usPdCurr;			/* Type-C 电流 (mA) */
	vu16				usQcCurr;			/* Type-A 电流 (mA) */
	vu16				usPdPwr;			/* Type-C 功率 (W) */
	vu16				usQcPwr;			/* Type-A 功率 (W) */
}USB_IC_T;
#pragma pack()

/* [0]: PD100W+QC300W芯片; [1]: PD100W芯片 */
static USB_IC_T s_t_usb_ic[2] = {0};

//PD100W温度滤波器
#define 		usbPD_TEMP_FILTER_BUFF_SIZE     		10 
static s32 s_usa_pd_temp_buff[usbPD_TEMP_FILTER_BUFF_SIZE];
static FilterHandler_T s_t_adc_pd_temp_filter_mad_avg = {s_usa_pd_temp_buff, usbPD_TEMP_FILTER_BUFF_SIZE, 0, 0, 0, 0, 0};

//****************************************************Function Declaration******************************************************//


/***********************************************************************************************************************
 * 函数功能    : 指令:获取参数
 * 说明(备注)  : p_i2c_obj: 传入&tUSB_IC1_I2C或&tUSB_IC2_I2C
 * 传入参数    : p_i2c_obj: I2C对象指针
 * 输出参数    : none
 * 返回值      : -1:IC丢失  0:进行中  1:成功
 ************************************************************************************************************************/
s8 c_usb_cs_get_ic_param(const I2cObj_T *p_i2c_obj)
{
	static u8 s_uca_index[2] = {0};
	static u8 s_uca_lost_cnt[2] = {0};
	static u8 s_uca_buff[2][6] = {0};
	u8 ch = (p_i2c_obj == &tUSB_IC1_I2C) ? 0 : 1;
	u8 data[1] = {0};
	USB_IC_T *p_ic = &s_t_usb_ic[ch];

	switch (s_uca_index[ch])
	{
		/* 初始化温度ADC转换 */
		case 0:
		{
			data[0] = 0x06;
			if (cI2C_WriteBytes(p_i2c_obj, SW3516_ADC_CFG_ADDR, data, sizeof(data)) <= 0)
			{
				if (s_uca_lost_cnt[ch] < 0xff) s_uca_lost_cnt[ch]++;
				break;
			}

			s_uca_lost_cnt[ch] = 0;
			s_uca_index[ch]++;
		}

		/* 读取温度AD */
		case 1:
		{
			memset(&data, 0, sizeof(data));
			if (cI2C_ReadBytes(p_i2c_obj, SW3516_ADC_DATA_H_ADDR, data, sizeof(data)) <= 0)
			{
				if (s_uca_lost_cnt[ch] < 0xff) s_uca_lost_cnt[ch]++;
				break;
			}
			else 
				s_uca_lost_cnt[ch] = 0;

			s_uca_buff[ch][0] = data[0];
			
			memset(&data, 0, sizeof(data));
			if (cI2C_ReadBytes(p_i2c_obj, SW3516_ADC_DATA_L_ADDR, data, sizeof(data)) <= 0)
			{
				if (s_uca_lost_cnt[ch] < 0xff) s_uca_lost_cnt[ch]++;
				break;
			}
			else 
				s_uca_lost_cnt[ch] = 0;

			s_uca_buff[ch][1] = data[0];
			s_uca_index[ch]++;
		}
		
		/* 读取电压与Type-C电流 */
		case 2:
		{
			memset(&data, 0, sizeof(data));
			if (cI2C_ReadBytes(p_i2c_obj, SW3516_VOUT_ADDR, data, sizeof(data)) <= 0)
			{
				if(s_uca_lost_cnt[ch] < 0xff) s_uca_lost_cnt[ch]++;
				break;
			}
			else 
				s_uca_lost_cnt[ch] = 0;

			s_uca_buff[ch][2] = data[0];
			
			memset(&data, 0, sizeof(data));
			if (cI2C_ReadBytes(p_i2c_obj, SW3516_IOUT_C_ADDR, data, sizeof(data)) <= 0)
			{
				if (s_uca_lost_cnt[ch] < 0xff) s_uca_lost_cnt[ch]++;
				break;
			}
			else 
				s_uca_lost_cnt[ch] = 0;

			s_uca_buff[ch][3] = data[0];
			s_uca_index[ch]++;
		}

		/* 读取Type-A (QC)电流 */
		case 3:
		{
			memset(&data, 0, sizeof(data));
			if (cI2C_ReadBytes(p_i2c_obj, SW3516_IOUT_A_ADDR, data, sizeof(data)) <= 0)
			{
				if (s_uca_lost_cnt[ch] < 0xff) s_uca_lost_cnt[ch]++;
				break;
			}
			else 
				s_uca_lost_cnt[ch] = 0;

			s_uca_buff[ch][4] = data[0];
			s_uca_index[ch]++;
		}

		/* 结算 */
		case 4:
		{
			/* ********************************************************************************* */
			s32 temp = (s_uca_buff[ch][0] << 4) | (s_uca_buff[ch][1] & 0x0f);  /* 温度AD值 */
			temp = lFilter_MadianAverage(&s_t_adc_pd_temp_filter_mad_avg, &temp);
			/* 用户通讯 */
//			if(s_uca_buff[ch][0] > 100 )  
				p_ic->cTemp   = (int8_t)(sNtc_CalcTempByAd((uint16_t)temp) / 2);
			
			p_ic->usVolt   = (uint16_t)s_uca_buff[ch][2] * 96;          /* mV */
			/* if(p_ic->usVolt >= 100) */
			/* 	p_ic->usVolt -= 100; */
			
			p_ic->usPdCurr = (uint16_t)s_uca_buff[ch][3] * 40;          /* mA */
			/* if(p_ic->usPdCurr >= 255) */
			/* 	p_ic->usPdCurr -= 255; */
			
			p_ic->usQcCurr = (uint16_t)s_uca_buff[ch][4] * 40;          /* mA */

			/* ********************************************************************************* */
			p_ic->usPdPwr  = (uint16_t)(((uint32_t)p_ic->usPdCurr * p_ic->usVolt) / 1000000UL);
			p_ic->usQcPwr = (uint16_t)(((uint32_t)p_ic->usQcCurr * p_ic->usVolt) / 1000000UL);
			p_ic->sPower   = p_ic->usPdPwr + p_ic->usQcPwr;
			us_usb_total_out_pwr += p_ic->sPower;

			if (tUsb.eDevState == DS_WORK)
				s_max_temp = MAX2(s_max_temp, p_ic->cTemp);

			s_uca_index[ch] = 0;

			if (uPrint.tFlag.bUsbTask)
				sMyPrint("bUsbTask:SW3518[%d]电压 = %dmV, 功率 = %dW \r\n", ch, p_ic->usVolt, p_ic->sPower);
		}
		break;

		default:
		{
			s_uca_index[ch] = 0;
		}
		break ;
	}

	/* 状态 */
	if(s_uca_lost_cnt[ch] >= 16)  /* 丢失 */
	{
		s_uca_lost_cnt[ch] = 0;
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
	else if(!s_uca_lost_cnt[ch])  /* 正常 */
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


/***********************************************************************************************************************
 * 函数功能    : 控制USB-A和USB-C通道开关
 * 说明(备注)  : 解锁寄存器REG 0x15后，写入操作寄存器REG 0x16以控制开关状态
 * 传入参数    : b_open - true: 强制开启通道；false: 强制关闭通道
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vUSB_ControlPorts(bool b_open)
{
	uint8_t data[1];
	uint8_t val = b_open ? 0x00 : 0x01; /* 0x02: 强制开启Buck; 0x01: 强制关闭Buck */
	
	//下面代码无需执行
	return;

	if(b_open == true)
	{
		/* 控制第一路 USB 芯片 (IC1) */
		data[0] = 0x20;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x15, data, 1);
		data[0] = 0x40;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x15, data, 1);
		data[0] = 0x80;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x15, data, 1);

		data[0] = val;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x16, data, 1);

		data[0] = 0;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x76, data, 1);

		/* 控制第二路 USB 芯片 (IC2) */
		data[0] = 0x20;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x15, data, 1);
		data[0] = 0x40;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x15, data, 1);
		data[0] = 0x80;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x15, data, 1);

		data[0] = val;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x16, data, 1);

		data[0] = 0;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x76, data, 1);
	}
	else
	{
		/* 控制第一路 USB 芯片 (IC1) */
		data[0] = 3;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x76, data, 1);

		data[0] = 0x20;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x15, data, 1);
		data[0] = 0x40;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x15, data, 1);
		data[0] = 0x80;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x15, data, 1);

		data[0] = val;
		cI2C_WriteBytes(&tUSB_IC1_I2C, 0x16, data, 1);

		/* 控制第二路 USB 芯片 (IC2) */
		data[0] = 3;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x76, data, 1);

		data[0] = 0x20;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x15, data, 1);
		data[0] = 0x40;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x15, data, 1);
		data[0] = 0x80;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x15, data, 1);

		data[0] = val;
		cI2C_WriteBytes(&tUSB_IC2_I2C, 0x16, data, 1);
	}
}

#endif  /* boardUSB_EN */

