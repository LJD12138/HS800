/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\Protocol\Baiku
 * File    : baiku_proto.c
 * Date    : 2026-09-22
 * Author  : LJD(291483914@qq.com)
 * Desc    : 协议解析构造
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Baiku/baiku_proto.h"
#include "check.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

//****************************************************Macros********************************************************************//
#define			protoHEAD_CODE							0xAA	//头码

//****************************************************Function Declaration******************************************************//
static bool b_baiku_jump_step(BaikuProtoRx_t* proto, BaikuRxStep_E step);
static s8 c_baiku_proto_decrypt(BaikuProtoRx_t* proto);

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
s8 cBaiku_ProtoRecInit(BaikuProtoRx_t** proto, u16 buff_len, u8 dev_addr, u16 cycle_time)
{
	s8 result = 1;
	
	if(buff_len < 6) 
		return -1;
	
	mainENTER_CRITICAL();
	
//	sMyPrint("Free Heap: %u\n", xPortGetFreeHeapSize());
	
	// 动态分配内存
    size_t total_size = sizeof(BaikuProtoRx_t) + buff_len;
	#if(boardUSE_OS)
	*proto = (BaikuProtoRx_t*)pvPortMalloc(total_size);
	if (*proto != NULL)
		(*proto)->ucpRemainData = (u8*)pvPortMalloc(buff_len);
	#else
	*proto = (BaikuProtoRx_t*)malloc(total_size);
	if (*proto != NULL)
		(*proto)->ucpRemainData = (u8*)malloc(buff_len);
	#endif  /* boardUSE_OS */
    
	
	if(*proto != NULL && (*proto)->ucpRemainData != NULL)
	{
		(*proto)->ucHead = protoHEAD_CODE;
		(*proto)->ucAddr = dev_addr;
		(*proto)->usTaskCycleTime = cycle_time;
		(*proto)->usRecOverTimeCnt = 0;
		(*proto)->usLostOverTimeCnt = 0;
		
		lwrb_init(&(*proto)->tRxBuff, (*proto)->ucaData, buff_len);
		lwrb_reset(&(*proto)->tRxBuff);

		b_baiku_jump_step(*proto, RS_HEAD);
	}
	else 
	{
		if (*proto != NULL)
		{
			#if(boardUSE_OS)
			if ((*proto)->ucpRemainData != NULL)
				vPortFree((*proto)->ucpRemainData);
			vPortFree((*proto));
			#else
			if ((*proto)->ucpRemainData != NULL)
				free((*proto)->ucpRemainData);
			free((*proto));
			#endif  /* boardUSE_OS */
		}
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
s8 cBaiku_ProtoSendInit(BaikuProtoTx_t** proto, u16 buff_len, u8 dev_addr)
{
	s8 result = 1;
	
	if(buff_len < 6) 
		return -1;
	
	mainENTER_CRITICAL();
	// 动态分配内存
	size_t total_size = sizeof(BaikuProtoTx_t) + buff_len;
	#if(boardUSE_OS)
    *proto = (BaikuProtoTx_t*)pvPortMalloc(total_size);
	#else
    *proto = (BaikuProtoTx_t*)malloc(total_size);
	#endif  /* boardUSE_OS */
	
	if(*proto == NULL)
		result =  -2;
	else
	{
		#if (boardUSE_OS)
		(*proto)->xTaskToNotify = NULL;
		(*proto)->bWaitAck = false;
		(*proto)->ucWaitCmd = 0;
		(*proto)->cAckResult = 0;
		#endif  /* boardUSE_OS */

		(*proto)->ucHead = protoHEAD_CODE;
		(*proto)->ucAddr = dev_addr;
		(*proto)->usBuffSize = buff_len;
	}
	
	mainEXIT_CRITICAL();
	
	return result;
}

/***********************************************************************************************************************
 * 函数功能    : 构造协议帧
 * 说明(备注)  : 在临界区内组帧，帧总长度(len + 6)不可超过协议缓冲区大小
 * 传入参数    : proto: 协议发送结构体; cmd: 指令号; data: 载荷数据指针; len: 载荷长度
 * 输出参数    : 无
 * 返回值      : 大于0: 操作成功; 0: 未操作; 小于0: 操作失败(-2: 参数为空; -1: 长度越界)
 ************************************************************************************************************************/
s8 cBaiku_ProtoCreate(BaikuProtoTx_t* proto,u8 cmd, u8* data, u8 len)
{
	s8 result = 1;
	
	//帧总长度(len+6)不可超过缓冲区大小
	if(proto == NULL)
		return -2;

	if((u16)len + 6U > proto->usBuffSize)
		return -1;
	
	mainENTER_CRITICAL();

	if(result > 0)
	{
		//组建数据帧
		proto->ucaFrameData[0] = proto->ucHead;
		proto->ucaFrameData[1] = proto->ucAddr;
		proto->ucaFrameData[2] = len + 3;    
		proto->ucaFrameData[3] = cmd;        //指令号,从这个开始校验    
		proto->ucaFrameData[4] = 0;          //SN
		
		if(data != NULL && len != 0)
			memcpy((u8*)&proto->ucaFrameData[5], data, len);//payload数据

		//总的长度
		proto->ucFrameLen = len + 6;
		//获取校验码
		proto->ucaFrameData[proto->ucFrameLen -1] = 
		ucCheck_SumReflect((u8*)&proto->ucaFrameData[3],proto->ucaFrameData[2] - 1);
	}

	mainEXIT_CRITICAL();
	
    return result;
}


/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cBaiku_ProtoCheck(BaikuProtoRx_t* proto)
{
	s8 c_result = 0;
	vu16 delay_cnt = 512;
	u8 temp[3] = {0};
	
	if(proto == NULL)
		return -2;
	
	if(lwrb_get_full(&proto->tRxBuff) >= proto->ucWaitRecLen)
	{
		switch(proto->eStep)
		{
			case RS_HEAD:  //帧头
			{
				lwrb_read(&proto->tRxBuff, temp, 2);
				while(1)
				{
					//匹对成功
					if(temp[0] == proto->ucHead && (temp[1] == proto->ucAddr))
					{
						lwrb_read(&proto->tRxBuff, &temp[2], 1);
						//长度错误
						if(temp[2] < 3)
						{
							b_baiku_jump_step(proto, RS_HEAD);
							return -1;
						}
												
						proto->ucWaitRecLen = temp[2];
						b_baiku_jump_step(proto, RS_LEN);
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
				if(proto->eStep == RS_HEAD)
					return -4;
			}

			case RS_LEN: //长度
			{
				if(lwrb_get_full(&proto->tRxBuff) >= proto->ucWaitRecLen)
				{
					//取出长度
					proto->ucRemainLen = proto->ucWaitRecLen;
					
					//********************************************开始累加和校验**********************************************
					if(c_baiku_proto_decrypt(proto) <= 0)
					{
						b_baiku_jump_step(proto, RS_HEAD);  //校验出错，立即重置状态机为搜索新帧头，消除失步盲区
						return -2;
					}
					
					b_baiku_jump_step(proto, RS_END);
				}
				else 
					break;
			}
			
			case RS_END: //结束
			{
				c_result = 1;
				b_baiku_jump_step(proto, RS_HEAD);
			}
			break;
			
			default:
			{
				b_baiku_jump_step(proto, RS_HEAD);
			}
			break; 
		}
	}
	return c_result;
}


/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cBaiku_UpdateCheck(BaikuProtoRx_t* proto, u8* ucp_data, u16 len)
{
	s8 c_result = 0;
	int i = 0;

	if(proto == NULL)
		return -1;
	
	if(ucp_data == NULL || len < 6)
		return -2;
	
	for(; i <= len - 3; i++)
	{
		// 检查当前元素及其后三个元素是否与目标数组匹配
		if(ucp_data[i] == proto->ucHead
		  	&& (ucp_data[i + 1] == proto->ucAddr)
			&& ucp_data[i + 2] <= len)
		{
			proto->ucHead = ucp_data [i];
			proto->ucAddr = ucp_data [i + 1];
			proto->ucRemainLen = ucp_data [i + 2];
			
			//剩余的长度不够
			if(((len - i) - 3) < proto->ucRemainLen)
				return -3;
			
			c_result = 1;
			break;
		}
	}
	
	if(c_result == 0)
		return -4;

	//累加和计算  从cmd开始到payload  去除checksum
	vu8 ChkSum = ucCheck_SumReflect(&ucp_data[i + 3], (proto->ucRemainLen - 1));
	//累加和校验
	if(ucp_data[i + proto->ucRemainLen + 2] != ChkSum)
		return -5; 
	
	memcpy(proto->ucpRemainData, &ucp_data[i + 3], proto->ucRemainLen);

	//取出指令cmd
	proto->ucCmd = proto->ucpRemainData[0];          //CMD
	//取出数据帧序号
	proto->ucSN  = proto->ucpRemainData[1];          //SN
	//取出有效数据长度
	proto->ucValidLen = proto->ucRemainLen - 3; 
	//取出有效数据部分
	if(proto->ucValidLen)
		proto->ucpValidData = (u8*)&proto->ucpRemainData[2]; 
	else
		proto->ucpValidData = NULL;
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cBaiku_StepWaitOutTime(BaikuProtoRx_t* proto)
{
	if(proto == NULL)
		return -1;
	
	b_baiku_jump_step(proto, RS_HEAD);
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 重置接受协议BUFF
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cBaiku_ResetRxBuff(BaikuProtoRx_t* proto)
{
	if(proto == NULL)
		return -1;
	
	lwrb_reset(&proto->tRxBuff);
	b_baiku_jump_step(proto, RS_HEAD);
	
	return 1;
}


/***********************************************************************************************************************
 * 函数功能    : 重置发送协议BUFF
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
s8 cProto_ResetTxBuff(BaikuProtoTx_t* proto)
{
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 唤醒等待 BMS 应答的发送任务
 * 说明(备注)  : 仅当收到的回复命令字与期望匹配时才唤醒
 * 传入参数    : uc_cmd: 接收到的应答命令字, c_result: 解析处理结果
 * 输出参数    : 无
 * 返回值      : 小于0:校验失败   等于0:未回复    大于0:回复成功
 ************************************************************************************************************************/
#if (boardUSE_OS)
s8 cBaiku_NotifyAck(BaikuProtoTx_t* proto, u8 uc_cmd, s8 c_result)
{
    TaskHandle_t xToNotify = NULL;

    if (proto == NULL)
        return -2;

    /* 状态转换原子化: 与 WaitReply 超时清理互斥, 挡住抢占交错 */
    mainENTER_CRITICAL();
    if (proto->bWaitAck)
    {
        if (uc_cmd != proto->ucWaitCmd)     /* 去掉 cmd == 0 通配 */
        {
            mainEXIT_CRITICAL();
            return -1;
        }
        proto->cAckResult     = c_result;
        proto->bWaitAck       = false;
        xToNotify             = proto->xTaskToNotify;
        proto->xTaskToNotify  = NULL;
    }
    mainEXIT_CRITICAL();

    if (xToNotify != NULL)
    {
        xTaskNotifyGive(xToNotify);          /* 临界区外 give, 迟到的通知会被下次 drain 吃掉 */
        return 1;
    }
    return 0;
}


/***********************************************************************************************************************
 * 函数功能    : 等待回复
 * 说明(备注)  : 仅当收到的回复命令字与期望匹配时才唤醒
 * 传入参数    : proto: 协议发送结构体, uc_cmd: 发送的请求命令字, timeout_ms: 超时时间(ms)
 * 输出参数    : 无
 * 返回值      : 小于0:校验失败/超时   等于0:未回复    大于0:回复成功
 ************************************************************************************************************************/
s8 cBaiku_WaitReply(BaikuProtoTx_t* proto, u8 uc_cmd, u16 timeout_ms)
{
    if (proto == NULL)
        return -2;

    s8 result = 0;

    /* 仅在真正发起等待时激活事务状态机并绑定当前任务 */
    proto->xTaskToNotify = xTaskGetCurrentTaskHandle();
    proto->ucWaitCmd     = (uc_cmd == baikuCMD_GET_PARAM) ? baikuCMD_REPLY_PARAM : (uc_cmd + 1);
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
		/* 应答成功*/
		if (proto->cAckResult > 0)
			result = 1;
		
		/* 应答帧校验失败*/
		else if (proto->cAckResult < 0)
			result = -2;
		
		/* 应答失败*/
		else
			result = -3;
	}

	proto->bWaitAck      = false;
	proto->xTaskToNotify = NULL;
	proto->cAckResult    = 0;
	proto->ucWaitCmd     = 0;
    return result;
}
#endif  /* boardUSE_OS */


/***********************************************************************************************************************
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
static bool b_baiku_jump_step(BaikuProtoRx_t* proto, BaikuRxStep_E step)
{
	if(proto == NULL)
		return false;
	
	switch(step)
	{
		case RS_HEAD:  //帧头
		{
			proto->ucWaitRecLen = sizeof(proto->ucHead) + sizeof(proto->ucAddr);
			proto->usRecOverTimeCnt = 0;
		}
		break;
		
		case RS_LEN:  //长度
		{
			proto->usRecOverTimeCnt = (2000/proto->usTaskCycleTime);
		}
		break;
		
		case RS_END: //结束
		{
			proto->usRecOverTimeCnt = (1000/proto->usTaskCycleTime);
			proto->usLostOverTimeCnt = (10000/proto->usTaskCycleTime);
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
 * 函数功能    : 解析协议
 * 说明(备注)  : none
 * 传入参数    : FrameInf协议的结构体
 * 输出参数    : none
 * 返回值      : 小于0:操作失败   等于0:没操作    大于0:操作成功
 ************************************************************************************************************************/
static s8 c_baiku_proto_decrypt(BaikuProtoRx_t* proto)
{
	s8 result = 1;
	
	if(proto == NULL || proto->ucpRemainData == NULL)
		return -2;

	//最短的指令为明文无Payload Data，6字节
	//Len后最短长度为3
    if(proto->ucRemainLen < 3)   
        return -1;
	
	mainENTER_CRITICAL();

	if(result > 0)
	{
		//取出剩余的帧数据   
		lwrb_read(&proto->tRxBuff, proto->ucpRemainData, proto->ucRemainLen);
		//累加和计算  从cmd开始到payload  去除checksum
		vu8 ChkSum = ucCheck_SumReflect(proto->ucpRemainData, (proto->ucRemainLen - 1));
		//累加和校验
		if(proto->ucpRemainData[proto->ucRemainLen - 1] != ChkSum)
			result = -3;      
		
		if(result > 0)
		{
			//取出指令cmd
			proto->ucCmd = proto->ucpRemainData[0];          //CMD
			//取出数据帧序号
			proto->ucSN  = proto->ucpRemainData[1];          //SN
			//取出有效数据长度
			proto->ucValidLen = proto->ucRemainLen - 3; 
			//取出有效数据部分
			if(proto->ucValidLen)
				proto->ucpValidData = (u8*)&proto->ucpRemainData[2]; 
			else
				proto->ucpValidData = NULL;
		}
	}

	mainEXIT_CRITICAL();
    
    return result;
}

