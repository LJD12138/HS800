/*****************************************************************************************************************
*                                                                                                                *
 *                                         LCD错误显示                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "MD_Display/md_display_task.h"

#if(boardDISPLAY_EN)
#include "Sys/sys_task.h"

#if(boardBUZ_EN)
#include "Buz/buz_task.h"
#endif  //boardBUZ_EN

#if(boardUSB_EN)
#include "Usb/usb_task.h"
#endif  //boardUSB_EN

#if(boardDC_EN)
#include "Dc/dc_task.h"
#endif  //boardDC_EN

#if(boardBMS_EN)
#include "MD_Bms/md_bms_task.h"
#endif  //boardBMS_EN

#if(boardMPPT_EN)
#include "MD_Mppt/md_mppt_task.h"
#endif  //boardMPPT_EN

#if(boardDCAC_EN)
#include "MD_Dcac/md_dcac_task.h"
#include "MD_Dcac/md_dcac_rec_task.h"
#endif  //boardDCAC_EN



/***********************************************************************************************************************
-----函数功能    LCD错误代码显示
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      错误代码,大于99没有错误
************************************************************************************************************************/
u16 usDisp_ErrCodeDisplay(void)
{
	static vu16 us_err_step = 0;
	static vu16 us_err_last_step = 0;
	static vu16 us_err_disp_cnt = 0;
	
	
	if(tSysInfo.uErrCode.usCode == 0

		#if(boardUSB_EN)
		&& tUsb.uErrCode.ucErrCode == 0
		#endif  //boardUSB_EN

		#if(boardDC_EN)
		&& tDc.uErrCode.ucErrCode == 0
		#endif  //boardDC_EN

		#if(boardBMS_EN)
		&& tBms.uErrCode.ulCode == 0
		#endif  //boardBMS_EN

		#if(boardMPPT_EN)
		&& tMppt.uErrCode.ulCode == 0
		#endif  //boardMPPT_EN

		#if(boardDCAC_EN)
		&& tDcac.uErrCode.ulCode == 0
		#endif  //boardDCAC_EN

		)
	{
		return 100;
	}

	switch(us_err_step)
	{
		//------------------------SYS 0~9------------------------------------
		{
		case 0:
			if(tSysInfo.uErrCode.tCode.bOT)
				break;
			else
				us_err_step++;
			
		case 1:
			if(tSysInfo.uErrCode.tCode.bUT)
				break;
			else
				us_err_step++;
			
		case 2:
			if(tSysInfo.uErrCode.tCode.bOV)
				break;
			else
				us_err_step++;
		
		case 3:
			if(tSysInfo.uErrCode.tCode.bUV)
				break;
			else
				us_err_step++;
			
		case 4:
			if(tSysInfo.uErrCode.tCode.bOL)
				break;
			else
				us_err_step++;
		
		case 5:
			us_err_step++;
		case 6:
			us_err_step++;
		case 7:
			us_err_step++;
		case 8:
			us_err_step++;
		case 9:
			us_err_step++;
		}
		
		//------------------------BMS 10~35----------------------------------
		{
		case 10:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bCellOV)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 11:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bCellUV)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 12:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bEnvOT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		
		case 13:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bEnvUT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 14:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bCOT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		
		case 15:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bCUT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 16:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bDCOT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 17:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bDCUT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		
		case 18:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bCOC)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 19:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bDCOC)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		case 20:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bSC)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 21:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bBatFull)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		
		case 22:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bAfeLost)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 23:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bCurrErr)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		case 24:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bPerchgFault)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		case 25:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.uBmsCode.tCode.bLowVoltOL)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		case 26:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.bSysDevLost)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 27:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.bSysChgOT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 28:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.bSysDisChgOT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		
		case 29:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.bSysChgUT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
			
		case 30:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.bSysDisChgUT)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		
		case 31:
			#if(boardBMS_EN)
			if(tBms.uErrCode.tCode.bSysLV)
				break;
			else
			#endif  //boardBMS_EN
				us_err_step++;
		case 32:
			us_err_step++;
		case 33:
			us_err_step++;
		case 34:
			us_err_step++;
		case 35:
			us_err_step++;
		}
		
		//------------------------MPPT 36~53---------------------------------
		{
		case 36:
			us_err_step++;
			
		case 37:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptInOV)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
			
		case 38:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptInSC)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
		
		case 39:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptInOC)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
			
		case 40:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptOutOV)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
		
		case 41:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptOutOC)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
			
		case 42:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptOutSC)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
			
		case 43:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bMpptOT)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
		
		case 44:
			us_err_step++;
			
		case 45:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bDevLost)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
		case 46:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bSysOT)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
			
		case 47:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bSysUT)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
		
		case 48:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bSysOV)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
			
		case 49:
			#if(boardMPPT_EN)
			if(tMppt.uErrCode.tCode.bSysOL)
				break;
			else
			#endif  //boardMPPT_EN
				us_err_step++;
		case 50:
			us_err_step++;
		case 51:
			us_err_step++;
		case 52:
			us_err_step++;
		case 53:
			us_err_step++;
		}
		
		//------------------------DCAC 54~83---------------------------------
		{
		case 54:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacBatUV)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 55:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacBatOV)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 56:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacOT)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 57:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacNtc)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 58:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacOL)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 59:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacSC)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 60:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacOutVolt)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 61:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacInFreq)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 62:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacInVolt)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 63:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacPara)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 64:
//			if(tDcac.uErrCode.tCode.bUV1 ||
//				tDcac.uErrCode.tCode.bUV2 ||
//				tDcac.uErrCode.tCode.bUV3)
//				break;
//			else
				us_err_step++;
			
		case 65:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacOC)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 66:
//			if(tDcac.uErrCode.tCode.bUF1 ||
//				tDcac.uErrCode.tCode.bUF2)
//				break;
//			else
				us_err_step++;
			
		case 67:
//			if(tDcac.uErrCode.tCode.bAcErr)
//				break;
//			else
				us_err_step++;
			
		case 68:
//			if(tDcac.uErrCode.tCode.bLockErr)
//				break;
//			else
				us_err_step++;
			
		case 69:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacFuse ||
				tDcac.uErrCode.tCode.bDcacFuse)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 70:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysDevLost)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 71:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysOT)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 72:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysUT)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 73:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysOV)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 74:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysLV)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		
		case 75:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysSetInProte)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
		case 76:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysOutOL)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 77:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysOutErr)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 78:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bSysInOC)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;
			
		case 79:
			#if(boardDCAC_EN)
			if(tDcac.uErrCode.tCode.bDcacEeprom)
				break;
			else
			#endif  //boardDCAC_EN
				us_err_step++;

		case 80:
			us_err_step++;
		case 81:
			us_err_step++;
		case 82:
			us_err_step++;
		case 83:
//			if(tDcacRx.uInErr.tCode.bFuse ||
//				tDcacRx.uInErr.tCode.bRelay)
//				break;
//			else
				us_err_step++;
		}
	
		//------------------------DC 84~91------------------------------------
		{
		case 84:
			#if(boardDC_EN)
			if(tDc.uErrCode.tCode.bPowerErr)
				break;
			else
			#endif  //boardDC_EN
				us_err_step++;
			
		case 85:
			#if(boardDC_EN)
			if(tDc.uErrCode.tCode.bOT)
				break;
			else
			#endif  //boardDC_EN
				us_err_step++;
			
		case 86:
			#if(boardDC_EN)
			if(tDc.uErrCode.tCode.bOL)
				break;
			else
			#endif  //boardDC_EN
				us_err_step++;
		
		case 87:
			#if(boardDC_EN)
			if(tDc.uErrCode.tCode.bCloseFail)
				break;
			else
			#endif  //boardDC_EN
				us_err_step++;
			
		case 88:
			#if(boardDC_EN)
			if(tDc.uErrCode.tCode.bOutLow)
				break;
			else
			#endif  //boardDC_EN
				us_err_step++;
			
		case 89:
			#if(boardDC_EN)
			if(tDc.uErrCode.tCode.bOutHigh)
				break;
			else
			#endif  //boardDC_EN
				us_err_step++;
		
		case 90:
			us_err_step++;
		case 91:
			us_err_step++;
		}
		
		//------------------------USB 92~99------------------------------------
		{
		case 92:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bPowerErr)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;
			
		case 93:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bOT)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;
			
		case 94:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bOL)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;
		
		case 95:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bBatUV)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;
			
		case 96:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bIc1Lost)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;
			
		case 97:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bIc2Lost)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;

		case 98:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bBootFault)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;

		case 99:
			#if(boardUSB_EN)
			if(tUsb.uErrCode.tCode.bCloseFault)
				break;
			else
			#endif  //boardUSB_EN
				us_err_step++;
		}
		
		default:
		case 100:
			break;
		
	}
	
	if(us_err_last_step != us_err_step)
	{
		us_err_last_step = us_err_step;
		us_err_disp_cnt = 0;
	}
	
	us_err_disp_cnt++;
	if(us_err_disp_cnt >= (1000/boardDISP_REFRESH_TMIE))
	{
		us_err_disp_cnt = 0;
		us_err_step++;
		if(us_err_step > 100)
			us_err_step = 0;	
		
		return 100;
	}
	else
	{
		return us_err_step;
	}	
}
#endif  //boardDISPLAY_EN
