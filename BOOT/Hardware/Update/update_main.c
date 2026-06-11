#include "Update/update_main.h"

#if(boardUPDATE)
#include "Flash/flash_iface.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task.h"
#include "Sys/sys_queue_task_update.h"

#include "boot_info.h"



//****************************************************函数声明****************************************************//
static s8 c_frame_trans_cd(u8 cmd, u8 *buf, u16 len);
static s8 c_frame_rec_cd(u8 *buf, u16 buf_len, u16 *len);
static void v_proc_check_ok_rec_data_cd(u8 *buf, u16 len);
static void v_rec_start_cd(void);
static void v_rec_end_cd(u8 code);

/***********************************************************************************************************************
-----函数功能    Xmodem协议结构体对象初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
Xmodem_T tXmodem={
	.bStartSendFrm 				= true,               	//可以发送数据
	.frm_cnt 					= 1,                  	//帧计数 指向下一帧 从1开始
	.usRecLen 					= 0,					//接受长度
	.usFrmOvertimeCnt 			= 0,					//计数清零
	.usWaitStartOutTimeCnt 		= XMODEM_START_TIMEOUT_MS,//计数清零
	.usWaitExitOutTimeCnt 		= XMODEM_END_TIMEOUT_MS,//计数清零
    .eFrameLen 					= FRAME_LEN_128,   		//128的长度
    .eCheckMode 				= CHECK_MODE_ADD,  		//累加和
    .eState 					= XMODEM_STATE_IDLE,  	//工作状态为空
	.eRecState 					= REC_STATE_IDLE,
	.buf 						= {0},                	//缓冲区
    .c_xmodem_trans_data 		= c_frame_trans_cd,         
    .c_xmodem_rec_data 			= c_frame_rec_cd,        
    .v_proc_check_ok_rec_data 	= v_proc_check_ok_rec_data_cd,   
	.v_rec_start 				= v_rec_start_cd,
	.v_rec_end 					= v_rec_end_cd,
};

/***********************************************************************************************************************
-----函数功能    Baiku私有协议结构体对象初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
BaiKuProto_T tBaiKuProto={
	.bStartSendFrm 				= true,               	//可以发送数据
	.ucFrmCnt 					= 1,                  	//帧计数 指向下一帧 从1开始
	.usFrmOvertimeCnt 			= 0,					//计数清零
	.usWaitStartOutTimeCnt 		= BAIKU_START_TIMEOUT_MS,//计数清零
	.usWaitExitOutTimeCnt 		= BAIKU_END_TIMEOUT_MS,//计数清零
    .eState 					= BAIKU_STATE_IDLE,  	//工作状态为空
    .c_xmodem_trans_data 		= c_frame_trans_cd,         
    .c_xmodem_rec_data 			= c_frame_rec_cd,        
    .v_proc_check_ok_rec_data 	= v_proc_check_ok_rec_data_cd,   
	.v_rec_start 				= v_rec_start_cd,
	.v_rec_end 					= v_rec_end_cd,
};


/***********************************************************************************************************************
-----函数功能    数据帧发送回调函数
-----说明(备注)  协议生成需要发送的数据,就会调用此函数把数据发送出去
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static s8 c_frame_trans_cd(u8 cmd, u8 *buf, u16 len)
{
	s8 c_result = 1;
	
	switch(tUpdate.eChType)
	{
		#if(boardCONSOLE_EN)
		case CT_CONSOLE:
		{
			switch(tUpdate.eProtoType)
			{
				case PT_XMODEM:
				{
					bConsole_DataSendStart(&cmd, 1);
				}
				break;
				
				case PT_BAIKU:
				{
					if(cmd == 0)
						return -1;
					
					c_result = cBaiku_ProtoCreate(tUpdate.tpProtoTx, cmd, buf, len);
					if(c_result > 0)
					{
						if(bConsole_DataSendStart(tUpdate.tpProtoTx->ucaFrameData, tUpdate.tpProtoTx->ucFrameLen) == true)
							return true;
						
						c_result = 0;
					}
					
				}
				break;
				
				default:
					break;
			}
		}
		break;
		#endif
		
		#if(boardPRINT_IFACE)
		case CT_PRINT:
		{
			switch(tUpdate.eProtoType)
			{
				case PT_XMODEM:
				{
					lwrb_write(tUpdate.pTxBuff, &cmd, 1);
					if(len)
						lwrb_write(tUpdate.pTxBuff, buf, len);
					bPrint_SendDataToUsart();
				}
				break;
				
				case PT_BAIKU:
				{
					if(cmd == 0)
						return -1;
					
					c_result = cBaiku_ProtoCreate(tUpdate.tpProtoTx, cmd, buf, len);
					if(c_result > 0)
					{
						lwrb_write(tUpdate.pTxBuff, tUpdate.tpProtoTx->ucaFrameData, tUpdate.tpProtoTx->ucFrameLen);
						bPrint_SendDataToUsart();
						return true;
					}
				}
				break;
				
				default:
					break;
			}
		}
		break;
		#endif
		
		#if(boardWIFI_USARTX)
		case CT_WIFI:
		{
			memcpy(ucaWiFiTxDmaBuffData, buf, len);
			bWIFI_DataSendStart(len);
		}
		break;
		#endif
		
		default:
			log_i("当前通道%d未开启,请重新选择通道",tUpdate.eChType);
			c_result = 0;
			break;
	}
	
	return c_result;
}


/***********************************************************************************************************************
-----函数功能    检测接收到的数据
-----说明(备注)  协议通过不断检测此函数,判断是否有数据接收到,把接收到的数据读取出来
-----传入参数    
				buf:缓冲区
				buf_len:需要接收的长度
				len:实际接受的长度
-----输出参数    none
-----返回值     -1:超时  0等待  1:接受完成  2:传输完成
************************************************************************************************************************/
static s8 c_frame_rec_cd(u8 *buf, u16 buf_len, u16 *len)
{
	//Xmodem
	vu16 rx_len = 0;
	vu16 rx_len_temp = 0;

	//Baiku
	s8 c_result = 0;
	
	switch(tUpdate.eProtoType)
	{
		case PT_XMODEM:
		{
			 while(rx_len < buf_len)
			 {
				if(lwrb_get_full(tUpdate.pRxBuff))  //收到数据
				{
					rx_len_temp = lwrb_get_full(tUpdate.pRxBuff) ;  //获取可以读取的长度
					
					if( (rx_len_temp + rx_len) >  buf_len )  //超出要求接收的长度
						rx_len_temp = buf_len - rx_len;
					
					lwrb_read(tUpdate.pRxBuff , &buf[rx_len], rx_len_temp );   //读取数据
					rx_len += rx_len_temp ;  //记录当前获取数据的长度
				}
				
				//若收到EOT则直接返回接收完成
				if((XMODEM_FRM_FLAG_EOT == buf[0]) && (1 == rx_len_temp))
				{
					*len=rx_len;
					if(uPrint.tFlag.bUpdate)
						sMyPrint("bUpdate:----接收到符号EOT,接受结束----\n\r");
					return 2;
				}
				
				if(tXmodem.eState != XMODEM_STATE_RECEIVING)
				{
					bool b_ret = false;
					if(tUpdate.eChType == CT_PRINT)
					{
						#if(boardPRINT_IFACE)
						if(cBaiku_UpdateCheck(tUpdate.tpProtoRx, buf, rx_len) > 0)
						{
							if(tUpdate.tpProtoRx->ucCmd == baikuCMD_SET_PROTO 
								&& tUpdate.tpProtoRx->ucpValidData != NULL
								&& tUpdate.tpProtoRx->ucValidLen == 3)
							{
								if(tUpdate.tpProtoRx->ucpValidData[0] == PT_BAIKU)
								{
									memcpy((u16*)&tUpdate.usTotalFrmValue, &tUpdate.tpProtoRx->ucpValidData[1], 2);
									b_ret = true;
								}
							}
						}
						#else
						log_w("当前通道%d未开启,请重新选择通道",tUpdate.eChType);
						#endif
					}
					else if(tUpdate.eChType == CT_CONSOLE)
					{
						#if(boardCONSOLE_EN)
						b_ret = bConsole_DataProc(buf, rx_len);
						#else
						log_w("当前通道%d未开启,请重新选择通道",tUpdate.eChType);
						#endif
					}
					else
					{
						if(uPrint.tFlag.bUpdate)
							log_w("bUpdate:通道%d未定义",tUpdate.eChType);
						return -1;
					}
					
					//切换协议
					if(b_ret == true)
					{
						u8 temp[4] = {0};
						temp[0] = PT_BAIKU;
						if(cUpdate_ProtoSelect((ProtoType_E)temp[0]) == true)
						{
							memcpy(&temp[1], (u8*)&tUpdate.usTotalFrmValue, 2);
							c_frame_trans_cd(baikuCMD_REPLY_SET_PROTO, temp, 3);
							tBaiKuProto.bStartSendFrm = true;
							return 99;
						}
					}
					
				}

				//等待超时
				if(tXmodem.usFrmOvertimeCnt == 0)
				{
					if(uPrint.tFlag.bUpdate)
						log_w("bUpdate:接受等待超时");
					return -1;
				}
			 }
			 *len=rx_len;
		}
		break;
		
		case PT_BAIKU:
		{
			c_result = cBaiku_ProtoCheck(tUpdate.tpProtoRx);
			if(c_result > 0)
			{
				switch(tUpdate.tpProtoRx->ucCmd)
				{
					case baikuCMD_SET_PROTO://回复设置协议
					{
						memcpy((u16*)&tUpdate.usTotalFrmValue, &tUpdate.tpProtoRx->ucpValidData[1], 2);
						
						if(tUpdate.tpProtoRx->ucpValidData[0] == PT_BAIKU)
						{
							u8 temp[4] = {0};

							temp[0] = PT_BAIKU;
							memcpy(&temp[1], (u8*)&tUpdate.usTotalFrmValue, 2);
							c_frame_trans_cd(baikuCMD_REPLY_SET_PROTO, temp, 3);
							
							if(uPrint.tFlag.bUpdate)
								sMyPrint("bUpdate:设置升级协议为%d \n\r", PT_BAIKU);
							
							return 0;
						}
					}break;
				}
			}
			
			if(c_result < 0)
			{
				if(uPrint.tFlag.bUpdate)
					log_w("bUpdate:数据解析错误,代码%d", c_result);
			}
			
			return c_result;
		}
		
		default:
			break;
	}
	return 1;
}


/***********************************************************************************************************************
-----函数功能    处理校验完成的接收数据
-----说明(备注)  
				XMODEM数据回调函数类型，XMODEM调用此函数向上层通知接收到数据
				回调函数通过以下参数告知当前数据状态：
				buf为空     len为0------>表示传输故障，所有传输数据无效
				buf不为空   len不为0----->表示当前数据有效
				buf不为空   len为0------>表示当前传输完成

-----传入参数    buf:接收到的有效数据缓存BUFF   
				 len:有效数据的长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_proc_check_ok_rec_data_cd(u8 *buf, u16 len)
{
	//不为非空
	if(!buf)
		return;
	
	tUpdate.usRecFrameCnt++;
	
	//写入APP的Flash
	bFlash_WriteDataToFlash(buf, len);
	
	if(uPrint.tFlag.bUpdate)
	{
		u16 pos=0;
		while(pos<len)
		{
			for(u8 i=0;i<16;i++)
			{
				if(i==15)
					sMyPrint("%x",buf[pos++]);
				else
					sMyPrint("%x,",buf[pos++]);
				if(pos>=len)
				{
					sMyPrint("\n\r");
					return;
				}
			}
			sMyPrint("\n\r");
		}
	}
}

/*****************************************************************************************************************
-----函数功能    接收开始
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
static void v_rec_start_cd(void)
{
	tUpdate.usRecFrameCnt = 0;
	vFlash_WriteDataToFlashInit();  // 初始化Flash
	cBoot_CtrlUpdate(true, AS_ERASE);
}

/*****************************************************************************************************************
-----函数功能    接收结束
-----说明(备注)  none
-----传入参数    code:0无错误,传输完成  ,1:升级错误   2:还没开始,等待升级超时
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
static void v_rec_end_cd(u8 code)
{
	//有错误,停止接收
    if(code == 1)
    {
		tUpdate.usRecFrameCnt = 0;
		
		if(uPrint.tFlag.bUpdate)
			log_e("bUpdate:因错误导致中断升级");
		return;
    }
	else if(code == 2)
    {
		/* APP栈顶指针合法 */
		if (0x20000000 == ((*(__IO u32*)flashAPP_START) & 0x2FFE0000) && tBootMemParam.tParam.eAppState == AS_OK)
		{
			cBoot_CtrlUpdate(false, AS_OK);
		}
		
		if(uPrint.tFlag.bUpdate)
			log_w("bUpdate:升级等待超时!");
		return;
    }
	else   //升级成功
	{
		if(uPrint.tFlag.bUpdate) 
		{
			sMyPrint("bUpdate:----升级完成!----\n\r");
			sMyPrint("bUpdate:栈顶地址 = %x \r\n",((*(__IO u32*)flashAPP_START) & 0x2FFE0000));
			sMyPrint("bUpdate:APP地址 = %x!\r\n",flashAPP_START);
		}
		
		/* APP栈顶指针合法 */
		if (0x20000000 == ((*(__IO u32*)flashAPP_START) & 0x2FFE0000))
		{
			cBoot_CtrlUpdate(false, AS_FINISH);
		}
		return;
	}
	
}
#endif //boardUPDATE
