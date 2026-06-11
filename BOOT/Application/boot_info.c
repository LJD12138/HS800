/*****************************************************************************************************************
*                                                                                                                *
 *                                         Boot信息                                                             *
*                                                                                                                *
******************************************************************************************************************/
#include "boot_info.h"
#include "board_config.h"
#include "gpio_init.h"
#include "flash_allot_table.h"

#include "Flash/flash_iface.h"
#include "Print/print_task.h"


#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

//为了编译版本、日期和时间正确，需要进行设置：总是编译
//在option for ...中勾选 always build

//****************************************************参数初始化**************************************************//
__ALIGNED(4) BootMemParam_T  	tBootMemParam;

const char tBootMemParamStr[]	= "tBootMemParam";
const char tBootVerInfoStr[]	= "tBootVerInfo";
const char tBootParamStr[] 	= "tBootParam";

//把版本信息写入BOOT的Flash中
//tBootInfo 指向的地址是Flash区,当对其Flash区擦除后,tBootInfo也被清空了
#if (boardIC_TYPE == boardIC_GD32F50X)
__attribute__((section(".ARM.__at_0x08000F00"))) const VerInfo_T tBootDefaultVer = {
#else
__attribute__((at(flashBOOT_START + FLASH_PAGE_SIZE))) const VerInfo_T tBootDefaultVer = {
#endif
	boardSOFTWARE_VERSION,
	__DATE__,
	__TIME__,
};

#if(boardEASY_FLASH)
/* default environment variables set for user */
const ef_env default_env_set[] = {
	{(char*)tBootVerInfoStr, (u8*)&tBootDefaultVer, sizeof(tBootDefaultVer)},
    {(char*)tBootParamStr,&tBootMemParam.tParam, sizeof(tBootMemParam.tParam)},
};
#endif

//****************************************************函数定义**************************************************//
static void v_print_info(void);
	











/*****************************************************************************************************************
-----函数功能    读取记忆参数去初始化任务
-----说明(备注)  none
-----传入参数    init true:强制初始化    false:自主判断
-----输出参数    none
-----返回值      SysTaskId_E
******************************************************************************************************************/
SysTaskId_E eBoot_InfoInit(bool init)
{
	s8 c_ret = 0;
	
	const char* p_obj_str = tBootMemParamStr;
	
	//---------强制初始化----------
	if(init == true)
		goto init_loop;
	
	//--------------------------------------获取消息-------------------------------------------
	//读取参数
	c_ret = cBoot_GetMemParam(p_obj_str);
	//读取参数失败
	if(c_ret < 0)
	{
		if(uPrint.tFlag.bBootInfo)
			sMyPrint("bBootInfo:参数读取失败 代码%d\r\n",c_ret);
		return STI_ERR;
	}
	if(c_ret == 0)
		goto init_loop;
	
	//--------------------------------------校验消息-------------------------------------------
	//---------初始化----------
	if(bBoot_CmdExist(tBootMemParam.tParam.ulCmd) ==false)
	{
		goto init_loop;
	}
	//---------跳转APP----------
	else if(tBootMemParam.tParam.ulCmd ==mainINIT_FINISH_FLAG && //APP跳转升级
		tBootMemParam.tParam.ucAppFaultCnt < 5 && 	//APP启动失败次数过多
		tBootMemParam.tParam.eAppState != AS_ERASE)	//APP已经擦除
	{
		if(uPrint.tFlag.bBootInfo == 1)
			sMyPrint("跳转APP任务!\r\n");
		return STI_ENTER_APP;
	}
	
	//---------低功耗显示----------
	#if(boardDISPLAY_EN)
	else if(tBootMemParam.tParam.ulCmd ==mainDISPLAY_FLAG)
	{
		tDisp.bSleepShow = true;
		if(uPrint.tFlag.bBootInfo == 1)
			sMyPrint("低功耗显示任务!\r\n");
		return STI_DISPLAY;
	}
	#endif  //boardDISPLAY_EN
	
	//---------低功耗----------
	#if(boardLOW_POWER)
	else if(tBootMemParam.tParam.ulCmd ==mainLOW_POWER_FLAG)
	{
		LCD.bSleepShow = false;
		if(uPrint.tFlag.bBootInfo == 1)
			sMyPrint("进入低功耗!\r\n");
		return TS_LowPower;
	}
	#endif  //boardLOW_POWER
	
	//---------需要升级----------
	#if( boardPRINT_IFACE )
	else if(tBootMemParam.tParam.ulCmd == mainUPDATE_FLAG	||
			(tBootMemParam.tParam.ucAppFaultCnt >= 5 &&  tBootMemParam.tParam.ucAppFaultCnt != 0xff)|| 	//APP启动失败次数过多
			tBootMemParam.tParam.eAppState == AS_ERASE)	//APP已经擦除
	{
		#if(boardDISPLAY_EN)
		tDisp.bSleepShow = true;
		#endif  //boardDISPLAY_EN
		if(uPrint.tFlag.bBootInfo == 1)
			sMyPrint("升级任务!\r\n");
		return STI_UPDATE;
	}
	#endif  //boardPRINT_IFACE

	
	//--------------------------------------开始初始化---------------------------------------------
	init_loop:
    #if(boardDISPLAY_EN)
//    vExRTC_WriteDefaultTime();
	#endif  //boardDISPLAY_EN
	
	//初始化数据
	c_ret = cBoot_MemParamInit(p_obj_str);
	//读取参数失败
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bBootInfo)
			sMyPrint("bBootInfo:参数初始化失败 代码%d\r\n",c_ret);
		return STI_ERR;
	}
	
	//更新参数
	c_ret = cBoot_UpdateMemParam(p_obj_str);
	//更新失败
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bBootInfo)
			sMyPrint("bBootInfo:参数更新失败 代码%d\r\n",c_ret);
		return STI_ERR;
	}

	if(uPrint.tFlag.bBootInfo == 1)
	{
		v_print_info();
		sMyPrint("bBootInfo:参数重置成功\r\n");
		
		#if( boardPRINT_IFACE )
		bPrint_SendDataToUsart();
		#endif  //boardPRINT_IFACE
	}
	
	return STI_ENTER_APP;
}

/*****************************************************************************************************************
-----函数功能    APP记忆参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:写入成功  false:写入失败
******************************************************************************************************************/
s8 cBoot_MemParamInit(const char* id_str)
{
	if (strcmp(id_str, tBootMemParamStr) == 0)
	{
		//把版本信息录入
		tBootMemParam.tVerInfo 				= tBootDefaultVer;
		tBootMemParam.tParam.ulCmd 			= mainINIT_FINISH_FLAG;  //完成初始化
		tBootMemParam.tParam.eAppState 		= AS_NULL;
		tBootMemParam.tParam.ucAppFaultCnt 	= 0;
	}
	//初始化版本信息
	else if (strcmp(id_str, tBootVerInfoStr) == 0)
	{
		tBootMemParam.tVerInfo 				= tBootDefaultVer;
	}
	//初始化参数信息
	else if (strcmp(id_str, tBootParamStr) == 0)
	{
		tBootMemParam.tParam.ulCmd 			= mainINIT_FINISH_FLAG;  //完成初始化
		tBootMemParam.tParam.eAppState 		= AS_NULL;
		tBootMemParam.tParam.ucAppFaultCnt 	= 0;
	}
	else
		return -99;
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    更新APP记忆参数
-----说明(备注)  none
-----传入参数    id_str:需要更新的对象
-----输出参数    none
-----返回值      >0 写入的字节数   0:未操作  <0:错误
******************************************************************************************************************/
s16 cBoot_UpdateMemParam(const char* id_str)
{
	if(id_str == NULL)
		return -1;
	
	#if(boardEASY_FLASH)
	int write_len = 0;
	
	//全部重置
	if (strcmp(id_str, tBootMemParamStr) == 0)
	{
		if(ef_env_set_default() != EF_NO_ERR)
			return -2;
	}
	//写入版本信息
	else if (strcmp(id_str, tBootVerInfoStr) == 0)
	{
		write_len = sizeof(tBootMemParam.tVerInfo);
		if(ef_set_env_blob(id_str, &tBootMemParam.tVerInfo, write_len) != EF_NO_ERR)
			return -10;
	}
	//写入参数
	else if (strcmp(id_str, tBootParamStr) == 0)
	{
		write_len = sizeof(tBootMemParam.tParam);
		if(ef_set_env_blob(id_str, &tBootMemParam.tParam, write_len) != EF_NO_ERR)
			return -11;
	}
	else
		return -99;
	#else
	//擦除Falsh准备写入
	if(cFlash_EraseSector(flashAPP_INFO_SATRT, flashAPP_INFO_END) <= 0)
		return -2;
	//开始写入数据
	if(cFlash_Write8BitData(flashAPP_INFO_SATRT, (u8*)&tBootMemParam, sizeof(tBootMemParam)) <= 0)
		return -3;
	#endif
	return 1;
}

/*****************************************************************************************************************
-----函数功能    更新APP记忆参数
-----说明(备注)  none
-----传入参数    id_str:需要更新的对象
-----输出参数    none
-----返回值      >0 写入的字节数   0:未操作  <0:错误
******************************************************************************************************************/
s16 cBoot_GetMemParam(const char* id_str)
{
	if(id_str == NULL)
		return -1;
	
	#if(boardEASY_FLASH)
	int read_len = 0;
	int return_len = 0;
	
	//读取所有
	if (strcmp(id_str, tBootMemParamStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tVerInfo);
		return_len = ef_get_env_blob(tBootVerInfoStr, &tBootMemParam.tVerInfo, read_len, NULL);
		if(return_len != read_len)
		{
			if(return_len == 0)
			{
				tBootMemParam.tParam.ulCmd = 0;
				return 0;
			}
			return -2;
		}
		
		read_len = sizeof(tBootMemParam.tParam);
		return_len = ef_get_env_blob(tBootParamStr, &tBootMemParam.tParam, read_len, NULL);
		if(return_len != read_len)
		{
			if(return_len == 0)
			{
				tBootMemParam.tParam.ulCmd = 0;
				return 0;
			}
			return -3;
		}
	}
	//读取版本信息
	else if (strcmp(id_str, tBootVerInfoStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tVerInfo);
		return_len = ef_get_env_blob(tBootVerInfoStr, &tBootMemParam.tVerInfo, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	//读取参数
	else if (strcmp(id_str, tBootParamStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tParam);
		return_len = ef_get_env_blob(tBootParamStr, &tBootMemParam.tParam, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	else
		return -99;
	
	if(return_len != read_len)
		return -40;
	#else
	//读取数据
	if(cFlash_Read8BitData(flashAPP_INFO_SATRT, (u8*)&tBootMemParam, sizeof(tBootMemParam)) <= 0)
		return -41;
	#endif
	return 1;
}

/*****************************************************************************************************************
-----函数功能    获取总的默认参数大小
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      byte数量大小
******************************************************************************************************************/
u16 usBoot_GetMemParamSize(void)
{
	#if(boardEASY_FLASH)
	return sizeof(default_env_set) / sizeof(default_env_set[0]);
	#else
	return sizeof(tBootMemParam);
	#endif
	
}

/*****************************************************************************************************************
-----函数功能    指令是否存在
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:存在  false:不存在
******************************************************************************************************************/
bool bBoot_CmdExist(u32 cmd)
{
	switch(cmd)
	{
		case mainINIT_FINISH_FLAG:
		case mainUPDATE_FLAG:
		case mainDISPLAY_FLAG:
		case mainLOW_POWER_FLAG:
		case mainINIT_APP_PARAM_FLAG:
			return true;
		
		default:
			return false;
	}
}

/*****************************************************************************************************************
-----函数功能    控制进入升级接收
-----说明(备注)  向BOOT的INFO Flash区写入升级标志位
-----传入参数    en true:升级    false:不升级
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
#if(boardUPDATE)
s8 cBoot_CtrlUpdate(bool en, AppState_E state)
{
	if(en == true)
	{
		tBootMemParam.tParam.ulCmd = mainUPDATE_FLAG;
		cQueue_AddQueueTask(tpSysTask, STI_UPDATE, 0, false);
		if(uPrint.tFlag.bBootInfo == 1)
			sMyPrint(" Start Update!\r\n");
	}
	else 
	{
		tBootMemParam.tParam.ulCmd = mainINIT_FINISH_FLAG;
		cQueue_AddQueueTask(tpSysTask, STI_ENTER_APP, 0, false);
		if(uPrint.tFlag.bBootInfo == 1)
			sMyPrint(" Exit Update!\r\n");
	}
	
	tBootMemParam.tParam.eAppState = state;
	
	if(state == AS_ERASE || state == AS_FINISH)
		tBootMemParam.tParam.ucAppFaultCnt = 0;
	else 
		tBootMemParam.tParam.ucAppFaultCnt++;
	
	//更新参数
	return cBoot_UpdateMemParam(tBootParamStr);
}
#endif


/*****************************************************************************************************************
-----函数功能    输出录入信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
static void v_print_info(void)
{
	sMyPrint("Boot: Version         : %s\r\n", tBootMemParam.tVerInfo.saVersion);
	sMyPrint("Boot: buildTime       : %s\r\n", tBootMemParam.tVerInfo.saBuildDate);
	sMyPrint("Boot: buildTime       : %s\r\n", tBootMemParam.tVerInfo.saBuildTime);
	sMyPrint("Boot: ulCmd           : %x\r\n", tBootMemParam.tParam.ulCmd);
	sMyPrint("Boot: eAppState       : %d\r\n", tBootMemParam.tParam.eAppState);
	sMyPrint("Boot: ucAppFaultCnt	: %d\r\n", tBootMemParam.tParam.ucAppFaultCnt);
}


