#include "Update/xmodem_proto.h"

#if(boardUPDATE)
#include "Print/print_task.h"

#include "boot_info.h"
#include "check.h"

#ifdef __cplusplus
extern "C" {
#endif



//处理结果
typedef enum
{
    REC_RESULT_ERR,                  //接收数据错误
    REC_RESULT_OK,                   //接收数据成功
    REC_RESULT_FAULT,                //接收过程故障
    REC_RESULT_FINISH                //接收数据完成
}RecResult_E;

/*****************************************************************************************************************
-----函数功能    获取定义的接收帧的长度
-----说明(备注)  bXmodem对象
-----传入参数    buf:校验数据的指针  len:数据字节长度   chk_val:校验值
-----输出参数    none
-----返回值      true:校验通过   反之失败
*****************************************************************************************************************/
static bool chk_add8(u8 *buf, u16 len, u8 chk_val)
{
    u8 result=0;
	
    for(u16 i=0;i<len;i++)
        result+=buf[i];
	
    if(chk_val==result)
        return true;
	else
	{
		if(uPrint.tFlag.bXmodem)
			sMyPrint("校验错误:chk_val1=%d,cal_val2=%d\n\r",chk_val,result);
		return false;
	}
}

/*****************************************************************************************************************
-----函数功能    获取定义的接收帧的长度
-----说明(备注)  bXmodem对象
-----传入参数    buf:校验数据的指针  len:数据字节长度   chk_val:校验值
-----输出参数    none
-----返回值      true:校验通过   反之失败
*****************************************************************************************************************/
static bool chk_crc16(u8 *buf, u16 len, u16 chk_val)
{
	if(chk_val==usCheck_GetCrc16Tab(buf,len))
		return true;
    else 
		return false;
}

/*****************************************************************************************************************
-----函数功能    解析协议
-----说明(备注)  none
-----传入参数    XMODEM对象
-----输出参数    none
-----返回值      
				REC_RESULT_ERR表示数据错误
				REC_RESULT_OK表示数据成功
				REC_RESULT_FAULT表示接收故障
				REC_RESULT_FINISH表示接收完成
*****************************************************************************************************************/
static RecResult_E e_rec_proc(Xmodem_T *obj)
{
    if((!obj))
    {
		if(uPrint.tFlag.bXmodem)
			log_e("bXmodem:解析协议obj对象非法");
        return REC_RESULT_FAULT;
    }
    
    if(0 == obj->usRecLen)
    {
		if(uPrint.tFlag.bXmodem)
			log_e("bXmodem:解析协议的长度为0");
        return REC_RESULT_ERR;
    }
	
    //根据校验模式调整帧长度
    u16 frm_len=(CHECK_MODE_ADD==obj->eCheckMode)?XMODEM_FRM_FLAG_LEN_ADD:XMODEM_FRM_FLAG_LEN_CRC;
    u16 data_len=0;
    u16 buf_len_add=0;
    u16 buf_len_crc=0;
    
    switch(obj->buf[0])//检查帧头标志
    {
        case XMODEM_FRM_FLAG_EOT://发送结束标志
		{
            return REC_RESULT_FINISH;
		}
        
        case XMODEM_FRM_FLAG_SOH://若为128字节数据
		{
            data_len=FRAME_LEN_LEN_128;
            buf_len_add=XMODEM_BUF_LEN_128_ADD;
            buf_len_crc=XMODEM_BUF_LEN_128_CRC;
        }break;
		
        case XMODEM_FRM_FLAG_STX: //若为1024字节数据
		{
            data_len=FRAME_LEN_LEN_1K;
            buf_len_add=XMODEM_BUF_LEN_1K_ADD;
            buf_len_crc=XMODEM_BUF_LEN_1K_CRC;
        }break;
		
        default:
		{
			//非法帧头则返回错误
			if(uPrint.tFlag.bXmodem)
				log_e("bXmodem:解析协议的帧头错误");
			return REC_RESULT_ERR;
		}
    }
	
	//检查帧长
    frm_len += data_len;
    if(frm_len != obj->usRecLen)
    {
		if(uPrint.tFlag.bXmodem)
			log_e("bXmodem:解析协议的帧长错误, %d, %d",frm_len,obj->usRecLen);
        return REC_RESULT_ERR;
    }
	
    //校验接收包序号
    if(0 != (obj->buf[1]&obj->buf[2]))
    {
		if(uPrint.tFlag.bXmodem)
			log_e("bXmodem:解析协议包序号错,%d,%d",obj->buf[1],obj->buf[2]);
        return REC_RESULT_ERR;
    }
	
    //帧号合法性检测
	    //首帧接收的帧序号不对(frm_cnt为程序要求接收的序号)
	    //接收帧号小于当前帧号-1
	    //接收帧号大于当前帧号
    if(((1 == obj->frm_cnt)&&(1 != obj->buf[1]))||\
       ((obj->frm_cnt-1) > obj->buf[1])||\
       (obj->frm_cnt < obj->buf[1]))
    {
		if(uPrint.tFlag.bXmodem)
			log_e("bXmodem:解析协议的帧序号错误，%d, %d",obj->frm_cnt,obj->buf[1]);
        return REC_RESULT_FAULT;
    }
	
    //选择校验方式
    if(CHECK_MODE_ADD == obj->eCheckMode ||
	   CHECK_MODE_CRC==obj->eCheckMode)
    {
		if(CHECK_MODE_ADD == obj->eCheckMode)
		{
			if(chk_add8(&obj->buf[3],data_len,obj->buf[buf_len_add-1]) == false)
			{
				if(uPrint.tFlag.bXmodem)
					log_e("bXmodem:解析协议的累加和校验错误");
				return REC_RESULT_ERR;
			}
		}
		else 
		{
			if(chk_crc16(&obj->buf[3],data_len,(obj->buf[buf_len_crc-2]<<8)|(obj->buf[buf_len_crc-1])) == false)
			{
				if(uPrint.tFlag.bXmodem)
					log_e("bXmodem:解析协议的CRC16校验错误");
				return REC_RESULT_ERR;
			}
		}
        
		
        //接收的帧序号等于期待接收的序号
        if(obj->frm_cnt==obj->buf[1])
        {
			//第一包数据准备开始处理
			if(obj->eState == XMODEM_STATE_STANDBY &&
				obj->buf[1] == 1)
			{
				(*obj->v_rec_start)();
			}
			
			//把接收到的数据添加的处理回调函数
            (*obj->v_proc_check_ok_rec_data)(&obj->buf[3],data_len);
            //包计数增加
            obj->frm_cnt++;
        }
		else 
			return REC_RESULT_ERR;
		
        return REC_RESULT_OK;
    }
	else
	{
		if(uPrint.tFlag.bXmodem)
			log_e("bXmodem:解析协议的模式有错误");
		return REC_RESULT_FAULT;
	}
	
}

/*****************************************************************************************************************
-----函数功能    获取定义的接收帧的长度
-----说明(备注)  bXmodem对象
-----传入参数    none
-----输出参数    none
-----返回值      帧的长度
*****************************************************************************************************************/
u16 us_get_frame_len(Xmodem_T *obj)
{
    if(CHECK_MODE_ADD==obj->eCheckMode)
    {
        if(FRAME_LEN_128==obj->eFrameLen)
            return XMODEM_BUF_LEN_128_ADD;
		else
			return XMODEM_BUF_LEN_1K_ADD;
    }
	else
	{
		if(FRAME_LEN_128==obj->eFrameLen)
			return XMODEM_BUF_LEN_128_CRC;
		else
			return XMODEM_BUF_LEN_1K_CRC;
	}
    
}


/*****************************************************************************************************************
-----函数功能    获取定义的接收帧的长度
-----说明(备注)  bXmodem对象
-----传入参数    none
-----输出参数    none
-----返回值      帧的长度
*****************************************************************************************************************/
static void v_set_work_state(Xmodem_T *obj, WorkState_E state)
{
	switch(state)
	{
		case XMODEM_STATE_IDLE:
		{
			obj->eRecState=REC_STATE_IDLE;//将XMODEM接收状态恢复到空闲状态
		}break;
		
		case XMODEM_STATE_STANDBY:
		{
			obj->eRecState=REC_STATE_IDLE;//将XMODEM接收状态恢复到空闲状态
		}break;
		
		case XMODEM_STATE_RECEIVING:
		{
			obj->eRecState=REC_STATE_IDLE;//将XMODEM接收状态恢复到空闲状态
		}break;
		
		case XMODEM_STATE_FINISH:
		{
			obj->eRecState=REC_STATE_IDLE;//将XMODEM接收状态恢复到空闲状态
		}break;
		
		case XMODEM_STATE_CANCEL:
		{
			obj->eRecState=REC_STATE_IDLE;//将XMODEM接收状态恢复到空闲状态
		}break;
		
		case XMODEM_STATE_STOP:
		{
			obj->eRecState=REC_STATE_IDLE;//将XMODEM接收状态恢复到空闲状态
		}break;
		
		default:
			break;
	}
	
    obj->eState = state;
}




/*****************************************************************************************************************
-----函数功能    获取定义的接收帧的长度
-----说明(备注)  bXmodem对象
-----传入参数    none
-----输出参数    none
-----返回值      帧的长度
*****************************************************************************************************************/
static bool b_send_cmd(Xmodem_T *obj, u8 cmd)
{
	if(obj->bStartSendFrm == false)
		return false;
	
	if(cmd == XMODEM_FRM_FLAG_ECHO)
		cmd = (CHECK_MODE_ADD==obj->eCheckMode?XMODEM_FRM_FLAG_ADD_ECHO:XMODEM_FRM_FLAG_CRC_ECHO);//根据模式设置ECHO类型
	
	//开始发送,第二个参数buff不可用,回调没有实现
	if((*obj->c_xmodem_trans_data)(cmd, NULL, 0) <= 0) 
		return false;
	
	obj->usRecLen = 0;
	obj->usFrmOvertimeCnt = XMODEM_RX_TIMEOUT_MS;
	obj->bStartSendFrm = false;
	return true;
}



/*****************************************************************************************************************
-----函数功能    获取定义的接收帧的长度
-----说明(备注)  bXmodem对象
-----传入参数    none
-----输出参数    none
-----返回值      帧的长度
*****************************************************************************************************************/
static RecState_E e_get_rec_state(Xmodem_T *obj, s8 resule)
{
	//成功或完成
	if(resule == 1 || resule ==2)
	{
		obj->bStartSendFrm = true;
		
		return REC_STATE_OK;
	}
	//继续等待
	else if(resule == 0)
		return obj->eRecState;
	//超时
	else if(resule < 0)
	{
		obj->bStartSendFrm = true;
		obj->usRecLen = 0;
		return REC_STATE_TIMEOUT;
	}
	//继续接受
	else
	{
		obj->bStartSendFrm = true;
		obj->usWaitStartOutTimeCnt = XMODEM_START_TIMEOUT_MS;
		obj->usWaitExitOutTimeCnt = XMODEM_END_TIMEOUT_MS;
		return REC_STATE_IDLE;
	}
		
}


















//************************************************************************************************************************************//
//*********************************************************全局函数********************************************************************//
//************************************************************************************************************************************//


/*****************************************************************************************************************
-----函数功能    bXmodem协议程序
-----说明(备注)  XMODEM接收系统主处理过程
-----传入参数    XMODEM对象
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void vXmodem_Proto(Xmodem_T *obj)
{
    if(!obj)
        return;
	
	s8 result = 0;
	
//    //首先判断等待状态，若处于等待状态则直接退出，用于非阻塞接收
//    if(REC_STATE_WAIT==obj->eRecState)
//    {
//        //再次更新获取读取接收状态
//        result = (*obj->c_xmodem_rec_data)(obj->buf,us_get_frame_len(obj),(u16*)&obj->usRecLen);
//		obj->eRecState = e_get_rec_state(obj, result);
//        
//        if(REC_STATE_WAIT==obj->eRecState)
//            return;
//    }
	
    switch(obj->eState)
    {
        //IDLE状态则检查是否需要启动传输
		//*****************************************************空闲状态*********************************************************//
        case XMODEM_STATE_IDLE:
        {
            v_set_work_state(obj, XMODEM_STATE_STANDBY);
        }break;
		
		//*****************************************************就绪状态*********************************************************//
		//等待接收第一个有效数据帧
        case XMODEM_STATE_STANDBY:
		{
			switch(obj->eRecState)
			{
				case REC_STATE_IDLE:
				{
					//发送一个ECHO
					b_send_cmd(obj, XMODEM_FRM_FLAG_ECHO);
					
					//设置超时，等待接收数据
					result = (*obj->c_xmodem_rec_data)(obj->buf,us_get_frame_len(obj),(u16*)&obj->usRecLen);
					
					//协议切换
					if(result == 99)
					{
						bXmodem_Reset(obj);
						return;
					}
					
					obj->eRecState = e_get_rec_state(obj, result);
				}break;
				
				case REC_STATE_OK:
				{
					switch(e_rec_proc(obj))  //rx_proc处理接收到的数据
					{
						//解析错误
						case REC_RESULT_ERR://接收错误则按超时处理
							obj->eRecState=REC_STATE_TIMEOUT;
						break;
						
						//解析成功
						case REC_RESULT_OK:
							v_set_work_state(obj, XMODEM_STATE_RECEIVING);
						break;
						
						//升级完成
						case REC_RESULT_FINISH:
						{
							//发送一个ACK
							b_send_cmd(obj, XMODEM_FRM_FLAG_ACK);
							v_set_work_state(obj, XMODEM_STATE_STOP);
						}break;
						
						//解析失败
						case REC_RESULT_FAULT:
						default:
							v_set_work_state(obj, XMODEM_STATE_CANCEL);
						break;
					}
				}break;
				
				case REC_STATE_TIMEOUT:
				{
					if(obj->usWaitStartOutTimeCnt == 0)
					{
						(*obj->v_rec_end)(2);
						
						v_set_work_state(obj, XMODEM_STATE_STOP);
						break;
					}
					//发送一个ECHO
					b_send_cmd(obj, XMODEM_FRM_FLAG_ECHO);
					//设置超时，等待接收数据
					result = (*obj->c_xmodem_rec_data)(obj->buf, us_get_frame_len(obj), (u16*)&obj->usRecLen);
					obj->eRecState = e_get_rec_state(obj, result);
				}break;
				
				default:
					break;
			}
        }break;
			
			
		//*****************************************************接收状态*********************************************************//	
        //若当前处于接收数据状态
        case XMODEM_STATE_RECEIVING:
		{
			switch(obj->eRecState)
			{
				case REC_STATE_IDLE:
				{
					//发送一个ACK
					b_send_cmd(obj, XMODEM_FRM_FLAG_ACK);
					
					//等待接收数据
					result = (*obj->c_xmodem_rec_data)(obj->buf, us_get_frame_len(obj), (u16*)&obj->usRecLen);
					obj->eRecState = e_get_rec_state(obj, result);
					
				}break;
				
				case REC_STATE_OK:
				{
					switch(e_rec_proc(obj))
					{
						//解析错误
						case REC_RESULT_ERR:
							obj->eRecState=REC_STATE_TIMEOUT;
						break;
						
						//解析成功
						case REC_RESULT_OK:
							obj->eRecState = e_get_rec_state(obj, 3);  //进入REC_STATE_IDLE
						break;
						
						//升级完成
						case REC_RESULT_FINISH:
							v_set_work_state(obj, XMODEM_STATE_FINISH);
						break;
						
						//当前传输被从机中断则发送CAN并切换到取消模式或者其他状态
						case REC_RESULT_FAULT:
						default:
							v_set_work_state(obj, XMODEM_STATE_CANCEL);
						break;
					}
				}break;
				
				case REC_STATE_TIMEOUT:
				{
					if(obj->usWaitExitOutTimeCnt == 0)
					{
						if(uPrint.tFlag.bXmodem)
							log_w("bXmodem:等待超时,停止升级");
						
						v_set_work_state(obj, XMODEM_STATE_CANCEL);
						break;
					}
					//发送一个ECHO
					b_send_cmd(obj, XMODEM_FRM_FLAG_ECHO);
					
					//继续等待接受
					result = (*obj->c_xmodem_rec_data)(obj->buf, us_get_frame_len(obj), (u16*)&obj->usRecLen);
					obj->eRecState = e_get_rec_state(obj, result);
				}break;
				
				default:
					break;
			}
        }break;
			
			
		//*****************************************************完成状态*********************************************************//	
        //XMODEM传输完成处理
        case XMODEM_STATE_FINISH:
		{
			//发送一个ACK
			b_send_cmd(obj, XMODEM_FRM_FLAG_ACK);
			
            //产生回调，告知上层当前传输已完成
            (*obj->v_rec_end)(0);
			
            //复位XMODEM
            bXmodem_Reset(obj);
        }break;
			
		//*****************************************************取消状态*********************************************************//
        case XMODEM_STATE_CANCEL:
		{
			if(uPrint.tFlag.bXmodem)
				sMyPrint("vXmodem_Proto:XMODEM_STATE_CANCEL\n\r");

			b_send_cmd(obj, XMODEM_FRM_FLAG_CAN);
			
			v_set_work_state(obj, XMODEM_STATE_STOP);
        }break;
		
		//*****************************************************结束状态*********************************************************//
        case XMODEM_STATE_STOP:
		{
			if(uPrint.tFlag.bXmodem)
				sMyPrint("vXmodem_Proto:XMODEM_STATE_STOP\n\r");
			
            //产生回调，告知上层当前传输已停止
            (*obj->v_rec_end)(1);
            //复位XMODEM
            bXmodem_Reset(obj);
			
			//v_set_work_state(obj, XMODEM_STATE_IDLE);
        }break;
        
		default:
        break;
    }
}

/***********************************************************************************************************************
-----函数功能    Tick计时
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vXmodem_TickTime(Xmodem_T *obj)
{
	if(obj == NULL)
		return;
	
	//帧等待计时
	if(obj->usFrmOvertimeCnt > 0)
		obj->usFrmOvertimeCnt--;
	
	//等待开始计时
	if(obj->usWaitStartOutTimeCnt > 0)
		obj->usWaitStartOutTimeCnt--;
	
	//
	if(obj->usWaitExitOutTimeCnt > 0)
		obj->usWaitExitOutTimeCnt--;
}

/*****************************************************************************************************************
-----函数功能    复位协议
-----说明(备注)  内部函数，复位XMODEM对象状态，外部不可调用
-----传入参数    obj:XMODEM传输对象
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
void bXmodem_Reset(Xmodem_T *obj)
{
	if(obj == NULL)
		return;
	
    //将XMODEM恢复到空闲状态
    obj->eState=XMODEM_STATE_IDLE;
    //将XMODEM接收状态恢复到空闲状态
    obj->eRecState=REC_STATE_IDLE;
    //清除传输等待计数
    obj->usWaitStartOutTimeCnt=XMODEM_START_TIMEOUT_MS;
    //清除接收超时次数计数
    obj->usWaitExitOutTimeCnt=XMODEM_END_TIMEOUT_MS;
    //清除接收缓冲
	memset(obj->buf,0,sizeof(obj->buf));
    //清除当前接收数据长度
    obj->usRecLen=0;
    //清除帧计帧
    obj->frm_cnt=1;
}

#endif //boardUPDATE

#ifdef __cplusplus
}
#endif
