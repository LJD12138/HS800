/*****************************************************************************************************************
*                                                                                                                *
 *                                         TFT显示接口                                                          *
*                                                                                                                *
******************************************************************************************************************/
#include "lv_port_tft_disp.h"

#if(boardDISPLAY)
#include "lv_port_tft_gpio.h"
#include "lv_port_tft_font.h"
#include "lv_port_disp.h"
#include "function.h"

//****************************************************局部定义****************************************************//
//扫描方向定义
#define 		L2R_U2D  								0 //从左到右,从上到下
#define 		L2R_D2U  								1 //从左到右,从下到上
#define 		R2L_U2D  								2 //从右到左,从上到下
#define 		R2L_D2U  								3 //从右到左,从下到上

#define 		U2D_L2R  								4 //从上到下,从左到右
#define 		U2D_R2L  								5 //从上到下,从右到左
#define 		D2U_L2R  								6 //从下到上,从左到右
#define			D2U_R2L  								7 //从下到上,从右到左

//****************************************************参数初始化**************************************************//
#if(tftIFACE_DISP_BUFF_SIZE)
static u8 usa_tft_disp_buff[tftIFACE_DISP_BUFF_SIZE]={0};
#endif

u8 	us_tft_scan_dir;
u16	us_tft_point_color;
u16	us_tft_back_color; 

#if(!boardUSE_OS)
volatile bool bDispDataSending = 0;	//正在发送数据
#else
#include "FreeRTOS.h"
#include "task.h"
#endif  //boardUSE_OS

TFT_T 	tTFT;


/***********************************************************************************************************************
-----函数功能    写指令
-----说明(备注)  none
-----传入参数    cmd:指令  len:长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_write_cmd(u8 cmd)
{
	tftSET_DC_GPIO(0);
	
	hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
	HAL_SPI_Init(&hspi4);
	
	HAL_SPI_Transmit(&hspi4, &cmd, 1, 1);
}


/***********************************************************************************************************************
-----函数功能    写数据
-----说明(备注)  none
-----传入参数    data:数据
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_write_data(u8 data)
{	
	tftSET_DC_GPIO(1);
	
	hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
	HAL_SPI_Init(&hspi4);
	
	HAL_SPI_Transmit(&hspi4, &data, 1, 1);
}		


/***********************************************************************************************************************
-----函数功能    写u16的数据
-----说明(备注)  none
-----传入参数    data:数据  en:大小端是否切换
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_write_u16_data(u16 data, bool en)
{
	static vu16 data_temp = 0;
		
	tftSET_DC_GPIO(1);
	
	hspi4.Init.DataSize = SPI_DATASIZE_8BIT;
	HAL_SPI_Init(&hspi4);
	
	if(en)
		data_temp = usFunc_SwapU16(data);
	else 
		data_temp = usFunc_SwapU16(data);
	
	HAL_SPI_Transmit(&hspi4, (u8*)&data_temp, 2, 1);
	
	
}


/***********************************************************************************************************************
-----函数功能    写BUff
-----说明(备注)  none
-----传入参数    data:指令  len:字节长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_write_buff(u8 *data,u16 len)
{					
	tftSET_DC_GPIO(1);
	
	hspi4.Init.DataSize = SPI_DATASIZE_16BIT;
	HAL_SPI_Init(&hspi4);
	
	#if(tftDISP_SPI_SELECT == 1)
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	
	HAL_SPI_Transmit(&hspi4, data, len, 1);
	
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif

	disp_enable_update(0);
	
	#elif(tftDISP_SPI_SELECT == 2)
	
	#if(!boardUSE_OS)
	int num = 0xfff;
	while(bDispDataSending)
	{
		num--;
		if(num == 0)
		{
			bDispDataSending = false;
			return;
		}
	}
	#endif

	if(HAL_SPI_Transmit_DMA(&hspi4, data, len) != HAL_OK)
		return;
	
	#if(!boardUSE_OS)
	bDispDataSending = 1;
	#endif
	
	#endif
	
	
}

/***********************************************************************************************************************
-----函数功能    写指令
-----说明(备注)  none
-----传入参数    reg:寄存器地址  data:寄存器数据
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_write_reg(u8 reg, u16 data)
{	
	v_tft_write_cmd(reg);
	v_tft_write_data(data);
}	 

/***********************************************************************************************************************
-----函数功能    写GRAM
-----说明(备注)  none
-----传入参数    cmd:指令  len:长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_write_gram(void)
{
 	v_tft_write_cmd(tTFT.wramcmd);
}	 

/***********************************************************************************************************************
-----函数功能    设置光标位置
-----说明(备注)  none
-----传入参数    Xpos:横坐标   Ypos:纵坐标
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_set_cursor(u16 Xpos, u16 Ypos)
{	 
	v_tft_write_cmd(tTFT.setxcmd); 
	v_tft_write_u16_data(Xpos,true);
  	
	v_tft_write_cmd(tTFT.setycmd); 
	v_tft_write_u16_data(Ypos,true);
}

/***********************************************************************************************************************
-----函数功能    设置扫描方向
-----说明(备注)  注意:其他函数可能会受到此函数设置的影响(尤其是9341/6804这两个奇葩),
				 所以,一般设置为L2R_U2D即可,如果设置为其他扫描方式,可能导致显示不正常.
				 9320/9325/9328/4531/4535/1505/b505/8989/5408/9341等IC已经实际测试.
-----传入参数    dir:0~7,代表8个方向(具体定义见lcd.h)
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_set_scan_dir(u8 dir)
{
	u16 regval=0;
	u8 dirreg=0;
	switch(dir)//方向转换
	{
		case 0:dir=6;break;
		case 1:dir=7;break;
		case 2:dir=4;break;
		case 3:dir=5;break;
		case 4:dir=1;break;
		case 5:dir=0;break;
		case 6:dir=3;break;
		case 7:dir=2;break;	     
	}
	switch(dir)
	{
		case L2R_U2D://从左到右,从上到下
			regval|=(0<<7)|(0<<6)|(0<<5); 
			break;
		case L2R_D2U://从左到右,从下到上
			regval|=(1<<7)|(0<<6)|(0<<5); 
			break;
		case R2L_U2D://从右到左,从上到下
			regval|=(0<<7)|(1<<6)|(0<<5); 
			break;
		case R2L_D2U://从右到左,从下到上
			regval|=(1<<7)|(1<<6)|(0<<5);  
			break;	 
		case U2D_L2R://从上到下,从左到右   //1
			regval|=(0<<7)|(0<<6)|(1<<5); 
			break;
		case U2D_R2L://从上到下,从右到左
			regval|=(0<<7)|(1<<6)|(1<<5); //0
			break;
		case D2U_L2R://从下到上,从左到右
			regval|=(1<<7)|(0<<6)|(1<<5); 
			break;
		case D2U_R2L://从下到上,从右到左
			regval|=(1<<7)|(1<<6)|(1<<5); 
			break;	 
	}
	dirreg=0X36; 
	regval|=0x08;	//Bit3  0:RGB  1BGR
	v_tft_write_reg(dirreg,regval);
			
	v_tft_write_cmd(tTFT.setxcmd); 
	v_tft_write_data(0);v_tft_write_data(0);
	v_tft_write_data((tTFT.width-1)>>8);v_tft_write_data((tTFT.width-1)&0XFF);
	v_tft_write_cmd(tTFT.setycmd); 
	v_tft_write_data(0);v_tft_write_data(0);
	v_tft_write_data((tTFT.height-1)>>8);v_tft_write_data((tTFT.height-1)&0XFF);  
		
  	
} 

/***********************************************************************************************************************
-----函数功能    设置显示方向
-----说明(备注)  6804不支持横屏显示.
-----传入参数    dir:0,0度  1:90度   2:180度   3:270度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void v_tft_set_disp_dir(u8 dir)
{
	tTFT.wramcmd=0X2C;
	tTFT.setxcmd=0X2A;
	tTFT.setycmd=0X2B; 
	
	switch(dir)
	{		  
		case 0:					 	 		
			tTFT.width=tftSCREEN_WIDTH;
			tTFT.height=tftSCREEN_HIGH;
			us_tft_scan_dir=U2D_R2L;
		break;
		case 1:
			tTFT.width=tftSCREEN_HIGH;
			tTFT.height=tftSCREEN_WIDTH;
			us_tft_scan_dir=L2R_U2D;
		break;
		case 2:						 	 		
			tTFT.width=tftSCREEN_WIDTH;
			tTFT.height=tftSCREEN_HIGH;
			us_tft_scan_dir=U2D_L2R;
		break;
		case 3:
			tTFT.width=tftSCREEN_HIGH;
			tTFT.height=tftSCREEN_WIDTH;
			us_tft_scan_dir=L2R_D2U;
		break;	
		default:break;
	}
	
	v_tft_set_scan_dir(us_tft_scan_dir);	//默认扫描方向
}
























/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/


/***********************************************************************************************************************
-----函数功能    TFT接口参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_IfaceParamInit(void)
{
	vTFT_SetColor(tftCOLOR_RED,tftCOLOR_BLACK);
	v_tft_set_disp_dir(0);		 	//0，竖屏；1，横屏。
	vTFT_DispSwitch(true);			//点亮背光
//	vTFT_AllClear(tftCOLOR_BLACK);
}


/***********************************************************************************************************************
-----函数功能    接口初始化
-----说明(备注)  该初始化函数可以初始化各种ILI93XX液晶,但是其他函数是基于ILI9320的!!!
				 在其他型号的驱动芯片上没有测试!
-----传入参数    dir:0,竖屏；1,横屏
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_DispIfaceInit(void)
{
	MX_SPI4_Init();
	
	//LCD复位 
	tftSET_RST_GPIO(1);
	vTFT_DelayMs(1);
	tftSET_RST_GPIO(0);
	vTFT_DelayMs(80);
	tftSET_RST_GPIO(1);
	vTFT_DelayMs(10);

	
//************* Start Initial Sequence **********//
	v_tft_write_cmd(0xCF);  
	v_tft_write_data(0x00); 
	v_tft_write_data(0xC9);      //laoli   C1
	v_tft_write_data(0X30); 
	v_tft_write_cmd(0xED);  
	v_tft_write_data(0x64); 
	v_tft_write_data(0x03); 
	v_tft_write_data(0X12); 
	v_tft_write_data(0X81); 
	v_tft_write_cmd(0xE8);  
	v_tft_write_data(0x85); 
	v_tft_write_data(0x10); 
	v_tft_write_data(0x7A); 
	v_tft_write_cmd(0xCB);  
	v_tft_write_data(0x39); 
	v_tft_write_data(0x2C); 
	v_tft_write_data(0x00); 
	v_tft_write_data(0x34); 
	v_tft_write_data(0x02); 
	v_tft_write_cmd(0xF7);  
	v_tft_write_data(0x20); 
	v_tft_write_cmd(0xEA);  
	v_tft_write_data(0x00); 
	v_tft_write_data(0x00); 
	
	//Power control 
	v_tft_write_cmd(0xC0);    
	v_tft_write_data(0x1B);   //VRH[5:0] 

	v_tft_write_cmd(0xC1);    //Power control 
	v_tft_write_data(0x00);   //SAP[2:0];BT[3:0]     //laoli   01
	
	//VCM control 
	v_tft_write_cmd(0xC5);    
	v_tft_write_data(0x30); 	 //3F
	v_tft_write_data(0x30); 	 //3C
	
	v_tft_write_cmd(0xC7);    //VCM control2 
	v_tft_write_data(0XB7); 
	
	// Memory Access Control
	v_tft_write_cmd(0x36);     
	v_tft_write_data(0x08);    //laoli   48
	
	v_tft_write_cmd(0x3A);   
	v_tft_write_data(0x55); 
	
	v_tft_write_cmd(0xB1);   
	v_tft_write_data(0x00);   
	v_tft_write_data(0x1A); 
	
	// Display Function Control 
	v_tft_write_cmd(0xB6);    
	v_tft_write_data(0x0A); 
	v_tft_write_data(0xA2); 
	
	// 3Gamma Function Disable 
	v_tft_write_cmd(0xF2);    
	v_tft_write_data(0x00); 
	
	v_tft_write_cmd(0x26);    //Gamma curve selected 
	v_tft_write_data(0x01); 
	
	// Set Gamma
	v_tft_write_cmd(0xE0);
	v_tft_write_data(0x0F); 
	v_tft_write_data(0x2A); 
	v_tft_write_data(0x28); 
	v_tft_write_data(0x08); 
	v_tft_write_data(0x0E); 
	v_tft_write_data(0x08); 
	v_tft_write_data(0x54); 
	v_tft_write_data(0XA9); 
	v_tft_write_data(0x43); 
	v_tft_write_data(0x0A); 
	v_tft_write_data(0x0F); 
	v_tft_write_data(0x00); 
	v_tft_write_data(0x00); 
	v_tft_write_data(0x00); 
	v_tft_write_data(0x00); 
	
	v_tft_write_cmd(0XE1);
	v_tft_write_data(0x00); 
	v_tft_write_data(0x15); 
	v_tft_write_data(0x17); 
	v_tft_write_data(0x07); 
	v_tft_write_data(0x11); 
	v_tft_write_data(0x06); 
	v_tft_write_data(0x2B); 
	v_tft_write_data(0x56); 
	v_tft_write_data(0x3C); 
	v_tft_write_data(0x05); 
	v_tft_write_data(0x10); 
	v_tft_write_data(0x0F); 
	v_tft_write_data(0x3F); 
	v_tft_write_data(0x3F); 
	v_tft_write_data(0x0F);
	
	// 设置显示区域	
	v_tft_write_cmd(0x2B); 
	v_tft_write_data(0x00);
	v_tft_write_data(0x00);
	v_tft_write_data(0x01);
	v_tft_write_data(0x3f);
	
	v_tft_write_cmd(0x2A); 
	v_tft_write_data(0x00);
	v_tft_write_data(0x00);
	v_tft_write_data(0x00);
	v_tft_write_data(0xef);	

	// 退出睡眠模式
	v_tft_write_cmd(0x11);
	vTFT_DelayMs(120);
	
	// 开启显示
	v_tft_write_cmd(0x29);
	
	vTFT_IfaceParamInit();
}  

/***********************************************************************************************************************
-----函数功能    显示开关
-----说明(备注)  none
-----传入参数    sw
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_DispSwitch(bool sw)
{
	if(sw)
	{
		
	}
	else
	{
		
	}
	tftSET_BL_GPIO(sw);
}


/***********************************************************************************************************************
-----函数功能    清理全屏
-----说明(备注)  none
-----传入参数    color:要清屏的填充色
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_AllClear(u16 color)
{
	u32 index=0;      
	u32 totalpoint=tTFT.width;
	
	totalpoint*=tTFT.height; 	//得到总点数
	v_tft_set_cursor(0x00,0x0000);	//设置光标位置
	v_tft_write_gram();     //开始写入GRAM	
	for(index=0;index<totalpoint;index++)
	{
		v_tft_write_u16_data(color,false);	
	}
}  

/***********************************************************************************************************************
-----函数功能    设置窗口大小
-----说明(备注)  none
-----传入参数    开启坐标:xStar,yStar    结束:xEnd,yEnd
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_SetWindows(u16 xStar, u16 yStar,u16 xEnd,u16 yEnd)
{	
	v_tft_write_cmd(tTFT.setxcmd);
	v_tft_write_u16_data(xStar,true);
	v_tft_write_u16_data(xEnd,true);	

	v_tft_write_cmd(tTFT.setycmd);
	v_tft_write_u16_data(yStar,true);
	v_tft_write_u16_data(yEnd,true);

	v_tft_write_gram();		
}   

/***********************************************************************************************************************
-----函数功能    区域填充单种颜色
-----说明(备注)  区域大小为:(ex-sx+1)*(ey-sy+1)
-----传入参数    开启坐标:xStar,yStar    
				 结束坐标:xEnd,yEnd    
				 color:要填充的颜色
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_Fill(u16 sx,u16 sy,u16 ex,u16 ey,u16 color)
{          
	u16 i,j;			
	u16 width=ex-sx+1; 		
	u16 height=ey-sy+1;	
	
	//设置显示窗口
	vTFT_SetWindows(sx,sy,ex,ey);
	
	for(i=0;i<height;i++)
	{
		for(j=0;j<width;j++)
		{
			v_tft_write_u16_data(color,false); 
		}
        
	}
	
	//恢复窗口设置为全屏
	vTFT_SetWindows(0,0,tTFT.width,tTFT.height);
}  

/***********************************************************************************************************************
-----函数功能    区域填充单种颜色
-----说明(备注)  区域大小为:(ex-sx+1)*(ey-sy+1)
-----传入参数    开启坐标:xStar,yStar    
				 结束坐标:xEnd,yEnd    
				 color:要填充的颜色
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_ColorFill(u16 sx,u16 sy,u16 ex,u16 ey,u16 *color)
{  
	u16 height,width;
	u16 i,j;
	
	width=ex-sx+1; 		//得到填充的宽度
	height=ey-sy+1;		//高度
	
 	for(i=0;i<height;i++)
	{
 		v_tft_set_cursor(sx,sy+i);   	//设置光标位置 
		v_tft_write_gram();     //开始写入GRAM
		
		for(j=0;j<width;j++)
		{
			v_tft_write_u16_data(color[i*height+j],false);//写入数据 
		}
			
	}	  
}


/***********************************************************************************************************************
-----函数功能    画点
-----说明(备注)  none
-----传入参数    x:横坐标   y:纵坐标
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_DrawPoint(u16 x,u16 y)
{
	v_tft_set_cursor(x,y);//设置光标位置 
	v_tft_write_gram();	//开始写入GRAM
	v_tft_write_u16_data(us_tft_point_color,false); //此点的颜色
}

/***********************************************************************************************************************
-----函数功能    画线
-----说明(备注)  none
-----传入参数    开启坐标:xStar,yStar    
				 结束坐标:xEnd,yEnd    
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_DrawLine(u16 x1, u16 y1, u16 x2, u16 y2)
{
	u16 t; 
	int xerr=0,yerr=0,delta_x,delta_y,distance; 
	int incx,incy,uRow,uCol; 
	
	delta_x=x2-x1; //计算坐标增量 
	delta_y=y2-y1; 
	uRow=x1; 
	uCol=y1; 
	
	if(delta_x>0)incx=1; //设置单步方向 
	else if(delta_x==0)incx=0;//垂直线 
	else {incx=-1;delta_x=-delta_x;} 
	
	if(delta_y>0)incy=1; 
	else if(delta_y==0)incy=0;//水平线 
	else{incy=-1;delta_y=-delta_y;} 
	
	if( delta_x>delta_y)distance=delta_x; //选取基本增量坐标轴 
	else distance=delta_y; 
	
	for(t=0;t<=distance+1;t++ )//画线输出 
	{  
		vTFT_DrawPoint(uRow,uCol);//画点 
		xerr+=delta_x ; 
		yerr+=delta_y ; 
		if(xerr>distance) 
		{ 
			xerr-=distance; 
			uRow+=incx; 
		} 
		if(yerr>distance) 
		{ 
			yerr-=distance; 
			uCol+=incy; 
		} 
	}  
}  


/***********************************************************************************************************************
-----函数功能    画矩形
-----说明(备注)  none
-----传入参数    开启坐标:xStar,yStar    
				 结束坐标:xEnd,yEnd    
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_DrawRectangle(u16 x1, u16 y1, u16 x2, u16 y2)
{
	vFTF_DrawLine(x1,y1,x2,y1);
	vFTF_DrawLine(x1,y1,x1,y2);
	vFTF_DrawLine(x1,y2,x2,y2);
	vFTF_DrawLine(x2,y1,x2,y2);
}

/***********************************************************************************************************************
-----函数功能    画圆
-----说明(备注)  none
-----传入参数    x,y:中心点    
				 r  :半径   
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_DrawCircle(u16 x0,u16 y0,u8 r)
{
	int a,b;
	int di;
	a=0;b=r;	  
	di=3-(r<<1);             //判断下个点位置的标志
	while(a<=b)
	{
		vTFT_DrawPoint(x0+a,y0-b);             //5
 		vTFT_DrawPoint(x0+b,y0-a);             //0           
		vTFT_DrawPoint(x0+b,y0+a);             //4               
		vTFT_DrawPoint(x0+a,y0+b);             //6 
		vTFT_DrawPoint(x0-a,y0+b);             //1       
 		vTFT_DrawPoint(x0-b,y0+a);             
		vTFT_DrawPoint(x0-a,y0-b);             //2             
  		vTFT_DrawPoint(x0-b,y0-a);             //7     	         
		a++;
		//使用Bresenham算法画圆     
		if(di<0)di +=4*a+6;	  
		else
		{
			di+=10+4*(a-b);   
			b--;
		} 						    
	}
} 									  

/***********************************************************************************************************************
-----函数功能    显示字符
-----说明(备注)  none
-----传入参数       
				 x,y:起始坐标
				 num:要显示的字符:" "--->"~"
				 size:字体大小 12/16
				 mode:叠加方式(1)还是非叠加方式(0)   
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_ShowChar(u16 x,u16 y,u8 num,u8 size,u8 mode)
{  							  
  u8 temp,t1,t;
	u16 y0=y;
	u16 colortemp= us_tft_point_color;  
  u8 add=0;
	//设置窗口		   
	num = num - ' ';//得到偏移后的值
  if (size == 24)
    add = 12;
	if(mode) //是否带底色
	{
    for(t=0;t<size+add;t++)
    {   
      switch(size)
      {
        case 12:
          temp=asc2_1206[num][t];  //调用1206字体
          break;
        case 16:
          temp=asc2_1608[num][t];	//调用1608字体 	                          
          break;
        case 24:
          temp=asc2_2412[num][t];	//调用1608字体 
          break;
      }
      for(t1=0;t1<8;t1++)
			{			    
        if(temp&0x80)
           us_tft_point_color=colortemp;
				else 
           us_tft_point_color=us_tft_back_color;
				vTFT_DrawPoint(x,y);	
				temp<<=1;
				y++;
				if(x>=tTFT.width)//超区域了
        {
           us_tft_point_color=colortemp;
          return;
        }
				if((y - y0) == size)
				{
					y=y0;
					x++;
					if(x>=tTFT.width)//超区域了
          {
             us_tft_point_color=colortemp;
            return;
          }
					break;
				}
			}  	 
    }    
	}
  else//叠加方式
	{
    for(t=0;t<size+add;t++)
    {   
      switch(size)
      {
        case 12:
          temp=asc2_1206[num][t];  //调用1206字体
          break;
        case 16:
          temp=asc2_1608[num][t];		 //调用1608字体 	                          
          break;
        case 24:
          temp=asc2_2412[num][t];		 //调用1608字体 
          break;
      }
      for(t1=0;t1<8;t1++)
			{			    
        if(temp&0x80)vTFT_DrawPoint(x,y); 
				temp<<=1;
				y++;
				if(x>=tTFT.height){ us_tft_point_color=colortemp;return;}//超区域了
				if((y-y0)==size)
				{
					y=y0;
					x++;
					if(x>=tTFT.width){ us_tft_point_color=colortemp;return;}//超区域了
					break;
				}
			}  	 
    }     
	}
	 us_tft_point_color=colortemp;	    	   	 	  
}   
			 

/***********************************************************************************************************************
-----函数功能    显示数字
-----说明(备注)  高位为0,则不显示
-----传入参数       
				 x,y :起点坐标	 
				 len :数字的位数
				 size:字体大小
				 color:颜色 
				 num:数值(0~4294967295);  
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_ShowNum(u16 x,u16 y,u32 num,u8 len,u8 size)
{         	
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/ulFunc_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				vFTF_ShowChar(x+(size/2)*t,y,' ',size,0);
				continue;
			}else enshow=1; 
		 	 
		}
	 	vFTF_ShowChar(x+(size/2)*t,y,temp+'0',size,0); 
	}
} 

/***********************************************************************************************************************
-----函数功能    显示数字
-----说明(备注)  高位为0,则显示
-----传入参数       
				 x,y:起点坐标
				 num:数值(0~999999999);	 
				 len:长度(即要显示的位数)
				 size:字体大小
				 mode:
				 [7]:0,不填充;1,填充0.
				 [6:1]:保留
				 [0]:0,非叠加显示;1,叠加显示. 
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_ShowNumZero(u16 x,u16 y,u32 num,u8 len,u8 size,u8 mode)
{  
	u8 t,temp;
	u8 enshow=0;						   
	for(t=0;t<len;t++)
	{
		temp=(num/ulFunc_Pow(10,len-t-1))%10;
		if(enshow==0&&t<(len-1))
		{
			if(temp==0)
			{
				if(mode&0X80)vFTF_ShowChar(x+(size/2)*t,y,'0',size,mode&0X01);  
				else vFTF_ShowChar(x+(size/2)*t,y,' ',size,mode&0X01);  
 				continue;
			}else enshow=1; 
		 	 
		}
	 	vFTF_ShowChar(x+(size/2)*t,y,temp+'0',size,mode&0X01); 
	}
} 

/***********************************************************************************************************************
-----函数功能    显示字符串
-----说明(备注)  高位为0,则显示
-----传入参数       
				 x,y:起点坐标
				 width,height:区域大小  
				 size:字体大小
				 *p:字符串起始地址		
				 mode: 是否带底色 
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFTF_ShowString(u16 x,u16 y,u16 width,u16 height,u8 size,u8 *p, u8 mode)
{         
	u8 x0=x;
	width+=x;
	height+=y;
    while((*p<='~')&&(*p>=' '))//判断是不是非法字符!
    {       
        if(x>=width){x=x0;y+=size;}
        if(y>=height)break;//退出
        vFTF_ShowChar(x,y,*p,size,mode);
        x+=size/2;
        p++;
    }  
}


/***********************************************************************************************************************
-----函数功能    区域快速绘制
-----说明(备注)  区域大小为:(ex - sx + 1) * (ey - sy + 1)
-----传入参数       
				 sx,sy:起点坐标
				 ex,ey:结束坐标		
				 color: 颜色数据指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_FastDrawColor(int16_t sx, int16_t sy,int16_t ex, int16_t ey, uint16_t *color)
{
	uint32_t draw_size = (ex-sx+1) * (ey-sy+1);
	
	disp_disable_update();
	
	//分段发送
	#if(tftIFACE_DISP_BUFF_SIZE)
	uint16_t us_uint16_len = tftIFACE_DISP_BUFF_SIZE/2;
	#endif
	
	//设置需要画的窗口大小
	vTFT_SetWindows(sx, sy, ex, ey);
	v_tft_write_gram();
	
	#if(tftIFACE_DISP_BUFF_SIZE)
	while(draw_size)
	{
		if(draw_size > us_uint16_len)
		{
			v_tft_write_buff((u8*)color, us_uint16_len);
			color = color + us_uint16_len;
			draw_size = draw_size - us_uint16_len;
		}
		else
		{
			v_tft_write_buff((u8*)color, draw_size);
			draw_size = 0;
		}
	}
	#else
	v_tft_write_buff((u8*)color, draw_size);
	#endif  //tftIFACE_DISP_BUFF_SIZE
}


/***********************************************************************************************************************
-----函数功能    延时函数
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_DelayMs(u16 delay)
{
	#if(boardUSE_OS)
	vTaskDelay(delay);
	#else
	vu16 temp = 50;
	for(int i = delay; i > 0; i--)
		while(temp--);
	#endif
}


/***********************************************************************************************************************
-----函数功能    设置颜色
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vTFT_SetColor(u16 point_color, u16 back_color)
{
	us_tft_point_color = point_color;
	us_tft_back_color = back_color;
}	

#endif  //boardDISPLAY
