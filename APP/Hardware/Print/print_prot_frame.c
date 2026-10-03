/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Print
 * File    : print_prot_frame.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : Print通信协议帧组包、解析、分发及中继处理实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_prot_frame.h"
#include "lwrb.h"
#include "main.h"

#if (boardPRINT_IFACE)
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

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

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

#if (boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif  /* boardUSE_OS */

//****************************************************Macros********************************************************************//
#define			printDEV_ADRR							printCONSOLE_MASTER_ADDR
#define			printWAIT_NOTIFY_OUTTIME				1000	/* 任务通知超时时间 MS */
#define			printTX_FRAME_SIZE						256
#define			printRX_FRAME_SIZE						256

/* 任务状态调试结构体（用于向上位机上报任务运行状态，共10字节） */
#pragma pack(1)
typedef struct
{
	u8					ucID;				/* 当前任务ID */
	u8					ucStep;				/* 当前步骤 */
	u16					usInParam;			/* 函数参数 */
	u16					usStepWaitCnt;		/* 步骤等待次数 */
	u16					usStepRepeatCnt;	/* 步骤重复次数 */
	u16					usTaskWaitCnt;		/* 任务等待次数 */
}TaskDebug_T;
#pragma pack()

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) BaikuProtoTx_t *tpPrintProtoTx = NULL;	/* 发送协议 */
__ALIGNED(4) BaikuProtoRx_t *tpPrintProtoRx = NULL;

vu8 uc_next_cmd = 0;

//****************************************************Function Declaration******************************************************//
static s8 c_print_data_trans(u8 cmd, u8 *data, u8 len);
static s8 c_print_data_trans_for_update(u8 cmd, u8* data, u8 len);
static s8 c_get_console_ver_info(u8* data, u8* data_len);
static s8 c_relay_console_info(BaikuProtoRx_t* proto);
static s8 c_set_console_info(BaikuProtoRx_t* proto);


/***********************************************************************************************************************
 * 函数功能    : 发送通讯协议初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_SendProtInit(void)
{
	s8 c_result = cBaiku_ProtoSendInit(&tpPrintProtoTx,		/* 协议指针 */
	                                   printTX_FRAME_SIZE,	/* 协议缓存器大小 */
	                                   printDEV_ADRR);		/* 协议设备ID */
	if (c_result <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 接收通讯协议初始化
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
bool bPrint_RecProtInit(void)
{
	s8 c_result = cBaiku_ProtoRecInit(&tpPrintProtoRx,				/* 协议指针 */
	                                  printRX_FRAME_SIZE,			/* 协议缓存器大小 */
	                                  sysDEV_ADRR,					/* 协议设备ID */
	                                  boardREPET_TIMER_CYCLE_TMIE);	/* 计数器采样时间 */
	if (c_result <= 0)
		return false;
	
	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 周期循环回复数据
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : 1: 发送成功, 0: 未触发
 ************************************************************************************************************************/
s8 c_cycle_relay_data(void)
{
	static vu16 us_delay_cnt = 0;
	
	if (uc_next_cmd != 0x0D)
	{
		us_delay_cnt = 0;
		return 0;
	}
	
	us_delay_cnt++;
	if (us_delay_cnt >= (100 / printTASK_CYCLE_TIME))
		us_delay_cnt = 0;
		/* c_relay0E_mppt_param(); */
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 回复模块开关结果  0x02
 * 说明(备注)  : none
 * 传入参数    : data: [0]开关对象, [1]开关动作与执行结果
 * 输出参数    : none
 * 返回值      : true:成功   false:失败
 ************************************************************************************************************************/
s8 c_relay02_switch_result(uint8_t data[])
{
    if(data[0] == 0x00)
    {
        if(data[1] == 0x00)
            cSys_Switch(SO_KEY, ST_OFF, false);
        else if (data[1] == 0x01)
            cSys_Switch(SO_KEY,ST_ON, false);
        c_print_data_trans(0x02, data, 2);  
    }

	#if(boardUSB_EN)
    else if(data[0] == 0x01)
    {
        if(data[1] == 0x00)
            cUsb_Switch(ST_OFF, false);
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
            bLight_Switch(ST_OFF);
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
            cDCAC_Switch(DSO_AC_OUT,ST_OFF, true);
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
            cDc_Switch(ST_OFF, false);
        else if (data[1] == 0x01)
        {
            //打开失败
            if(cDc_Switch(ST_ON, false) <= 0)
                data[1] = 0x00;
        }
        c_print_data_trans(0x02, data, 2);  
    }
	#endif  //boardDC_EN

	#if(boardDCAC_EN)
	else if(data[0] == 0x05)
    {
       if(data[1] == 0x00)
           cDCAC_Switch(DSO_AC_IN,ST_OFF, true);
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
 * 函数功能    : 回复系统概览参数  0x08
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay08_param(void)
{
    #pragma pack(1)
    struct
    {
		u8				ucSysDevState;
		u16				usVolt;
		s16				sTotalCurr;
		u8				ucChgPerm;
		u8				ucDisChgPerm;
		u8				ucUsbDevState;
		u8				ucLightDevState;
		u8				ucDcacDisChgState;
		u8				ucDcDevState;
		u8				ucDcacChgState;
		u8				ucMpptDevState;
		u8				ucBmsSoc;
    } t_frame = {0};
    #pragma pack()

    t_frame.ucSysDevState = (u8)tSysInfo.eDevState;

    #if(boardBMS_EN)
    t_frame.usVolt     = tBmsRx.tDevInfo[0].usVolt;
    t_frame.sTotalCurr = tBmsRx.sTotalCurr;
    t_frame.ucBmsSoc   = ucBms_GetSoc();
    #endif  //boardBMS_EN

    t_frame.ucChgPerm    = tSysInfo.uPerm.tPerm.bChgPerm;
    t_frame.ucDisChgPerm = tSysInfo.uPerm.tPerm.bDisChgPerm;

    #if(boardUSB_EN)
    t_frame.ucUsbDevState = (u8)tUsb.eDevState;
    #endif  //boardUSB_EN

    #if(boardLIGHT_EN)
    t_frame.ucLightDevState = (u8)tLight.eDevState;
    #endif  //boardLIGHT_EN

    #if(boardDCAC_EN)
    t_frame.ucDcacDisChgState = (u8)tDcac.eDisChgState;
    t_frame.ucDcacChgState    = (u8)tDcac.eChgState;
    #endif  //boardDCAC_EN

    #if(boardDC_EN)
    t_frame.ucDcDevState = (u8)tDc.eDevState;
    #endif  //boardDC_EN

    #if(boardMPPT_EN)
    t_frame.ucMpptDevState = (u8)tMppt.eDevState;
    #endif  //boardMPPT_EN

    return c_print_data_trans(0x08, (u8*)&t_frame, sizeof(t_frame));
}


/***********************************************************************************************************************
 * 函数功能    : 回复电池参数  0x0A
 * 说明(备注)  : 包含主机状态、电池从机数据以及任务调试信息
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay0A_bat_param(void)
{
    #if(boardBMS_EN)
    #pragma pack(1)
    struct
    {
        // BMS 基础状态 (19 字节)
        struct
        {
			u8			ucDevState;			// 设备状态
			u64			ullErrCode;			// 错误代码 (8字节)
			u8			ucWorkState;		// 工作状态
			u8			ucPerm;				// 许可
			u16			usAutoOffCnt;		// 自动关机计时
			u16			usAutoOffTime;		// 自动关机时间
			s16			sMaxTemp;			// 最高温度
			s16			sMinTemp;			// 最低温度
		}tBms;

        // BMS 统计与状态 (16 字节)
        struct
        {
			u16			usSOC;				// 总的SOC (1%)
			s16			sTotalCurr;			// 总的电流 (0.01A)
			u16			usChgFullTime;		// 总充满时间 (1min)
			u16			usDisChgEmptyTime;	// 总放空时间 (1min)
			u16			usPermMaxChgPwr;	// 许可最大充电功率 (W)
			u16			usPermMaxDisChgPwr;	// 许可最大放电功率 (W)
            struct
            {
				u8		ucOnlineNum;		// 在线设备数
				u8		ucMasterNum;		// 选中的数量
			}tDevNum;
			u16			usState;			// 主机系统状态
		}tBmsRx;

        // 电池从机详细信息 (6台 * 20字节 = 120字节)
        struct
        {
			u16			usSOC;				// SOC (1%)
			u16			usVolt;				// 电压 (0.01V)
			s16			sCurr;				// 电流 (0.01A)
			u16			usCalcCapAH;		// 估算容量 (0.1AH)
			u16			usCycleCnt;			// 循环次数
			s16			sMaxTemp;			// 主机最高温度 (1℃)
			s16			sMinTemp;			// 主机最低温度 (1℃)
			s16			sBoardTempMax;		// 板载最高温
			u32			ulErrCode;			// 错误代码
		}				atDevInfo[bmsDEV_NUM];

        // 任务队列调试状态 (10 字节)
		TaskDebug_T		tTask;
    } t_frame = {0};
    #pragma pack()

    t_frame.tBms.ucDevState     = (u8)tBms.eDevState;
    t_frame.tBms.ullErrCode     = tBms.uErrCode.ullCode;
    t_frame.tBms.ucWorkState    = (u8)tBms.eWorkState;
    t_frame.tBms.ucPerm         = tBms.uPerm.ucPerm;
    t_frame.tBms.usAutoOffCnt   = tBms.usAutoOffCnt;
    t_frame.tBms.usAutoOffTime  = tBms.usAutoOffTime;
    t_frame.tBms.sMaxTemp       = tBms.sMaxTemp;
    t_frame.tBms.sMinTemp       = tBms.sMinTemp;

    t_frame.tBmsRx.usSOC              = tBmsRx.usSOC;
    t_frame.tBmsRx.sTotalCurr         = tBmsRx.sTotalCurr;
    t_frame.tBmsRx.usChgFullTime      = tBmsRx.usChgFullTime;
    t_frame.tBmsRx.usDisChgEmptyTime  = tBmsRx.usDisChgEmptyTime;
    t_frame.tBmsRx.usPermMaxChgPwr    = tBmsRx.usPermMaxChgPwr;
    t_frame.tBmsRx.usPermMaxDisChgPwr = tBmsRx.usPermMaxDisChgPwr;
    t_frame.tBmsRx.tDevNum.ucOnlineNum = tBmsRx.tDevNum.ucOnlineNum;
    t_frame.tBmsRx.tDevNum.ucMasterNum = tBmsRx.tDevNum.ucMasterNum;
    memcpy(&t_frame.tBmsRx.usState, (void *)&tBmsRx.tState, sizeof(t_frame.tBmsRx.usState));

    for(u8 i = 0; i < bmsDEV_NUM; i++)
    {
        t_frame.atDevInfo[i].usSOC         = tBmsRx.tDevInfo[i].usSOC;
        t_frame.atDevInfo[i].usVolt        = tBmsRx.tDevInfo[i].usVolt;
        t_frame.atDevInfo[i].sCurr         = tBmsRx.tDevInfo[i].sCurr;
        t_frame.atDevInfo[i].usCalcCapAH   = tBmsRx.tDevInfo[i].usCalcCapAH;
        t_frame.atDevInfo[i].usCycleCnt    = tBmsRx.tDevInfo[i].usCycleCnt;
        t_frame.atDevInfo[i].sMaxTemp      = tBmsRx.tDevInfo[i].sMaxTemp;
        t_frame.atDevInfo[i].sMinTemp      = tBmsRx.tDevInfo[i].sMinTemp;
        t_frame.atDevInfo[i].sBoardTempMax = tBmsRx.tDevInfo[i].sBoardTempMax;
        t_frame.atDevInfo[i].ulErrCode     = tBmsRx.tDevInfo[i].uErrCode.ulCode;
    }

    if(tpBmsTask != NULL)
        memcpy(&t_frame.tTask, (u8*)tpBmsTask, sizeof(TaskDebug_T));

    return c_print_data_trans(0x0A, (u8*)&t_frame, sizeof(t_frame));
    #else
    return false;
    #endif  //boardBMS_EN
}


/***********************************************************************************************************************
 * 函数功能    : 回复DCAC参数  0x0C
 * 说明(备注)  : 包含DCAC运行状态、接收数据以及任务调试信息
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay0C_dcac_param(void)
{
    #if(boardDCAC_EN)
    #pragma pack(1)
    struct
    {
        // DCAC 基础状态 (15 字节)
        struct
        {
			u8			ucDevState;			// 设备状态
			u32			ulErrCode;			// 错误状态 (4字节)
			u8			ucChgState;			// 充电状态
			u8			ucDisChgState;		// 放电状态
			u8			ucParanInState;		// 并网状态
			u8			ucPerm;				// 许可
			u16			usAutoOffCnt;		// 自动关机计时
			u16			usAutoOffTime;		// 自动关机时间
			s16			sMaxTemp;			// 最高温度
		}tDcac;

        // DCAC 接收数据 (42 字节)
        struct
        {
			u16			usInVolt;			// 输入电压 (0.1V)
			u16			usInCurr;			// 输入电流 (0.1A)
			u16			usInPwr;			// 输入功率 (W)
			u16			usInChgPwr;			// 输入充电功率 (W)
			u16			usInFreq;			// 输入频率 (0.1HZ)
			u16			usOutVolt;			// 输出电压 (0.1V)
			u16			usOutCurr;			// 输出电流 (0.1A)
			u16			usOutPwr;			// 输出功率 (W)
			u16			usOutFreq;			// 输出频率 (0.1HZ)
			u16			usParaInMaxPwr;		// 并网最大功率 (W)
			u16			usParaInPwr;		// 并网功率 (W)
			u16			usParaInMode;		// 并网模式
			s16			sChgPwr;			// 充电功率 (W)
			u16			usMaxInPwr;			// 最大输入功率 (W)
			s16			sMaxTemp;			// 最高温度 (1℃)
			s16			sMinTemp;			// 最低温度 (1℃)
			u16			usState;			// 状态
			u16			usErrCode[4];		// 错误代码 (8字节)
		}tDcacRx;

        // 任务队列调试状态 (10 字节)
		TaskDebug_T		tTask;
    } t_frame = {0};
    #pragma pack()

    t_frame.tDcac.ucDevState     = (u8)tDcac.eDevState;
    t_frame.tDcac.ulErrCode      = tDcac.uErrCode.ulCode;
    t_frame.tDcac.ucChgState     = (u8)tDcac.eChgState;
    t_frame.tDcac.ucDisChgState  = (u8)tDcac.eDisChgState;
    t_frame.tDcac.ucParanInState = (u8)tDcac.eParanInState;
    t_frame.tDcac.ucPerm         = tDcac.uPerm.ucPerm;
    t_frame.tDcac.usAutoOffCnt   = tDcac.usAutoOffCnt;
    t_frame.tDcac.usAutoOffTime  = tDcac.usAutoOffTime;
    t_frame.tDcac.sMaxTemp       = tDcac.sMaxTemp;

    t_frame.tDcacRx.usInVolt       = tDcacRx.usInVolt;
    t_frame.tDcacRx.usInCurr       = tDcacRx.usInCurr;
    t_frame.tDcacRx.usInPwr        = tDcacRx.usInPwr;
    t_frame.tDcacRx.usInChgPwr     = tDcacRx.usInChgPwr;
    t_frame.tDcacRx.usInFreq       = tDcacRx.usInFreq;
    t_frame.tDcacRx.usOutVolt      = tDcacRx.usOutVolt;
    t_frame.tDcacRx.usOutCurr      = tDcacRx.usOutCurr;
    t_frame.tDcacRx.usOutPwr       = tDcacRx.usOutPwr;
    t_frame.tDcacRx.usOutFreq      = tDcacRx.usOutFreq;
    t_frame.tDcacRx.usParaInMaxPwr = tDcacRx.usParaInMaxPwr;
    t_frame.tDcacRx.usParaInPwr    = tDcacRx.usParaInPwr;
    t_frame.tDcacRx.usParaInMode   = tDcacRx.usParaInMode;
    t_frame.tDcacRx.sChgPwr        = tDcacRx.usChgPwr;
    t_frame.tDcacRx.usMaxInPwr     = tDcacRx.usMaxInPwr;
    t_frame.tDcacRx.sMaxTemp       = tDcacRx.sMaxTemp;
    t_frame.tDcacRx.sMinTemp       = tDcacRx.sMinTemp;
    t_frame.tDcacRx.usState        = tDcacRx.uState.usState;
    t_frame.tDcacRx.usErrCode[0]   = tDcacRx.uErrCode.usCode[0];
    t_frame.tDcacRx.usErrCode[1]   = tDcacRx.uErrCode.usCode[1];
    t_frame.tDcacRx.usErrCode[2]   = tDcacRx.uErrCode.usCode[2];
    t_frame.tDcacRx.usErrCode[3]   = tDcacRx.uErrCode.usCode[3];

    if(tpDcacTask != NULL)
        memcpy(&t_frame.tTask, (u8*)tpDcacTask, sizeof(TaskDebug_T));

    return c_print_data_trans(0x0C, (u8*)&t_frame, sizeof(t_frame));
    #else
    return false;
    #endif  //boardDCAC_EN
}

/***********************************************************************************************************************
 * 函数功能    : 回复MPPT参数  0x0E
 * 说明(备注)  : 包含MPPT状态、接收数据以及任务调试信息
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay0E_mppt_param(void)
{
    #if(boardMPPT_EN)
    #pragma pack(1)
    struct
    {
        // MPPT 基础状态 (15 字节)
        struct
        {
			u8			ucDevState;			// 设备状态
			u8			ucWorkMode;			// 工作模式
			u32			ulErrCode;			// MPPT错误状态 (4字节)
			u16			usAutoOffTime;		// 关闭逆变器的时间 (0为不开启)
			u16			usAutoOffCnt;		// 时间戳计数
			u16			usInPwr;			// 输入功率 (W)
			u8			ucChgPerm;			// 充电许可
			s16			sMaxTemp;			// 最高温度
		}tMppt;

        // MPPT 接收数据 (19 字节)
        struct
        {
			u8			ucInType;			// 输入类型 (1字节)
			u16			usErrCode;			// 错误状态 (2字节)
			u16			usInVolt;			// 输入电压 (0.1V)
			u16			usInCurr;			// 输入电流 (0.01A)
			u16			usInPwr;			// 输入功率 (0.1W)
			u16			usOutVolt;			// 输出电压 (0.1V)
			u16			usOutCurr;			// 输出电流 (0.01A)
			u16			usOutPwr;			// 输出功率 (0.1W)
			u16			usMaxInPwr;			// 最大输入功率 (0.1W)
			s16			sMaxTemp;			// 最大温度 (℃)
		}tMpptRx;

        // 任务队列调试状态 (10 字节)
		TaskDebug_T		tTask;
    } t_frame = {0};
    #pragma pack()

    t_frame.tMppt.ucDevState    = (u8)tMppt.eDevState;
    t_frame.tMppt.ucWorkMode    = (u8)tMppt.eWorkMode;
    t_frame.tMppt.ulErrCode     = tMppt.uErrCode.ulCode;
    t_frame.tMppt.usAutoOffTime = tMppt.usAutoOffTime;
    t_frame.tMppt.usAutoOffCnt  = tMppt.usAutoOffCnt;
    t_frame.tMppt.usInPwr       = tMppt.usInPwr;
    t_frame.tMppt.ucChgPerm     = (u8)tMppt.bChgPerm;
    t_frame.tMppt.sMaxTemp      = tMppt.sMaxTemp;

    t_frame.tMpptRx.ucInType    = (u8)tMpptRx.uInType;
    t_frame.tMpptRx.usErrCode   = tMpptRx.uErrCode.usCode;
    t_frame.tMpptRx.usInVolt    = tMpptRx.usInVolt;
    t_frame.tMpptRx.usInCurr    = tMpptRx.usInCurr;
    t_frame.tMpptRx.usInPwr     = tMpptRx.usInPwr;
    t_frame.tMpptRx.usOutVolt   = tMpptRx.usOutVolt;
    t_frame.tMpptRx.usOutCurr   = tMpptRx.usOutCurr;
    t_frame.tMpptRx.usOutPwr    = tMpptRx.usOutPwr;
    t_frame.tMpptRx.usMaxInPwr  = tMpptRx.usMaxInPwr;
    t_frame.tMpptRx.sMaxTemp    = tMpptRx.sMaxTemp;

    if(tpMpptTask != NULL)
        memcpy(&t_frame.tTask, (u8*)tpMpptTask, sizeof(TaskDebug_T));

    return c_print_data_trans(0x0E, (u8*)&t_frame, sizeof(t_frame));
    #else
    return false;
    #endif  //boardMPPT_EN
}

/***********************************************************************************************************************
 * 函数功能    : 回复USB参数  0x10
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay10_usb_param(void)
{
    #if(boardUSB_EN)
    #pragma pack(1)
    struct
    {
		u8				ucDevState;			// 设备状态
		u16				usErrCode;			// 错误代码
		u16				usAutoOffTime;		// 自动关机时间 (S)
		u16				usAutoOffCnt;		// 自动关机计时 (S)
		u16				usInVolt;			// 0.1V
		u16				usInCurr;			// 0.1A
		u16				usOutPwr;			// W
		s16				sMaxTemp;			// 摄氏度
    } t_frame = {0};
    #pragma pack()

    t_frame.ucDevState    = (u8)tUsb.eDevState;
    t_frame.usErrCode     = tUsb.uErrCode.ucErrCode;
    t_frame.usAutoOffTime = tUsb.usAutoOffTime;
    t_frame.usAutoOffCnt  = tUsb.usAutoOffCnt;
    t_frame.usInVolt      = tUsb.usInVolt;
    t_frame.usInCurr      = tUsb.usInCurr;
    t_frame.usOutPwr      = tUsb.usOutPwr;
    t_frame.sMaxTemp      = tUsb.sMaxTemp;

    return c_print_data_trans(0x10, (u8*)&t_frame, sizeof(t_frame));
    #else
    return false;
    #endif  //boardUSB_EN
}

/***********************************************************************************************************************
 * 函数功能    : 回复DC参数  0x12
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败   
 ************************************************************************************************************************/
s8 c_relay12_dc_param(void)
{
    #if(boardDC_EN)
    #pragma pack(1)
    struct
    {
		u8				ucDevState;			// 设备状态
		u8				ucErrCode;			// DC任务错误状态
		u16				usInVolt;			// 0.1V
		u16				usInCurr;			// 0.1A
		u16				usOutVolt;			// 0.1V
		u16				usOutCurr;			// 0.1A
		u16				usOutPwr;			// W
		u16				usAutoOffTime;		// 自动关机时间 (S)
		u16				usAutoOffCnt;		// 自动关机计时 (S)
		s16				sMaxTemp;			// 摄氏度
    } t_frame = {0};
    #pragma pack()

    t_frame.ucDevState    = (u8)tDc.eDevState;
    t_frame.ucErrCode     = tDc.uErrCode.ucErrCode;
    t_frame.usInVolt      = tDc.usInVolt;
    t_frame.usInCurr      = tDc.usInCurr;
    t_frame.usOutVolt     = tDc.usOutVolt;
    t_frame.usOutCurr     = tDc.usOutCurr;
    t_frame.usOutPwr      = tDc.usOutPwr;
    t_frame.usAutoOffTime = tDc.usAutoOffTime;
    t_frame.usAutoOffCnt  = tDc.usAutoOffCnt;
    t_frame.sMaxTemp      = tDc.sMaxTemp;

    return c_print_data_trans(0x12, (u8*)&t_frame, sizeof(t_frame));
    #else
    return false;
    #endif  //boardDC_EN
}

/***********************************************************************************************************************
 * 函数功能    : 回复系统任务参数  0x14
 * 说明(备注)  : 包含系统基础状态以及任务调试信息
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败   
 ************************************************************************************************************************/
s8 c_relay14_sysinfo_param(void)
{
    #pragma pack(1)
    struct
    {
        // 系统任务基础信息 (31 字节)
        struct
        {
			u8			ucDevState;			// 设备状态
			u16			usErrCode;			// 错误代码
			u8			ucPerm;				// 许可
			u16			usInit;				// 初始化完成标志
			u8			ucPowerType;		// 系统供电类型
            struct
            {
				u16		usMPPT;				// MPPT充电功率
				u16		usDCAC;				// DCAC充电功率
			}tSetChgPwr;
			u16			usAutoOffCnt;		// 自动关闭计时
			u16			usAutoOffTime;		// 自动关闭时间
			u16			usNeedSleepCnt;		// 需要休眠计时
			s16			sMaxTemp;			// 整机最高温
			s16			sMinTemp;			// 整机最低温
			s16			sBoardTempMax;		// 板载最高温
			u16			usVoltMax;			// 最高电压
			u16			usVoltMin;			// 最低电压
			u16			usOutPwr;			// 输出功率
			u16			usInPwr;			// 输入功率
		}tSysInfo;

        // 任务队列调试状态 (10 字节)
		TaskDebug_T		tTask;
    } t_frame = {0};
    #pragma pack()

    t_frame.tSysInfo.ucDevState        = (u8)tSysInfo.eDevState;
    t_frame.tSysInfo.usErrCode         = tSysInfo.uErrCode.usCode;
    t_frame.tSysInfo.ucPerm            = tSysInfo.uPerm.ucPerm;
    t_frame.tSysInfo.usInit            = tSysInfo.uInit.State;
    t_frame.tSysInfo.ucPowerType       = (u8)tSysInfo.ePowerType;
    t_frame.tSysInfo.tSetChgPwr.usMPPT = tSysInfo.tSetChgPwr.usMPPT;
    t_frame.tSysInfo.tSetChgPwr.usDCAC = tSysInfo.tSetChgPwr.usDCAC;
    t_frame.tSysInfo.usAutoOffCnt      = tSysInfo.usAutoOffCnt;
    t_frame.tSysInfo.usAutoOffTime     = tSysInfo.usAutoOffTime;
    t_frame.tSysInfo.usNeedSleepCnt    = tSysInfo.usNeedSleepCnt;
    t_frame.tSysInfo.sMaxTemp          = tSysInfo.sMaxTemp;
    t_frame.tSysInfo.sMinTemp          = tSysInfo.sMinTemp;
    t_frame.tSysInfo.sBoardTempMax     = tSysInfo.sBoardTempMax;
    t_frame.tSysInfo.usVoltMax         = tSysInfo.usVoltMax;
    t_frame.tSysInfo.usVoltMin         = tSysInfo.usVoltMin;
    t_frame.tSysInfo.usOutPwr          = tSysInfo.usOutPwr;
    t_frame.tSysInfo.usInPwr           = tSysInfo.usInPwr;

    if(tpSysTask != NULL)
        memcpy(&t_frame.tTask, (u8*)tpSysTask, sizeof(TaskDebug_T));

    return c_print_data_trans(0x14, (u8*)&t_frame, sizeof(t_frame));
}

/***********************************************************************************************************************
 * 函数功能    : 回复设置充电功率  0x40
 * 说明(备注)  : none
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败   
 ************************************************************************************************************************/
s8 c_relay40_set_chg_pwr(BaikuProtoRx_t* proto)
{
    if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	#pragma pack(1)
	struct
	{
		u8				module;
		u16				pwr;
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
 * 函数功能    : 请求校准  0x44
 * 说明(备注)  : none
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : 1:成功   -1:参数错误
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
 * 函数功能    : 回复校准结果  0x45
 * 说明(备注)  : none
 * 传入参数    : temp: 校准状态结果
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败   
 ************************************************************************************************************************/
s8 c_relay45_cali(u16 temp)
{
	return c_print_data_trans(baikuCMD_REPLY_CALI, (u8*)&temp, sizeof(temp));
}

/***********************************************************************************************************************
 * 函数功能    : 获取记忆参数  0x80
 * 说明(备注)  : none
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : 1:成功   -1:长度错误   -2:模式错误
 ************************************************************************************************************************/
s8 c_relay80_get_mem_param(BaikuProtoRx_t* proto)
{
	u8 uc_mode = 0;

	if(proto == NULL || proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return -1;

	uc_mode = proto->ucpValidData[0];

	switch(uc_mode)
	{
		case MO_DEFAULT:
		case MO_CONSOLE:    //0x01: 主控 (Console)
		{
			c_relay_console_info(proto);
		}
		break;
		
		#if(boardBMS_EN)
		case MO_BMS:        //0x02: BMS模块 (BMS)
		{
			TaskInParam_U u_in_param;
			memcpy(&u_in_param.usTaskInParam, &proto->ucpValidData[1], 2);
			cQueue_AddQueueTask(tpBmsTask, BTI_GET_INFO, u_in_param.usTaskInParam, false);
		}
		break;
		#endif  //boardBMS_EN

		default:
			return -2;
	}
	
	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 写入记忆参数信息  0x82
 * 说明(备注)  : none
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败   
 ************************************************************************************************************************/
s8 c_relay82_write_mem_info(BaikuProtoRx_t* proto)
{
	u8 uc_mode = 0;

	if(proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen < 3)
		return -1;

	uc_mode = proto->ucpValidData[0];

	switch(uc_mode)
	{
		case MO_DEFAULT:
		case MO_CONSOLE:    //0x01: 主控 (Console)
		{
			c_set_console_info(proto);
		}
		break;

		#if(boardBMS_EN)
		case MO_BMS:        //0x02: BMS模块 (BMS)
		{
			u8 uc_result = 0xFF;
			c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &uc_result, 1);
		}
		break;
		#endif  //boardBMS_EN

		default:
		{
			u8 uc_result = 0xFF;
			c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &uc_result, 1);
			return -2;
		}
	}

	return 1;
}

/***********************************************************************************************************************
 * 函数功能    : 设置print状态  0x84
 * 说明(备注)  : none
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay84_set_print_state(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	u8 len = sizeof(uPrint) + 1;
	u8 obj = 0;

	if(proto == NULL || proto->ucValidLen != len || proto->ucpValidData == NULL)
	{
		temp = 0xFF;
		c_print_data_trans(baikuCMD_REPLY_SET_PRINT_STATE, &temp, 1);
		return -10;
	}

	obj = proto->ucpValidData[0];
	if(obj != 0)
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
 * 函数功能    : 获取print状态  0x86
 * 说明(备注)  : 回复命令字为 baikuCMD_REPLY_PRINT_STATE (0x87)
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay86_get_print_state(BaikuProtoRx_t* proto)
{
    if(proto->ucValidLen != 1 || proto->ucpValidData == NULL || proto->ucpValidData[0] != 0)
        return -10;

    #pragma pack(1)
    struct
    {
		u8				ucObj;
		u32				ulPrintFlag;
    } t_frame = {0};
    #pragma pack()

    t_frame.ucObj       = proto->ucpValidData[0];
    t_frame.ulPrintFlag = uPrint.ulFlag;

    return c_print_data_trans(baikuCMD_REPLY_PRINT_STATE, (u8*)&t_frame, sizeof(t_frame));
}

/***********************************************************************************************************************
 * 函数功能    : 系统设置/控制命令  0x88
 * 说明(备注)  : none
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : 1:成功   负数:错误码
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
	if(t_sys_set_param.obj == MO_DEFAULT ||
		t_sys_set_param.obj == MO_CONSOLE)
	{
		temp = 0x00;
		if(c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1) <= 0)
			return -2;
		
		vApp_JumpToBoot(t_sys_set_param.cmd);
	}
	//BMS
	#if(boardBMS_EN)
	else if(t_sys_set_param.obj == MO_BMS)
	{
		//进入升级
		if(t_sys_set_param.cmd == mainUPDATE_FLAG)
		{
			if(cUpdate_ChSelect((ModuleObject_E)t_sys_set_param.obj, CT_PRINT) <= 0)
				return -5;
		}
		//其他设置
		else
		{
			if(tpBmsTask == NULL || tpBmsTask->tReplyBuff.buff == NULL)
				return -3;

			lwrb_reset(&tpBmsTask->tReplyBuff);
			lwrb_write(&tpBmsTask->tReplyBuff, &t_sys_set_param, sizeof(t_sys_set_param));

			if(cQueue_AddQueueTask(tpBmsTask, BTI_REQ_SET_CMD, 0, true) <= 0)
				return -4;
		}
	}
	#endif  //boardBMS_EN

	#if(boardDCAC_EN)
	else if (t_sys_set_param.obj == MO_DCAC ||
			 t_sys_set_param.obj == MO_MGMT_AC ||
			 t_sys_set_param.obj == MO_MGMT_DC)
	{
		//进入升级
		if(t_sys_set_param.cmd == mainUPDATE_FLAG)
		{
			if(cUpdate_ChSelect((ModuleObject_E)t_sys_set_param.obj, CT_PRINT) <= 0)
				return -5;
			
			if(cUpdate_ProtoSelect((ModuleObject_E)t_sys_set_param.obj, PT_BAIKU) <= 0)
				return -7;
		}
		else
		{
			temp = 0xFF;
			if(c_print_data_trans(baikuCMD_REPLY_SYS_SET, &temp, 1) <= 0)
				return -6;
		}
	}
	#endif  //boardDCAC_EN
	
	return 1;
	#else
	return 0;
	#endif   //boardUPDATE
}

/***********************************************************************************************************************
 * 函数功能    : 回复BMS APP信息
 * 说明(备注)  : none
 * 传入参数    : data: 数据指针
 *               len: 数据长度
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_relay_bms_app_info(u8* data, u16 len)
{
	if(len > 256)
		return false;
	
	return c_print_data_trans(baikuCMD_REPLY_MEM_PARAM, data, len);
}

/***********************************************************************************************************************
 * 函数功能    : 回复主控记忆参数与信息  0x81
 * 说明(备注)  : 支持SYS(记忆参数/版本/Boot参数/App参数)、BMS、DCAC、MPPT、USB、Display记忆参数
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : true:成功   false:失败
 ************************************************************************************************************************/
static s8 c_relay_console_info(BaikuProtoRx_t* proto)
{
	u8 uc_mode = 0;
	u8 uc_obj = 0;
	u8 s_uca_index = 0;
	u8 len = 0;
	s8 c_ret = 0;
	static u8 s_uca_buff[256];
	
	if(proto->ucValidLen != 3 || proto->ucpValidData == NULL)
		return false;
	
	uc_mode = proto->ucpValidData[0];
	uc_obj = proto->ucpValidData[1];
	s_uca_index = proto->ucpValidData[2];
	
	if(uc_mode != MO_DEFAULT && uc_mode != MO_CONSOLE)
		return false;
	
	memcpy(s_uca_buff, proto->ucpValidData, 3);
	
	switch(uc_obj)
	{
		case 0x00: // SYS
		{
			switch(s_uca_index)
			{
				case 0x00: // tSysMemParam 记忆参数
				{
					len = sizeof(SysMemParam_T);
					memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tSYS, len);
				}
				break;

				case 0x01: // 版本信息
				{
					len = sizeof(s_uca_buff) - 3;
					if(c_get_console_ver_info(&s_uca_buff[3], &len) == false)
						return false;
				}
				break;

				case 0x02: // tBootMemParam
				{
					len = sizeof(BootMemParam_T);
					memcpy(&s_uca_buff[3], (u8*)&tBootMemParam, len);
				}
				break;

				case 0x03: // tAppVerAndParam
				{
					len = sizeof(AppVerAndParam_T);
					memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tVerInfo, len);
				}
				break;

				default:
					return false;
			}
		}
		break;

		case 0x01: // BMS
		{
			#if (boardBMS_EN)
			if (s_uca_index == 0x00) // tBmsMemParam 记忆参数
			{
				len = sizeof(BmsMemParam_T);
				memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tBMS, len);
			}
			else
				return false;
			#else
			return false;
			#endif  /* boardBMS_EN */
		}
		break;

		case 0x02: // DCAC
		{
			#if (boardDCAC_EN)
			if (s_uca_index == 0x00) // tDcacMemParam 记忆参数
			{
				len = sizeof(DcacMemParam_T);
				memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tDCAC, len);
			}
			else
				return false;
			#else
			return false;
			#endif  /* boardDCAC_EN */
		}
		break;

		case 0x03: // MPPT
		{
			#if (boardMPPT_EN)
			if (s_uca_index == 0x00) // tMpptMemParam 记忆参数
			{
				len = sizeof(MpptMemParam_T);
				memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tMPPT, len);
			}
			else
				return false;
			#else
			return false;
			#endif  /* boardMPPT_EN */
		}
		break;

		case 0x04: // USB
		{
			#if (boardUSB_EN)
			if (s_uca_index == 0x00) // tUsbMemParam 记忆参数
			{
				len = sizeof(UsbMemParam_T);
				memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tUSB, len);
			}
			else
				return false;
			#else
			return false;
			#endif  /* boardUSB_EN */
		}
		break;

		case 0x05: // Display
		{
			#if (boardDISPLAY_EN)
			if (s_uca_index == 0x00) // tDispMemParam 记忆参数
			{
				len = sizeof(DispMemParam_T);
				memcpy(&s_uca_buff[3], (u8*)&tAppMemParam.tDISP, len);
			}
			else
				return false;
			#else
			return false;
			#endif  /* boardDISPLAY_EN */
		}
		break;

		default:
			return false;
	}
	
	c_ret = c_print_data_trans(baikuCMD_REPLY_MEM_PARAM, s_uca_buff, len + 3);
	if(c_ret <= 0)
		return false;

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 写入主控记忆参数并持久化  0x82/0x83
 * 说明(备注)  : 支持SYS(0x00)、BMS(0x01)、DCAC(0x02)、MPPT(0x03)、USB(0x04)、Display(0x05) 记忆参数
 * 传入参数    : proto: 协议结构体指针
 * 输出参数    : none
 * 返回值      : true:成功   false:失败
 ************************************************************************************************************************/
static s8 c_set_console_info(BaikuProtoRx_t* proto)
{
	u8 uc_obj = 0;
	u8 s_uca_index = 0;
	u8 uc_data_len = 0;
	u8 *p_data = NULL;
	u8 uc_result = 0xFF;  /* 0x00: 操作成功, 0xFF: 操作失败 */
	s8 c_ret = 0;

	if (proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen < 3)
	{
		uc_result = 0xFF;
		return c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &uc_result, 1);
	}

	uc_obj = proto->ucpValidData[1];
	s_uca_index = proto->ucpValidData[2];
	uc_data_len = proto->ucValidLen - 3;
	p_data = &proto->ucpValidData[3];

	/* 项目代码固定为 0x00 (记忆参数) */
	if (s_uca_index != 0x00)
	{
		uc_result = 0xFF;
		return c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &uc_result, 1);
	}

	switch (uc_obj)
	{
		case 0x00: /* SYS 系统记忆参数 (7字节) */
		{
			if (uc_data_len == sizeof(SysMemParam_T))
			{
				memcpy((u8*)&tAppMemParam.tSYS, p_data, sizeof(SysMemParam_T));
				if (cApp_UpdateMemParam(tSysMemParamStr) > 0)
				{
					vSys_RefreshOffTime();
					uc_result = 0x00;
				}
			}
		}
		break;

		case 0x01: /* BMS 电池保护记忆参数 (10字节) */
		{
			#if (boardBMS_EN)
			if (uc_data_len == sizeof(BmsMemParam_T))
			{
				memcpy((u8*)&tAppMemParam.tBMS, p_data, sizeof(BmsMemParam_T));
				if (cApp_UpdateMemParam(tBmsMemParamStr) > 0)
					uc_result = 0x00;
			}
			#endif  /* boardBMS_EN */
		}
		break;

		case 0x02: /* DCAC 逆变记忆参数 (25字节) */
		{
			#if (boardDCAC_EN)
			if (uc_data_len == sizeof(DcacMemParam_T))
			{
				memcpy((u8*)&tAppMemParam.tDCAC, p_data, sizeof(DcacMemParam_T));
				if (cApp_UpdateMemParam(tDcacMemParamStr) > 0)
				{
					vDcac_RefreshOffTime();
					uc_result = 0x00;
				}
			}
			#endif  /* boardDCAC_EN */
		}
		break;

		case 0x03: /* MPPT 充电记忆参数 (11字节) */
		{
			#if (boardMPPT_EN)
			if (uc_data_len == sizeof(MpptMemParam_T))
			{
				memcpy((u8*)&tAppMemParam.tMPPT, p_data, sizeof(MpptMemParam_T));
				if (cApp_UpdateMemParam(tMpptMemParamStr) > 0)
					uc_result = 0x00;
			}
			#endif  /* boardMPPT_EN */
		}
		break;

		case 0x04: /* USB 记忆参数 (9字节) */
		{
			#if (boardUSB_EN)
			if (uc_data_len == sizeof(UsbMemParam_T))
			{
				memcpy((u8*)&tAppMemParam.tUSB, p_data, sizeof(UsbMemParam_T));
				if (cApp_UpdateMemParam(tUsbMemParamStr) > 0)
					uc_result = 0x00;
			}
			#endif  /* boardUSB_EN */
		}
		break;

		case 0x05: /* Display 屏幕记忆参数 (4字节) */
		{
			#if (boardDISPLAY_EN)
			if (uc_data_len == sizeof(DispMemParam_T))
			{
				memcpy((u8*)&tAppMemParam.tDISP, p_data, sizeof(DispMemParam_T));
				if (cApp_UpdateMemParam(tDispMemParamStr) > 0)
					uc_result = 0x00;
			}
			#endif  /* boardDISPLAY_EN */
		}
		break;

		default:
		{
			uc_result = 0xFF;
		}
		break;
	}

	c_ret = c_print_data_trans(baikuCMD_REPLY_WRITE_MEM_PARAM, &uc_result, 1);
	if (c_ret <= 0)
		return false;

	return true;
}

/***********************************************************************************************************************
 * 函数功能    : 获取主控版本信息字符串
 * 说明(备注)  : 拼接软件版本号与硬件版本号
 * 传入参数    : data: 存储返回数据的指针
 *               data_len: 传入可用缓冲区最大长度，返回实际写入长度
 * 输出参数    : data, data_len
 * 返回值      : true:成功   false:失败
 ************************************************************************************************************************/
static s8 c_get_console_ver_info(u8* data, u8* data_len)
{
	u8 len = 0;
	u8 char_len = 0;
	const char temp[] = "\r\n";
	const char soft_temp[] = boardSOFTWARE_VERSION;
	const char ver_temp[] = boardHARDWARE_VERSION;
	
	char_len = sizeof(soft_temp);
	if(len + char_len > *data_len) return false;
	memcpy(&data[len], (u8*)soft_temp, char_len);
	len += char_len; 
	
	char_len = sizeof(temp);
	if(len + char_len > *data_len) return false;
	memcpy(&data[len], (u8*)temp, char_len);
	len += char_len;
	
	char_len = sizeof(ver_temp);
	if(len + char_len > *data_len) return false;
	memcpy(&data[len], (u8*)ver_temp, char_len);
	len += char_len;
	
	char_len = sizeof(temp);
	if(len + char_len > *data_len) return false;
	memcpy(&data[len], (u8*)temp, char_len);
	len += char_len;
	
	*data_len = len;
	return true;
}


#if(boardBMS_EN && boardRUN_LOG_EN)
/***********************************************************************************************************************
 * 函数功能    : 解锁运行日志中继 (0x8A)
 * 说明(备注)  : 向上位机校验钥匙并向BMS任务队列添加解锁任务
 * 传入参数    : proto: 协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : 1: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_relay8A_log_unlock(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	if(proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen != 1)
	{
		temp = 0xFF;    //参数错误
		return c_print_data_trans(baikuCMD_REPLY_LOG_UNLOCK, &temp, 1);
	}
	if(proto->ucpValidData[0] != 0x5A)
	{
		temp = 0xFE;    //钥匙错误
		return c_print_data_trans(baikuCMD_REPLY_LOG_UNLOCK, &temp, 1);
	}

	return cQueue_AddQueueTask(tpBmsTask, BTI_LOG_UNLOCK, proto->ucpValidData[0], false);
}

/***********************************************************************************************************************
 * 函数功能    : 获取运行日志中继 (0xB4)
 * 说明(备注)  : 向BMS任务队列添加获取运行日志任务
 * 传入参数    : proto: 协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : 1: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_relayB4_get_run_log(BaikuProtoRx_t* proto)
{
	if(proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen != 2)
		return -1;

	u16 us_num = (u16)proto->ucpValidData[0] | ((u16)proto->ucpValidData[1] << 8);
	return cQueue_AddQueueTask(tpBmsTask, BTI_GET_RUN_LOG, us_num, false);
}

/***********************************************************************************************************************
 * 函数功能    : 读取运行日志指定槽中继 (0xB6)
 * 说明(备注)  : 向BMS任务队列添加读取运行日志槽任务
 * 传入参数    : proto: 协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : 1: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_relayB6_read_run_log_slot(BaikuProtoRx_t* proto)
{
	if(proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen != 2)
		return -1;

	u16 us_slot = (u16)proto->ucpValidData[0] | ((u16)proto->ucpValidData[1] << 8);
	return cQueue_AddQueueTask(tpBmsTask, BTI_READ_RUN_LOG_SLOT, us_slot, false);
}

/***********************************************************************************************************************
 * 函数功能    : 重置运行日志中继 (0xB8)
 * 说明(备注)  : 向上位机校验钥匙并向BMS任务队列添加重置运行日志任务
 * 传入参数    : proto: 协议接收结构体指针
 * 输出参数    : 无
 * 返回值      : 1: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_relayB8_reset_run_log(BaikuProtoRx_t* proto)
{
	u8 temp = 0;
	if(proto == NULL || proto->ucpValidData == NULL || proto->ucValidLen != 1)
	{
		temp = 0xFF;    //参数错误
		return c_print_data_trans(baikuCMD_REPLY_RESET_RUN_LOG, &temp, 1);
	}
	if(proto->ucpValidData[0] != 0x5A)
	{
		temp = 0xFE;    //钥匙错误
		return c_print_data_trans(baikuCMD_REPLY_RESET_RUN_LOG, &temp, 1);
	}

	return cQueue_AddQueueTask(tpBmsTask, BTI_RESET_RUN_LOG, proto->ucpValidData[0], false);
}

/***********************************************************************************************************************
 * 函数功能    : 回复解锁运行日志 (0x8B)
 * 说明(备注)  : 向上位机发送日志解锁结果响应帧
 * 传入参数    : res: 解锁结果码
 * 输出参数    : 无
 * 返回值      : >0: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_print_cs_reply_log_unlock(u8 res)
{
	return c_print_data_trans(baikuCMD_REPLY_LOG_UNLOCK, &res, 1);
}

/***********************************************************************************************************************
 * 函数功能    : 回复读取运行日志指定槽 (0xB7)
 * 说明(备注)  : data包含: 结果码(1B) + 槽号(2B) + 记录(46B), 共49字节
 * 传入参数    : data: 数据指针, len: 数据长度
 * 输出参数    : 无
 * 返回值      : >0: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_print_cs_reply_run_log_slot(u8* data, u8 len)
{
	return c_print_data_trans(baikuCMD_REPLY_RUN_LOG_SLOT, data, len);
}

/***********************************************************************************************************************
 * 函数功能    : 回复重置运行日志 (0xB9)
 * 说明(备注)  : 向上位机发送日志重置结果响应帧
 * 传入参数    : res: 重置结果码
 * 输出参数    : 无
 * 返回值      : >0: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_print_cs_reply_reset_run_log(u8 res)
{
	return c_print_data_trans(baikuCMD_REPLY_RESET_RUN_LOG, &res, 1);
}

/***********************************************************************************************************************
 * 函数功能    : 发送单条运行日志记录帧 (0xB5)
 * 说明(备注)  : 向上位机逐条上报运行日志记录数据
 * 传入参数    : rec_data: 记录数据指针, len: 数据长度
 * 输出参数    : 无
 * 返回值      : >0: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_print_cs_send_run_log_rec(u8* rec_data, u8 len)
{
	return c_print_data_trans(baikuCMD_REPLY_RUN_LOG, rec_data, len);
}

/***********************************************************************************************************************
 * 函数功能    : 发送运行日志结束帧 (0xB5)
 * 说明(备注)  : 0字节空负载表示日志上报结束
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : >0: 成功, <=0: 失败
 ************************************************************************************************************************/
s8 c_print_cs_send_run_log_end(void)
{
	return c_print_data_trans(baikuCMD_REPLY_RUN_LOG, NULL, 0);
}
#endif  //boardBMS_EN && boardRUN_LOG_EN

/***********************************************************************************************************************
 * 函数功能    : 回复设置升级协议  0xC3
 * 说明(备注)  : none
 * 传入参数    : data: 协议数据
 *               len: 数据长度
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
#if(boardUPDATE)
s8 c_print_cs_C3_reply_set_proto(u8* data, u8 len)
{
	s8 c_ret = 0;

	if(data == NULL || len == 0)
		return -1;

	c_ret = c_print_data_trans(baikuCMD_REPLY_SET_PROTO, data, len);
	if(c_ret <= 0)
		vUpdate_ResetRecTimeout(true);
	return c_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 请求开始发送  0xC4
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_print_cs_C4_req_start_send(void)
{
	s8 c_ret = c_print_data_trans_for_update(baikuCMD_RRQ_START_SEND, NULL, 0);
	return c_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 请求重发当前帧  0xC4
 * 说明(备注)  : 升级数据阶段，收到错误包或等待超时时，使用 C4 要求上位机重发当前帧
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_print_cs_C4_req_resend_curr(void)
{
	s8 c_ret = c_print_data_trans_for_update(baikuCMD_RRQ_START_SEND, NULL, 0);
	return c_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 请求继续发送  0xC6
 * 说明(备注)  : C6 用于请求上位机继续发送下一帧数据，不携带载荷
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:发送成功   false:发送失败
 ************************************************************************************************************************/
s8 c_print_cs_C6_req_cont_send(void)
{
	s8 c_ret = c_print_data_trans_for_update(baikuCMD_RRQ_CONT_SEND, NULL, 0);
	if(c_ret <= 0)
		vUpdate_ResetRecTimeout(true);
	return c_ret;
}

/***********************************************************************************************************************
 * 函数功能    : 指令:取消发送 C8
 * 说明(备注)  : none
 * 传入参数    : none
 * 输出参数    : none
 * 返回值      : true:成功   false:失败
 ************************************************************************************************************************/
s8 c_print_cs_C8_trans_cancel(void)
{
	s8 c_ret = c_print_data_trans_for_update(baikuCMD_REPLY_CANEL, NULL, 0);
	if(c_ret <= 0)
		vUpdate_ResetRecTimeout(true);
	return c_ret;
}
#endif  //boardUPDATE


/***********************************************************************************************************************
 * 函数功能    : 数据传输
 * 说明(备注)  : none
 * 传入参数    : cmd: 指令
 *               data: 指向数据指针
 *               len: 数据的长度
 * 输出参数    : none
 * 返回值      : -1: 写入的Len超出最大长度, -2: 等待回复超时, -3: 数据发送错误, 0: 无操作, 1: 操作成功
 ************************************************************************************************************************/
static s8 c_print_data_trans(u8 cmd, u8 *data, u8 len)
{
	s8 result = 0;
	
	if (tpPrintProtoTx == NULL)
		return 0;
	
	#if (boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if (result > 0)
	{
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
		return bPrint_SendDataToUsart();
	}
	#endif  /* boardPRINT_IFACE */
	return result;
}

/***********************************************************************************************************************
 * 函数功能    : 升级数据传输
 * 说明(备注)  : none
 * 传入参数    : cmd: 指令
 *               data: 指向数据指针
 *               len: 数据的长度
 * 输出参数    : none
 * 返回值      : -1:写入的Len超出最大长度   -2:等会回复超时   -3:数据发送错误   0:无操作   1:操作成功
 ************************************************************************************************************************/
#if(boardUPDATE)
static s8 c_print_data_trans_for_update(u8 cmd, u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpPrintProtoTx == NULL)
		return 0;

	#if(boardUSE_OS)
	if(PrintSemaphoreBinary == NULL)
		return -1;
	#endif  //boardUSE_OS
	
	if(bPrint_CheckSendFinish() == false)
		return -2;
	
	#if(boardPRINT_IFACE)
	result = cBaiku_ProtoCreate(tpPrintProtoTx, cmd, data, len);
	if(result > 0)
	{
		lwrb_reset(&tPrintTxBuff);
		lwrb_write(&tPrintTxBuff, tpPrintProtoTx->ucaFrameData, tpPrintProtoTx->ucFrameLen);
		#if(boardUSE_OS)
		if(xSemaphoreTake(PrintSemaphoreBinary, ( TickType_t) 0 ) == pdPASS) 
		#endif  //boardUSE_OS
		{
			if(bPrint_DataSendStart(tpPrintProtoTx->ucFrameLen) == false)
				result = -3;
			
			//释放信号量
			#if (boardUSE_OS)
			xSemaphoreGive(PrintSemaphoreBinary);
			#endif  /* boardUSE_OS */
		}
	}
	#endif  /* boardPRINT_IFACE */
	return result;
}

/***********************************************************************************************************************
 * 函数功能    : 信息传输
 * 说明(备注)  : none
 * 传入参数    : data: 指向数据指针
 *               len: 数据的长度
 * 输出参数    : none
 * 返回值      : -1:写入的Len超出最大长度   -2:等会回复超时   -3:数据发送错误   0:无操作   1:操作成功
 ************************************************************************************************************************/
s8 c_print_info_trans(u8* data, u8 len)
{
	s8 result = 0;
	
	if(tpPrintProtoTx == NULL)
		return 0;

	#if(boardUSE_OS)
	if(PrintSemaphoreBinary == NULL)
		return -1;
	#endif  //boardUSE_OS
	
	if(bPrint_CheckSendFinish() == false)
		return -2;

	lwrb_reset(&tPrintTxBuff);
	lwrb_write(&tPrintTxBuff, data, len);
	#if(boardUSE_OS)
	if(xSemaphoreTake(PrintSemaphoreBinary, ( TickType_t) 0 ) == pdPASS) 
	#endif  //boardUSE_OS
	{
		if(bPrint_DataSendStart(len) == false)
			result = -3;
		
		//释放信号量
		#if (boardUSE_OS)
		xSemaphoreGive(PrintSemaphoreBinary);
		#endif  /* boardUSE_OS */
	}
	return result;
}
#endif  //boardUPDATE

#endif  //boardPRINT_IFACE
