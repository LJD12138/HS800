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
static const u8 ucaModbusCmdBuff[5] = {modbusWRITE_MULTI_REG, 
								modbusWRITE_SINGLE_REG,
								modbusREAD_MULTI_REG,
								modbusREAD_MULTI_BIT,
								modbusWRITE_SINGLE_BIT};

//****************************************************函数声明****************************************************//
static bool b_modbus_jump_step(ModbusProtoRx_t* proto, ModbusRxStep_E step);
static s8 c_check_cmd_exist(u8 cmd);
static s8 c_proto_decrypt(ModbusProtoRx_t* proto);

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
	ModbusProtoRx_t* new_proto = NULL;
	u8* frame_data = NULL;
	
	if(proto == NULL)
		return -3;
	
	if(buff_len < 6) 
		return -1;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	
//	sMyPrint("Free Heap: %u\n", xPortGetFreeHeapSize());
	
	// 动态分配内存
    size_t total_size = sizeof(ModbusProtoRx_t) + buff_len;
	#if(boardUSE_OS)
	new_proto = (ModbusProtoRx_t*)pvPortMalloc(total_size);
	if(new_proto != NULL)
		frame_data = (u8*)pvPortMalloc(buff_len);
	#else
	new_proto = (ModbusProtoRx_t*)malloc(total_size);
	if(new_proto != NULL)
		frame_data = (u8*)malloc(buff_len);
	#endif
    
	if(new_proto != NULL && frame_data != NULL)
	{
		*proto = new_proto;
		(*proto)->ucpFrameData = frame_data;
		(*proto)->usFrameDataSize = buff_len;
		(*proto)->ucAddr = dev_addr;
		(*proto)->usTaskCycleTime = cycle_time;
		(*proto)->usRecOverTimeCnt = 0;
		(*proto)->usLostOverTimeCnt = 0;
		(*proto)->ucpValidData = NULL;
		(*proto)->ucValidLen = 0;
		
		lwrb_init(&(*proto)->tRxBuff, (*proto)->ucaData, buff_len);
		lwrb_reset(&(*proto)->tRxBuff);

		b_modbus_jump_step(*proto, MRS_ADDR);
	}
	else 
	{
		#if(boardUSE_OS)
		if(frame_data != NULL)
			vPortFree(frame_data);
		if(new_proto != NULL)
			vPortFree(new_proto);
		#else
		if(frame_data != NULL)
			free(frame_data);
		if(new_proto != NULL)
			free(new_proto);
		#endif
		*proto = NULL;
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
	ModbusProtoTx_t* new_proto = NULL;
	
	if(proto == NULL)
		return -3;
	
	if(buff_len < 6) 
		return -1;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	// 动态分配内存
	size_t total_size = sizeof(ModbusProtoTx_t) + buff_len;
	#if(boardUSE_OS)
    new_proto = (ModbusProtoTx_t*)pvPortMalloc(total_size);
	#else
    new_proto = (ModbusProtoTx_t*)malloc(total_size);
	#endif
	
	if(new_proto != NULL)
	{
		*proto = new_proto;
		(*proto)->ucAddr = dev_addr;
		(*proto)->usFrameDataSize = buff_len;
		(*proto)->ucCharLen = 0;
		(*proto)->ucFrameLen = 0;
		(*proto)->usRegSize = 0;
		(*proto)->usRegAddr = 0;
		(*proto)->usRegData = 0;
		memset((*proto)->ucaFrameData, 0, buff_len);
	}
	else
	{
		*proto = NULL;
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
	u16 us_total_frame_len = 0;
	u16 us_tx_crc = 0;
	
	if(proto == NULL)
		return -2;
	
	if(c_check_cmd_exist(cmd) <= 0)
		return -3;
	
	if((cmd == modbusWRITE_MULTI_REG && len > 123) ||
	   (cmd == modbusREAD_MULTI_REG && len > 125) ||
	   (cmd == modbusREAD_MULTI_BIT && len > 2000))
		return -1;
	
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
			proto->ucCharLen = (len + 7) / 8;
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
	
	us_total_frame_len = us_total_len + 2;
	if(proto->usFrameDataSize != 0 && us_total_frame_len > proto->usFrameDataSize)
		return -5;
	
	//计算数据帧CRC码
	us_tx_crc = usCheck_CRC16(proto->ucaFrameData, us_total_len);
	
	proto->ucaFrameData[us_total_len] = us_tx_crc & 0x00ff;
	proto->ucaFrameData[us_total_len + 1] = us_tx_crc >> 8;
	
	proto->ucFrameLen = us_total_frame_len;
	
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
						proto->ucWaitRecLen = 3;
						b_modbus_jump_step(proto, MRS_ERR);
						return 0;
					}
					else if(proto->ucCmd == modbusWRITE_MULTI_REG)
						proto->ucWaitRecLen = sizeof(proto->usRegAddr) + sizeof(proto->usRegSize);
					else if(proto->ucCmd == modbusREAD_MULTI_REG || proto->ucCmd == modbusREAD_MULTI_BIT)
						proto->ucWaitRecLen = sizeof(proto->ucCharLen);
					else if(proto->ucCmd == modbusWRITE_SINGLE_REG || proto->ucCmd == modbusWRITE_SINGLE_BIT)
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
				else if(proto->ucCmd == modbusREAD_MULTI_REG || proto->ucCmd == modbusREAD_MULTI_BIT)
				{
					lwrb_read(&proto->tRxBuff, temp, 1);
					
					proto->ucCharLen = temp[0];
					if(((u16)proto->ucCharLen + 5) > proto->usFrameDataSize)
					{
						b_modbus_jump_step(proto, MRS_ADDR);
						return -5;
					}
					
					memcpy(&proto->ucpFrameData[2], temp, 1);
					
					proto->ucWaitRecLen = proto->ucCharLen + 2;
				}
				else if(proto->ucCmd == modbusWRITE_SINGLE_REG || proto->ucCmd == modbusWRITE_SINGLE_BIT)
				{
					lwrb_read(&proto->tRxBuff, temp, 2);
	
					bFunc_SwapU16Array((u8*)&proto->usRegAddr, &temp[0], 1);
					
					memcpy(&proto->ucpFrameData[2], temp, 2);
					
					proto->ucWaitRecLen = sizeof(u16) + 2;
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
				c_result = c_proto_decrypt(proto);
				if(c_result <= 0)
				{
					b_modbus_jump_step(proto, MRS_ADDR);
					return (-10 + c_result);  //校验出错
				}
				
				b_modbus_jump_step(proto, MRS_ADDR);
				return 1;
			}
		}
		break;
		
		case MRS_ERR: //错误
		{
			if(lwrb_get_full(&proto->tRxBuff) < proto->ucWaitRecLen)
				break;
			
			c_result = c_proto_decrypt(proto);
			if(c_result <= 0)
			{
				b_modbus_jump_step(proto, MRS_ADDR);
				return (-20 + c_result);
			}
			
			b_modbus_jump_step(proto, MRS_ADDR);
			return -29;
		}
		
		default:
			b_modbus_jump_step(proto, MRS_ADDR);
		break; 
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
	vModbus_RecEnd(proto);
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
	
	proto->ucCharLen = 0;
	proto->ucFrameLen = 0;
	proto->usRegSize = 0;
	proto->usRegAddr = 0;
	proto->usRegData = 0;
	memset(proto->ucaFrameData, 0, len);
	
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
	proto->ucpValidData = NULL;
}

/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    FrameInf协议的结构体
-----输出参数    none
-----返回值      小于0:操作失败   等于0:没操作    大于0:操作成功
******************************************************************************************************************/
static bool b_modbus_jump_step(ModbusProtoRx_t* proto, ModbusRxStep_E step)
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
		
		case MRS_ERR: //错误
		{
			proto->usRecOverTimeCnt = (1000/proto->usTaskCycleTime);
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
static s8 c_check_cmd_exist(u8 cmd)
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
static s8 c_proto_decrypt(ModbusProtoRx_t* proto)
{
	s8 result = 1;
	u16 us_rx_crc = 0;
	u16 us_total_len = 0;  //不包含校验
	u8 uc_cmd = 0;
	
	if(proto == NULL || proto->ucpFrameData == NULL)
		return -1;
	
	uc_cmd = proto->ucCmd & 0x7F;
	if(uc_cmd != proto->ucCmd)
	{
		us_total_len = 3;
		proto->ucCharLen = 1;
		lwrb_read(&proto->tRxBuff, &proto->ucpFrameData[2], 1);
		proto->ucpValidData = &proto->ucpFrameData[2];
		proto->ucValidLen = 1;
	}
	else switch(uc_cmd)
	{
		case modbusWRITE_MULTI_REG:
		{
			us_total_len = 6;
			proto->ucValidLen = 0;
			proto->ucpValidData = NULL;
		}
		break;
		
		case modbusWRITE_SINGLE_REG:
		case modbusWRITE_SINGLE_BIT:
		{
			us_total_len = 6;
			proto->ucCharLen = 2;
			
			lwrb_read(&proto->tRxBuff, &proto->ucpFrameData[4], proto->ucCharLen);
			proto->ucpValidData = &proto->ucpFrameData[4];
			proto->ucValidLen = proto->ucCharLen;
		}
		break;
		
		case modbusREAD_MULTI_REG:
		case modbusREAD_MULTI_BIT:
		{
			us_total_len = 3 + proto->ucCharLen;
			
			lwrb_read(&proto->tRxBuff, &proto->ucpFrameData[3], proto->ucCharLen);
			proto->ucpValidData = &proto->ucpFrameData[3];
			proto->ucValidLen = proto->ucCharLen;
		}
		break;
		
		default:
			return -2;
	}
	
	//取出校验   
	lwrb_read(&proto->tRxBuff, (u8*)&us_rx_crc, 2);
	
	//计算校验位
	u16 us_crc = usCheck_GetModbusCrc16(proto->ucpFrameData, us_total_len);
	
	if(us_rx_crc != us_crc)
		return -3;
	
    return result;
}


