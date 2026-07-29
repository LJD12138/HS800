/*******************************************************************************************************************************
 * Project : ProjectTeam
 * Module  : G:\1-Baiku_Projects\15-M50\1.software\M5004-3\APP\Hardware\Key
 * File    : key_func_eng.c
 * Date    : 2026-03-18 15:10:03
 * Author  : LJD(291483914@qq.com)
 * Desc    : 工程模式按键处理 - TFT+LVGL版本
 *           按键映射:
 *             LIGHT_SHORT -> KeyUp    (上移/增值)
 *             USB_SHORT   -> KeyDown  (下移/减值)
 *             POWER_SHORT -> KeyEnter (确认/选择)
 *             DC_SHORT    -> KeyLeft  (左切Tab)
 *             AC_SHORT    -> KeyRight (右切Tab)
 *             DC_LONG     -> KeyBack  (返回/退出)
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

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/user_ui/eng_mode_ui.h"
#endif  //boardDISPLAY_EN


//****************************************************Macros*******************************************************************//



//****************************************************Parameter Initialization************************************************//
/* TFT+LVGL 按键映射 */
u8 const KeyTriType_UpBuff[ 2 ]     = { KTE_LIGHT_SHORT, KTE_FUN_NULL};         //上/增
u8 const KeyTriType_DownBuff[ 2 ]   = { KTE_USB_SHORT, KTE_FUN_NULL};          	//下/减
u8 const KeyTriType_EnterBuff[ 2 ]  = { KTE_POWER_SHORT, KTE_FUN_NULL};       	//确认
u8 const KeyTriType_RightBuff[ 2 ]  = { KTE_AC_SHORT, KTE_FUN_NULL};        	//右切Tab
u8 const KeyTriType_LeftBuff[ 2 ]   = { KTE_DC_SHORT, KTE_FUN_NULL};       		//左切Tab
u8 const KeyTriType_BackBuff[ 2 ]   = { KTE_DC_LONG, KTE_FUN_NULL};       		//返回


//****************************************************Function Declaration****************************************************//



void v_key_func_eng(u8* pKeyTriTypeBuff)
{
#if(boardDISPLAY_EN)
	/* TFT+LVGL模式: 按键直接路由到UI层 */
	if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_UpBuff, sizeof(KeyTriType_UpBuff)) )
	{
		vEngMode_KeyUp();
		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_DownBuff, sizeof(KeyTriType_DownBuff)) )
	{
		vEngMode_KeyDown();
		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_EnterBuff, sizeof(KeyTriType_EnterBuff)) )
	{
		vEngMode_KeyEnter();
		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_RightBuff, sizeof(KeyTriType_RightBuff)) )
	{
		vEngMode_KeyRight();
		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_LeftBuff, sizeof(KeyTriType_LeftBuff)) )
	{
		vEngMode_KeyLeft();
		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif
	}
	else if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_BackBuff, sizeof(KeyTriType_BackBuff)) )
	{
		vEngMode_KeyBack();
		#if(boardBUZ_EN)
		bBuz_Tweet(SHORT_1);
		#endif
	}
#else
	/* 非TFT显示模式: 保持空实现 */
#endif  //boardDISPLAY_EN
}

#endif  //boardENG_MODE_EN
