/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : app_info.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : APP运行信息、固件版本、持久化记忆参数实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "app_info.h"

#if (boardEASY_FLASH)
#include "easyflash.h"
#endif  /* boardEASY_FLASH */

#include "Sys/sys_task.h"
#include "Print/print_task.h"

/* 芯片唯一ID基地址 (96bit = 12字节 = 6 x u16) */
#if (boardIC_TYPE == boardIC_GD32F30X || boardIC_TYPE == boardIC_GD32F50X)
#define			appUID_BASE								0x1FFFF7E8UL
#elif (boardIC_TYPE == boardIC_STM32H7XX)
#define			appUID_BASE								0x1FF1E800UL
#endif  /* boardIC_TYPE */

/* atAppParamItem 表项索引:组合ID"tAppVerAndParam"固定取表前两项,顺序不可调换 */
#define			appITEM_VERINFO							0
#define			appITEM_PARAM							1
#define			appITEM_COUNT_VER_AND_PARAM				2

//****************************************************Parameter Initialization**************************************************//
__ALIGNED(4) AppMemParam_T  tAppMemParam;
__ALIGNED(4) BootMemParam_T tBootMemParam;

const char tBootMemParamStr[]   = "tBootMemParam";
const char tBootVerInfoStr[]    = "tBootVerInfo";
const char tBootParamStr[]      = "tBootParam";

const char tAppMemParamStr[]    = "tAppMemParam";
const char tAppVerInfoStr[]     = "tAppVerInfo";
const char tAppParamStr[]       = "tAppParam";
const char tAppVerAndParamStr[] = "tAppVerAndParam";

#if (boardDISPLAY_EN)
const char tDispMemParamStr[]   = "tDISP";
#define			APP_PARAM_LIST_DISP(X)					X(tDispMemParamStr, tDISP, DispMemParam_T, bDisp_MemParamInit)
#endif  /* boardDISPLAY_EN */

#if (boardDC_EN)
const char tDcMemParamStr[]     = "tDC";
#define			APP_PARAM_LIST_DC(X)					X(tDcMemParamStr, tDC, DcMemParam_T, bDc_MemParamInit)
#endif  /* boardDC_EN */

#if (boardUSB_EN)
const char tUsbMemParamStr[]    = "tUSB";
#define			APP_PARAM_LIST_USB(X)					X(tUsbMemParamStr, tUSB, UsbMemParam_T, bUsb_MemParamInit)
#endif  /* boardUSB_EN */

#if (boardBMS_EN)
const char tBmsMemParamStr[]    = "tBMS";
#define			APP_PARAM_LIST_BMS(X)					X(tBmsMemParamStr, tBMS, BmsMemParam_T, bBms_MemParamInit)
#endif  /* boardBMS_EN */

#if (boardMPPT_EN)
const char tMpptMemParamStr[]   = "tMPPT";
#define			APP_PARAM_LIST_MPPT(X)					X(tMpptMemParamStr, tMPPT, MpptMemParam_T, bMppt_MemParamInit)
#endif  /* boardMPPT_EN */

#if (boardDCAC_EN)
const char tDcacMemParamStr[]   = "tDCAC";
#define			APP_PARAM_LIST_DCAC(X)					X(tDcacMemParamStr, tDCAC, DcacMemParam_T, bDcac_MemParamInit)
#endif  /* boardDCAC_EN */

const char tSysMemParamStr[]    = "tSYS";
#define			APP_PARAM_LIST_SYS(X)					X(tSysMemParamStr, tSYS, SysMemParam_T, bSys_MemParamInit)

/* 把版本信息写入APP的Falsh中,要加偏移,flashAPP_START是NVIC中断向量表 */
#if defined(__CC_ARM)
const __attribute__((at(flashAPP_START + FLASH_PAGE_SIZE)))  VerInfo_T tAppDefaultVer =
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
/* ARMCLANG不支持at属性,使用section放置到固定地址 */
const __attribute__((section(".ARM.__at_0x08020000"))) __attribute__((used)) VerInfo_T tAppDefaultVer =
#else
const VerInfo_T tAppDefaultVer =
#endif  /* defined(__CC_ARM) */
{
	boardSOFTWARE_VERSION,
	__DATE__,
	__TIME__,
};

/* APP持久化参数对象描述符表项结构
 * 字段:ENV键名 / RAM镜像地址 / 镜像字节数 / 出厂默认值初始化器 */
typedef struct
{
	const char			*id_str;				/* ENV键名 */
	void				*ram;					/* RAM镜像地址 */
	u16					size;					/* 镜像字节数 */
	void				(*init)(void *p_arg);	/* 出厂默认值初始化器 */
}AppParamItem_T;

//****************************************************Function Declaration******************************************************//
static void v_ver_info_default_init(void *p_arg);
static void v_param_default_init(void *p_arg);
static const AppParamItem_T *p_find_param_item(const char *p_id_str);
static s8 c_param_locate(const char *p_id_str, const AppParamItem_T **pp_first, u16 *p_count);
static void v_print_info(void);

/* 通过绑定的元素生成对应的初始化函数 */
#define			APP_DEF_INIT_FN(key, member, type, init_fn)	\
	static void v_##member##_default_init(void *p_arg) { (void)init_fn((type *)p_arg); }

#if (boardDISPLAY_EN)
APP_PARAM_LIST_DISP(APP_DEF_INIT_FN)
#endif  /* boardDISPLAY_EN */

#if (boardDC_EN)
APP_PARAM_LIST_DC(APP_DEF_INIT_FN)
#endif  /* boardDC_EN */

#if (boardUSB_EN)
APP_PARAM_LIST_USB(APP_DEF_INIT_FN)
#endif  /* boardUSB_EN */

#if (boardBMS_EN)
APP_PARAM_LIST_BMS(APP_DEF_INIT_FN)
#endif  /* boardBMS_EN */

#if (boardMPPT_EN)
APP_PARAM_LIST_MPPT(APP_DEF_INIT_FN)
#endif  /* boardMPPT_EN */

#if (boardDCAC_EN)
APP_PARAM_LIST_DCAC(APP_DEF_INIT_FN)
#endif  /* boardDCAC_EN */

APP_PARAM_LIST_SYS(APP_DEF_INIT_FN)
#undef APP_DEF_INIT_FN

//****************************************************Parameter Initialization**************************************************//
/* APP持久化参数对象描述符表 —— 单一事实来源(模块表项由上方清单展开生成)
 * 约束:前两项顺序固定(appITEM_VERINFO/appITEM_PARAM), 组合ID"tAppVerAndParam"依赖此顺序 */
static const AppParamItem_T atAppParamItem[] = {
	/* [appITEM_VERINFO] 版本信息 */
	{tAppVerInfoStr,    &tAppMemParam.tVerInfo, (u16)sizeof(tAppMemParam.tVerInfo), v_ver_info_default_init},
	/* [appITEM_PARAM] APP参数 */
	{tAppParamStr,      &tAppMemParam.tParam,   (u16)sizeof(tAppMemParam.tParam),   v_param_default_init},

	/*--------------------------------------------------
	 * 展开: 模块参数表项(顺序即AppMemParam_T成员序)
	 * 注:init_fn已由展开一生成的包装函数承载, 此处不使用
	 *-------------------------------------------------*/
#define			APP_ITEM(key, member, type, init_fn)	\
	{key, &tAppMemParam.member, (u16)sizeof(tAppMemParam.member), v_##member##_default_init},

	#if (boardDISPLAY_EN)
	APP_PARAM_LIST_DISP(APP_ITEM)
	#endif  /* boardDISPLAY_EN */

	#if (boardDC_EN)
	APP_PARAM_LIST_DC(APP_ITEM)
	#endif  /* boardDC_EN */

	#if (boardUSB_EN)
	APP_PARAM_LIST_USB(APP_ITEM)
	#endif  /* boardUSB_EN */

	#if (boardBMS_EN)
	APP_PARAM_LIST_BMS(APP_ITEM)
	#endif  /* boardBMS_EN */

	#if (boardMPPT_EN)
	APP_PARAM_LIST_MPPT(APP_ITEM)
	#endif  /* boardMPPT_EN */

	#if (boardDCAC_EN)
	APP_PARAM_LIST_DCAC(APP_ITEM)
	#endif  /* boardDCAC_EN */

	APP_PARAM_LIST_SYS(APP_ITEM)

	#undef APP_ITEM
};

#if (boardEASY_FLASH)
/* EasyFlash出厂默认ENV表(模块表项由清单展开生成, 与atAppParamItem键名/镜像/顺序天然同步)
 * 注意:版本信息项的默认值指向flash常量tAppDefaultVer(非RAM镜像), 确保ENV重建后版本信息立即可用 */
const ef_env default_env_set[] = {
	{(char *)tAppVerInfoStr, (u8 *)&tAppDefaultVer, sizeof(tAppDefaultVer)},
	{(char *)tAppParamStr,   &tAppMemParam.tParam,  sizeof(tAppMemParam.tParam)},

	/*--------------------------------------------------
	 * 展开: 模块参数表项(默认ENV值 = RAM镜像)
	 *-------------------------------------------------*/
#define			APP_ENV_ITEM(key, member, type, init_fn)\
	{(char *)(key), &tAppMemParam.member, sizeof(tAppMemParam.member)},

	#if (boardDISPLAY_EN)
	APP_PARAM_LIST_DISP(APP_ENV_ITEM)
	#endif  /* boardDISPLAY_EN */

	#if (boardDC_EN)
	APP_PARAM_LIST_DC(APP_ENV_ITEM)
	#endif  /* boardDC_EN */

	#if (boardUSB_EN)
	APP_PARAM_LIST_USB(APP_ENV_ITEM)
	#endif  /* boardUSB_EN */

	#if (boardBMS_EN)
	APP_PARAM_LIST_BMS(APP_ENV_ITEM)
	#endif  /* boardBMS_EN */

	#if (boardMPPT_EN)
	APP_PARAM_LIST_MPPT(APP_ENV_ITEM)
	#endif  /* boardMPPT_EN */

	#if (boardDCAC_EN)
	APP_PARAM_LIST_DCAC(APP_ENV_ITEM)
	#endif  /* boardDCAC_EN */

	APP_PARAM_LIST_SYS(APP_ENV_ITEM)
	
	#undef APP_ENV_ITEM
};
#endif  /* boardEASY_FLASH */

/***********************************************************************************************************************
 * 函数功能    : 跳转到Boot程序
 * 说明(备注)  : 根据跳转指令配置 BOOT 参数镜像，保存到存储介质并触发 NVIC 系统复位
 * 传入参数    : ul_cmd: 跳转指令码 (mainINIT_FINISH_FLAG / mainUPDATE_FLAG / mainDISPLAY_FLAG 等)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vApp_JumpToBoot(uint32_t ul_cmd)
{
	s8 c_ret = 0;

	if (uPrint.tFlag.bAppInfo)
	{
		if (ul_cmd == mainINIT_FINISH_FLAG)
			sMyPrint("准备跳转到Boot初始化完成任务\r\n");
		else if (ul_cmd == mainUPDATE_FLAG)
			sMyPrint("准备跳转到Boot升级任务\r\n");
		else if (ul_cmd == mainDISPLAY_FLAG)
			sMyPrint("准备跳转到Boot显示任务\r\n");
		else if (ul_cmd == mainLOW_POWER_FLAG)
			sMyPrint("准备跳转到Boot低功耗任务\r\n");
		else if (ul_cmd == mainINIT_APP_PARAM_FLAG)
			sMyPrint("初始化APP参数\r\n");
		else
			sMyPrint("准备跳转到Boot初始化任务\r\n");
	}

	if (ul_cmd == mainINIT_APP_PARAM_FLAG)
		cQueue_AddQueueTask(tpSysTask, STI_RESET, NULL, true);
	else
	{
		/* 标记指令 */
		tBootMemParam.tParam.ulCmd = ul_cmd;
		tBootMemParam.tParam.eAppState = AS_OK;
		tBootMemParam.tParam.ucAppFaultCnt = 0;

		/* 开始写入数据 */
		c_ret = cApp_BootUpdateMemParam(tBootParamStr);
		if (c_ret <= 0)
		{
			if (uPrint.tFlag.bAppInfo)
				sMyPrint("bAppInfo:BOOT参数更新失败 代码%d, 退出重启\r\n", c_ret);
			return;
		}

		if (ul_cmd != mainINIT_FINISH_FLAG)
		{
			__disable_irq();  /* 关闭总中断 */

			/* 关闭中断, 确保跳转过程中不会进入中断导致跳转失败 */
			__set_FAULTMASK(1);
			NVIC_SystemReset();
		}
	}
}

/***********************************************************************************************************************
 * 函数功能    : 系统Boot信息初始化
 * 说明(备注)  : 读取 BOOT 引导参数，标记当前 APP 正常运行并回写 BOOT 参数区
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : APPINFO_OK: 成功, APPINFO_ERR_READ/APPINFO_ERR_WRITE: 失败
 ************************************************************************************************************************/
s8 cApp_BootInfoInit(void)
{
	s8 c_ret = 0;

	/* 读取BOOT数据 */
	c_ret = cApp_BootGetMemParam(tBootMemParamStr);
	if (c_ret <= 0)
	{
		if (uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:BOOT参数读取失败 代码%d\r\n", c_ret);
		return APPINFO_ERR_READ;
	}

	/* 标记 */
	tBootMemParam.tParam.eAppState = AS_OK;
	tBootMemParam.tParam.ucAppFaultCnt = 0;

	/* 更新 */
	c_ret = cApp_BootUpdateMemParam(tBootParamStr);
	if (c_ret <= 0)
	{
		if (uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:BOOT参数更新失败 代码%d\r\n", c_ret);
		return APPINFO_ERR_WRITE;
	}

	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 获取APP运行信息与参数初始化
 * 说明(备注)  : 参数已存在且完好时直接返回；读取异常或未初始化时触发全系统出厂自愈
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 1: 参数已存在, 2: 刚完成出厂初始化, APPINFO_ERR_*: 失败
 ************************************************************************************************************************/
s8 cApp_AppInfoInit(void)
{
	s8 c_ret = 0;
	bool b_need_self_heal = false;

	#if (boardEASY_FLASH)
	const char *p_obj_str = tAppVerAndParamStr;
	#else
	const char *p_obj_str = tAppMemParamStr;
	#endif  /* boardEASY_FLASH */

	/* --------------------------------------获取APP消息------------------------------------------- */
	/* 读取参数 */
	c_ret = cApp_GetMemParam(p_obj_str);
	if (c_ret < 0)
	{
		if (uPrint.tFlag.bAppInfo || uPrint.tFlag.bImportant)
			log_w("bAppInfo:APP参数读取异常(代码%d), 触发自动出厂自愈重置", c_ret);
		b_need_self_heal = true;
	}
	else if (tAppMemParam.tParam.usInitFinish == appPARAM_INIT_MAGIC)
	{
		c_ret = cApp_GetMemParam(tSysMemParamStr);
		if (c_ret < 0)
		{
			if (uPrint.tFlag.bAppInfo || uPrint.tFlag.bImportant)
				log_w("bAppInfo:tSYS参数读取异常(代码%d), 触发自动出厂自愈重置", c_ret);
			b_need_self_heal = true;
		}
		else
		{
			if (uPrint.tFlag.bAppInfo)
			{
				v_print_info();
				sMyPrint("bAppInfo:APP参数不需要重置\r\n");
			}
			tSysInfo.uInit.tFinish.bIF_SysInit = true;
			return 1;
		}
	}

	/* --------------------------------------自愈或出厂初始化--------------------------------------- */
	/* 全系统参数初始化为出厂默认值 */
	c_ret = cApp_MemParamInit(tAppMemParamStr);
	if (c_ret <= 0)
	{
		if (uPrint.tFlag.bAppInfo || uPrint.tFlag.bImportant)
			log_e("bAppInfo:APP参数初始化失败 代码%d", c_ret);
		return APPINFO_ERR_INIT;
	}

	/* 标记完成 */
	tAppMemParam.tParam.usInitFinish = appPARAM_INIT_MAGIC;

	/* 全系统参数写入Flash */
	c_ret = cApp_UpdateMemParam(tAppMemParamStr);
	if (c_ret <= 0)
	{
		if (uPrint.tFlag.bAppInfo || uPrint.tFlag.bImportant)
			log_e("bAppInfo:APP参数更新失败 代码%d", c_ret);
		return APPINFO_ERR_WRITE;
	}

	if (uPrint.tFlag.bAppInfo || uPrint.tFlag.bImportant)
	{
		v_print_info();
		log_i("bAppInfo:APP出厂默认参数%s成功", b_need_self_heal ? "自愈修复" : "初始配置");
	}
	tSysInfo.uInit.tFinish.bIF_SysInit = false;
	return 2;
}

/***********************************************************************************************************************
 * 函数功能    : APP记忆参数初始化 (将RAM镜像置为出厂默认值)
 * 说明(备注)  : 依据 atAppParamItem 表驱动，区间解析见 c_param_locate
 *              "tAppMemParam"=全部对象, "tAppVerAndParam"=表前两项
 * 传入参数    : p_id_str: 参数对象ID字符串
 * 输出参数    : 无
 * 返回值      : APPINFO_OK: 成功, APPINFO_ERR_NULL/APPINFO_ERR_ID: 失败
 ************************************************************************************************************************/
s8 cApp_MemParamInit(const char *p_id_str)
{
	const AppParamItem_T *p_first;
	u16 us_count;
	u16 i;
	s8 c_ret;

	/* 解析对象区间 */
	c_ret = c_param_locate(p_id_str, &p_first, &us_count);
	if (c_ret != APPINFO_OK)
		return c_ret;

	/* 逐项执行出厂默认值初始化器 */
	for (i = 0; i < us_count; i++)
		p_first[i].init(p_first[i].ram);

	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 更新APP记忆参数 (将RAM镜像写入ENV/Flash)
 * 说明(备注)  : EasyFlash模式按表逐项写入；裸Flash模式整体擦写
 * 传入参数    : p_id_str: 需要更新的对象ID字符串
 * 输出参数    : 无
 * 返回值      : APPINFO_OK: 成功, APPINFO_ERR_*: 失败
 ************************************************************************************************************************/
s8 cApp_UpdateMemParam(const char *p_id_str)
{
	#if (boardEASY_FLASH)
	const AppParamItem_T *p_first;
	u16 us_count;
	u16 i;
	s8 c_ret;

	/* 解析对象区间 */
	c_ret = c_param_locate(p_id_str, &p_first, &us_count);
	if (c_ret != APPINFO_OK)
		return c_ret;

	/* 逐项写入ENV */
	for (i = 0; i < us_count; i++)
	{
		if (ef_set_env_blob(p_first[i].id_str, p_first[i].ram, p_first[i].size) != EF_NO_ERR)
			return APPINFO_ERR_WRITE;
	}
	#else
	/* 擦除Falsh准备写入 */
	if (cFlash_EraseSector(flashAPP_INFO_START, flashAPP_INFO_END) <= 0)
		return APPINFO_ERR_FLASH;
	/* 开始写入数据 */
	if (cFlash_Write8BitData(flashAPP_INFO_START, (u8 *)&tAppMemParam, sizeof(tAppMemParam)) <= 0)
		return APPINFO_ERR_FLASH;
	#endif  /* boardEASY_FLASH */
	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 获取APP记忆参数 (将ENV/Flash数据读入RAM镜像)
 * 说明(备注)  : EasyFlash模式按表逐项读取；裸Flash模式整体读取
 * 传入参数    : p_id_str: 需要读取的对象ID字符串
 * 输出参数    : 无
 * 返回值      : APPINFO_OK: 成功, APPINFO_UNINIT: 未初始化, APPINFO_ERR_*: 失败
 ************************************************************************************************************************/
s8 cApp_GetMemParam(const char *p_id_str)
{
	#if (boardEASY_FLASH)
	int return_len = 0;
	#endif  /* boardEASY_FLASH */

	if (p_id_str == NULL)
		return APPINFO_ERR_NULL;

	#if (boardEASY_FLASH)
	/* 版本信息 + APP参数 (带"ENV未初始化"识别) */
	if (strcmp(p_id_str, tAppVerAndParamStr) == 0)
	{
		const AppParamItem_T *p_ver_item  = &atAppParamItem[appITEM_VERINFO];
		const AppParamItem_T *p_parm_item = &atAppParamItem[appITEM_PARAM];

		return_len = (int)ef_get_env_blob(p_ver_item->id_str, p_ver_item->ram, p_ver_item->size, NULL);
		if (return_len != p_ver_item->size)
		{
			/* 还没初始化:清零标志,交由上层触发出厂初始化 */
			if (return_len == 0)
			{
				tAppMemParam.tParam.usInitFinish = 0;
				return APPINFO_UNINIT;
			}
			return APPINFO_ERR_READ;
		}

		return_len = (int)ef_get_env_blob(p_parm_item->id_str, p_parm_item->ram, p_parm_item->size, NULL);
		if (return_len != p_parm_item->size)
			return APPINFO_ERR_READ;
	}
	/* 全部对象:任一项读取失败即报错(无"未初始化"宽容) */
	else if (strcmp(p_id_str, tAppMemParamStr) == 0)
	{
		u16 i;
		u16 count = (u16)mainARRAY_SIZE(atAppParamItem);

		for (i = 0; i < count; i++)
		{
			return_len = (int)ef_get_env_blob(atAppParamItem[i].id_str, atAppParamItem[i].ram, atAppParamItem[i].size, NULL);
			if (return_len != atAppParamItem[i].size)
				return APPINFO_ERR_READ;
		}
	}
	/* 单个对象 */
	else
	{
		const AppParamItem_T *item = p_find_param_item(p_id_str);
		if (item == NULL)
			return APPINFO_ERR_ID;

		return_len = (int)ef_get_env_blob(item->id_str, item->ram, item->size, NULL);
		if (return_len == 0)
			return APPINFO_UNINIT;		/* 未初始化(合法状态) */
		if (return_len != item->size)
			return APPINFO_ERR_READ;	/* 长度不匹配(数据损坏) */
	}
	#else
	/* 读取数据 */
	if (cFlash_Read8BitData(flashAPP_INFO_START, (u8 *)&tAppMemParam, sizeof(tAppMemParam)) <= 0)
		return APPINFO_ERR_FLASH;
	#endif  /* boardEASY_FLASH */
	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 获取总的默认参数大小
 * 说明(备注)  : EasyFlash分支返回default_env_set条目数；非EasyFlash分支返回tAppMemParam字节数
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : EasyFlash: ENV条目数, 非EasyFlash: 参数结构体字节数
 ************************************************************************************************************************/
u16 sApp_GetMemParamSize(void)
{
	#if (boardEASY_FLASH)
	return sizeof(default_env_set) / sizeof(default_env_set[0]);
	#else
	return sizeof(tAppMemParam);
	#endif  /* boardEASY_FLASH */
}

/***********************************************************************************************************************
 * 函数功能    : 更新BOOT记忆参数
 * 说明(备注)  : 传入tBootMemParamStr时调用ef_env_set_default()整体重置ENV分区；仅限出厂复位流程调用
 * 传入参数    : p_id_str: 需要更新的对象ID字符串
 * 输出参数    : 无
 * 返回值      : APPINFO_OK: 成功, APPINFO_ERR_*: 失败
 ************************************************************************************************************************/
s8 cApp_BootUpdateMemParam(const char *p_id_str)
{
	if (p_id_str == NULL)
		return APPINFO_ERR_NULL;

	#if (boardEASY_FLASH)
	/* 全部重置(含APP侧ENV) */
	if (strcmp(p_id_str, tBootMemParamStr) == 0)
	{
		if (ef_env_set_default() != EF_NO_ERR)
			return APPINFO_ERR_WRITE;
	}
	/* 写入版本信息 */
	else if (strcmp(p_id_str, tBootVerInfoStr) == 0)
	{
		if (ef_set_env_blob(p_id_str, &tBootMemParam.tVerInfo, sizeof(tBootMemParam.tVerInfo)) != EF_NO_ERR)
			return APPINFO_ERR_WRITE;
	}
	/* 写入参数 */
	else if (strcmp(p_id_str, tBootParamStr) == 0)
	{
		if (ef_set_env_blob(p_id_str, &tBootMemParam.tParam, sizeof(tBootMemParam.tParam)) != EF_NO_ERR)
			return APPINFO_ERR_WRITE;
	}
	else
		return APPINFO_ERR_ID;

	#else
	/* 擦除Falsh准备写入 */
	if (cFlash_EraseSector(flashAPP_INFO_START, flashAPP_INFO_END) <= 0)
		return APPINFO_ERR_FLASH;
	/* 开始写入数据 */
	if (cFlash_Write8BitData(flashAPP_INFO_START, (u8 *)&tBootMemParam, sizeof(tBootMemParam)) <= 0)
		return APPINFO_ERR_FLASH;
	#endif  /* boardEASY_FLASH */
	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 获取BOOT记忆参数
 * 说明(备注)  : 读取tBootParam时若ENV中无该键,自动写入默认值并返回(自愈)
 * 传入参数    : p_id_str: 需要读取的对象ID字符串
 * 输出参数    : 无
 * 返回值      : APPINFO_OK: 成功, 2/3: 自愈写入默认值, APPINFO_UNINIT: 未初始化, APPINFO_ERR_*: 失败
 ************************************************************************************************************************/
s8 cApp_BootGetMemParam(const char *p_id_str)
{
	if (p_id_str == NULL)
		return APPINFO_ERR_NULL;

	#if (boardEASY_FLASH)
	int read_len = 0;
	int return_len = 0;

	/* 读取所有 */
	if (strcmp(p_id_str, tBootMemParamStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tVerInfo);
		return_len = ef_get_env_blob(tBootVerInfoStr, &tBootMemParam.tVerInfo, read_len, NULL);
		if (return_len != read_len)
			return APPINFO_ERR_READ;

		read_len = sizeof(tBootMemParam.tParam);
		return_len = ef_get_env_blob(tBootParamStr, &tBootMemParam.tParam, read_len, NULL);
		if (return_len == 0)
		{
			tBootMemParam.tParam.ulCmd = mainINIT_FINISH_FLAG;
			tBootMemParam.tParam.eAppState = AS_OK;
			tBootMemParam.tParam.ucAppFaultCnt = 0;
			cApp_BootUpdateMemParam(tBootParamStr);
			return 2;
		}
		if (return_len != read_len)
			return APPINFO_ERR_READ;
	}
	/* 读取版本信息 */
	else if (strcmp(p_id_str, tBootVerInfoStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tVerInfo);
		return_len = ef_get_env_blob(p_id_str, &tBootMemParam.tVerInfo, read_len, NULL);
		if (return_len == 0)
			return APPINFO_UNINIT;
	}
	/* 读取参数 */
	else if (strcmp(p_id_str, tBootParamStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tParam);
		return_len = ef_get_env_blob(p_id_str, &tBootMemParam.tParam, read_len, NULL);
		if (return_len == 0)
		{
			tBootMemParam.tParam.ulCmd = mainINIT_FINISH_FLAG;
			tBootMemParam.tParam.eAppState = AS_OK;
			tBootMemParam.tParam.ucAppFaultCnt = 0;
			cApp_BootUpdateMemParam(tBootParamStr);
			return 3;
		}
		if (return_len != read_len)
			return APPINFO_ERR_READ;
	}
	else
		return APPINFO_ERR_ID;

	if (return_len != read_len)
		return APPINFO_ERR_READ;
	#else
	/* 读取数据 */
	if (cFlash_Read8BitData(flashAPP_INFO_START, (u8 *)&tBootMemParam, sizeof(tBootMemParam)) <= 0)
		return APPINFO_ERR_FLASH;
	#endif  /* boardEASY_FLASH */
	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 版本信息对象出厂默认值初始化器
 * 说明(备注)  : 将 Flash 常量 tAppDefaultVer 载入 RAM 镜像
 * 传入参数    : p_arg: 参数指针 (未使用)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_ver_info_default_init(void *p_arg)
{
	(void)p_arg;
	tAppMemParam.tVerInfo = tAppDefaultVer;			/* 版本信息 */
}

/***********************************************************************************************************************
 * 函数功能    : APP参数对象出厂默认值初始化器
 * 说明(备注)  : 清除完成标志并读取 MCU 硬件唯一 UID
 * 传入参数    : p_arg: 参数指针 (未使用)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_param_default_init(void *p_arg)
{
	(void)p_arg;
	tAppMemParam.tParam.usInitFinish = 0;			/* 初始化完成标志 */

	/* 芯片ID */
	#if defined(appUID_BASE)
	memcpy(tAppMemParam.tParam.usUniqueID, (uint16_t *)appUID_BASE, sizeof(tAppMemParam.tParam.usUniqueID));
	#endif  /* defined(appUID_BASE) */
}

/***********************************************************************************************************************
 * 函数功能    : 按ID查找参数描述符表项
 * 说明(备注)  : 遍历 atAppParamItem 查找匹配键名
 * 传入参数    : p_id_str: 参数对象ID字符串
 * 输出参数    : 无
 * 返回值      : 匹配的描述符表项指针，未命中返回 NULL
 ************************************************************************************************************************/
static const AppParamItem_T *p_find_param_item(const char *p_id_str)
{
	u16 i;

	for (i = 0; i < (u16)mainARRAY_SIZE(atAppParamItem); i++)
	{
		if (strcmp(p_id_str, atAppParamItem[i].id_str) == 0)
			return &atAppParamItem[i];
	}
	return NULL;
}

/***********************************************************************************************************************
 * 函数功能    : 解析参数对象ID为表区间 [pp_first, pp_first + p_count)
 * 说明(备注)  : "tAppMemParam"=全表, "tAppVerAndParam"=表前两项, 其余=单对象
 * 传入参数    : p_id_str: 参数对象ID字符串
 * 输出参数    : pp_first: 起始表项指针二级指针, p_count: 表项数量指针
 * 返回值      : APPINFO_OK: 成功, APPINFO_ERR_NULL/APPINFO_ERR_ID: 失败
 ************************************************************************************************************************/
static s8 c_param_locate(const char *p_id_str, const AppParamItem_T **pp_first, u16 *p_count)
{
	if (p_id_str == NULL)
		return APPINFO_ERR_NULL;

	/* 全部对象 */
	if (strcmp(p_id_str, tAppMemParamStr) == 0)
	{
		*pp_first = atAppParamItem;
		*p_count  = (u16)mainARRAY_SIZE(atAppParamItem);
	}
	/* 版本信息 + APP参数(固定取表前两项) */
	else if (strcmp(p_id_str, tAppVerAndParamStr) == 0)
	{
		*pp_first = atAppParamItem;
		*p_count  = appITEM_COUNT_VER_AND_PARAM;
	}
	/* 单个对象 */
	else
	{
		*pp_first = p_find_param_item(p_id_str);
		if (*pp_first == NULL)
			return APPINFO_ERR_ID;
		*p_count = 1;
	}

	return APPINFO_OK;
}

/***********************************************************************************************************************
 * 函数功能    : 输出录入信息
 * 说明(备注)  : 调试打印当前 BOOT 与 APP 的版本、编译时间、指令和 UID
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_print_info(void)
{
	sMyPrint("Boot: Version		: %s\r\n", tBootMemParam.tVerInfo.saVersion);
	sMyPrint("Boot: buildDate	: %s\r\n", tBootMemParam.tVerInfo.saBuildDate);
	sMyPrint("Boot: buildTime	: %s\r\n", tBootMemParam.tVerInfo.saBuildTime);
	sMyPrint("Boot: ulCmd		: %x\r\n", tBootMemParam.tParam.ulCmd);
	sMyPrint("Boot: eAppState	: %d\r\n", tBootMemParam.tParam.eAppState);
	sMyPrint("Boot: ucAppFaultCnt: %d\r\n", tBootMemParam.tParam.ucAppFaultCnt);

	sMyPrint("APP : usInitFinish: %x\r\n", tAppMemParam.tParam.usInitFinish);
	sMyPrint("APP : Version     : %s\r\n", tAppMemParam.tVerInfo.saVersion);
	sMyPrint("APP : buildDate   : %s\r\n", tAppMemParam.tVerInfo.saBuildDate);
	sMyPrint("APP : buildTime   : %s\r\n", tAppMemParam.tVerInfo.saBuildTime);

	sMyPrint("APP : usUniqueID  :");
	for (int i = 0; i < (int)mainARRAY_SIZE(tAppMemParam.tParam.usUniqueID); i++)
	{
		sMyPrint(" %x ", tAppMemParam.tParam.usUniqueID[i]);
	}
	sMyPrint("\r\n");
}
