#ifndef KEY_TASK_H_
#define KEY_TASK_H_

#include "board_config.h"

#if(boardKEY_EN)
#include "Adc/adc_task.h"
//#define		//4Tab									//10Tab
#define     	keyTASK_CYCLE_TIME                		10  //按键任务更新时间
#define     	keyNUM                            		5   //按键的数量
#define     	keyGROUP_NUM                      		10  //组合按键种类   
#define     	keySHORT_PRESS_TIME               		2   //短按按键的最小时间*10ms
#define     	keyLONG_PRESS_TIME                		90 //长按按键的最小时间*10ms
#define     	keySUPER_LONG_PRESS_TIME           		250	//超长按按键的最小时间*10ms
#define     	keyNUPRESS_MAX_TIME               		35  //组合按键最大的等待时间*10ms
#define     	keyADD_SPACE_TIME                 		20  //长按累加间隔


//#define     	keyGPIO_POWER_RCU       				RCU_GPIOC
//#define     	keyGPIO_POWER_PORT      				GPIOC
//#define     	keyGPIO_POWER_PIN       				GPIO_PIN_4

#define     	keyGPIO_AC_RCU          				RCU_GPIOB
#define     	keyGPIO_AC_PORT         				GPIOB
#define     	keyGPIO_AC_PIN          				GPIO_PIN_13

#define     	keyGPIO_LIGHT_RCU       				RCU_GPIOB 
#define     	keyGPIO_LIGHT_PORT      				GPIOB
#define     	keyGPIO_LIGHT_PIN       				GPIO_PIN_8

#define     	keyGPIO_USB_RCU        					RCU_GPIOB
#define     	keyGPIO_USB_PORT        				GPIOB
#define     	keyGPIO_USB_PIN        					GPIO_PIN_14

#define     	keyGPIO_DC_RCU          				RCU_GPIOC   
#define     	keyGPIO_DC_PORT         				GPIOC
#define     	keyGPIO_DC_PIN          				GPIO_PIN_12

//#define     	keyGPIO_WP_RCU          				RCU_GPIOA
//#define    	keyGPIO_WP_GPIO         				GPIOA
//#define     	keyGPIO_WP_PIN          				GPIO_PIN_0


//__STATIC_INLINE bool bKey_PowerIsPress(void)          
//{    
//    if((GPIO_ISTAT(keyGPIO_POWER_PORT)&(keyGPIO_POWER_PIN)) == 0)//读取按键
//        return false;
//    else
//        return true;
//}
__STATIC_INLINE bool bKey_PowerIsPress(void)          
{    
    if(usAdc_GetChannelValue(adcKEY_POWER) > 200)//读取按键
        return true;
    else
        return false;
}

__STATIC_INLINE bool bKey_AcIsPress(void)
{
    if((GPIO_ISTAT(keyGPIO_AC_PORT)&(keyGPIO_AC_PIN)) == 0)//读取按键
        return true;
    else
        return false;
}

__STATIC_INLINE bool bKey_LightIsPress(void)
{
    if((GPIO_ISTAT(keyGPIO_LIGHT_PORT)&(keyGPIO_LIGHT_PIN)) == 0)//读取按键
        return true;
    else
        return false;
}

__STATIC_INLINE bool bKey_UsbIsPress(void)
{
    if((GPIO_ISTAT(keyGPIO_USB_PORT)&(keyGPIO_USB_PIN)) == 0)//读取按键
        return true;
    else
        return false;
}

__STATIC_INLINE bool bKey_DcIsPress(void)
{
    if((GPIO_ISTAT(keyGPIO_DC_PORT)&(keyGPIO_DC_PIN)) == 0)//读取按键
        return true;
    else
        return false;
}

typedef enum
{
	KTE_FUN_NULL = 0,
	KTE_POWER_LONG,
	KTE_POWER_SHORT,
	KTE_POWER_SUPER_LONG,
	KTE_AC_LONG,
	KTE_AC_SHORT,
	KTE_AC_SUPER_LONG,
	KTE_LIGHT_LONG,
	KTE_LIGHT_SHORT,
	KTE_LIGHT_SUPER_LONG,
	KTE_USB_LONG,
	KTE_USB_SHORT,
	KTE_USB_SUPER_LONG,
	KTE_DC_LONG,
	KTE_DC_SHORT,
	KTE_DC_SUPER_LONG,
}KeyTriEvent_e;  //触发事件 



typedef struct
{
    bool    			(*IsPress)(void);        
    vs16    			sOnPressCnt;	  	//按键按下计时  0:表示按键没触发   -1:表示按键功能已经被记录
	bool        		bEnMulitFunKey;     //使能多功能按键
	bool        		bEnLongPressAdd;    //使能长按累加      
}KeyHandler_t;


void vKey_TaskInit(void);
void vKey_PowerIsTri(void);
void vKey_ParamInit(void);

#if(!boardUSE_OS)
void vKey_Task(void *pvParameters);
#endif

#if(boardLOW_POWER)
void vKey_EnterLowPower(void);
void vKey_ExitLowPower(void);
#endif

#endif  //boardKEY_EN

#endif  //KEY_TASK_H_


