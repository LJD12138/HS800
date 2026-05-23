
 /* @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, QD electronic SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
**************************************************************************************************/	
#include "lv_port_tft_touch.h" 

#if(boardDISPLAY)
#include "lv_port_tft_disp.h"
#include "lv_port_tft_gpio.h"
#include "Key/key_task.h"
#include "Print/print_task.h"

#include "filtration.h"
#include "function.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

//****************************************************函数定义*****************************************************//
static u8 uc_touch_scan(u8 tp);
static u8 uc_touch_is_press(void);
static void v_touch_adjust(void);
static void v_touch_save_adjdata(void);
static u8 uc_touch_get_adjdata(void);


//****************************************************参数初始化**************************************************//

//触摸AD滤波器
#define 		tftTOUCH_AD_FILTER_BUFF_SIZE     	5 
static u16		usa_touch_ad_rec_value[tftTOUCH_AD_FILTER_BUFF_SIZE] = {0};


u16 ColorTab[5]={tftCOLOR_RED,tftCOLOR_GREEN,tftCOLOR_BLUE,tftCOLOR_YELLOW,tftCOLOR_BRED};//定义颜色数组

Touch_T tTouch=
{
	uc_touch_scan,
	uc_touch_is_press,
	v_touch_adjust,
	0,
	0,
 	0,
	0,
	0,
	0,
	0,
	0,	  	 		
	0,
	0,	  	 		
};					
//默认为touchtype=0的数据.
u8 CMD_RDX=0XD0;
u8 CMD_RDY=0X90;

/*****************************************************************************
 * @name       :static void v_touch_delay_us(u16 delay)  
 * @date       :2018-08-09 
 * @function   :延时
 * @parameters :delay us
 * @retvalue   :None
******************************************************************************/
#if(tftTOUCH_SPI_SELECT == 1)
static void v_touch_delay_us(u16 delay)
{
	for(int i = 0; i < delay; i++)
	{
		u16 m = 400;
		while(m)
		{
			m--;
		}
	}
}
#endif

/*****************************************************************************
 * @name       :void v_touch_write_byte(u8 num)   
 * @date       :2018-08-09 
 * @function   :Write a byte data to the touch screen IC with SPI bus
 * @parameters :num:Data to be written
 * @retvalue   :None
******************************************************************************/  	 			    					   
static bool b_touch_write_byte(u8 num)    
{
	#if(tftTOUCH_SPI_SELECT == 1)
	u8 count=0;   
	for(count=0;count<8;count++)  
	{ 	  
		if(num&0x80)tftSET_MOSI_GPIO(1);  
		else tftSET_MOSI_GPIO(0);   
		num<<=1;    
		tftSET_SCLK_GPIO(0); 	 
		tftSET_SCLK_GPIO(1);		//上升沿有效	        
	}
	return true;
	#elif(tftTOUCH_SPI_SELECT == 2)
	if(HAL_SPI_Transmit(&hspi2, &num, 1, 1) != HAL_OK)
		return false;
	else
		return true;
	#endif
}

/*****************************************************************************
 * @name       :u16 us_touch_read_ad_value(u8 CMD)	  
 * @date       :2018-08-09 
 * @function   :Reading adc values from touch screen IC with SPI bus
 * @parameters :CMD:Read command,0xD0 for x,0x90 for y
 * @retvalue   :Read data
******************************************************************************/    
static u16 us_touch_read_ad_value(u8 cmd)	  
{
	u16 us_read_value=0; 
	
	tftSET_TCS_GPIO(0); 		//选中触摸屏IC

	#if(tftTOUCH_SPI_SELECT == 1)
	tftSET_SCLK_GPIO(0);		//先拉低时钟 	 
	tftSET_MOSI_GPIO(0); 		//拉低数据线
	b_touch_write_byte(cmd);	//发送命令字
	v_touch_delay_us(6);		//ADS7846的转换时间最长为6us
	tftSET_SCLK_GPIO(0); 	     	    
	v_touch_delay_us(6);    	   
	tftSET_SCLK_GPIO(1);		//给1个时钟，清除BUSY	    	    
	tftSET_SCLK_GPIO(0);
	
	for(int count=0;count<16;count++)//读出16位数据,只有高12位有效 
	{ 				  
		us_read_value<<=1; 	 
		tftSET_SCLK_GPIO(0);	//下降沿有效  	    	   
		tftSET_SCLK_GPIO(1);
		if(tftREAD_MISO_GPIO)
			us_read_value++; 		 
	}  	
	
	#elif(tftTOUCH_SPI_SELECT == 2)
	b_touch_write_byte(cmd);	//发送命令字
	
	if(HAL_SPI_Receive(&hspi2, (u8*)&us_read_value, 2, 1) != HAL_OK)
	{
//		sMyPrint("触摸接收失败!/r/n");
	}

	us_read_value = usFunc_SwapU16(us_read_value);
	
	#endif
//	sMyPrint("%d \r\n",us_read_value);
	us_read_value>>=4;   	//只有高12位有效.
	tftSET_TCS_GPIO(1);		//释放片选
	return us_read_value;  
}


/*****************************************************************************
 * @name       :u16 us_touch_read_xoy(u8 xy)  
 * @date       :2018-08-09 
 * @function   :Read the touch screen coordinates (x or y),带滤波的坐标读取(X/Y)
								Read the READ_TIMES secondary data in succession 
								and sort the data in ascending order,
								Then remove the lowest and highest number of LOST_VAL 
								and take the average
 * @parameters :xy:Read command(CMD_RDX/CMD_RDY)
 * @retvalue   :Read data
******************************************************************************/  
static u16 sum=0;
static u16 temp;
#define LOST_VAL 1	  	//丢弃值
static u16 us_touch_read_xoy(u8 xy)
{
	for(int i=0;i<tftTOUCH_AD_FILTER_BUFF_SIZE;i++)
	{
		usa_touch_ad_rec_value[i]=us_touch_read_ad_value(xy);	
	}
	
	for(int i=0;i<tftTOUCH_AD_FILTER_BUFF_SIZE-1; i++)//排序
	{
		for(int j=i+1;j<tftTOUCH_AD_FILTER_BUFF_SIZE;j++)
		{
			if(usa_touch_ad_rec_value[i]>usa_touch_ad_rec_value[j])//升序排列
			{
				temp=usa_touch_ad_rec_value[i];
				usa_touch_ad_rec_value[i]=usa_touch_ad_rec_value[j];
				usa_touch_ad_rec_value[j]=temp;
			}
		}
	}
	
	sum=0;
	for(int i=LOST_VAL;i<tftTOUCH_AD_FILTER_BUFF_SIZE-LOST_VAL;i++)
	{
		sum+=usa_touch_ad_rec_value[i];
	}
	temp=sum/(tftTOUCH_AD_FILTER_BUFF_SIZE-2*LOST_VAL);
	
	return temp;   
} 

/*****************************************************************************
 * @name       :u8 uc_touch_read_xy(u16 *x,u16 *y)
 * @date       :2018-08-09 
 * @function   :Read touch screen x and y coordinates,
								The minimum value can not be less than 100
 * @parameters :x:Read x coordinate of the touch screen
								y:Read y coordinate of the touch screen
 * @retvalue   :0-fail,1-success
******************************************************************************/ 
static u8 uc_touch_read_xy(u16 *x,u16 *y)
{
	u16 xtemp,ytemp;			 	 		  
	xtemp=us_touch_read_xoy(CMD_RDX);
	ytemp=us_touch_read_xoy(CMD_RDY);	  												   
	//if(xtemp<100||ytemp<100)return 0;//读数失败
	*x=xtemp;
	*y=ytemp;
	return 1;//读数成功
}


#define ERR_RANGE 50 //误差范围 
/*****************************************************************************
 * @name       :u8 uc_touch_read_xy2(u16 *x,u16 *y) 
 * @date       :2018-08-09 
 * @function   :Read the touch screen coordinates twice in a row, 带加强滤波的双方向坐标读取
								and the deviation of these two times can not exceed ERR_RANGE, 
								satisfy the condition, then think the reading is correct, 
								otherwise the reading is wrong.
								This function can greatly improve the accuracy.
 * @parameters :x:Read x coordinate of the touch screen
								y:Read y coordinate of the touch screen
 * @retvalue   :0-fail,1-success
******************************************************************************/ 
u8 uc_touch_read_xy2(u16 *x,u16 *y) 
{
	u16 x1,y1;
 	u16 x2,y2;
 	u8 flag;    
    flag=uc_touch_read_xy(&x1,&y1);   
    if(flag==0)return(0);
    flag=uc_touch_read_xy(&x2,&y2);	   
    if(flag==0)return(0);   
    if(((x2<=x1&&x1<x2+ERR_RANGE)||(x1<=x2&&x2<x1+ERR_RANGE))//前后两次采样在+-50内
    &&((y2<=y1&&y1<y2+ERR_RANGE)||(y1<=y2&&y2<y1+ERR_RANGE)))
    {
        *x=(x1+x2)/2;
        *y=(y1+y2)/2;
        return 1;
    }else return 0;	  
} 


/*****************************************************************************
 * @name       :u8 uc_touch_scan(u8 tp)
 * @date       :2018-08-09 
 * @function   :Scanning touch event				
 * @parameters :tp:0-screen coordinate 
									 1-Physical coordinates(For special occasions such as calibration)
 * @retvalue   :Current touch screen status,
								0-no touch
								1-touch
******************************************************************************/  					  
static u8 uc_touch_scan(u8 tp)
{			   
	if(tftREAD_INT_GPIO==0)//有按键按下
	{
		#if(boardUSE_OS)
		taskENTER_CRITICAL();
		#endif
		if(tp)uc_touch_read_xy2(&tTouch.x,&tTouch.y);//读取物理坐标
		else if(uc_touch_read_xy2(&tTouch.x,&tTouch.y))//读取屏幕坐标
		{
	 		tTouch.x=tTouch.xfac*tTouch.x+tTouch.xoff;//将结果转换为屏幕坐标
			tTouch.y=tTouch.yfac*tTouch.y+tTouch.yoff;  
	 	} 
		if((tTouch.ucState&TP_PRES_DOWN)==0)//之前没有被按下
		{		 
			tTouch.ucState=TP_PRES_DOWN|TP_CATH_PRES;//按键按下  
			tTouch.x0=tTouch.x;//记录第一次按下时的坐标
			tTouch.y0=tTouch.y;  	   			 
		}
		sMyPrint("%d  %d  \r\n",tTouch.x,tTouch.y);
		#if(boardUSE_OS)
		taskEXIT_CRITICAL();
		#endif
	}
	else
	{
		if(tTouch.ucState&TP_PRES_DOWN)//之前是被按下的
		{
			tTouch.ucState&=~(1<<7);//标记按键松开	
		}else//之前就没有被按下
		{
			tTouch.x0=0;
			tTouch.y0=0;
			tTouch.x=0xffff;
			tTouch.y=0xffff;
		}
	}
	return tTouch.ucState&TP_PRES_DOWN;//返回当前的触屏状态
}


/***********************************************************************************************************************
-----函数功能    写指令
-----说明(备注)  none
-----传入参数    cmd:指令  len:长度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static u8 uc_touch_is_press(void)
{
	if(tftREAD_INT_GPIO == 0)
		return 1;
	else 
		return 0;
}


/*****************************************************************************
 * @name       :u8 v_touch_adjust(void)
 * @date       :2018-08-09 
 * @function   :Calibration touch screen and Get 4 calibration parameters
 * @parameters :None
 * @retvalue   :None
******************************************************************************/ 		 
static void v_touch_adjust(void)
{								 
	u16 pos_temp[4][2];//坐标缓存值
	u8  cnt=0;	
	u16 d1,d2;
	u32 tem1,tem2;
	float fac; 	
	u16 outtime=0;
 	cnt=0;				
	
	vFTF_ShowString(10,40, 200, 20, 16,(u8*)"Please use the stylus click",1);//显示提示信息
	vFTF_ShowString(10,56, 200, 20, 16,(u8*)"the cross on the screen.",1);//显示提示信息
	vFTF_ShowString(10,72, 200, 20, 16,(u8*)"The cross will always move",1);//显示提示信息
	vFTF_ShowString(10,88, 200, 20, 16,(u8*)"until the screen adjustment",1);//显示提示信息
	vFTF_ShowString(10,104, 200, 20, 16,(u8*)"is completed.",1);//显示提示信息
	 
	vTft_DrowTouchPoint(20,20,tftCOLOR_RED);//画点1 
	tTouch.ucState=0;//消除触发信号 
	tTouch.xfac=0;//xfac用来标记是否校准过,所以校准之前必须清掉!以免错误	 
	while(1)//如果连续10秒钟没有按下,则自动退出
	{
		tTouch.scan(1);//扫描物理坐标
		if((tTouch.ucState&0xc0)==TP_CATH_PRES)//按键按下了一次(此时按键松开了.)
		{	
			outtime=0;		
			tTouch.ucState&=~(1<<6);//标记按键已经被处理过了.
						   			   
			pos_temp[cnt][0]=tTouch.x;
			pos_temp[cnt][1]=tTouch.y;
			cnt++;	  
			switch(cnt)
			{			   
				case 1:						 
					vTft_DrowTouchPoint(20,20,tftCOLOR_WHITE);				//清除点1 
					vTft_DrowTouchPoint(tTFT.width-20,20,tftCOLOR_RED);	//画点2
					break;
				case 2:
 					vTft_DrowTouchPoint(tTFT.width-20,20,tftCOLOR_WHITE);	//清除点2
					vTft_DrowTouchPoint(20,tTFT.height-20,tftCOLOR_RED);	//画点3
					break;
				case 3:
 					vTft_DrowTouchPoint(20,tTFT.height-20,tftCOLOR_WHITE);			//清除点3
 					vTft_DrowTouchPoint(tTFT.width-20,tTFT.height-20,tftCOLOR_RED);	//画点4
					break;
				case 4:	 //全部四个点已经得到
	    		    //对边相等
					tem1=abs(pos_temp[0][0]-pos_temp[1][0]);//x1-x2
					tem2=abs(pos_temp[0][1]-pos_temp[1][1]);//y1-y2
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,2的距离
					
					tem1=abs(pos_temp[2][0]-pos_temp[3][0]);//x3-x4
					tem2=abs(pos_temp[2][1]-pos_temp[3][1]);//y3-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到3,4的距离
					fac=(float)d1/d2;
					if(fac<0.95f||fac>1.05f||d1==0||d2==0)//不合格
					{
						cnt=0;
 				    	vTft_DrowTouchPoint(tTFT.width-20,tTFT.height-20,tftCOLOR_WHITE);	//清除点4
   	 					vTft_DrowTouchPoint(20,20,tftCOLOR_RED);								//画点1
 						vTft_ShowTouchAdjInfo(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
 						continue;
					}
					tem1=abs(pos_temp[0][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[0][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,3的距离
					
					tem1=abs(pos_temp[1][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[1][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,4的距离
					fac=(float)d1/d2;
					if(fac<0.95f||fac>1.05f)//不合格
					{
						cnt=0;
 				    	vTft_DrowTouchPoint(tTFT.width-20,tTFT.height-20,tftCOLOR_WHITE);	//清除点4
   	 					vTft_DrowTouchPoint(20,20,tftCOLOR_RED);								//画点1
 						vTft_ShowTouchAdjInfo(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						continue;
					}//正确了
								   
					//对角线相等
					tem1=abs(pos_temp[1][0]-pos_temp[2][0]);//x1-x3
					tem2=abs(pos_temp[1][1]-pos_temp[2][1]);//y1-y3
					tem1*=tem1;
					tem2*=tem2;
					d1=sqrt(tem1+tem2);//得到1,4的距离
	
					tem1=abs(pos_temp[0][0]-pos_temp[3][0]);//x2-x4
					tem2=abs(pos_temp[0][1]-pos_temp[3][1]);//y2-y4
					tem1*=tem1;
					tem2*=tem2;
					d2=sqrt(tem1+tem2);//得到2,3的距离
					fac=(float)d1/d2;
					if(fac<0.95f||fac>1.05f)//不合格
					{
						cnt=0;
 				    	vTft_DrowTouchPoint(tTFT.width-20,tTFT.height-20,tftCOLOR_WHITE);	//清除点4
   	 					vTft_DrowTouchPoint(20,20,tftCOLOR_RED);								//画点1
 						vTft_ShowTouchAdjInfo(pos_temp[0][0],pos_temp[0][1],pos_temp[1][0],pos_temp[1][1],pos_temp[2][0],pos_temp[2][1],pos_temp[3][0],pos_temp[3][1],fac*100);//显示数据   
						continue;
					}//正确了
					//计算结果
					tTouch.xfac=(float)(tTFT.width-40)/(pos_temp[1][0]-pos_temp[0][0]);//得到xfac		 
					tTouch.xoff=(tTFT.width-tTouch.xfac*(pos_temp[1][0]+pos_temp[0][0]))/2;//得到xoff
						  
					tTouch.yfac=(float)(tTFT.height-40)/(pos_temp[2][1]-pos_temp[0][1]);//得到yfac
					tTouch.yoff=(tTFT.height-tTouch.yfac*(pos_temp[2][1]+pos_temp[0][1]))/2;//得到yoff  
					if(fabs(tTouch.xfac)>2||fabs(tTouch.yfac)>2)//触屏和预设的相反了.
					{
						cnt=0;
 				    	vTft_DrowTouchPoint(tTFT.width-20,tTFT.height-20,tftCOLOR_WHITE);	//清除点4
   	 					vTft_DrowTouchPoint(20,20,tftCOLOR_RED);								//画点1
						vFTF_ShowString(0,0, 200, 20, 16,(u8*)"TP Need readjust!",1);
						tTouch.touchtype=!tTouch.touchtype;//修改触屏类型.
						if(tTouch.touchtype)//X,Y方向与屏幕相反
						{
							CMD_RDX=0X90;
							CMD_RDY=0XD0;	 
						}else				   //X,Y方向与屏幕相同
						{
							CMD_RDX=0XD0;
							CMD_RDY=0X90;	 
						}			    
						continue;
					}		
					us_tft_point_color=tftCOLOR_BLUE;
					vTFT_AllClear(tftCOLOR_WHITE);//清屏
					vFTF_ShowString(35,110, 200, 20, 16,(u8*)"Touch Screen Adjust OK!",1);
					vTFT_DelayMs(1000);
					v_touch_save_adjdata();  
 					vTFT_AllClear(tftCOLOR_WHITE);//清屏   
					return;//校正完成				 
			}
		}
		vTFT_DelayMs(10);
		outtime++;
		if(outtime>1000)
		{
			uc_touch_get_adjdata();
			break;
	 	} 
 	}
}



//////////////////////////////////////////////////////////////////////////	 
//保存在EEPROM里面的地址区间基址,占用13个字节(RANGE:SAVE_ADDR_BASE~SAVE_ADDR_BASE+12)
#define SAVE_ADDR_BASE 40
/*****************************************************************************
 * @name       :void v_touch_save_adjdata(void)
 * @date       :2018-08-09 
 * @function   :Save calibration parameters		
 * @parameters :None
 * @retvalue   :None
******************************************************************************/ 										    
static void v_touch_save_adjdata(void)
{
//	s32 temp;			 
//	//保存校正结果!		   							  
//	temp=tTouch.xfac*100000000;//保存x校正因素      
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE,temp,4);   
//	temp=tTouch.yfac*100000000;//保存y校正因素    
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE+4,temp,4);
//	//保存x偏移量
//    AT24CXX_WriteLenByte(SAVE_ADDR_BASE+8,tTouch.xoff,2);		    
//	//保存y偏移量
//	AT24CXX_WriteLenByte(SAVE_ADDR_BASE+10,tTouch.yoff,2);	
//	//保存触屏类型
//	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+12,tTouch.touchtype);	
//	temp=0X0A;//标记校准过了
//	AT24CXX_WriteOneByte(SAVE_ADDR_BASE+13,temp); 
}


/*****************************************************************************
 * @name       :u8 uc_touch_get_adjdata(void)
 * @date       :2018-08-09 
 * @function   :Gets the calibration values stored in the EEPROM		
 * @parameters :None
 * @retvalue   :1-get the calibration values successfully
								0-get the calibration values unsuccessfully and Need to recalibrate
******************************************************************************/ 	
static u8 uc_touch_get_adjdata(void)
{					  
//	s32 tempfac;
//	tempfac=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+13);//读取标记字,看是否校准过！ 		 
//	if(tempfac==0X0A)//触摸屏已经校准过了			   
//	{    												 
//		tempfac=AT24CXX_ReadLenByte(SAVE_ADDR_BASE,4);		   
//		tTouch.xfac=(float)tempfac/100000000;//得到x校准参数
//		tempfac=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+4,4);			          
//		tTouch.yfac=(float)tempfac/100000000;//得到y校准参数
//	    //得到x偏移量
//		tTouch.xoff=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+8,2);			   	  
// 	    //得到y偏移量
//		tTouch.yoff=AT24CXX_ReadLenByte(SAVE_ADDR_BASE+10,2);				 	  
// 		tTouch.touchtype=AT24CXX_ReadOneByte(SAVE_ADDR_BASE+12);//读取触屏类型标记
//		if(tTouch.touchtype)//X,Y方向与屏幕相反
//		{
//			CMD_RDX=0X90;
//			CMD_RDY=0XD0;	 
//		}else				   //X,Y方向与屏幕相同
//		{
//			CMD_RDX=0XD0;
//			CMD_RDY=0X90;	 
//		}		 
//		return 1;	 
//	}
    
	#if(tftTOUCH_SPI_SELECT == 1)
	tTouch.xfac=-0.06959f;//得到x校准参数			          
	tTouch.yfac=0.08289f;//得到y校准参数
	//得到x偏移量
	tTouch.xoff=255;			   	  
	//得到y偏移量
	tTouch.yoff=-12;				 	  
	tTouch.touchtype=0;//读取触屏类型标记
	#elif(tftTOUCH_SPI_SELECT == 2)
	tTouch.xfac=-0.150037512;//得到x校准参数			          
	tTouch.yfac=0.170212761;//得到y校准参数
	//得到x偏移量
	tTouch.xoff=578;			   	  
	//得到y偏移量
	tTouch.yoff=-362;				 	  
	tTouch.touchtype=0;//读取触屏类型标记
	#endif
	return 0;
}











/************************************************************************************************************************
*************************************************************************************************************************
                                                  全局函数
*************************************************************************************************************************
*************************************************************************************************************************/
/*****************************************************************************
 * @name       :u8 uc_touch_init(void)
 * @date       :2018-08-09 
 * @function   :Initialization touch screen
 * @parameters :None
 * @retvalue   :0-no calibration
								1-Has been calibrated
******************************************************************************/  
s8 cTft_TouchIfaceInit(void)
{			    		   
	vTFT_TouchSpInit();
 	   
//  	uc_touch_read_xy(&tTouch.x,&tTouch.y);//第一次读取初始化	 

//	if(uc_touch_get_adjdata())return 0;//已经校准
//	else			   //未校准?
//	{ 										    
//		vTFT_AllClear(tftCOLOR_WHITE);//清屏
//	    v_touch_adjust();  //屏幕校准 
//		v_touch_save_adjdata();	 
//	}			
	uc_touch_get_adjdata();	
	
	return 1; 									 
}


/*****************************************************************************
 * @name       :void vTft_DrowTouchPoint(u16 x,u16 y,u16 color)
 * @date       :2018-08-09 
 * @function   :Draw a touch point,Used to calibrate 画一个坐标校准点							
 * @parameters :x:Read x coordinate of the touch screen
								y:Read y coordinate of the touch screen
								color:the color value of the touch point
 * @retvalue   :None
******************************************************************************/  
void vTft_DrowTouchPoint(u16 x,u16 y,u16 color)
{
	us_tft_point_color=color;
	vFTF_DrawLine(x-12,y,x+13,y);//横线
	vFTF_DrawLine(x,y-12,x,y+13);//竖线
	vTFT_DrawPoint(x+1,y+1);
	vTFT_DrawPoint(x-1,y+1);
	vTFT_DrawPoint(x+1,y-1);
	vTFT_DrawPoint(x-1,y-1);
	vFTF_DrawCircle(x,y,6);//画中心圈
}	

/*****************************************************************************
 * @name       :void vTft_DrawTouchBigPoint(u16 x,u16 y,u16 color)
 * @date       :2018-08-09 
 * @function   :Draw a big point(2*2)					
 * @parameters :x:Read x coordinate of the point
								y:Read y coordinate of the point
								color:the color value of the point
 * @retvalue   :None
******************************************************************************/   
void vTft_DrawTouchBigPoint(u16 x,u16 y,u16 color)
{	    
	us_tft_point_color=color;
	vTFT_DrawPoint(x,y);//中心点 
	vTFT_DrawPoint(x+1,y);
	vTFT_DrawPoint(x,y+1);
	vTFT_DrawPoint(x+1,y+1);	 	  	
}

/*****************************************************************************
 * @name       :void vTft_ShowTouchAdjInfo(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
 * @date       :2018-08-09 
 * @function   :Display calibration results	
 * @parameters :x0:the x coordinates of first calibration point
								y0:the y coordinates of first calibration point
								x1:the x coordinates of second calibration point
								y1:the y coordinates of second calibration point
								x2:the x coordinates of third calibration point
								y2:the y coordinates of third calibration point
								x3:the x coordinates of fourth calibration point
								y3:the y coordinates of fourth calibration point
								fac:calibration factor 
 * @retvalue   :None
******************************************************************************/ 	 					  
void vTft_ShowTouchAdjInfo(u16 x0,u16 y0,u16 x1,u16 y1,u16 x2,u16 y2,u16 x3,u16 y3,u16 fac)
{	  
	us_tft_point_color=tftCOLOR_RED;
	vFTF_ShowString(40, 140, 200, 20, 16, (u8*)("x1:"), 1);
 	vFTF_ShowString(40+80,140, 200, 20, 16,(u8*)"y1:",1);
 	vFTF_ShowString(40,160, 200, 20, 16,(u8*)"x2:",1);
 	vFTF_ShowString(40+80,160,  200, 20, 16,(u8*)"y2:",1);
	vFTF_ShowString(40,180,  200, 20, 16,(u8*)"x3:",1);
 	vFTF_ShowString(40+80,180,  200, 20, 16,(u8*)"y3:",1);
	vFTF_ShowString(40,200,  200, 20, 16,(u8*)"x4:",1);
 	vFTF_ShowString(40+80,200,  200, 20, 16,(u8*)"y4:",1);  
 	vFTF_ShowString(40,220,  200, 20, 16,(u8*)"fac is:",1);     
	vFTF_ShowNum(40+24,140,x0,4,16);		//显示数值
	vFTF_ShowNum(40+24+80,140,y0,4,16);	//显示数值
	vFTF_ShowNum(40+24,160,x1,4,16);		//显示数值
	vFTF_ShowNum(40+24+80,160,y1,4,16);	//显示数值
	vFTF_ShowNum(40+24,180,x2,4,16);		//显示数值
	vFTF_ShowNum(40+24+80,180,y2,4,16);	//显示数值
	vFTF_ShowNum(40+24,200,x3,4,16);		//显示数值
	vFTF_ShowNum(40+24+80,200,y3,4,16);	//显示数值
 	vFTF_ShowNum(40+56,220,fac,3,16); 	//显示数值,该数值必须在95~105范围之内.
}

/*****************************************************************************
 * @name       :void Touch_Test(void)
 * @date       :2018-08-09 
 * @function   :touch test
 * @parameters :None
 * @retvalue   :None
******************************************************************************/
void Touch_Test(void)
{
	u16 i=0;
	u16 j=0;
	u16 colorTemp=0;
	vFTF_ShowString(0, 0, 200,20,16,(u8*)("开始触摸测试0"), 1);
	 us_tft_point_color=tftCOLOR_RED;
	vFTF_Fill(tTFT.width-52,2,tTFT.width-50+20,18,tftCOLOR_RED); 
	while(1)
	{
		tTouch.scan(0); 		 
		if(tTouch.ucState&TP_PRES_DOWN)			//触摸屏被按下
		{	
		 	if(tTouch.x<tTFT.width&&tTouch.y<tTFT.height)
			{	
				if(tTouch.x>(tTFT.width-24)&&tTouch.y<16)
				{
					vFTF_ShowString(0, 0, 200,20,16,(u8*)("开始触摸测试1"), 1);
					 us_tft_point_color=colorTemp;
					vFTF_Fill(tTFT.width-52,2,tTFT.width-50+20,18, us_tft_point_color); 
				}
				else if((tTouch.x>(tTFT.width-60)&&tTouch.x<(tTFT.width-50+20))&&tTouch.y<20)
				{
					vFTF_Fill(tTFT.width-52,2,tTFT.width-50+20,18,ColorTab[j%5]); 
					 us_tft_point_color=ColorTab[(j++)%5];
					colorTemp= us_tft_point_color;
					vTFT_DelayMs(10);
				}
				else 
				{
					vTft_DrawTouchBigPoint(tTouch.x,tTouch.y, us_tft_point_color);		//画图	
				}  			   
			}
		}
		else 
		{
			vTFT_DelayMs(10);	//没有按键按下的时候 
		}
	    
		if(bKey_0_IsPress()==true)	//KEY_RIGHT按下,则执行校准程序
		{

			vTFT_AllClear(tftCOLOR_WHITE);//清屏
		    v_touch_adjust();  //屏幕校准 
			v_touch_save_adjdata();	 
			vFTF_ShowString(0, 0, 200,20,16,(u8*)("开始触摸测试2"), 1);
			us_tft_point_color=colorTemp;
			vFTF_Fill(tTFT.width-52,2,tTFT.width-50+20,18, us_tft_point_color); 
		}
		
		i++;
		if(i==30)
		{
			i=0;
			HAL_GPIO_TogglePin(GPIOD,GPIO_PIN_9);
			//break;
		}
	}   
}

#endif  //boardDISPLAY
