/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\15-M50\1.software\M5004-3\APP\Hardware\Key
 * File    : key_func_eng.c
 * Date    : 2026-03-18 15:10:03
 * Author  : LJD(291483914@qq.com)
 * Desc    : description
 * -------------------------------------------------------
 * todo    :
 * 1.
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
*******************************************************************************************************************************/


//****************************************************Includes******************************************************************//
#include "key_func_eng.h"

#if(boardENG_MODE_EN)
#include "Key/key_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task_eng.h"

#include "function.h"

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

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

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#endif  //DCAC使能

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  //boardMPPT_EN

//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//
//工程模式按键功能
enum
{			
	EMKF_NULL = 0, 		//空按键
	EMKF_CHARGE,   		//改变
	EMKF_ADD,      		//增加
	EMKF_REDUCE,   		//减少
	EMKF_NEXT_OBJ,  	//下一项
	EMKF_COMFIRM,  		//确认
}eEngModeKeyFunc;
//工程模式按键类型
u8 const KeyTriType_SetBuff[ 3 ]    = { KTE_AC_LONG, KTE_POWER_LONG, KTE_FUN_NULL};  //进入
u8 const KeyTriType_Set1Buff[ 3 ]   = { KTE_POWER_LONG, KTE_AC_LONG, KTE_FUN_NULL};
u8 const KeyTriType_NextOptionBuff[ 3 ]= { KTE_POWER_SHORT,KTE_POWER_SHORT, KTE_FUN_NULL};  //对象
u8 const KeyTriType_ChargeBuff[ 2 ] = { KTE_POWER_SHORT, KTE_FUN_NULL};  		//项目
u8 const KeyTriType_AddBuff[ 2 ]    = { KTE_AC_SHORT, KTE_FUN_NULL};    	//增加
u8 const KeyTriType_ReduceBuff[ 2 ] = { KTE_DC_SHORT, KTE_FUN_NULL};    	//减少
u8 const KeyTriType_ComfirmBuff[ 2 ]= { KTE_POWER_LONG, KTE_FUN_NULL};      //确认
u8 const KeyTriType_NullBuff[ 2 ]= { KTE_LIGHT_SHORT, KTE_FUN_NULL};      	//空闲


//****************************************************Function Declaration****************************************************//
static bool eng_mode_key_deal(u8 func);





void v_key_func_eng(u8* pKeyTriTypeBuff)
{
	if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ChargeBuff, sizeof(KeyTriType_ChargeBuff)) )   //单击电源键
	{
		eEngModeKeyFunc = EMKF_CHARGE;

		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif  //boardBUZ_EN

		if(uPrint.tFlag.bKeyTask)
			sMyPrint("项目选择\r\n");
		
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_AddBuff, sizeof(KeyTriType_AddBuff)) )  //AC键短按  +
	{
		eEngModeKeyFunc = EMKF_ADD;

		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif  //boardBUZ_EN

		if(uPrint.tFlag.bKeyTask)
			sMyPrint("增加\r\n");
		
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ReduceBuff, sizeof(KeyTriType_ReduceBuff))) //USB键短按  -
	{
		eEngModeKeyFunc = EMKF_REDUCE;

		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif  //boardBUZ_EN

		if(uPrint.tFlag.bKeyTask)
			sMyPrint("减少\r\n");
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_NextOptionBuff, sizeof(KeyTriType_NextOptionBuff))) //LIGHT键短按
	{
		eEngModeKeyFunc = EMKF_NEXT_OBJ;

		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif  //boardBUZ_EN

		if(uPrint.tFlag.bKeyTask)
			sMyPrint("对象选择\r\n");
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_ComfirmBuff, sizeof(KeyTriType_ComfirmBuff))) //LIGHT键长按
	{
		eEngModeKeyFunc = EMKF_COMFIRM;

		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif  //boardBUZ_EN
		
		if(uPrint.tFlag.bKeyTask)
			sMyPrint("确认\r\n");
	}
	else 
	{
		eEngModeKeyFunc = EMKF_NULL;
		if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_NullBuff, sizeof(KeyTriType_NullBuff)))
		{
			#if(boardBUZ_EN)
			bBuz_Tweet(SHORT_1);
			#endif  //boardBUZ_EN
		}
	}
	
	if(eEngModeKeyFunc != EMKF_NULL)
		eng_mode_key_deal(eEngModeKeyFunc);
}


/***********************************************************************************************************************
-----函数功能    工程模式按键功能
-----说明(备注)  none
-----传入参数    按键功能
				 EMKF_NULL = 0, //空按键
				 EMKF_CHARGE,   //改变
				 EMKF_ADD,      //增加
				 EMKF_REDUCE,   //减少
				 EMKF_COMFIRM,  //确认
-----输出参数    none
-----返回值      true:允许  false:不允许
************************************************************************************************************************/
static bool eng_mode_key_deal(u8 func)
{
	vu8 uc_item_num = 0;
	//累加设置
//	if(step ==EMS_TIME)
//	{
//		tKeyAC.bEnLongPressAdd = true;
//		tKeyLight.bEnLongPressAdd = true;
//	}
//	else 
//	{
//		tKeyAC.bEnLongPressAdd = false;
//		tKeyLight.bEnLongPressAdd = false;
//	}
	
	vEng_RefreshEngModeTime();
	
	switch(tpSysTask->ucStep)
	{
		case EMS_INIT:
		{
			uc_item_num = 0;
			
			if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_系统设置-------\r\n");
			}
			
		}break;
		
		case EMS_SYS:
		{
			uc_item_num = 7;
			
			if(func == EMKF_ADD)
			{
				tEngMode.cEngModeState = 1;
				if(tEngMode.ucEngModeItem >= 2)
					vSys_MemParamSet(tEngMode.ucEngModeItem,true);

			}
			else if(func == EMKF_REDUCE)
			{
				tEngMode.cEngModeState = 0;
				if(tEngMode.ucEngModeItem >= 2)
					vSys_MemParamSet(tEngMode.ucEngModeItem,false);
			}
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_LCD设置-------\r\n");
			}
		}break;
		
		#if(boardDISPLAY_EN)
		case EMS_LCD:
		{
			uc_item_num = 3;
			
			if(func == EMKF_ADD)
				vDisp_MemParamSet(true);
			else if(func == EMKF_REDUCE)
				vDisp_MemParamSet(false);
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_电池设置-------\r\n");
			}
		}break;
		#endif
		
		#if(boardBMS_EN)
		case EMS_BAT:
		{
			uc_item_num = 6;
			
			if(func == EMKF_ADD)
				vBms_MemParamSet(tEngMode.ucEngModeItem,true);
			else if(func == EMKF_REDUCE)
				vBms_MemParamSet(tEngMode.ucEngModeItem,false);
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_MPPT设置-------\r\n");
			}
			
		}break;
		#endif
		
		#if(boardMPPT_EN)
		case EMS_MPPT:
		{
			uc_item_num = 5;
			
			if(func == EMKF_ADD)
				vMppt_MemParamSet(tEngMode.ucEngModeItem,true);
			else if(func == EMKF_REDUCE)
				vMppt_MemParamSet(tEngMode.ucEngModeItem,false);
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_DCAC设置-------\r\n");
			}
			
		}break;
		#endif
		
		#if(boardDCAC_EN)
		case EMS_DCAC:
		{
			uc_item_num = 13;
			
			if(func == EMKF_ADD)
				vDcac_MemParamSet(tEngMode.ucEngModeItem,true);
			else if(func == EMKF_REDUCE)
				vDcac_MemParamSet(tEngMode.ucEngModeItem,false);
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_ADC采样-------\r\n");
			}
			
		}break;
		#endif
		
		#if(boardADC_EN)
		case EMS_ADC:
		{
			uc_item_num = 3;
			
			if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_USB-------\r\n");
			}
		}break;
		#endif
		
		#if(boardUSB_EN)
		case EMS_USB:
		{
			uc_item_num = 5;
			
			if(func == EMKF_ADD)
				vUsb_MemParamSet(tEngMode.ucEngModeItem,true);
			else if(func == EMKF_REDUCE)
				vUsb_MemParamSet(tEngMode.ucEngModeItem,false);
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_AD-------\r\n");
			}
		}break;
		#endif
		
		#if(boardDC_EN)
		case EMS_DC:
		{
			uc_item_num = 6;
			
			if(func == EMKF_ADD)
				vDc_MemParamSet(tEngMode.ucEngModeItem,true);
			else if(func == EMKF_REDUCE)
				vDc_MemParamSet(tEngMode.ucEngModeItem,false);
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_照明-------\r\n");
			}
		}break;
		#endif
		
		#if(boardLIGHT_EN)
		case EMS_LIGHT:
		{
			uc_item_num = 0;
			
			if(func == EMKF_ADD)
				;
			else if(func == EMKF_REDUCE)
				;
			else if(func == EMKF_NEXT_OBJ)
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_设置-------\r\n");
			}
		}break;
		#endif
		
		case EMS_SET:
		{
			uc_item_num = 3;
			
			if(func == EMKF_NEXT_OBJ)  //下一项
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_完成-------\r\n");
			}
			else if(func == EMKF_ADD)
				tEngMode.cEngModeState = 1;
			else if(func == EMKF_REDUCE)
				tEngMode.cEngModeState = 0;
			
		}break;
		
		default:
		case EMS_FINISH:
		{
			uc_item_num = 0;
			
			if(func == EMKF_NEXT_OBJ)  //下一项
			{
				if(uPrint.tFlag.bKeyTask)
					sMyPrint("Key_Task:-------工程模式_初始化设置-------\r\n");
			}
			else if(func == EMKF_ADD)
				tEngMode.cEngModeState = 1;
			else if(func == EMKF_REDUCE)
				tEngMode.cEngModeState = 0;
			
		}break;
	}
	
	//下一个对象
	if(func == EMKF_NEXT_OBJ)
	{
		tEngMode.ucEngModeItem = 0;
		tEngMode.cEngModeState = -1;
		
		if(tpSysTask->ucStep >= EMS_FINISH)
			cQueue_GotoStep(tpSysTask, EMS_INIT);
		else 
			cQueue_GotoStep(tpSysTask, STEP_NEXT );  //下一步
	}
	//保存退出
	else if(func == EMKF_COMFIRM)
	{
		cSys_Switch(SO_KEY, ST_ON, false);      //开机
	}
	else if(func == EMKF_CHARGE)
	{
		tEngMode.cEngModeState = -1;
		tEngMode.ucEngModeItem++;
		if(tEngMode.ucEngModeItem >= uc_item_num)
		{
			tEngMode.ucEngModeItem = 0;
		}
	}

	return true;
}

#endif  //boardENG_MODE_EN
