/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\13-G20\1.software\G20\APP\Middlewares\MultiFuncKey
 * File    : mf_key.c
 * Date    : 2026-08-19
 * Author  : LJD(291483914@qq.com)
 * Desc    : 通用多功能按键中间件实现 - 按键状态机/去抖/长短按/超长按/组合键时序算法
 *           算法行为:
 *           1. 长按累加路径录入短按事件并立即分发
 *           2. 非多功能键长按录入后(其他键按住时等待组合)立即分发
 *           3. 多功能键超长按录入长按事件(每次按住仅触发一轮)
 *           4. 非多功能键松开立即分发, 多功能键等待组合键窗口到期统一分发
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "mf_key.h"

//****************************************************Module Variables*********************************************************//
static const MfKeyCfg_T     *sp_tMwCfg  = NULL;  /* 中间件全局配置 */
static const MfKeyItemCfg_T *sp_tKeyCfg = NULL;  /* 按键描述表 */
static u8   suc_key_num = 0;                     /* 已注册按键数量 */
static vu8  suc_seq_cnt = 0;                     /* 已录入事件计数 */
static vu16 sus_unpress_tim = 0;                 /* 全部松开计时 */

/* 运行期按键状态 */
typedef struct
{
	vs16 sOnPressCnt;    /* 按下计时: 0=未触发, -1=已录入/已被处理 */
	bool bEnMultiKey;    /* 多功能键使能(运行期可调) */
} MfKeyState_T;

static MfKeyState_T S_tMfKeyState[MF_KEY_MAX];

//****************************************************Local Function Declaration***********************************************//
static void v_mf_key_record(u8 uc_idx, bool b_long);
static bool b_mf_key_other_is_tri(void);
static void v_mf_key_dispatch(void);

//****************************************************Interface Implementation*************************************************//
/***********************************************************************************************************************
 * 函数功能    : 多功能按键中间件初始化
 * 传入参数    : p_cfg: 全局配置(时序/缓冲/回调), p_key_tbl: 按键描述表, uc_key_num: 按键数量
 * 返回值      : none
 ************************************************************************************************************************/
void vMfKey_Init(const MfKeyCfg_T *p_cfg, const MfKeyItemCfg_T *p_key_tbl, u8 uc_key_num)
{
	u8 i;

	sp_tMwCfg  = p_cfg;
	sp_tKeyCfg = p_key_tbl;
	suc_key_num = (uc_key_num > MF_KEY_MAX) ? MF_KEY_MAX : uc_key_num;

	for(i = 0; i < suc_key_num; i++)
	{
		S_tMfKeyState[i].sOnPressCnt = 0;
		S_tMfKeyState[i].bEnMultiKey = p_key_tbl[i].bEnMultiKey;
	}

	sus_unpress_tim = 0;
	vMfKey_ClearSeq();
}

/***********************************************************************************************************************
 * 函数功能    : 清空事件序列缓冲(填充空闲码, 计数归零)
 * 传入参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vMfKey_ClearSeq(void)
{
	u8 i;

	if(sp_tMwCfg == NULL || sp_tMwCfg->pSeqBuff == NULL)
		return;

	suc_seq_cnt = 0;
	for(i = 0; i < sp_tMwCfg->ucSeqBuffLen; i++)
		sp_tMwCfg->pSeqBuff[i] = sp_tMwCfg->ucSeqEndSymbol;
}

/***********************************************************************************************************************
 * 函数功能    : 运行期调整多功能键使能
 * 传入参数    : uc_idx: 按键索引, b_en: 使能状态
 * 返回值      : none
 ************************************************************************************************************************/
void vMfKey_SetMultiKeyEn(u8 uc_idx, bool b_en)
{
	if(uc_idx >= suc_key_num)
		return;
	
	S_tMfKeyState[uc_idx].bEnMultiKey = b_en;
}

/***********************************************************************************************************************
 * 函数功能    : 标记按键事件已被外部处理(按下计数置-1, 防止重复触发)
 * 传入参数    : uc_idx: 按键索引
 * 返回值      : none
 ************************************************************************************************************************/
void vMfKey_MarkProcessed(u8 uc_idx)
{
	if(uc_idx >= suc_key_num)
		return;

	S_tMfKeyState[uc_idx].sOnPressCnt = -1;
}

/***********************************************************************************************************************
 * 函数功能    : 录入按键事件(短按/长按)
 * 说明(备注)  : 录入前调用业务预处理回调, 返回false则吞掉该事件(如息屏唤醒场景)
 * 传入参数    : uc_idx: 按键索引, b_long: true=长按, false=短按
 * 返回值      : none
 ************************************************************************************************************************/
static void v_mf_key_record(u8 uc_idx, bool b_long)
{
	/* 事件录入前预处理: 返回false吞掉该事件 */
	if(sp_tMwCfg->pfEventPreProc != NULL)
	{
		if(sp_tMwCfg->pfEventPreProc(uc_idx, b_long) == false)
		{
			S_tMfKeyState[uc_idx].sOnPressCnt = -1;
			return;
		}
	}

	sp_tMwCfg->pSeqBuff[suc_seq_cnt] = b_long ? sp_tKeyCfg[uc_idx].ucLongEvent : sp_tKeyCfg[uc_idx].ucShortEvent;
	if(suc_seq_cnt < (sp_tMwCfg->ucSeqBuffLen - 1))
		suc_seq_cnt++;

	S_tMfKeyState[uc_idx].sOnPressCnt = -1;
}

/***********************************************************************************************************************
 * 函数功能    : 检查是否有其他按键处于按下计数状态
 * 说明(备注)  : 当前键刚录入后计数为-1, 天然被排除
 * 传入参数    : none
 * 返回值      : true: 存在其他按键正在按下, false: 无
 ************************************************************************************************************************/
static bool b_mf_key_other_is_tri(void)
{
	u8 i;

	for(i = 0; i < suc_key_num; i++)
	{
		if(S_tMfKeyState[i].sOnPressCnt > 0)
			return true;
	}
	return false;
}

/***********************************************************************************************************************
 * 函数功能    : 序列就绪分发(回调业务分发函数, 随后清空序列)
 * 传入参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
static void v_mf_key_dispatch(void)
{
	if(sp_tMwCfg->pfSequenceReady != NULL)
		sp_tMwCfg->pfSequenceReady(sp_tMwCfg->pSeqBuff);

	vMfKey_ClearSeq();
}

/***********************************************************************************************************************
 * 函数功能    : 按键扫描状态机(需周期调用, 周期应与配置的时序参数单位一致)
 * 传入参数    : none
 * 返回值      : none
 ************************************************************************************************************************/
void vMfKey_Scan(void)
{
	u8 uc_key;

	if(sp_tMwCfg == NULL || sp_tKeyCfg == NULL)
	{
		return;
	}

	for(uc_key = 0; uc_key < suc_key_num; uc_key++)
	{
		//******************************************按键 按下状态***********************************************
		if(sp_tMwCfg->pfIsPressById != NULL && sp_tMwCfg->pfIsPressById(uc_key))
		{
			//记录按下的时间--------------------------------------------------------------------------
			if(S_tMfKeyState[uc_key].sOnPressCnt < 0xfff &&
			   S_tMfKeyState[uc_key].sOnPressCnt >= 0)
				S_tMfKeyState[uc_key].sOnPressCnt++;

			//使能长按累加按键-----------------------------------------------------------------------
			if(sp_tKeyCfg[uc_key].bEnLongPressAdd == true)
			{
				if(S_tMfKeyState[uc_key].sOnPressCnt >= sp_tMwCfg->usLongPressTime) //满足长按时长
				{
					v_mf_key_record(uc_key, false); //录入短按事件(累加路径固定录入短按)

					v_mf_key_dispatch();            //立刻处理

					S_tMfKeyState[uc_key].sOnPressCnt = sp_tMwCfg->usLongPressTime - sp_tMwCfg->usAddSpaceTime;
				}
			}
			//不使能组合按键--------------------------------------------------------------------------
			else if(S_tMfKeyState[uc_key].bEnMultiKey == false)
			{
				if(S_tMfKeyState[uc_key].sOnPressCnt >= sp_tMwCfg->usLongPressTime) //满足长按事件,记录
				{
					v_mf_key_record(uc_key, true); //执行长按事件

					if(b_mf_key_other_is_tri() == true)
						continue; //其他按键正在按下, 结束本次循环等待组合

					v_mf_key_dispatch();          //立刻处理
				}
			}
			//使能多功能按键--------------------------------------------------------------------------
			else
			{
				if(S_tMfKeyState[uc_key].sOnPressCnt >= sp_tMwCfg->usSuperLongPressTime) //满足超长按事件,提示
				{
					if(sp_tMwCfg->pfOnSuperLong != NULL)
						sp_tMwCfg->pfOnSuperLong(uc_key);

					v_mf_key_record(uc_key, true); //记录长按事件
				}
			}

			sus_unpress_tim = 0;

			if(sp_tMwCfg->pfOnAnyPress != NULL)
				sp_tMwCfg->pfOnAnyPress();
		}
		//****************************************************按键 放开状态*******************************************
		else
		{
			//按键已经松开,记录当前按键事件,并等待是否还有组合按键触发------------------------------------------------
			if(sus_unpress_tim < sp_tMwCfg->usUnPressMaxTime && sus_unpress_tim >= 4)
			{
				//短按: 按下时间在短按阈值与长按累加阈值之间
				if(RANGE(S_tMfKeyState[uc_key].sOnPressCnt, sp_tMwCfg->usShortPressTime,
				         (sp_tMwCfg->usLongPressTime - sp_tMwCfg->usAddSpaceTime - 1)))
				{
					v_mf_key_record(uc_key, false);
					if(S_tMfKeyState[uc_key].bEnMultiKey == false) //非多功能按键不等待, 直接触发
						goto MfKeyTri;
				}
				else if(S_tMfKeyState[uc_key].sOnPressCnt >= sp_tMwCfg->usLongPressTime) //长按
				{
					v_mf_key_record(uc_key, true);
					if(S_tMfKeyState[uc_key].bEnMultiKey == false) //非多功能按键不等待, 直接触发
						goto MfKeyTri;
				}
			}
			//组合键等待窗到期统一分发------------------------------------------------------------------------------
			else if(sus_unpress_tim == sp_tMwCfg->usUnPressMaxTime)
			{
				MfKeyTri:
				v_mf_key_dispatch();
			}

			//每轮扫描仅在首个按键放开时累加一次松开计时------------------------------------------------------------
			if(uc_key == 0)
			{
				if(sus_unpress_tim < 0xffff)
					sus_unpress_tim++;
			}

			//松开消抖延时到期后复位按键状态------------------------------------------------------------------------
			if(sus_unpress_tim == 5)
				S_tMfKeyState[uc_key].sOnPressCnt = 0;
		}
	}
}
