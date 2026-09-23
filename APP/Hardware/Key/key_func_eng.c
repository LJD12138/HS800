/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Key
 * File    : key_func_eng.c
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : 工程模式按键处理 - TFT+LVGL版本(表驱动优化)
 *           按键映射:
 *             LIGHT_SHORT -> KeyUp    (上移/增值)
 *             USB_SHORT   -> KeyDown  (下移/减值)
 *             POWER_SHORT -> KeyEnter (确认/选择)
 *             AC_SHORT    -> KeyRight (右切Tab)
 *             DC_SHORT    -> KeyLeft  (左切Tab)
 *             DC_LONG     -> KeyBack  (返回/退出)
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "key_func_eng.h"

#if (boardENG_MODE_EN)
#include "Key/key_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"
#include "Sys/sys_queue_task_eng.h"
#include "function.h"

#if (boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  /* boardBUZ_EN */

#if (boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#include "MD_Display/user_ui/eng_mode_ui.h"
#endif  /* boardDISPLAY_EN */

//****************************************************Macros********************************************************************//
typedef void (*fnEngAction_T)(void);

typedef struct
{
	const uint8_t		*pPattern;			/* 匹配序列指针 */
	uint8_t				ucLen;				/* 序列长度 */
	fnEngAction_T		pfAction;			/* 对应动作回调 */
	const char			*pLog;				/* 打印日志 */
}EngKeyActionItem_T;

static const uint8_t s_uc_key_up[]    = { KTE_LIGHT_SHORT, KTE_FUN_NULL };             /* 上移/增值 */
static const uint8_t s_uc_key_down[]  = { KTE_USB_SHORT,   KTE_FUN_NULL };             /* 下移/减值 */
static const uint8_t s_uc_key_enter[] = { KTE_POWER_SHORT, KTE_FUN_NULL };             /* 确认/选择 */
static const uint8_t s_uc_key_right[] = { KTE_AC_SHORT,    KTE_FUN_NULL };             /* 右切Tab */
static const uint8_t s_uc_key_left[]  = { KTE_DC_SHORT,    KTE_FUN_NULL };             /* 左切Tab */
static const uint8_t s_uc_key_back[]  = { KTE_DC_LONG,     KTE_FUN_NULL };             /* 返回/退出 */

#if (boardDISPLAY_EN)
static void v_act_eng_up(void)    { vEngMode_KeyUp(); }
static void v_act_eng_down(void)  { vEngMode_KeyDown(); }
static void v_act_eng_enter(void) { vEngMode_KeyEnter(); }
static void v_act_eng_right(void) { vEngMode_KeyRight(); }
static void v_act_eng_left(void)  { vEngMode_KeyLeft(); }
static void v_act_eng_back(void)  { vEngMode_KeyBack(); }
#endif  /* boardDISPLAY_EN */

//****************************************************Parameter Initialization**************************************************//
static const EngKeyActionItem_T s_t_eng_action_tbl[] =
{
	#if (boardDISPLAY_EN)
	{ s_uc_key_up,    sizeof(s_uc_key_up),    v_act_eng_up,    "EngMode:KeyUp\r\n"    },
	{ s_uc_key_down,  sizeof(s_uc_key_down),  v_act_eng_down,  "EngMode:KeyDown\r\n"  },
	{ s_uc_key_enter, sizeof(s_uc_key_enter), v_act_eng_enter, "EngMode:KeyEnter\r\n" },
	{ s_uc_key_right, sizeof(s_uc_key_right), v_act_eng_right, "EngMode:KeyRight\r\n" },
	{ s_uc_key_left,  sizeof(s_uc_key_left),  v_act_eng_left,  "EngMode:KeyLeft\r\n"  },
	{ s_uc_key_back,  sizeof(s_uc_key_back),  v_act_eng_back,  "EngMode:KeyBack\r\n"  },
	#endif  /* boardDISPLAY_EN */
};

#define			ENG_KEY_ACTION_NUM						(sizeof(s_t_eng_action_tbl) / sizeof(s_t_eng_action_tbl[0]))

//****************************************************Function Declaration******************************************************//

/***********************************************************************************************************************
 * 函数功能    : 工程模式按键分发处理函数
 * 说明(备注)  : 表驱动匹配序列，直接路由至 LVGL UI 界面并提示蜂鸣
 * 传入参数    : p_uc_key_tri_type_buff: 事件序列缓冲区指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void v_key_func_eng(uint8_t *p_uc_key_tri_type_buff)
{
	for (uint8_t i = 0; i < ENG_KEY_ACTION_NUM; i++)
	{
		if (bFun_DataCompare(p_uc_key_tri_type_buff, (u8 *)s_t_eng_action_tbl[i].pPattern, s_t_eng_action_tbl[i].ucLen))
		{
			if (s_t_eng_action_tbl[i].pfAction != NULL)
				s_t_eng_action_tbl[i].pfAction();

			#if (boardBUZ_EN)
			bBuz_Tweet(SHORT_1);
			#endif  /* boardBUZ_EN */

			if (uPrint.tFlag.bKeyTask && s_t_eng_action_tbl[i].pLog != NULL)
				sMyPrint(s_t_eng_action_tbl[i].pLog);

			break;
		}
	}
}

#endif  /* boardENG_MODE_EN */
