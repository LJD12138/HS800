
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
#ifndef LV_PORT_TFT_IFACE_H_
#define LV_PORT_TFT_IFACE_H_

#include "board_config.h"

#if(boardDISPLAY)

#include "main.h"


//屏幕像素 竖屏排布
#define 		tftSCREEN_WIDTH 						240
#define 		tftSCREEN_HIGH 							320

//显示缓存数组大小
#define 		tftIFACE_DISP_BUFF_SIZE					0    //0:关闭分段发送 这个必须为双数
	 
//5红     6蓝     5绿
//画笔颜色
#define 		tftCOLOR_WHITE     	 					0xFFFF
#define 		tftCOLOR_BLACK      					0x0000	  
#define 		tftCOLOR_RED        					0xF800 
#define 		tftCOLOR_GREEN      					0x07E0
#define 		tftCOLOR_BLUE       					0x001F

#define 		tftCOLOR_BRED       					0XF81F
#define 		tftCOLOR_GRED 		   					0XFFE0
#define 		tftCOLOR_GBLUE		   					0X07FF
#define 		tftCOLOR_MAGENTA    					0xF81F
#define 		tftCOLOR_CYAN       					0x7FFF
#define 		tftCOLOR_YELLOW     					0xFFE0
#define 		tftCOLOR_BROWN 	   						0XBC40 	//棕色
#define 		tftCOLOR_BRRED 	   						0XFC07 	//棕红色
#define 		tftCOLOR_GRAY  	   						0X8430 	//灰色
#define 		tftCOLOR_DARKBLUE      	 				0X01CF	//深蓝色
#define 		tftCOLOR_LIGHTBLUE      	 			0X7D7C	//浅蓝色  
#define 		tftCOLOR_GRAYBLUE       	 			0X5458 	//灰蓝色
#define 		tftCOLOR_LIGHTGREEN     	 			0X841F 	//浅绿色
#define 		tftCOLOR_LIGHTGRAY        				0XEF5B 	//浅灰色(PANNEL)
#define 		tftCOLOR_LGRAY 				 			0XC618 	//浅灰色(PANNEL),窗体背景色
#define 		tftCOLOR_LGRAYBLUE        				0XA651 	//浅灰蓝色(中间层颜色)
#define 		tftCOLOR_LBBLUE           				0X2B12 	//浅棕蓝色(选择条目的反色)

//LCD重要参数集
typedef struct  
{
	//1Tab				//5Tab				//5Tab
	u16 				width;				//LCD 宽度
	u16 				height;				//LCD 高度
	u16 				id;					//LCD ID
	u8  				dir;				//横屏还是竖屏控制：0，竖屏；1，横屏。	
	u8					wramcmd;			//开始写gram指令
	u8  				setxcmd;			//设置x坐标指令
	u8  				setycmd;			//设置y坐标指令	 
}TFT_T; 	  
extern TFT_T 			tTFT;


//LCD的画笔颜色和背景色
extern 	u16  			us_tft_point_color;		//默认红色    
extern 	u16  			us_tft_back_color; 		//背景颜色.默认为白色
extern 	u8 				us_tft_scan_dir;		//扫描方向
#if(!boardUSE_OS)
extern volatile bool 	bDispDataSending;		//正在发送数据
#endif

	    															  
void vTFT_DispIfaceInit(void);													//初始化
void vTFT_DispSwitch(bool sw);												//显示开关
void vTFT_AllClear(u16 Color);												//清屏
void vTFT_SetWindows(u16 sx,u16 sy,u16 width,u16 height);					//设置窗口大小
void vFTF_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color);		   				//填充单色
void vFTF_ColorFill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color);				//填充指定颜色
void vTFT_DrawPoint(u16 x,u16 y);											//画点
void vFTF_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2);							//画线
void vFTF_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2);		   			//画矩形
void vFTF_DrawCircle(u16 x0,u16 y0,u8 r);									//画圆
void vFTF_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode);						//显示一个字符
void vFTF_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size);  					//显示一个数字
void vFTF_ShowNumZero(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode);			//显示 数字
void vFTF_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p, u8 mode);		//显示一个字符串,12/16/24字体
void vTFT_FastDrawColor(int16_t sx, int16_t sy,int16_t ex, int16_t ey, uint16_t *color);  //区域快速绘制
void vTFT_DelayMs(u16 delay);
void vTFT_SetColor(u16 point_color, u16 back_color);

#endif  //boardDISPLAY

#endif  //LV_PORT_TFT_IFACE_H_




