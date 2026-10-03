/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_task.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 充电控制任务与参数管理
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_task.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_queue_task.h"
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_iface.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Adc/adc_task.h"

#include "app_info.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardUPDATE)
#include "Sys/sys_queue_task_update.h"
#endif  /* boardUPDATE */

//****************************************************Task Declaration**********************************************************//
#if (boardUSE_OS)
#define			MPPT_TASK_PRIO							3		/* 任务优先级(通信执行层) */
#define			MPPT_TASK_SIZE							192		/* 任务堆栈(字) */
TaskHandle_t tMpptTaskHandler = NULL;							/* 任务句柄 */
void         vMppt_Task(void *pvParameters);					/* 任务函数 */
#endif  /* boardUSE_OS */


//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) Mppt_T tMppt;
static Task_T      *p_task = NULL;

vu16 usDcInVolt = 0;//0.1V
vu16 usXT60InVolt = 0;//0.1V

/* MPPT 记忆参数步进配置表 */
typedef struct
{
	void				*pParam;
	int16_t				sMin;
	int16_t				sMax;
	uint8_t				ucType;				/* 0: uint16_t, 1: int8_t */
}MpptParamStep_T;

static const MpptParamStep_T s_tMpptParamTable[] = 
{
    {(void*)&tAppMemParam.tMPPT.cAllowMaxTemp, -127,  127,   1},
    {(void*)&tAppMemParam.tMPPT.usAutoOffTime,  0,    3600,  0},
    {(void*)&tAppMemParam.tMPPT.usMaxInVolt,    0,    30000, 0},
    {(void*)&tAppMemParam.tMPPT.usMinInVolt,    0,    30000, 0},
    {(void*)&tAppMemParam.tMPPT.usMaxInCurr,    0,    30000, 0},
    {(void*)&tAppMemParam.tMPPT.usInPwrRating,  0,    30000, 0},
};

//****************************************************Function Declaration******************************************************//
static bool b_mppt_update_dev_state(void);
static void v_mppt_param_update(void);
static void v_mppt_auto_switch_chg_source(void);

/***********************************************************************************************************************
 * 函数功能    : 任务参数初始化
 * 说明(备注)  : 复位 MPPT 运行结构体与队列
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功, false: 失败
 ************************************************************************************************************************/
static bool b_mppt_task_param_init(void)
{
    if (tpMpptTask == NULL)
        return false;

    memset(&tMppt, 0, sizeof(tMppt));
    lwrb_reset(&tpMpptTask->tQueueBuff);
    p_task = tpMpptTask;

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : MPPT 任务初始化
 * 说明(备注)  : 初始化协议、队列与 OS 任务
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 成功, -1: 发送协议初始化失败, -2: 队列初始化失败, -3: 参数初始化失败, -4: 任务创建失败
 ************************************************************************************************************************/
s8 cMppt_TaskInit(void)
{
	//接口初始化
	vMppt_IfaceInit();

    if (bMppt_SendProtInit() == false)
        return -1;

    if (bMppt_QueueInit() == false)
        return -2;

    if (b_mppt_task_param_init() == false)
        return -3;

    #if (boardUSE_OS)
    if (xTaskCreate((TaskFunction_t )vMppt_Task,
                    (const char*    )"MpptTask",
                    (uint16_t       )MPPT_TASK_SIZE,
                    (void*          )NULL,
                    (UBaseType_t    )MPPT_TASK_PRIO,
                    (TaskHandle_t*  )&tMpptTaskHandler) != pdPASS)
        return -4;
    vQueue_BindTaskHandler(tpMpptTask, tMpptTaskHandler);
    #endif  /* boardUSE_OS */

    return 1;
}

/***********************************************************************************************************************
 * 函数功能    : MPPT 任务主循环
 * 说明(备注)  : 轮询队列任务
 * 传入参数    : pvParameters: 任务入参
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_Task(void *pvParameters)
{
    #if (boardUSE_OS)
    for (;;)
    #endif  /* boardUSE_OS */
    {
        if (p_task == NULL
            #if (boardUPDATE)
            || (tSysInfo.eDevState == DS_UPDATE_MODE && tUpdate.eObj != MO_MPPT)
            #endif  /* boardUPDATE */
        )
        {
            if (p_task == NULL)
                b_mppt_task_param_init();

            #if (boardUSE_OS)
            vTaskDelay(mpptTASK_CYCLE_TIME);
            continue;
            #else
            return;
            #endif  /* boardUSE_OS */
        }

        vQueue_TaskPoll(p_task, mpptTASK_CYCLE_TIME);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 更新设备状态
 * 说明(备注)  : 根据当前故障码切换关闭、错误或丢失状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
static bool b_mppt_update_dev_state(void)
{
    if (tMppt.uErrCode.ulCode == 0)
    {
        if (tMppt.eDevState == DS_LOST || tMppt.eDevState == DS_ERR)
        {
            bMppt_SetDevState(DS_SHUT_DOWN);
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:错误清除,设置MPPT为关闭状态\r\n");
        }
    }
    else
    {
        if (tMppt.uErrCode.tCode.bDevLost)
        {
            if (tMppt.eDevState != DS_LOST)
                bMppt_SetDevState(DS_LOST);
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:存在错误,设置MPPT为丢失状态\r\n");
        }
        else if (tMppt.eDevState != DS_ERR)
        {
            bMppt_SetDevState(DS_ERR);
            if (uPrint.tFlag.bMpptTask)
                sMyPrint("bMpptTask:存在错误,设置MPPT为错误状态\r\n");
        }
    }
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 更新参数
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_mppt_param_update(void)
{
	usDcInVolt = tAdcSamp.usDcIn1Volt;//0.1V
	usXT60InVolt = tAdcSamp.usDcIn2Volt;//0.1V

	v_mppt_auto_switch_chg_source();
}

/***********************************************************************************************************************
 * 函数功能    : 自动切换充电源
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_mppt_auto_switch_chg_source(void)
{
    static uint32_t s_ul_xt60_start_tick = 0;
    static uint32_t s_ul_dc_start_tick = 0;

    // 都超过最大输入电压 → 全部关闭
    if(usDcInVolt > tAppMemParam.tMPPT.usMaxInVolt || usXT60InVolt > tAppMemParam.tMPPT.usMaxInVolt){
        mpptGPIO_DC_EN_OFF();
        mpptGPIO_XT60_EN_OFF();
        tMppt.eWorkMode = MWM_NULL;
        bMppt_SetErrCode( MEC_MPPT_IN_OV, true );
        s_ul_xt60_start_tick = 0;
        s_ul_dc_start_tick = 0;
        return;
    }
    
	if(tMppt.uErrCode.tCode.bMpptInOV == true)
    	bMppt_SetErrCode( MEC_MPPT_IN_OV, false );
    
    // PV 优先逻辑
    if(tMppt.eWorkMode == MWM_NULL) {
        if(usXT60InVolt > tAppMemParam.tMPPT.usMinInVolt)
        {
			s_ul_xt60_start_tick++;
			s_ul_dc_start_tick = 0;
			if(s_ul_xt60_start_tick > (4000 / mpptTASK_CYCLE_TIME))
            {
				tMppt.eWorkMode = MWM_PV;
				mpptGPIO_DC_EN_OFF();
				mpptGPIO_XT60_EN_ON();
				s_ul_xt60_start_tick = 0;
			}
        }
        else if(usDcInVolt > tAppMemParam.tMPPT.usMinInVolt)
        {
			s_ul_dc_start_tick++;
			s_ul_xt60_start_tick = 0;
			if(s_ul_dc_start_tick > (4000 / mpptTASK_CYCLE_TIME))
            {
				tMppt.eWorkMode = MWM_DC;
				mpptGPIO_DC_EN_ON();	
				mpptGPIO_XT60_EN_OFF();
				s_ul_dc_start_tick = 0;
			}
        }
        else
        {
            s_ul_xt60_start_tick = 0;
            s_ul_dc_start_tick = 0;
        }
    } else if(tMppt.eWorkMode == MWM_PV) {
        s_ul_xt60_start_tick = 0;
        s_ul_dc_start_tick = 0;
        if(usXT60InVolt < (tAppMemParam.tMPPT.usMinInVolt)) {
            tMppt.eWorkMode = MWM_NULL;
            mpptGPIO_DC_EN_OFF();
            mpptGPIO_XT60_EN_OFF();
        }
    } else if(tMppt.eWorkMode == MWM_DC) {
        s_ul_xt60_start_tick = 0;
        s_ul_dc_start_tick = 0;
        if(usDcInVolt < (tAppMemParam.tMPPT.usMinInVolt)) {
            tMppt.eWorkMode = MWM_NULL;
            mpptGPIO_DC_EN_OFF();
            mpptGPIO_XT60_EN_OFF();
        }
    }
}





























/***********************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/
/***********************************************************************************************************************
 * 函数功能    : 设置设备状态
 * 说明(备注)  : 联动系统模块在线状态位
 * 传入参数    : state: 目标状态
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bMppt_SetDevState(DevState_E state)
{
    if (tMppt.eDevState != state)
    {
        if (state == DS_LOST)
        {
            #if (boardSYS_DATA_UPADATA)
            STAT_CLR(tSysInfo.Mod_Exist, OL_MPPT);
            #endif  /* boardSYS_DATA_UPADATA */
        }
        else
        {
            #if (boardSYS_DATA_UPADATA)
            STAT_SET(tSysInfo.Mod_Exist, OL_MPPT);
            #endif  /* boardSYS_DATA_UPADATA */
        }
    }

    tMppt.eDevState = state;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置设备错误代码
 * 说明(备注)  : 设置/清除故障位，驱动报警并更新设备状态
 * 传入参数    : code: 错误码; set: true-设置, false-清除
 * 输出参数    : 无
 * 返回值      : true: 发生致命错误; false: 正常
 ************************************************************************************************************************/
bool bMppt_SetErrCode(MpptErrCode_E code, bool set)
{
    static MpptErrCode_E e_next_code;
    static bool          b_next_set;

    if (tSysInfo.uInit.tFinish.bIF_MpptTask == 0 && set == true)
    {
        if (uPrint.tFlag.bMpptTask)
            log_w("bMpptTask:MPPT模块未初始化完成，不允许标记错误%d", code);
        return false;
    }

    if (code == MEC_SYS_DEV_LOST)
    {
        if (set == false && tMppt.uErrCode.tCode.bDevLost == 0)
        {
            b_mppt_task_param_init();
            return false;
        }
    }

    if (uPrint.tFlag.bMpptTask || uPrint.tFlag.bImportant)
    {
        if (e_next_code != code || b_next_set != set)
        {
            log_e("bMpptTask:任务错误 代码%d 类型%d", code, set);
            e_next_code = code;
            b_next_set  = set;
        }
    }

    if (code > MEC_CLEAR_ALL)
    {
        if (code == MEC_SYS_DEV_LOST)
        {
            tMppt.uErrCode.ulCode   = 0;
            tMpptRx.uErrCode.usCode = 0;
            if (set)
            {
                bMppt_SetChgPerm(false);
                b_mppt_task_param_init();
                ERR_SET(tMppt.uErrCode.ulCode, (code - 1));
            }
        }
        else
        {
            if (set)
                ERR_SET(tMppt.uErrCode.ulCode, (code - 1));
            else
                ERR_CLR(tMppt.uErrCode.ulCode, (code - 1));
        }
    }
    else
    {
        tMppt.uErrCode.ulCode   = 0;
        tMpptRx.uErrCode.usCode = 0;
    }

    if (tMppt.uErrCode.ulCode)
    {
        if (tMppt.eDevState != DS_ERR && tMppt.eDevState != DS_LOST)
        {
            #if (boardBUZ_EN)
            bBuz_Tweet(LONG_3);
            #endif  /* boardBUZ_EN */
            return true;
        }
    }

    b_mppt_update_dev_state();
    return false;
}

/***********************************************************************************************************************
 * 函数功能    : 设置充电功率
 * 说明(备注)  : 更新系统目标 MPPT 充电功率，由主轮询任务自动下发执行
 * 传入参数    : pwr: 功率值 (W)
 * 输出参数    : 无
 * 返回值      : 1: 操作成功
 ************************************************************************************************************************/
s8 cMppt_SetChgPwr(u16 pwr)
{
	s8 result = 1;
	// //要求打开时候,设备处于丢失
    // if(tMppt.eDevState == DS_LOST && pwr > 0)                            
    // {    
    //     bBuz_Tweet(LONG_2);
    //     return -2;
    // }
	
	// if(uPrint.tFlag.bMpptTask)
	// 	sMyPrint("bMpptTask:添加设置充电功率%dW任务\r\n",pwr);
	
	// result = cQueue_AddQueueTask(tpMpptTask, MTI_SET_CHG_PWR,pwr,false);//打开
		
	// if((pwr > 0 && tMpptRx.usMaxInPwr == 0) ||
	// 	(pwr == 0 && tMpptRx.usMaxInPwr > 0))
	// {
	// 	bBuz_Tweet(LONG_1);
		
	// 	bDisp_Switch(ST_ON, false);
	// }
	
	// if(pwr == 0 && tMppt.uErrCode.ulCode)
	// 	bMppt_SetErrCode(MEC_CLEAR_ALL,true);  //清除所有错误
    
	// #if(boardSYS_DATA_UPADATA)
	// Sys_Update_Mod(MPPT_Mod,true );
	// #endif
	
    return result;
}

/***********************************************************************************************************************
 * 函数功能    : 设置充电许可
 * 说明(备注)  : 当禁止充电时立即将功率归零
 * 传入参数    : en: true-允许, false-禁止
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bMppt_SetChgPerm(bool en)
{
    if (tMppt.bChgPerm != en)
    {
        if (en == false)
            cMppt_SetChgPwr(0);
        tMppt.bChgPerm = en;
    }
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化参数
 * 说明(备注)  : 载入板级配置参数默认值
 * 传入参数    : p_mppt_mem: MPPT 记忆参数结构体指针
 * 输出参数    : 无
 * 返回值      : true: 成功
 ************************************************************************************************************************/
bool bMppt_MemParamInit(MpptMemParam_T *p_mppt_mem)
{
    p_mppt_mem->cAllowMaxTemp = boardMPPT_MAX_TEMP;
    p_mppt_mem->usAutoOffTime = boardMPPT_OFF_TIME;
    p_mppt_mem->usMaxInVolt   = boardMPPT_MAX_IN_VOLT;
    p_mppt_mem->usMinInVolt   = boardMPPT_MIN_IN_VOLT;
    p_mppt_mem->usMaxInCurr   = boardMPPT_MAX_IN_CURR;
    p_mppt_mem->usInPwrRating = boardMPPT_IN_PWR_RATING;
    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 设置记忆参数 (表驱动精简版)
 * 说明(备注)  : 支持 uint16 与 int8 类型，自动进行安全上下限防越界检查
 * 传入参数    : item: 参数索引; add: true-增加, false-减少
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vMppt_MemParamSet(u8 item, bool add)
{
    if (item >= mainARRAY_SIZE(s_tMpptParamTable))
        return;

    const MpptParamStep_T *p = &s_tMpptParamTable[item];
    if (p->ucType == 1)
    {
        int8_t *p_val = (int8_t*)p->pParam;
        if (add && *p_val < (int8_t)p->sMax)
            (*p_val)++;
        else if (!add && *p_val > (int8_t)p->sMin)
            (*p_val)--;
    }
    else
    {
        uint16_t *p_val = (uint16_t*)p->pParam;
        if (add && *p_val < (uint16_t)p->sMax)
            (*p_val)++;
        else if (!add && *p_val > (uint16_t)p->sMin)
            (*p_val)--;
    }
}

#endif  /* boardMPPT_EN */

