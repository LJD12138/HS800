/*****************************************************************************************************************
*                                                                                                                *
 *                                         协议解析构造                                                         *
*                                                                                                                *
******************************************************************************************************************/
#include "Modbus/modbus_proto.h"
#include "check.h"
#include "board_config.h"
#include "function.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

//****************************************************局部宏定义**************************************************//


//****************************************************参数初始化**************************************************//
const u8 ucaModbusCmdBuff[5] = {modbusWRITE_MULTI_REG, 
								modbusWRITE_SINGLE_REG,
								modbusREAD_MULTI_REG,
								modbusREAD_MULTI_BIT,
								modbusWRITE_SINGLE_BIT};

//****************************************************函数声明****************************************************//
bool b_modbus_jump_step(ModbusProtoRx_t* proto, ModbusRxStep_E step);
s8 c_check_cmd_exist(u8 cmd);
s8 c_proto_decrypt(ModbusProtoRx_t* proto);

/*****************************************************************************************************************
-----函数功能	接受协议初始化
-----说明(备注)	none
-----传入参数	proto:接受协议结构体
				buff_len::协议缓存器大小
				dev_addr:设备地址
				cycle_time:协议循环时基
-----输出参数	none
-----返回值		小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_RecProtoInit(ModbusProtoRx_t** proto, u16 buff_len, u8 dev_addr, u16 cycle_time)
{
	s8 result = 1;
	
	if(buff_len < 6) 
		return -1;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	
//	sMyPrint("Free Heap: %u\n", xPortGetFreeHeapSize());
	
	// 动态分配内存
    size_t total_size = sizeof(ModbusProtoRx_t) + buff_len;
	#if(boardUSE_OS)
	*proto = (ModbusProtoRx_t*)pvPortMalloc(total_size);
	(*proto)->ucpFrameData = (u8*)pvPortMalloc(buff_len);
	#else
	*proto = (ModbusProtoRx_t*)malloc(total_size);
	(*proto)->ucpFrameData = (u8*)malloc(buff_len);
	#endif
    
	
	if(*proto != NULL && (*proto)->ucpFrameData != NULL)
	{
		(*proto)->ucAddr = dev_addr;
		(*proto)->usTaskCycleTime = cycle_time;
		(*proto)->usRecOverTimeCnt = 0;
		(*proto)->usLostOverTimeCnt = 0;
		
		lwrb_init(&(*proto)->tRxBuff, (*proto)->ucaData, buff_len);
		lwrb_reset(&(*proto)->tRxBuff);

		b_modbus_jump_step(*proto, MRS_ADDR);
	}
	else 
	{
		#if(boardUSE_OS)
		vPortFree((*proto)->ucpFrameData);  //先释放子内存,要不会导致崩溃
		vPortFree((*proto));
		#else
		free((*proto)->ucpFrameData);  //先释放子内存,要不会导致崩溃
		free((*proto));
		#endif
		result =  -2;
	}
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	return result;
}

/*****************************************************************************************************************
-----函数功能	发送协议初始化
-----说明(备注)	none
-----传入参数	proto:接受协议结构体
				buff_len::协议缓存器大小
				dev_addr:设备地址
-----输出参数	none
-----返回值		小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_TransProtoInit(ModbusProtoTx_t** proto, u16 buff_len, u8 dev_addr)
{
	s8 result = 1;
	
	if(buff_len < 6) 
		return -1;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	// 动态分配内存
	size_t total_size = sizeof(ModbusProtoTx_t) + buff_len;
	#if(boardUSE_OS)
    *proto = (ModbusProtoTx_t*)pvPortMalloc(total_size);
	#else
    *proto = (ModbusProtoTx_t*)malloc(total_size);
	#endif
	
	(*proto)->ucAddr = dev_addr;
	
	if(*proto == NULL)
	{
		#if(boardUSE_OS)
		vPortFree((*proto));
		#else
		free((*proto));
		#endif
		result =  -2;
	}
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	return result;
}

/*****************************************************************************************************************
-----函数功能	构造协议
-----说明(备注)	none
-----传入参数	FrameInf协议的结构体
				[0]:Header
				[1]:Addr
				[2]:Len = Cmd~CheckSum;
				[3]:Cmd
				[4]:SN
				[5]:data
				[5+n]:payload data
				[6+n]:CheckSum
-----输出参数	none
-----返回值		小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_ProtoCreate(ModbusProtoTx_t* proto, u8 cmd, u16 reg_addr, u8* data, u16 len)
{
	s8 result = 1;
	u8 uc_char_len = 0;
	u16 us_total_len = 0;  //不包含校验
	u16 us_tx_crc = 0;
	
	//总长度不可以超过256(250+6)
    if(len > 250)   
        return -1;
	
	if(proto == NULL)
		return -2;
	
	if(c_check_cmd_exist(cmd) <= 0)
		return -3;
	
	switch(cmd)
	{
		case modbusWRITE_MULTI_REG:
		{
			if(data == NULL || len == 0)
				return -4;
			
			uc_char_len = len * 2;
			
			//组建数据帧
			proto->ucaFrameData[0] = proto->ucAddr;
			proto->ucaFrameData[1] = cmd;
			proto->ucaFrameData[2] = reg_addr >> 8;
			proto->ucaFrameData[3] = reg_addr & 0x00ff;
			proto->ucaFrameData[4] = len >> 8;  
			proto->ucaFrameData[5] = len & 0x00ff;
			proto->ucaFrameData[6] = uc_char_len;
			
			bFunc_SwapU16Array(&proto->ucaFrameData[7], (u8*)data, len);
			
			us_total_len = 7 + uc_char_len;
			
			proto->usRegAddr = reg_addr;
			proto->usRegSize = len;
		}
		break;
		
		case modbusWRITE_SINGLE_REG:
		{
			if(data == NULL || len != 1)
				return -4;
			
			uc_char_len = len * 2;
			
			//组建数据帧
			proto->ucaFrameData[0] = proto->ucAddr;
			proto->ucaFrameData[1] = cmd;
			proto->ucaFrameData[2] = reg_addr >> 8;
			proto->ucaFrameData[3] = reg_addr & 0x00ff;
			
			bFunc_SwapU16Array(&proto->ucaFrameData[4], (u8*)data, len);
			
			us_total_len = 4 + uc_char_len;
			
			proto->usRegAddr = reg_addr;
			memcpy((u8*)&proto->usRegData, data, uc_char_len);
		}
		break;
		
		case modbusREAD_MULTI_BIT:
		{
			if(len == 0)
				return -4;
			
			//组建数据帧
			proto->ucaFrameData[0] = proto->ucAddr;
			proto->ucaFrameData[1] = cmd;
			proto->ucaFrameData[2] = reg_addr >> 8;
			proto->ucaFrameData[3] = reg_addr & 0x00ff;
			proto->ucaFrameData[4] = len >> 8;  
			proto->ucaFrameData[5] = len & 0x00ff;
			
			us_total_len = 6;
			
			proto->usRegAddr = reg_addr;
			proto->ucCharLen = len / 8;
		}
		break;
		
		case modbusWRITE_SINGLE_BIT:
		{
			if(data == NULL || len != 1)
				return -4;
			
			uc_char_len = len * 2;
			
			//组建数据帧
			proto->ucaFrameData[0] = proto->ucAddr;
			proto->ucaFrameData[1] = cmd;
			proto->ucaFrameData[2] = reg_addr >> 8;
			proto->ucaFrameData[3] = reg_addr & 0x00ff;
			
			bFunc_SwapU16Array(&proto->ucaFrameData[4], (u8*)data, len);
			
			us_total_len = 4 + uc_char_len;
			
			proto->usRegAddr = reg_addr;
			memcpy((u8*)&proto->usRegData, data, uc_char_len);
		}
		break;
		
		case modbusREAD_MULTI_REG:
		{
			if(len == 0)
				return -4;
			
			//组建数据帧
			proto->ucaFrameData[0] = proto->ucAddr;
			proto->ucaFrameData[1] = cmd;
			proto->ucaFrameData[2] = reg_addr >> 8;
			proto->ucaFrameData[3] = reg_addr & 0x00ff;
			proto->ucaFrameData[4] = len >> 8;  
			proto->ucaFrameData[5] = len & 0x00ff;
			
			us_total_len = 6;
			
			proto->usRegAddr = reg_addr;
			proto->ucCharLen = len * 2;
		}
		break;
		
		default:
			return -4;
	}
	
	//计算数据帧CRC码
	us_tx_crc = usCheck_CRC16(proto->ucaFrameData, us_total_len);
	
	proto->ucaFrameData[us_total_len] = us_tx_crc & 0x00ff;
	proto->ucaFrameData[us_total_len + 1] = us_tx_crc >> 8;
	
	proto->ucFrameLen = us_total_len + 2;
	
    return result;
}


/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_ProtoCheck(ModbusProtoRx_t* proto)
{
	s8 c_result = 0;
	u8 temp[10] = {0};
	vu16 delay_cnt = 512;
	
	if(proto == NULL)
		return -2;
	
	switch(proto->eStep)
	{
		case MRS_ADDR:  //地址
		{
			if(lwrb_get_full(&proto->tRxBuff) < proto->ucWaitRecLen)
				return 0;
			
			lwrb_read(&proto->tRxBuff, temp, 2);
			
			while(1)
			{
				c_result = c_check_cmd_exist(temp[1]);
				
				//匹对成功
				if(temp[0] == proto->ucAddr && c_result != 0)
				{
					proto->ucAddr = temp[0];
					proto->ucCmd = temp[1];
					
					memcpy(proto->ucpFrameData, temp, 2);
					
					//故障码
					if(c_result < 0)
					{
						b_modbus_jump_step(proto, MRS_ERR);
						return 0;
					}
					else if(proto->ucCmd == modbusWRITE_MULTI_REG)
						proto->ucWaitRecLen = sizeof(proto->usRegAddr) + sizeof(proto->usRegSize);
					else if(proto->ucCmd == modbusREAD_MULTI_REG)
						proto->ucWaitRecLen = sizeof(proto->ucCharLen);
					else if(proto->ucCmd == modbusWRITE_SINGLE_REG)
						proto->ucWaitRecLen = sizeof(proto->usRegAddr);
					else
						proto->ucWaitRecLen = 2;
					
					b_modbus_jump_step(proto, MRS_LEN);
					break;
				}
				else 
				{
					//缓存区为空则退出循环
					if(lwrb_get_full(&proto->tRxBuff) == 0)
						break;
					
					temp[0] = temp[1];
					lwrb_read(&proto->tRxBuff, &temp[1], 1);
					
					//超时退出
					delay_cnt--;
					if(delay_cnt == 0)
						return -3;
				}
			}
			//引导码可能丢失
			if(proto->eStep == MRS_ADDR)
				return -4;
		}

		case MRS_LEN: //等待接收长度
		{
			if(lwrb_get_full(&proto->tRxBuff) >= proto->ucWaitRecLen)
			{
				//取出长度
				if(proto->ucCmd == modbusWRITE_MULTI_REG)
				{
					lwrb_read(&proto->tRxBuff, temp, 4);
	
					bFunc_SwapU16Array((u8*)&proto->usRegAddr, &temp[0], 1);
					bFunc_SwapU16Array((u8*)&proto->usRegSize, &temp[2], 1);
					
					memcpy(&proto->ucpFrameData[2], temp, 4);
					
					proto->ucWaitRecLen = 2;
				}
				else if(proto->ucCmd == modbusREAD_MULTI_REG)
				{
					lwrb_read(&proto->tRxBuff, temp, 1);
					
					proto->ucCharLen = temp[0];
					
					memcpy(&proto->ucpFrameData[2], temp, 1);
					
					proto->ucWaitRecLen = proto->ucCharLen + 2;
				}
				else if(proto->ucCmd == modbusWRITE_SINGLE_REG)
				{
					lwrb_read(&proto->tRxBuff, temp, 2);
	
					bFunc_SwapU16Array((u8*)&proto->usRegAddr, &temp[0], 1);
					
					memcpy(&proto->ucpFrameData[2], temp, 2);
					
					proto->ucWaitRecLen = 2;
				}
				else
					proto->ucWaitRecLen = 2;
				
				b_modbus_jump_step(proto, MRS_END);
			}
			else 
				break;
		}
		
		case MRS_END: //结束
		{
			if(lwrb_get_full(&proto->tRxBuff) >= proto->ucWaitRecLen)
			{
				if(c_proto_decrypt(proto) <= 0)
					return -2;  //校验出错
				
				c_result = 1;
				b_modbus_jump_step(proto, MRS_ADDR);
			}
		}
		break;
		
		case MRS_ERR: //错误
		{
			c_result = 1;
			b_modbus_jump_step(proto, MRS_ADDR);
		}
		break;
		
		default:
			b_modbus_jump_step(proto, MRS_ADDR);
		break; 
	}
	return c_result;
}

/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_StepWaitOutTime(ModbusProtoRx_t* proto)
{
	if(proto == NULL)
		return -1;
	
	b_modbus_jump_step(proto, MRS_ADDR);
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    重置接受协议BUFF
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_ResetRxBuff(ModbusProtoRx_t* proto)
{
	if(proto == NULL)
		return -1;
	
	lwrb_reset(&proto->tRxBuff);
	b_modbus_jump_step(proto, MRS_ADDR);
	
	return 1;
}


/*****************************************************************************************************************
-----函数功能    重置发送协议BUFF
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 cModbus_ResetTx(ModbusProtoTx_t* proto, u16 len)
{
	if(proto == NULL)
		return -1;
	
	memset(&proto[1], 0, len - 1);
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    重置接受协议BUFF
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
void vModbus_RecEnd(ModbusProtoRx_t* proto)
{
	if(proto == NULL)
		return;
	
	proto->ucCharLen = 0;
	proto->usRegSize = 0;
	proto->usRegAddr = 0;
	proto->ucValidLen = 0;
}

/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
bool b_modbus_jump_step(ModbusProtoRx_t* proto, ModbusRxStep_E step)
{
	if(proto == NULL)
		return false;
	
	switch(step)
	{
		case MRS_ADDR:  //地址
		{
			proto->ucWaitRecLen = sizeof(proto->ucAddr) + sizeof(proto->ucCmd);
			proto->usRecOverTimeCnt = 0;
		}
		break;
		
		case MRS_LEN:  //指令
		{
			proto->usRecOverTimeCnt = (2000/proto->usTaskCycleTime);
		}
		break;
		
		case MRS_END: //结束
		{
			proto->usRecOverTimeCnt = (1000/proto->usTaskCycleTime);
			proto->usLostOverTimeCnt = (10000/proto->usTaskCycleTime);
		}
		break;
		
		default:
			
		break; 
	}
	
	proto->eStep = step;
	
	return true;
}


/*****************************************************************************************************************
-----函数功能    检查协议是否存在
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 c_check_cmd_exist(u8 cmd)
{
	//去除错误bit
	u8 temp_cmd = cmd & 0x7F;
	
	for(int i = 0; i < sizeof(ucaModbusCmdBuff); i++)
	{
		if(ucaModbusCmdBuff[i] == temp_cmd)
		{
			if(temp_cmd == cmd)
				return 1;
			else 
				return -1;
		}
	}
	
	return 0;
}


/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
s8 c_proto_decrypt(ModbusProtoRx_t* proto)
{
	s8 result = 1;
	u16 us_rx_crc = 0;
	u16 us_total_len = 0;  //不包含校验
	
	if(proto == NULL || proto->ucpFrameData == NULL)
		return -1;
	
	switch(proto->ucCmd)
	{
		case modbusWRITE_MULTI_REG:
		{
			us_total_len = 6;
			proto->ucValidLen = 0;
		}
		break;
		
		case modbusWRITE_SINGLE_REG:
		{
			us_total_len = 6;
			proto->ucCharLen = 2;
			
			lwrb_read(&proto->tRxBuff, &proto->ucpFrameData[4], proto->ucCharLen);
			proto->ucpValidData = &proto->ucpFrameData[4];
			proto->ucValidLen = proto->ucCharLen;
		}
		break;
		
		case modbusREAD_MULTI_REG:
		{
			us_total_len = 3 + proto->ucCharLen;
			
			lwrb_read(&proto->tRxBuff, &proto->ucpFrameData[3], proto->ucCharLen);
			proto->ucpValidData = &proto->ucpFrameData[3];
			proto->ucValidLen = proto->ucCharLen;
		}
		break;
	}
	
	//取出校验   
	lwrb_read(&proto->tRxBuff, (u8*)&us_rx_crc, 2);
	
	u8 buff[50] = {0};
	memcpy(buff, proto->ucpFrameData, us_total_len);
	
	//计算校验位
	u16 us_crc = usCheck_GetModbusCrc16(proto->ucpFrameData, us_total_len);
	
	if(us_rx_crc != us_crc)
		return -2;
	
    return result;
}


