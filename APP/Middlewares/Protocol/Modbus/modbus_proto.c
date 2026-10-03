/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\Protocol\Modbus
 * File    : modbus_proto.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 协议解析构造
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Modbus/modbus_proto.h"
#include "check.h"
#include "function.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

//****************************************************Macros********************************************************************//


//****************************************************Parameter Initialization**************************************************//
static const u8 ucaModbusCmdBuff[5] = {modbusWRITE_MULTI_REG, 
								modbusWRITE_SINGLE_REG,
								modbusREAD_MULTI_REG,
								modbusREAD_MULTI_BIT,
								modbusWRITE_SINGLE_BIT};

//****************************************************Function Declaration******************************************************//
static bool b_modbus_jump_step(ModbusProtoRx_t* proto, ModbusRxStep_E step);
static s8 c_check_cmd_exist(u8 cmd);
static s8 c_proto_decrypt(ModbusProtoRx_t* proto);

/***********************************************************************************************************************
 * 函数功能    : 接受协议初始化
 * 说明(备注)  : none
 * 传入参数    : proto:接受协议结构体
 *               buff_len::协议缓存器大小
 *               dev_addr:设备地址
 *               cycle_time:协议循环时基
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cModbus_RecProtoInit(ModbusProtoRx_t** proto, u16 buff_len, u8 dev_addr, u16 cycle_time)
{
	s8 result = 1;
	ModbusProtoRx_t* new_proto = NULL;
	u8* frame_data = NULL;
	
	if(proto == NULL)
		return -3;
	
	if(buff_len < 6) 
		return -1;
	
	mainENTER_CRITICAL();
	
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
	#endif  /* boardUSE_OS */
    
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
		#endif  /* boardUSE_OS */
		*proto = NULL;
		result =  -2;
	}
	
	mainEXIT_CRITICAL();
	
	return result;
}

/***********************************************************************************************************************
 * 函数功能    : 发送协议初始化
 * 说明(备注)  : none
 * 传入参数    : proto:接受协议结构体
 *               buff_len::协议缓存器大小
 *               dev_addr:设备地址
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cModbus_TransProtoInit(ModbusProtoTx_t** proto, u16 buff_len, u8 dev_addr)
{
	s8 result = 1;
	ModbusProtoTx_t* new_proto = NULL;
	
	if(proto == NULL)
		return -3;
	
	if(buff_len < 6) 
		return -1;
	
	mainENTER_CRITICAL();
	// 动态分配内存
	size_t total_size = sizeof(ModbusProtoTx_t) + buff_len;
	#if(boardUSE_OS)
    new_proto = (ModbusProtoTx_t*)pvPortMalloc(total_size);
	#else
    new_proto = (ModbusProtoTx_t*)malloc(total_size);
	#endif  /* boardUSE_OS */
	
	if(new_proto != NULL)
	{
		*proto = new_proto;
		#if (boardUSE_OS)
		(*proto)->xTaskToNotify = NULL;
		(*proto)->bWaitAck      = false;
		(*proto)->ucWaitCmd     = 0;
		(*proto)->usWaitRegAddr = 0;
		(*proto)->cAckResult    = 0;
		#endif  /* boardUSE_OS */
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

	mainEXIT_CRITICAL();
	
	return result;
}

/***********************************************************************************************************************
 * 函数功能    : 构造协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 *               [0]:Header
 *               [1]:Addr
 *               [2]:Len = Cmd~CheckSum;
 *               [3]:Cmd
 *               [4]:SN
 *               [5]:data
 *               [5+n]:payload data
 *               [6+n]:CheckSum
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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


/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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
			lwrb_sz_t full_len = lwrb_get_full(&proto->tRxBuff);
			lwrb_sz_t skip_bytes = 0;
			bool b_found = false;

			if(full_len < proto->ucWaitRecLen)
				return 0;

			while((full_len - skip_bytes) >= 2)
			{
				/* 非破坏性查看当前偏移处的2字节 */
				if(lwrb_peek(&proto->tRxBuff, skip_bytes, temp, 2) != 2)
					break;

				c_result = c_check_cmd_exist(temp[1]);

				//匹对成功 (匹配从机地址且命令合法)
				if(temp[0] == proto->ucAddr && c_result != 0)
				{
					/* 先跳过帧头之前的噪声数据 */
					if(skip_bytes > 0)
						lwrb_skip(&proto->tRxBuff, skip_bytes);
					/* 消费并取出帧头2字节 */
					lwrb_read(&proto->tRxBuff, temp, 2);

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
					b_found = true;
					break;
				}
				else
				{
					/* 滑动1字节继续搜索 */
					skip_bytes++;
					delay_cnt--;
					if(delay_cnt == 0)
					{
						lwrb_skip(&proto->tRxBuff, skip_bytes);
						return -3;
					}
				}
			}

			if(!b_found)
			{
				/* 未找到帧头：检查最后1字节是否可能是目标地址 */
				if((full_len - skip_bytes) == 1)
				{
					u8 last_byte = 0;
					lwrb_peek(&proto->tRxBuff, skip_bytes, &last_byte, 1);
					if(last_byte == proto->ucAddr)
					{
						/* 保留这个潜在的地址字节，丢弃之前的垃圾数据，等待下一字节接收 */
						if(skip_bytes > 0)
							lwrb_skip(&proto->tRxBuff, skip_bytes);
						return 0;
					}
					else
						skip_bytes++;
				}
				if(skip_bytes > 0)
					lwrb_skip(&proto->tRxBuff, skip_bytes);
				return -4;
			}
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
		{
			b_modbus_jump_step(proto, MRS_ADDR);
		}
		break; 
	}
	return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cModbus_StepWaitOutTime(ModbusProtoRx_t* proto)
{
	if(proto == NULL)
		return -1;
	
	b_modbus_jump_step(proto, MRS_ADDR);
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 重置接受协议BUFF
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cModbus_ResetRxBuff(ModbusProtoRx_t* proto)
{
	if(proto == NULL)
		return -1;
	
	lwrb_reset(&proto->tRxBuff);
	vModbus_RecEnd(proto);
	b_modbus_jump_step(proto, MRS_ADDR);
	
	return 1;
}


/***********************************************************************************************************************
 * 函数功能    : 重置发送协议BUFF
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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

/***********************************************************************************************************************
 * 函数功能    : 重置接受协议BUFF
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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

/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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
		{
			
		}
		break; 
	}
	
	proto->eStep = step;
	
	return true;
}


/***********************************************************************************************************************
 * 函数功能    : 检查协议是否存在
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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


/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
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

/***********************************************************************************************************************
 * 函数功能    : 校验接收到的 Modbus 回包是否与当前发送事务匹配
 * 说明(备注)  : 用于校验迟到回复、命令字/读取字节数/写入寄存器与数据回显等协议一致性
 * 传入参数    : proto_tx: 发送协议结构体, proto_rx: 接收协议结构体
 * 输出参数    : 无
 * 返回值      : 1: 匹配且校验通过; 0: 未在等待应答; 负数: 校验失败(如迟到回包/字段不匹配)
 ************************************************************************************************************************/
s8 cModbus_CheckReply(const ModbusProtoTx_t* proto_tx, const ModbusProtoRx_t* proto_rx)
{
    if (proto_tx == NULL || proto_rx == NULL)
        return -1;

	#if (boardUSE_OS)
    /* 必须处于等待应答状态 */
    if (!proto_tx->bWaitAck)
        return 0;

    /* 校验命令字是否一致 */
    if (proto_tx->ucWaitCmd != 0 && proto_rx->ucCmd != proto_tx->ucWaitCmd)
        return -2;
	#else
    if (proto_tx->ucCmd != 0 && proto_rx->ucCmd != proto_tx->ucCmd)
        return -2;
	#endif  /* boardUSE_OS */

    /* 0x02/0x03 读多位/读多个寄存器：校验接收到的字节数与请求期望读取的字节数是否一致(防迟到串包) */
    if (proto_rx->ucCmd == modbusREAD_MULTI_REG || proto_rx->ucCmd == modbusREAD_MULTI_BIT)
    {
        if (proto_rx->ucCharLen != proto_tx->ucCharLen)
            return -3;
        if (proto_rx->ucValidLen != proto_rx->ucCharLen || proto_rx->ucpValidData == NULL)
            return -4;
    }
    /* 0x10 写多个寄存器：校验回显的寄存器起始地址与数量 */
    else if (proto_rx->ucCmd == modbusWRITE_MULTI_REG)
    {
        if (proto_rx->usRegAddr != proto_tx->usRegAddr || proto_rx->usRegSize != proto_tx->usRegSize)
            return -5;
    }
    /* 0x06 写单个寄存器 / 0x05 写单线圈：校验回显的寄存器地址与写入的数据值 */
    else if (proto_rx->ucCmd == modbusWRITE_SINGLE_REG || proto_rx->ucCmd == modbusWRITE_SINGLE_BIT)
    {
        if (proto_rx->usRegAddr != proto_tx->usRegAddr)
            return -6;
        if (proto_rx->ucValidLen != 2 || proto_rx->ucpValidData == NULL)
            return -7;

        u16 us_reg_data = 0;
        bFunc_SwapU16Array((u8 *)&us_reg_data, proto_rx->ucpValidData, 1);
        if (us_reg_data != proto_tx->usRegData)
            return -8;
    }

    return 1;
}

#if (boardUSE_OS)
/***********************************************************************************************************************
 * 函数功能    : 唤醒等待 Modbus 应答的发送任务
 * 说明(备注)  : 记录应答结果并唤醒等待任务
 * 传入参数    : proto: 发送协议结构体, c_result: 解析处理结果
 * 输出参数    : 无
 * 返回值      : 小于0:操作失败   等于0:未在等待应答    大于0:唤醒成功
 ************************************************************************************************************************/
s8 cModbus_NotifyAck(ModbusProtoTx_t* proto, s8 c_result)
{
    TaskHandle_t xToNotify = NULL;

    if (proto == NULL)
        return -2;

    mainENTER_CRITICAL();
    if (proto->bWaitAck)
    {
        proto->cAckResult    = c_result;
        proto->bWaitAck      = false;
        xToNotify            = proto->xTaskToNotify;
        proto->xTaskToNotify = NULL;
    }
    mainEXIT_CRITICAL();

    if (xToNotify != NULL)
    {
        xTaskNotifyGive(xToNotify);
        return 1;
    }

    return 0;
}

/***********************************************************************************************************************
 * 函数功能    : 等待回复
 * 说明(备注)  : 仅当收到的回复命令字与寄存器匹配时才唤醒
 * 传入参数    : proto: 发送协议结构体, uc_cmd: 发送的请求命令字, us_reg_addr: 期望寄存器地址, timeout_ms: 超时时间(ms)
 * 输出参数    : 无
 * 返回值      : 小于0:校验失败/超时   等于0:未回复    大于0:回复成功
 ************************************************************************************************************************/
s8 cModbus_WaitReply(ModbusProtoTx_t* proto, u8 uc_cmd, u16 us_reg_addr, u16 timeout_ms)
{
    if (proto == NULL)
        return -2;

    s8 result = 0;

    /* 仅在真正发起等待时激活事务状态机并绑定当前任务 */
    proto->xTaskToNotify = xTaskGetCurrentTaskHandle();
    proto->ucWaitCmd     = uc_cmd;
    proto->usWaitRegAddr = us_reg_addr;
    proto->cAckResult    = 0;
    proto->bWaitAck      = true;

    TickType_t xStartTick = xTaskGetTickCount();
    const TickType_t xWaitTicks = pdMS_TO_TICKS(timeout_ms);

    /* 循环等待：只有收到匹配的应答(bWaitAck清除)或真正超时才退出，免疫任务队列调度通知等外部干扰 */
    while (proto->bWaitAck)
    {
        TickType_t xElapsed = xTaskGetTickCount() - xStartTick;
        if (xElapsed >= xWaitTicks)
            break;

        ulTaskNotifyTake(pdTRUE, xWaitTicks - xElapsed);
    }

    /* 等待回复超时 */
    if (proto->bWaitAck)
        result = -1;
    else 
    {
        /* 应答成功 */
        if (proto->cAckResult > 0)
            result = 1;
        /* 应答帧校验失败 */
        else if (proto->cAckResult < 0)
            result = -2;
        /* 应答失败 */
        else
            result = -3;
    }

    proto->bWaitAck      = false;
    proto->xTaskToNotify = NULL;
    proto->cAckResult    = 0;
    proto->ucWaitCmd     = 0;
    proto->usWaitRegAddr = 0;
    return result;
}
#endif  /* boardUSE_OS */


