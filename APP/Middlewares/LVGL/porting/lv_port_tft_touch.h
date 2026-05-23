
/**************************************************************************************************/	
 /* @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**************************************************************************************************/	
#ifndef LV_PORT_TFT_TOUCH_H_
#define LV_PORT_TFT_TOUCH_H_

#include "board_config.h"

#if(boardDISPLAY)

#include "main.h"


#define 		TP_PRES_DOWN 							0x80  //触屏被按下	  
#define 		TP_CATH_PRES 							0x40  //有按键按下了 	  
										    
//触摸屏控制器
typedef struct
{
	//1Tab				//5Tab				//5Tab
	u8 					(*scan)(u8);		//扫描触摸屏.0,屏幕扫描;1,物理坐标;
	u8 					(*ucpIsPress)(void);//触摸被按下	
	void 				(*adjust)(void);	//触摸屏校准
	u16 				x0;					//原始坐标(第一次按下时的坐标)
	u16 				y0;
	u16 				x; 					//当前坐标(此次扫描时,触屏的坐标)
	u16 				y;						   	    
	u8  				ucState;			//笔的状态 
											//b7:按下1/松开0; 
											//b6:0,没有按键功能需要处理;
	                                        //   1,有按键功能需要处理.         			  
////////////////////////触摸屏校准参数/////////////////////////								
	float 				xfac;					
	float 				yfac;
	short 				xoff;
	short 				yoff;	   
//新增的参数,当触摸屏的左右上下完全颠倒时需要用到.
//touchtype=0的时候,适合左右为X坐标,上下为Y坐标的TP.
//touchtype=1的时候,适合左右为Y坐标,上下为X坐标的TP.
	u8 					touchtype;
}Touch_T;

extern 	Touch_T 		tTouch;	 
  

s8 cTft_TouchIfaceInit(void);
void vTft_DrowTouchPoint(u16 x,u16 y,u16 color);//画一个坐标校准点
void vTft_DrawTouchBigPoint(u16 x,u16 y,u16 color);	//画一个大点															 
void vTft_ShowTouchAdjInfo(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac);//显示校准信息
void Touch_Test(void);  

#endif  //boardDISPLAY

#endif  //LV_PORT_TFT_TOUCH_H_

















