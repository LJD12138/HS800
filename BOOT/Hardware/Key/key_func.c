/*****************************************************************************************************************
*                                                                                                                *
 *                                         按键功能                                                             *
*                                                                                                                *
******************************************************************************************************************/
#include "Key/key_func.h"

#if(boardKEY_EN)
#include "Key/key_task.h"
#include "Sys/sys_task.h"
#include "Print/print_task.h"

#include "function.h"

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
#endif  //boardDCAC_EN

#if(boardENG_MODE_EN)
#include "key_func_eng.h"
#endif  //boardENG_MODE_EN

//****************************************************参数初始化**************************************************//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////按键功能数组要求:不满十个触发类型的要加KTE_FUN_NULL作为结束符///////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//长按	开关机
u8 const KeyTriType_SysOnOffBuff[ 2 ] = { KTE_POWER_LONG, KTE_FUN_NULL};
//点按	开关机
u8 const KeyTriType_SysOnOffBuff1[ 2 ] = { KTE_POWER_LONG, KTE_FUN_NULL};
//连击	开关机
u8 const KeyTriType_SysProteOnOffBuff[ 4 ] = { KTE_POWER_LONG, KTE_POWER_LONG, KTE_POWER_LONG,KTE_FUN_NULL};


#if(boardDCAC_EN)
//单击	开关AC
u8 const KeyTriType_AcOnOffBuff[ 2 ] = { KTE_AC_SHORT, KTE_FUN_NULL};
//长按	开关AC
u8 const KeyTriType_AcOnOffBuff1[ 2 ] = { KTE_AC_LONG, KTE_FUN_NULL};
//连击	开启AC保护
u8 const KeyTriType_AcProteOnOffBuff[ 10 ] = { KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, 
                                                KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT, KTE_AC_SHORT };
#endif  //boardDCAC_EN
													
#if(boardLIGHT_EN)
//长按	开关灯
u8 const KeyTriType_LightOnOffBuff[ 2 ] = { KTE_LIGHT_LONG, KTE_FUN_NULL};  
//单击	切换灯
u8 const KeyTriType_LightChargeBuff[ 2 ] = { KTE_LIGHT_SHORT, KTE_FUN_NULL}; 
#endif  //boardLIGHT_EN

#if(boardUSB_EN)
//单击	开关USB
u8 const KeyTriType_USBOnOffBuff[ 2 ] = { KTE_USB_SHORT, KTE_FUN_NULL}; 
//长击	开关USB
u8 const KeyTriType_USBOnOffBuff1[ 2 ] = { KTE_USB_LONG, KTE_FUN_NULL};
#endif  //boardUSB_EN
 
#if(boardDC_EN)
//单击	开关DC
u8 const KeyTriType_DCOnOffBuff[ 2 ] = { KTE_DC_SHORT, KTE_FUN_NULL}; 
//长击	开关DC
u8 const KeyTriType_DCOnOffBuff1[ 2 ] = { KTE_DC_LONG, KTE_FUN_NULL};
#endif  //boardDC_EN

#if(boardDISPLAY_EN)
//单击 	开关背光
u8 const KeyTriType_BLOnOffBuff[ 2 ] = { KTE_POWER_SHORT, KTE_FUN_NULL};
//组合 	强制开关背光
u8 const KeyTriType_ForceOpenBLBuff1[ 3 ] = { KTE_DC_LONG, KTE_USB_LONG,KTE_FUN_NULL};
u8 const KeyTriType_ForceOpenBLBuff2[ 3 ] = { KTE_USB_LONG,KTE_DC_LONG ,KTE_FUN_NULL};
#endif  //boardDISPLAY_EN



/***********************************************************************************************************************
-----函数功能    按键功能处理函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vKey_ProcKeyFunc(u8* pKeyTriTypeBuff)
{
	//******************************************开关机************************************************
	if( bFun_DataCompare( pKeyTriTypeBuff, (u8*)&KeyTriType_SysOnOffBuff, sizeof(KeyTriType_SysOnOffBuff)))  
	{
        if (uPrint.tFlag.bKeyTask)
			sMyPrint("Key_Task:开关机 \r\n");
	}
	#if(boardENG_MODE_EN)
	//******************************************工程模式************************************************************
	else if(tSysInfo.eDevState == DS_ENG_MODE)
	{
	
	}
	#endif
	//*****************************************************工作状态下**************************************************//
//	else if ( (tSysInfo.eDevState == DS_WORK || tSysInfo.eDevState == DS_ERR))
//	{	
//		if(bFun_DataCompare(pKeyTriTypeBuff, (u8*)&KeyTriType_UpdateOnOffBuff, sizeof(KeyTriType_UpdateOnOffBuff))) //连击Power按键
//		{
//			vApp_JumpToBoot(mainUPDATE_FLAG);
//			if(uPrint.tFlag.bKeyTask)
//				sMyPrint("Key_Task:系统升级\r\n");
//		}
//		else if(bFun_DataCompare(pKeyTriTypeBuff, (u8*)&KeyTriType_SysInitBuff, sizeof(KeyTriType_SysInitBuff))) //连击Power按键
//		{
//			cSys_AddQueueTask(STI_RESET,NULL,true);
//			if(uPrint.tFlag.bKeyTask)
//				sMyPrint("Key_Task:系统重置\r\n");
//		}
//		else if(bFun_DataCompare(pKeyTriTypeBuff, (u8*)&KeyTriType_ParaOnOffBuff, sizeof(KeyTriType_ParaOnOffBuff))) //连击Power按键
//		{
//			
//		}
//	}
	vKey_ParamInit();
}
#endif  //boardKEY_EN

