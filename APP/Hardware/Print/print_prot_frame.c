#include "Print/print_prot_frame.h"

#if(boardPRINT_IFACE)
#include "Print/print_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_task.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

#include "app_info.h"

#if(boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  //boardUPDATE

#if(boardADC_EN)
#include "Adc/adc_task.h"
#endif  //boardADC_EN

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if(boardLIGHT_EN)
#include "MD_Light/md_light_task.h"
#endif  //boardLIGHT_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //boardDCAC_EN

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#endif  //boardMPPT_EN

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  //boardUSE_OS

#define     	printDEV_ADRR							printCONSOLE_MASTER_ADDR
#define    		printWAIT_NOTIFY_OUTTIME				1000     //任务通知超时时间 MS
#define       	printTX_FRAME_SIZE                     	256
#define       	printRX_FRAME_SIZE                     	256
														
//****************************************************参数初始化**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpPrintProtoTx = NULL;	//发送协议
__ALIGNED(4) BaikuProtoRx_t *tpPrintProtoRx = NULL;

vu8 uc_next_cmd = 0;

//****************************************************函数声明****************************************************//
static s8 c_print_data_trans(u8 cmd, u8* data, u8 len);
static s8 c_get_console_ver_info(u8* data, u8* data_len);
static s8 c_relay_console_info(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
-----函数功能    通讯协议初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
bool bPrint_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoSendInit(&tpPrintProtoTx,//协议指针
								printTX_FRAME_SIZE, 	//协议缓存器大小
								printDEV_ADRR);			//协议设备ID
	if(c_result <= 0)
		return false;
	
	return true;
}

bool bPrint_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpPrintProtoRx, 	//协议指针
								printRX_FRAME_SIZE,		//协议缓存器大小
								printDEV_ADRR,			//协议设备ID
								boardREPET_TIMER_CYCLE_TMIE);//计数器采样时间
	if(c_result <= 0)
		return false;
	
	return true;
}




/***********************************************************************************************************************
-----函数功能    持续回复
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_cycle_relay_data(void)
{
	static vu16 us_delay_cnt = 0;
	
	if(uc_next_cmd != 0x0D)
	{
		us_delay_cnt = 0;
		return 0;
	}
	
	us_delay_cnt++;
	if(us_delay_cnt >= (100/printTASK_CYCLE_TIME))
	{
		us_delay_cnt = 0;
//		c_relay0E_mppt_param();
	}
	return 1;
}

/***********************************************************************************************************************
-----函数功能    回复模块开关结果  0x02
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
s8 c_relay02_switch_result(uint8_t data[])
{
    if(data[0] == 0x00)
    {
        if(data[1] == 0x00)
        {
            cSys_Switch(SO_KEY, ST_OFF, false);
        }  
        else if (data[1] == 0x01)
        {
            cSys_Switch(SO_KEY,ST_ON, false);
        }
        c_print_data_trans(0x02, data, 2);  
    }

	#if(boardUSB_EN)
    else if(data[0] == 0x01)
    {
        if(data[1] == 0x00)
        {
            cUsb_Switch(ST_OFF, false);
        }  
        else if (data[1] == 0x01)
        {
            //打开失败
            if(cUsb_Switch(ST_ON, false) <= 0)
                data[1] = 0x00;
        }
        c_print_data_trans(0x02, data, 2);  
    }
	#endif  //boardUSB_EN

	#if(boardLIGHT_EN)
    else if(data[0] == 0x02)
    {
        if(data[1] == 0x00)
        {
            bLight_Switch(ST_OFF);
        }  
        else if (data[1] == 0x01)
        {
            //打开失败
            if(bLight_Switch(ST_ON) == false)
                data[1] = 0x00;
        }
        c_print_data_trans(0x02, data, 2);  
    }
	#endif  //boardLIGHT_EN

	#if(boardDCAC_EN)
    else if(data[0] == 0x03)
    {
        if(data[1] == 0x00)
        {
            cDCAC_Switch(DSO_AC_OUT,ST_OFF, true);
        }  
        else if (data[1] == 0x01)
        {
            //打开失败
            if(cDCAC_Switch(DSO_AC_OUT,ST_ON, true) < 0)
                data[1] = 0x00;
        }
        c_print_data_trans(0x02, data, 2);  
    }
	#endif  //boardDCAC_EN

	#if(boardDC_EN)
	else if(data[0] == 0x04)
    {
        if(data[1] == 0x00)
        {
            cDc_Switch(ST_OFF, false);
        }  
        else if (data[1] == 0x01)
        {
            //打开失败
            if(cDc_Switch(ST_ON, false) <= 0)
                data[1] = 0x00;
        }
        c_print_data_trans(0x02, data, 2);  
    }
	#endif  //boardUSB_EN

	#if(boardDCAC_EN)
	else if(data[0] == 0x05)
    {
       if(data[1] == 0x00)
       {
           cDCAC_Switch(DSO_AC_IN,ST_OFF, true);
       }  
       else if (data[1] == 0x01)
       {
           //打开失败
           if(cDCAC_Switch(DSO_AC_IN,ST_ON, true) < 0)
               data[1] = 0x00;
       }
        c_print_data_trans(0x02, data, 2);  
    }
	#endif  //boardDCAC_EN

    return true;
}




/***********************************************************************************************************************
-----函数功能    回复参数  0x08
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay08_param(void)
{
	uint8_t data[20] = {0};
    uint8_t len = 0;

	data[0] = tSysInfo.eDevState;

	#if(boardBMS_EN)
	memcpy((u8*)&data[1], (u8*)&tBmsRx.tDevInfo[0].usVolt,2);
	memcpy((u8*)&data[3], (u8*)&tBmsRx.sTotalCurr,2);
	#endif  //boardBMS_EN

	data[5] = tSysInfo.uPerm.tPerm.bChgPerm;
	data[6] = tSysInfo.uPerm.tPerm.bDisChgPerm;

	#if(boardUSB_EN)
	data[7] = tUsb.eDevState;
	#endif  //boardUSB_EN

	#if(boardLIGHT_EN)
	data[8] = tLight.eDevState;
	#endif  //boardLIGHT_EN

	#if(boardDCAC_EN)
	data[9] = tDcac.eDisChgState;
	#endif  //boardDCAC_EN

	#if(boardDC_EN)
	data[10] = tDc.eDevState;
	#endif  //boardDC_EN

	#if(boardDCAC_EN)
	data[11] = tDcac.eChgState;
	#endif  //boardDCAC_EN

	#if(boardMPPT_EN)
	data[12] = tMppt.eDevState;
	#endif  //boardMPPT_EN

	#if(boardBMS_EN)
	data[13] = ucBms_GetSoc();
	#endif  //boardBMS_EN

    len = 14;
    //len不可以超过data的最大长度
    return c_print_data_trans(0x08, data, len);
}


/***********************************************************************************************************************
-----函数功能    回复电池参数  0x0A
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay0A_bat_param(void)
{
	#if(boardBMS_EN)
	uint8_t data[255] = {0};
    uint8_t len = 0;
	
    len = sizeof(tBms); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tBms, sizeof(tBms));

    len += sizeof(tBmsRx); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - sizeof(tBmsRx)], (u8*)&tBmsRx, sizeof(tBmsRx));
	
	len += 10; 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - 10], (u8*)tpBmsTask, 10);
    
    //len不可以超过data的最大长度
    return c_print_data_trans(0x0A, data, len);
	#else
	return false;
	#endif  //boardBMS_EN
}


s8 c_relay0C_dcac_param(void)
{
	#if(boardDCAC_EN)
    uint8_t data[80] = {0};
    uint8_t len = 0;
	
    len = sizeof(tDcac);
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data, (u8*)&tDcac, sizeof(tDcac));
    
    len += sizeof(tDcacRx);
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len  - sizeof(tDcacRx)], (u8*)&tDcacRx, sizeof(tDcacRx));
	
	len += 10;
    if(len > sizeof(data)) return false;//data长度不足
	memcpy(&data[len  - 10], (u8*)tpDcacTask, 10);

    //发送失败
    return c_print_data_trans(0x0C, data, len);
	#else
	return false;
	#endif  //boardDCAC_EN
}

/***********************************************************************************************************************
-----函数功能    回复MPPT参数  0x0E
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay0E_mppt_param(void)
{
	#if(boardMPPT_EN)
	uint8_t data[128] = {0};
    uint8_t len = 0;

    len = sizeof(tMppt); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tMppt, sizeof(tMppt));

    len += sizeof(tMpptRx);
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - sizeof(tMpptRx)], (u8*)&tMpptRx, sizeof(tMpptRx));
	
	len += 10; 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - 10], (u8*)tpMpptTask, 10);

    //发送失败
    return c_print_data_trans(0x0E, data, len);
	#else
	return false;
	#endif  //boardMPPT_EN
}

/***********************************************************************************************************************
-----函数功能    回复USB参数  0x10
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/

s8 c_relay10_usb_param(void)
{
	#if(boardUSB_EN)
    uint8_t data[80] = {0};
    uint8_t len = 0;

    len = sizeof(tUsb); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tUsb, sizeof(tUsb));
    //发送失败
    return c_print_data_trans(0x10, data, len);
	#else
	return false;
	#endif  //boardUSB_EN
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x12
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay12_dc_param(void)
{
	#if(boardDC_EN)
    uint8_t data[20] = {0};
    uint8_t len = 0;

    len = sizeof(tDc); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tDc, sizeof(tDc));
    //发送失败
    return c_print_data_trans(0x12, data, len);
	#else
	return false;
	#endif  //boardDC_EN
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay14_sysinfo_param(void)
{
    uint8_t data[50] = {0};
    uint8_t len = 0;

    len = sizeof(tSysInfo); 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(data, (u8*)&tSysInfo, sizeof(tSysInfo));
	
	len += 10; 
    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - 10], (u8*)tpSysTask, 10);
	
    //发送失败
    return c_print_data_trans(0x14, data, len);
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay40_set_chg_pwr(BaikuProtoRx_t* proto)
{
    if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	#pragma pack(1)
	struct
	{
		u8 module;
		u16 pwr;
	}t_chg_pwr;
	#pragma pack()
	
	memcpy((u8*)&t_chg_pwr, proto->ucpValidData, proto->ucValidLen);
	
	if(t_chg_pwr.module != 0 && t_chg_pwr.module!= 1)
		return false;
	
	//MPPT
	#if(boardMPPT_EN)
	if(t_chg_pwr.module == 0)
	{
		if(t_chg_pwr.pwr > tAppMemParam.tMPPT.usInPwrRating)
			return false;
		
		cMppt_SetChgPwr(t_chg_pwr.pwr);
	}
	#endif  //boardMPPT_EN

	//DCAC
	#if(boardDCAC_EN)
	if(t_chg_pwr.module == 1)
	{
		if(t_chg_pwr.pwr > tAppMemParam.tDCAC.usInPwrRating)
			return false;
	}
	#endif  //boardDCAC_EN

	return c_print_data_trans(baikuCMD_REPLY_SET_CHG_PWR, (u8*)&t_chg_pwr, sizeof(t_chg_pwr));
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay44_cali(BaikuProtoRx_t* proto)
{
	if(proto->ucValidLen != 1 || proto->ucpValidData == NULL)
		return -1;
	
	if(proto->ucpValidData[0] == 0)
	{
		#if(boardBMS_EN)
		cQueue_AddQueueTask(tpBmsTask, BTI_CALI, 0, false);
		#endif  //boardBMS_EN
	}
	
	return 1;
}

/***********************************************************************************************************************
-----函数功能    回复系统任务参数  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay45_cali(u16 temp)
{
	return c_print_data_trans(baikuCMD_REPLY_CALI, (u8*)&temp, sizeof(temp));
}

/***********************************************************************************************************************
-----函数功能    回复记忆的参数  0x80
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay80_get_mem_param(BaikuProtoRx_t* proto)
{
	u8 uc_mode = proto->ucpValidData[0];
	
	if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	if(uc_mode != MO_BMS && uc_mode != MO_CONSOLE)
		return false;
	
	switch(uc_mode)
	{
		case MO_CONSOLE://主控
		{
			c_relay_console_info(proto);
		}
		break;
		
		#if(boardBMS_EN)
		case MO_BMS://电池包
		{
			TaskInParam_U u_in_param;
			memcpy(&u_in_param.usTaskInParam, &proto->ucpValidData[1], 2);
			cQueue_AddQueueTask(tpBmsTask, BTI_GET_INFO, u_in_param.usTaskInParam, false);
		}
		break;
		#endif  //boardBMS_EN
	}
	
	return true;
}

/***********************************************************************************************************************
-----函数功能    写入记忆参数信息  0x14
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败   
************************************************************************************************************************/
s8 c_relay82_write_mem_info(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	
	//数据长度不对
	if(sizeof(tAppMemParam) != proto->ucValidLen)
	{
		temp = 0xff;
		c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &temp, 1);
		return 1;
	}
	//写入参数
	// bApp_MemParamUpdate(proto->ucpValidData,proto->ucValidLen, true);
	temp = 0x00;
	return c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &temp, 1);
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay84_set_print_state(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	u8 len = sizeof(uPrint) + 1;
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != len || obj != 0 || proto->ucpValidData == NULL)
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
		return -10;
	}

	memcpy((u8*)&uPrint.ulFlag, &proto->ucpValidData[1], len -1);
	temp = 0x00;
	return c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay86_get_print_state(BaikuProtoRx_t* proto)
{
	uint8_t data[5] = {0};
    uint8_t len = 0;
	u8 obj = proto->ucpValidData[0];
	
	if(proto->ucValidLen != 1 || obj != 0 ||  proto->ucpValidData == NULL)
		return -10;
	
	len = 1;
	data[0] = obj;
	
    len += sizeof(uPrint); 
    if(len > sizeof(data)) return -11;//data长度不足    
    memcpy(&data[1], (u8*)&uPrint, sizeof(uPrint));

    return c_print_data_trans(baikuCMD_REPLY_PRINT_STATE, data, sizeof(data));
}

/***********************************************************************************************************************
-----函数功能    回复print状态  0x87
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay88_sys_set(BaikuProtoRx_t* proto)
{
	#if(boardUPDATE)
	u8 temp = 0;

	tSysSetParam t_sys_set_param = {0};
			
	if(proto->ucValidLen != sizeof(t_sys_set_param))
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1);
		return -1;
	}
	
	memcpy(&t_sys_set_param, proto->ucpValidData, proto->ucValidLen);
	//主控
	if(t_sys_set_param.obj == UO_DEFAULT ||
		t_sys_set_param.obj == UO_CONSOLE)
	{
		temp = 0x00;
		if(c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1) <= 0)
			return -2;
		
		vApp_JumpToBoot(t_sys_set_param.cmd);
	}
	//BMS
	#if(boardBMS_EN)
	else if(t_sys_set_param.obj == UO_BMS)
	{
		//进入升级
		if(t_sys_set_param.cmd == mainUPDATE_FLAG)
		{
			if(cUpdate_ChSelect((UpdateObj_E)t_sys_set_param.obj, CT_PRINT) <= 0)
				return -5;
		}
		//其他设置
		else
		{
			if(tpBmsTask->tReplyBuff.buff == NULL)
				return -3;

			lwrb_reset(&tpBmsTask->tReplyBuff);
			lwrb_write(&tpBmsTask->tReplyBuff, &t_sys_set_param, sizeof(t_sys_set_param));

			if(cQueue_AddQueueTask(tpBmsTask, BTI_REQ_SET_CMD, 0, true) <= 0)
				return -4;
		}
	}
	#endif  //boardBMS_EN
	
	return 1;
	#else
	return 0;
	#endif   //boardUPDATE
}

/***********************************************************************************************************************
-----函数功能    回复APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:发送成功   false:发送失败
************************************************************************************************************************/
s8 c_relay_bms_app_info(u8* data, u16 len)
{
	if(len > 256)
		return false;
	
	return c_print_data_trans(baikuCMD_REPLY_MEM_PARAM, data, len);
}






/***********************************************************************************************************************
-----函数功能    回复APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败
************************************************************************************************************************/
static s8 c_relay_console_info(BaikuProtoRx_t* proto)
{
	u8 uc_mode = 0;
	u8 uc_index = 0;
	u8 uc_obj = 0;
	u8 len = 0;
	s8 c_ret = 0;
	u8 buff[255] = {0};
	
	uc_mode = proto->ucpValidData[0];
	uc_index = proto->ucpValidData[1];
	uc_obj = proto->ucpValidData[2];
	
	if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	if(uc_mode != MO_CONSOLE)
		return false;
	
	memcpy(buff, proto->ucpValidData, 3);
	
	switch(uc_index)
	{
		case 0://SYS
		{
			//获取版本信息
			if(uc_obj == 1)
			{
				len = sizeof(tAppMemParam.tVerInfo);
				if(len > 245)
					return false;
				
				if(c_get_console_ver_info(&buff[3], &len) == false)
					goto get_mem_param_fail;
			}
			else
				goto get_mem_param_fail;
		}
		break;
	}
	
	c_ret = c_print_data_trans(baikuCMD_REPLY_MEM_PARAM, buff, len + 3);
	if(c_ret <= 0)
	{
		get_mem_param_fail:
		return false;
	}

	return true;
}

/***********************************************************************************************************************
-----函数功能    获取BMS的版本信息
-----说明(备注)  data指向的数组长度要足够,要不会溢出
-----传入参数    none
-----输出参数    none
-----返回值      true:成功   false:失败
************************************************************************************************************************/
static s8 c_get_console_ver_info(u8* data, u8* data_len)
{
	u8 len = 0;
	u8 char_len = 0;
	char temp[] = "\r\n";
	char ver_temp[] = boardHARDWARE_VERSION;
	
	char_len = sizeof(tAppMemParam.tVerInfo.saVersion);
    len += char_len; 
//    if(len > sizeof(data)) return false;//data长度不足    
    memcpy(&data[len - char_len], (u8*)&tAppMemParam.tVerInfo.saVersion, char_len);
	
	char_len = sizeof(temp);
	len += char_len;
//    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - char_len], (u8*)temp, char_len);
	
	char_len = sizeof(ver_temp);
	len += char_len;
//    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - char_len], (u8*)ver_temp, char_len);
	
	char_len = sizeof(temp);
	len += char_len;
//    if(len > sizeof(data)) return false;//data长度不足
    memcpy(&data[len - char_len], (u8*)temp, char_len);
	
	*data_len = len;
	return true;
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
static s8 c_print_data_trans(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpPrintProtoTx == NULL)
		return 0;
	
	#if(boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if(result > 0)
	{
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
			return bPrint_SendDataToUsart();
	}
	#endif
	return result;
}

#endif  //boardPRINT_IFACE
