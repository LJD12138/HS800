/*****************************************************************************************************************
*                                                                                                                *
 *                                         记忆参数                                                             *
*                                                                                                                *
******************************************************************************************************************/
#include "app_info.h"

#include "Sys/sys_task.h"
#include "Flash/flash_iface.h"
#include "Print/print_task.h"
#include "..\..\BOOT\Application\flash_allot_table.h"

//为了编译版本、日期和时间正确，需要进行设置：总是编译
//在option for ...中勾选 always build

//****************************************************参数初始化**************************************************//		
__align(4) AppMemParam_T  	tAppMemParam;
__align(4) BootMemParam_T  	tBootMemParam;

const char tBootMemParamStr[]	= "tBootMemParam";
const char tBootVerInfoStr[]	= "tBootVerInfo";
const char tBootParamStr[] 		= "tBootParam";

const char tAppMemParamStr[]	= "tAppMemParam";
const char tAppVerInfoStr[]		= "tAppVerInfo";
const char tAppParamStr[] 		= "tAppParam";
const char tAppVerAndParamStr[]	= "tAppVerAndParam";
#if(boardDISPLAY_EN)
const char tDispMemParamStr[]	= "tDISP";
#endif  //boardDISPLAY_EN
#if(boardDC_EN)
const char tDcMemParamStr[]		= "tDC";
#endif  //boardDC_EN
#if(boardUSB_EN)
const char tUsbMemParamStr[]	= "tUSB";
#endif  //boardUSB_EN
#if(boardBMS_EN)
const char tBmsMemParamStr[]	= "tBMS";
#endif  //boardBMS_EN
#if(boardMPPT_EN)
const char tMpptMemParamStr[]	= "tMPPT";
#endif  //boardMPPT_EN
#if(boardDCAC_EN)
const char tDcacMemParamStr[]	= "tDCAC";
#endif  //boardDCAC_EN
const char tSysMemParamStr[]	= "tSYS";

//把版本信息写入APP的Falsh中,要加偏移,flashAPP_START是NVIC中断向量表
const __attribute__((at(flashAPP_START + FLASH_PAGE_SIZE)))  VerInfo_T tAppDefaultVer = {
	boardSOFTWARE_VERSION,
	__DATE__,
	__TIME__,
};

#if(boardEASY_FLASH)
/* default environment variables set for user */
const ef_env default_env_set[] = {
	{(char*)tAppVerInfoStr, (u8*)&tAppDefaultVer, sizeof(tAppDefaultVer)},
    {(char*)tAppParamStr,&tAppMemParam.tParam, sizeof(tAppMemParam.tParam)},
#if(boardDC_EN)
    {(char*)tDcMemParamStr,&tAppMemParam.tDC, sizeof(tAppMemParam.tDC)},
#endif  //boardDC_EN
#if(boardMPPT_EN)
	{(char*)tMpptMemParamStr,&tAppMemParam.tMPPT, sizeof(tAppMemParam.tMPPT)},
#endif  //boardMPPT_EN
};
#endif

//****************************************************函数定义**************************************************//
void v_print_info(void);
	
/*****************************************************************************************************************
-----函数功能    跳转到Boot程序
-----说明(备注)  none
-----传入参数    cmd:跳转的指令
#define     mainINIT_FINISH_FLAG   0x88888888
#define     mainUPDATE_FLAG        0xAAAAAAAA
#define     mainDISPLAY_FLAG       0xAAAABBBB
#define     mainLOW_POWER_FLAG     0xBBBBCCCC
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vApp_JumpToBoot(uint32_t cmd)
{
	s8 c_ret = 0;
	
	if(uPrint.tFlag.bAppInfo)
	{
		if(cmd == mainINIT_FINISH_FLAG)
			sMyPrint("准备跳转到Boot初始化完成任务\r\n");
		else if(cmd == mainUPDATE_FLAG)
			sMyPrint("准备跳转到Boot升级任务\r\n");
		else if(cmd == mainDISPLAY_FLAG)
			sMyPrint("准备跳转到Boot显示任务\r\n");
		else if(cmd == mainLOW_POWER_FLAG)
			sMyPrint("准备跳转到Boot低功耗任务\r\n");
		else if(cmd == mainINIT_APP_PARAM_FLAG)
			sMyPrint("初始化APP参数\r\n");
		else 
			sMyPrint("准备跳转到Boot初始化任务\r\n");
	}
	
	if(cmd == mainINIT_APP_PARAM_FLAG)
	{
		cQueue_AddQueueTask(tpSysTask, STI_RESET, NULL ,true);
	}
	else 
	{
		//标记指令
		tBootMemParam.tParam.ulCmd = cmd;  
		tBootMemParam.tParam.eAppState = AS_OK;
		tBootMemParam.tParam.ucAppFaultCnt = 0;
		
		//开始写入数据
		c_ret = cApp_BootUpdateMemParam(tBootParamStr);
		if(c_ret <= 0)
		{
			if(uPrint.tFlag.bAppInfo)
				sMyPrint("bAppInfo:BOOT参数更新失败 代码%d, 退出重启\r\n",c_ret);
			return;
		}

		if(cmd != mainINIT_FINISH_FLAG)
		{	
			__disable_irq();  // 可以使用这个函数 关闭总中断
			
			//关闭中断,确保跳转过程中 不会进入中断,导致跳转失败 
			//此函数会将给定的值分配给故障掩码寄存器。
			__set_FAULTMASK(1);	
			//系统复位,复位后默认从MCU的起始地址开始, 
			//也就是BOOT的起始地址开始, 这样程序就从APP回到了BOOT,
			//再从BOOT跳转至APP的时候, 堆栈已经清除了,也就不会发生溢出
			NVIC_SystemReset();	
		}		
	}
}

/*****************************************************************************************************************
-----函数功能    系统信息初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:成功  false:失败
******************************************************************************************************************/
s8 cApp_BootInfoInit(void)
{
	s8 c_ret = 0;
	
	//读取BOOT数据
	c_ret = cApp_BootGetMemParam(tBootMemParamStr);
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:BOOT参数读取失败 代码%d\r\n",c_ret);
		return -1;
	}
	
	//标记
	tBootMemParam.tParam.eAppState = AS_OK;
	tBootMemParam.tParam.ucAppFaultCnt = 0;
	
	//更新
	c_ret = cApp_BootUpdateMemParam(tBootParamStr);
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:BOOT参数更新失败 代码%d\r\n",c_ret);
		return -2;
	}
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    获取APP信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:操作成功   false:操作失败
******************************************************************************************************************/
s8 cApp_AppInfoInit(void)
{
	s8 c_ret = 0;
	
	#if(boardEASY_FLASH)
	const char* p_obj_str = tAppVerAndParamStr;
	#else
	const char* p_obj_str = tAppMemParamStr;
	#endif
	
	//--------------------------------------获取APP消息-------------------------------------------
	//读取参数
	c_ret = cApp_GetMemParam(p_obj_str);
	//读取参数失败
	if(c_ret < 0)
	{
		if(uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:APP参数读取失败 代码%d\r\n",c_ret);
		return -1;
	}

	//--------------------------------------校验APP消息-------------------------------------------
	//---------已经初始化过----------
	if(tAppMemParam.tParam.usInitFinish == 0xAAAA)  //不需要初始化
	{
		if(uPrint.tFlag.bAppInfo)
		{
			v_print_info();
			sMyPrint("bAppInfo:APP参数不需要重置\r\n");
		}
		tSysInfo.uInit.tFinish.bIF_SysInit = true;
		return 1;
	}
	
	//---------还没初始化----------
	//初始化APP数据
	c_ret = cApp_MemParamInit(p_obj_str);
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:APP参数初始化失败 代码%d\r\n",c_ret);
		return -2;
	}
	
	//标记
	tAppMemParam.tParam.usInitFinish = 0xAAAA;
	//更新参数
	c_ret = cApp_UpdateMemParam(p_obj_str);
	if(c_ret <= 0)
	{
		if(uPrint.tFlag.bAppInfo)
			sMyPrint("bAppInfo:APP参数更新失败 代码%d\r\n",c_ret);
		return -3;
	}
	
	if(uPrint.tFlag.bAppInfo)
	{
		v_print_info();
		sMyPrint("bAppInfo:APP参数重置成功\r\n");
	}
	tSysInfo.uInit.tFinish.bIF_SysInit = false;
	return 2;
}

/*****************************************************************************************************************
-----函数功能    APP记忆参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      true:写入成功  false:写入失败
******************************************************************************************************************/
s8 cApp_MemParamInit(const char* id_str)
{
	if(id_str == NULL)
		return -1;
	
	if (strcmp(id_str, tAppMemParamStr) == 0)
	{
		//版本信息
		tAppMemParam.tVerInfo 				= tAppDefaultVer;					//版本信息
		
		//APP参数
		tAppMemParam.tParam.usInitFinish 	= 0;								//初始化完成标志
		#if(boardIC_TYPE == boardIC_GD32F30X)
		memcpy(tAppMemParam.tParam.usUniqueID,(uint16_t *)(0x1FFFF7E8),12);
		#elif(boardIC_TYPE == boardIC_STM32H7XX)
		memcpy(tAppMemParam.tParam.usUniqueID, (uint16_t *)(0x1FF1E800), 12);	//芯片ID
		#endif
		
		//显示参数
		#if(boardDISPLAY_EN)
		bDisp_MemParamInit(&tAppMemParam.tDISP);
		#endif  //boardDISPLAY_EN
		
		//DC参数
		#if(boardDC_EN)
		bDc_MemParamInit(&tAppMemParam.tDC);
		#endif  //boardDC_EN
		
		//USB参数
		#if(boardUSB_EN)
		bUsb_MemParamInit(&tAppMemParam.tUSB);
		#endif  //boardUSB_EN
		
		//BMS参数
		#if(boardBMS_EN)
		bBms_MemParamInit(&tAppMemParam.tBMS);
		#endif  //boardBMS_EN
		
		//MPPT参数
		#if(boardMPPT_EN)
		bMppt_MemParamInit(&tAppMemParam.tMPPT);
		#endif  //boardMPPT_EN
		
		//DCAC参数
		#if(boardDCAC_EN)
		bDcac_MemParamInit(&tAppMemParam.tDCAC);
		#endif  //boardDCAC_EN
		
		//SYS参数
		bSys_MemParamInit(&tAppMemParam.tSYS);
	}
	//版本信息
	else if (strcmp(id_str, tAppVerInfoStr) == 0)
	{
		tAppMemParam.tVerInfo 				= tAppDefaultVer;
	}
	//和APP参数
	else if (strcmp(id_str, tAppParamStr) == 0)
	{
		tAppMemParam.tParam.usInitFinish 	= 0;
		#if(boardIC_TYPE == boardIC_GD32F30X)
		memcpy(tAppMemParam.tParam.usUniqueID,(uint16_t *)(0x1FFFF7E8),12);
		#elif(boardIC_TYPE == boardIC_STM32H7XX)
		memcpy(tAppMemParam.tParam.usUniqueID, (uint16_t *)(0x1FF1E800), 12);	//芯片ID
		#endif
	}
	//版本信息和APP参数
	else if (strcmp(id_str, tAppVerAndParamStr) == 0)
	{
		//版本信息
		tAppMemParam.tVerInfo 				= tAppDefaultVer;					//版本信息
		
		//APP参数
		tAppMemParam.tParam.usInitFinish 	= 0;								//初始化完成标志
		#if(boardIC_TYPE == boardIC_GD32F30X)
		memcpy(tAppMemParam.tParam.usUniqueID,(uint16_t *)(0x1FFFF7E8),12);
		#elif(boardIC_TYPE == boardIC_STM32H7XX)
		memcpy(tAppMemParam.tParam.usUniqueID, (uint16_t *)(0x1FF1E800), 12);	//芯片ID
		#endif
	}
	//DISP
	#if(boardDISPLAY_EN)
	else if (strcmp(id_str, tDispMemParamStr) == 0)
		bDisp_MemParamInit(&tAppMemParam.tDISP);
	#endif  //boardDISPLAY_EN
	//DC
	#if(boardDC_EN)
	else if (strcmp(id_str, tDcMemParamStr) == 0)
		bDc_MemParamInit(&tAppMemParam.tDC);
	#endif  //boardDC_EN
	//USB
	#if(boardUSB_EN)
	else if (strcmp(id_str, tUsbMemParamStr) == 0)
		bUsb_MemParamInit(&tAppMemParam.tUSB);
	#endif  //boardUSB_EN
	//BMS
	#if(boardBMS_EN)
	else if (strcmp(id_str, tBmsMemParamStr) == 0)
		bBms_MemParamInit(&tAppMemParam.tBMS);
	#endif  //boardBMS_EN
	//MPPT
	#if(boardMPPT_EN)
	else if (strcmp(id_str, tMpptMemParamStr) == 0)
		bMppt_MemParamInit(&tAppMemParam.tMPPT);
	#endif  //boardMPPT_EN
	//DCAC
	#if(boardDCAC_EN)
	else if (strcmp(id_str, tDcacMemParamStr) == 0)
		bDcac_MemParamInit(&tAppMemParam.tDCAC);
	#endif  //boardDCAC_EN
	//SYS
	else if (strcmp(id_str, tSysMemParamStr) == 0)
		bSys_MemParamInit(&tAppMemParam.tSYS);
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
s16 cApp_UpdateMemParam(const char* id_str)
{
	if(id_str == NULL)
		return -1;
	
	#if(boardEASY_FLASH)
	int write_len = 0;
	
	//全部重置
	if (strcmp(id_str, tAppMemParamStr) == 0)
	{
		//版本信息
		write_len = sizeof(tAppMemParam.tVerInfo);
		if(ef_set_env_blob(tAppVerInfoStr, &tAppMemParam.tVerInfo, write_len) != EF_NO_ERR)
			return -2;
		
		//APP参数
		write_len = sizeof(tAppMemParam.tParam);
		if(ef_set_env_blob(tAppParamStr, &tAppMemParam.tParam, write_len) != EF_NO_ERR)
			return -3;
		
		//显示
		#if(boardDISPLAY_EN)
		write_len = sizeof(tAppMemParam.tDISP);
		if(ef_set_env_blob(tDispMemParamStr, &tAppMemParam.tDISP, write_len) != EF_NO_ERR)
			return -4;
		#endif  //boardDISPLAY_EN
		
		//DC参数
		#if(boardDC_EN)
		write_len = sizeof(tAppMemParam.tDC);
		if(ef_set_env_blob(tDcMemParamStr, &tAppMemParam.tDC, write_len) != EF_NO_ERR)
			return -5;
		#endif  //boardDC_EN
		
		//USB参数
		#if(boardUSB_EN)
		write_len = sizeof(tAppMemParam.tUSB);
		if(ef_set_env_blob(tUsbMemParamStr, &tAppMemParam.tUSB, write_len) != EF_NO_ERR)
			return -6;
		#endif  //boardUSB_EN
		
		//BMS参数
		#if(boardBMS_EN)
		write_len = sizeof(tAppMemParam.tBMS);
		if(ef_set_env_blob(tBmsMemParamStr, &tAppMemParam.tBMS, write_len) != EF_NO_ERR)
			return -7;
		#endif  //boardBMS_EN
		
		//MPPT参数
		#if(boardMPPT_EN)
		write_len = sizeof(tAppMemParam.tMPPT);
		if(ef_set_env_blob(tMpptMemParamStr, &tAppMemParam.tMPPT, write_len) != EF_NO_ERR)
			return -8;
		#endif  //boardMPPT_EN
		
		//DCAC参数
		#if(boardDCAC_EN)
		write_len = sizeof(tAppMemParam.tDCAC );
		if(ef_set_env_blob(tDcacMemParamStr, &tAppMemParam.tDCAC, write_len) != EF_NO_ERR)
			return -9;
		#endif  //boardDCAC_EN
		
		//系统参数
		write_len = sizeof(tAppMemParam.tSYS );
		if(ef_set_env_blob(tSysMemParamStr, &tAppMemParam.tSYS, write_len) != EF_NO_ERR)
			return -10;
	}
	//版本信息
	else if (strcmp(id_str, tAppVerInfoStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tVerInfo);
		if(ef_set_env_blob(id_str, &tAppMemParam.tVerInfo, write_len) != EF_NO_ERR)
			return -20;
	}
	//APP参数
	else if (strcmp(id_str, tAppParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tParam);
		if(ef_set_env_blob(id_str, &tAppMemParam.tParam, write_len) != EF_NO_ERR)
			return -21;
	}
	//版本信息和APP参数
	else if (strcmp(id_str, tAppVerAndParamStr) == 0)
	{
		//版本信息
		write_len = sizeof(tAppMemParam.tVerInfo);
		if(ef_set_env_blob(tAppVerInfoStr, &tAppMemParam.tVerInfo, write_len) != EF_NO_ERR)
			return -22;
		
		//APP参数
		write_len = sizeof(tAppMemParam.tParam);
		if(ef_set_env_blob(tAppParamStr, &tAppMemParam.tParam, write_len) != EF_NO_ERR)
			return -23;
	}
	//显示参数
	#if(boardDISPLAY_EN)
	else if (strcmp(id_str, tDispMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tDISP);
		if(ef_set_env_blob(id_str, &tAppMemParam.tDISP, write_len) != EF_NO_ERR)
			return -24;
	}
	#endif  //boardDISPLAY_EN
	//DC参数
	#if(boardDC_EN)
	else if (strcmp(id_str, tDcMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tDC);
		if(ef_set_env_blob(id_str, &tAppMemParam.tDC, write_len) != EF_NO_ERR)
			return -25;
	}
	#endif  //boardDC_EN
	//USB参数
	#if(boardUSB_EN)
	else if (strcmp(id_str, tUsbMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tUSB);
		if(ef_set_env_blob(id_str, &tAppMemParam.tUSB, write_len) != EF_NO_ERR)
			return -26;
	}
	#endif  //boardUSB_EN
	//BMS参数
	#if(boardBMS_EN)
	else if (strcmp(id_str, tBmsMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tBMS);
		if(ef_set_env_blob(id_str, &tAppMemParam.tBMS, write_len) != EF_NO_ERR)
			return -27;
	}
	#endif  //boardBMS_EN
	//MPPT参数
	#if(boardMPPT_EN)
	else if (strcmp(id_str, tMpptMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tMPPT);
		if(ef_set_env_blob(id_str, &tAppMemParam.tMPPT, write_len) != EF_NO_ERR)
			return -28;
	}
	#endif  //boardMPPT_EN
	//DCAC参数
	#if(boardDCAC_EN)
	else if (strcmp(id_str, tDcacMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tDCAC);
		if(ef_set_env_blob(id_str, &tAppMemParam.tDCAC, write_len) != EF_NO_ERR)
			return -29;
	}
	#endif  //boardDCAC_EN
	//SYS参数
	else if (strcmp(id_str, tSysMemParamStr) == 0)
	{
		write_len = sizeof(tAppMemParam.tSYS);
		if(ef_set_env_blob(id_str, &tAppMemParam.tSYS, write_len) != EF_NO_ERR)
			return -30;
	}
	else
		return -99;
	
	#else
	//擦除Falsh准备写入
	if(cFlash_EraseSector(flashAPP_INFO_SATRT, flashAPP_INFO_END) <= 0)
		return -2;
	//开始写入数据
	if(cFlash_Write8BitData(flashAPP_INFO_SATRT, (u8*)&tAppMemParam, sizeof(tAppMemParam)) <= 0)
		return -3;
	#endif
	return 1;
}

/*****************************************************************************************************************
-----函数功能    更新APP记忆参数
-----说明(备注)  none
-----传入参数    id_str:需要更新的对象
-----输出参数    none
-----返回值      >0 成功   0:读取长度异常  <0:错误
******************************************************************************************************************/
s16 cApp_GetMemParam(const char* id_str)
{
	if(id_str == NULL)
		return -1;
	
	#if(boardEASY_FLASH)
	int read_len = 0;
	int return_len = 0;
	
	//读取所有
	if (strcmp(id_str, tAppMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tVerInfo);
		return_len = ef_get_env_blob(tAppVerInfoStr, &tAppMemParam.tVerInfo, read_len, NULL);
		if(return_len != read_len)
			return -2;
		
		read_len = sizeof(tAppMemParam.tParam);
		return_len = ef_get_env_blob(tAppParamStr, &tAppMemParam.tParam, read_len, NULL);
		if(return_len != read_len)
			return -3;
		
		//显示
		#if(boardDISPLAY_EN)
		read_len = sizeof(tAppMemParam.tDISP);
		return_len = ef_get_env_blob(tDispMemParamStr, &tAppMemParam.tDISP, read_len, NULL);
		if(return_len != read_len)
			return -4;
		#endif  //boardDISPLAY_EN
		
		//DC
		#if(boardDC_EN)
		read_len = sizeof(tAppMemParam.tDC);
		return_len = ef_get_env_blob(tDcMemParamStr, &tAppMemParam.tDC, read_len, NULL);
		if(return_len != read_len)
			return -5;
		#endif  //boardDC_EN
		
		//USB
		#if(boardUSB_EN)
		read_len = sizeof(tAppMemParam.tUSB);
		return_len = ef_get_env_blob(tUsbMemParamStr, &tAppMemParam.tUSB, read_len, NULL);
		if(return_len != read_len)
			return -6;
		#endif  //boardUSB_EN
		
		//BMS
		#if(boardBMS_EN)
		read_len = sizeof(tAppMemParam.tBMS);
		return_len = ef_get_env_blob(tBmsMemParamStr, &tAppMemParam.tBMS, read_len, NULL);
		if(return_len != read_len)
			return -7;
		#endif  //boardBMS_EN
		
		//MPPT
		#if(boardMPPT_EN)
		read_len = sizeof(tAppMemParam.tMPPT);
		return_len = ef_get_env_blob(tMpptMemParamStr, &tAppMemParam.tMPPT, read_len, NULL);
		if(return_len != read_len)
			return -8;
		#endif  //boardMPPT_EN
		
		//DCAC
		#if(boardDCAC_EN)
		read_len = sizeof(tAppMemParam.tDCAC);
		return_len = ef_get_env_blob(tDcacMemParamStr, &tAppMemParam.tDCAC, read_len, NULL);
		if(return_len != read_len)
			return -9;
		#endif  //boardDCAC_EN
		
		//SYS
		read_len = sizeof(tAppMemParam.tSYS);
		return_len = ef_get_env_blob(tSysMemParamStr, &tAppMemParam.tSYS, read_len, NULL);
		if(return_len != read_len)
			return -10;
	}
	//读取版本信息
	else if (strcmp(id_str, tAppVerInfoStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tVerInfo);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tVerInfo, read_len, NULL);
	}
	//读取参数
	else if (strcmp(id_str, tAppParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tParam);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tParam, read_len, NULL);
	}
	//APP的版本和参数
	else if (strcmp(id_str, tAppVerAndParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tVerInfo);
		return_len = ef_get_env_blob(tAppVerInfoStr, &tAppMemParam.tVerInfo, read_len, NULL);
		if(return_len != read_len)
		{
			//还没初始化
			if(return_len == 0)
			{
				tAppMemParam.tParam.usInitFinish = 0;
				return 0;
			}
				
			return -20; 
		}
		
		read_len = sizeof(tAppMemParam.tParam);
		return_len = ef_get_env_blob(tAppParamStr, &tAppMemParam.tParam, read_len, NULL);
		if(return_len != read_len)
			return -21;
	}
	//读取显示参数
	#if(boardDISPLAY_EN)
	else if (strcmp(id_str, tDispMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tDISP);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tDISP, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	#endif  //boardDISPLAY_EN
	//读取DC参数
	#if(boardDC_EN)
	else if (strcmp(id_str, tDcMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tDC);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tDC, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	#endif  //boardDC_EN
	//读取USB参数
	#if(boardUSB_EN)
	else if (strcmp(id_str, tUsbMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tUSB);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tUSB, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	#endif  //boardUSB_EN
	//读取BMS参数
	#if(boardBMS_EN)
	else if (strcmp(id_str, tBmsMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tBMS);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tBMS, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	#endif  //boardBMS_EN
	//读取MPPT参数
	#if(boardMPPT_EN)
	else if (strcmp(id_str, tMpptMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tMPPT);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tMPPT, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	#endif  //boardMPPT_EN
	//读取DCAC参数
	#if(boardDCAC_EN)
	else if (strcmp(id_str, tDcacMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tDCAC);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tDCAC, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	#endif  //boardDCAC_EN
	//读取系统参数
	else if (strcmp(id_str, tSysMemParamStr) == 0)
	{
		read_len = sizeof(tAppMemParam.tSYS);
		return_len = ef_get_env_blob(id_str, &tAppMemParam.tSYS, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	else
		return -99;
	
	if(return_len != read_len)
		return -40;
	#else
	//读取数据
	if(cFlash_Read8BitData(flashAPP_INFO_SATRT, (u8*)&tAppMemParam, sizeof(tAppMemParam)) <= 0)
		return -2;
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
u16 usApp_GetMemParamSize(void)
{
	#if(boardEASY_FLASH)
	return sizeof(default_env_set) / sizeof(default_env_set[0]);
	#else
	return sizeof(tAppMemParam);
	#endif
	
}

/*****************************************************************************************************************
-----函数功能    更新BOOT记忆参数
-----说明(备注)  none
-----传入参数    id_str:需要更新的对象
-----输出参数    none
-----返回值      >0 写入的字节数   0:未操作  <0:错误
******************************************************************************************************************/
s16 cApp_BootUpdateMemParam(const char* id_str)
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
-----函数功能    更新BOOT记忆参数
-----说明(备注)  none
-----传入参数    id_str:需要更新的对象
-----输出参数    none
-----返回值      >0 写入的字节数   0:未操作  <0:错误
******************************************************************************************************************/
s16 cApp_BootGetMemParam(const char* id_str)
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
			return -2;
		
		read_len = sizeof(tBootMemParam.tParam);
		return_len = ef_get_env_blob(tBootParamStr, &tBootMemParam.tParam, read_len, NULL);
		if(return_len != read_len)
			return -3;
	}
	//读取版本信息
	else if (strcmp(id_str, tBootVerInfoStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tVerInfo);
		return_len = ef_get_env_blob(id_str, &tBootMemParam.tVerInfo, read_len, NULL);
		if(return_len == 0)
			return 0;
	}
	//读取参数
	else if (strcmp(id_str, tBootParamStr) == 0)
	{
		read_len = sizeof(tBootMemParam.tParam);
		return_len = ef_get_env_blob(id_str, &tBootMemParam.tParam, read_len, NULL);
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
-----函数功能    输出录入信息
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void v_print_info(void)
{
	sMyPrint("Boot: Version		: %s\r\n", tBootMemParam.tVerInfo.saVersion);
	sMyPrint("Boot: buildTime	: %s\r\n", tBootMemParam.tVerInfo.saBuildDate);
	sMyPrint("Boot: buildTime	: %s\r\n", tBootMemParam.tVerInfo.saBuildTime);
	sMyPrint("Boot: ulCmd		: %x\r\n", tBootMemParam.tParam.ulCmd);
	sMyPrint("Boot: eAppState	: %d\r\n", tBootMemParam.tParam.eAppState);
	sMyPrint("Boot: ucAppFaultCnt: %d\r\n", tBootMemParam.tParam.ucAppFaultCnt);

	
	sMyPrint("APP : usInitFinish: %x\r\n", tAppMemParam.tParam.usInitFinish);
	sMyPrint("APP : Version     : %s\r\n", tAppMemParam.tVerInfo.saVersion);
	sMyPrint("APP : buildTime   : %s\r\n", tAppMemParam.tVerInfo.saBuildDate);
	sMyPrint("APP : buildTime   : %s\r\n", tAppMemParam.tVerInfo.saBuildTime);

	
	sMyPrint("APP : usUniqueID  :");
	for(int i = 0; i < 6; i++)
	{
		sMyPrint(" %x ", tAppMemParam.tParam.usUniqueID[i]);
	}
	sMyPrint("\r\n");
}




void vApp_Test(void)
{
//	fmc_read_8bit_data( flashBOOT_INFO_START, sizeof(tBootInfo), (u8*)&tBootInfo);
//	sMyPrint("Boot: ulCmd: %x!!!!!!!!!!!!!!!!!!!!!!!\r\n", tBootInfo.ulCmd);
}

