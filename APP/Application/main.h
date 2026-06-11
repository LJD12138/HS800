/*!
    \file    main.h
    \brief   the header file of main

    \version 2016-08-15, V1.0.0, firmware for GD32F4xx
    \version 2018-12-12, V2.0.0, firmware for GD32F4xx
    \version 2020-09-30, V2.1.0, firmware for GD32F4xx
    \version 2022-03-09, V3.0.0, firmware for GD32F4xx
*/

/*
    Copyright (c) 2022, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

#ifndef __MAIN_H
#define __MAIN_H

#include "gd32f50x.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//#define		//4Tab									//10Tab
#define     	mainINIT_BOOT_PARAM_FLAG     			0x0000000	//初始化BOOT参数
#define     	mainINIT_FINISH_FLAG        			0x88888888	//系统初始化完成
#define     	mainUPDATE_FLAG             			0xAAAAAAAA	//升级
#define     	mainDISPLAY_FLAG            			0xAAAABBBB	//显示
#define     	mainLOW_POWER_FLAG          			0xBBBBCCCC	//进入低功耗
#define     	mainINIT_APP_PARAM_FLAG     			0xEFEFFEFE	//初始化APP参数

#define     	RANGE(val, min, max)     				((val < min) ? false : ((val > max) ? false : true))  //在范围内为true,包含min和max   
#define     	RANGE_NO(val, min, max)     			((val < min) ? true  : ((val > max) ? true  : false)) //在范围外为true
#define     	RANGE_M(val, max)     					((val > max) ? false : true)  							//在范围内为true,包含max  
#define     	LIMIT(X, min, MAX)          			((X) <= (min) ? (min) : ((X) >= (MAX) ? (MAX) : (X)))
#define     	LIMIT_MAX(X, MAX)           			((X) < (MAX) ? (X) : (MAX))
#define     	LIMIT_MIN(X, MIN)           			((X) > (MIN) ? (X) : (MIN))

#define     	MSEC(TIME)                  			((TIME) / tickTime)                         //ms转成实际的 tick时间
#define     	MAX2(a, b)                  			((a > b) ? a : b)
#define     	MAX3(a, b, c)               			(a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c))
#define     	MIN2(a, b)                  			((a < b) ? a : b)
#define    		MIN3(a, b, c)               			(a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c))
#define     	DIFFER(val1, val2)          			((val1 <= val2) ? (val2 - val1) : (val1 - val2))

#define     	wb(addr, value)             			(*((u8  volatile *) (addr)) = value)
#define     	rb(addr)                    			(*((u8  volatile *) (addr)))
#define     	whw(addr, value)            			(*((u16 volatile *) (addr)) = value)
#define     	rhw(addr)                   			(*((u16 volatile *) (addr)))
#define     	ww(addr, value)             			(*((u32 volatile *) (addr)) = value)
#define     	ARRAYNUM(arr_name)          			(uint32_t)(sizeof(arr_name)/sizeof(*(arr_name)))
	
#define 		uchar 									unsigned char
#define 		uint  									unsigned int

#ifndef   		__ALIGNED
#define 		__ALIGNED(x)                           	__attribute__((aligned(x)))
#endif

#define     	BIT_SET(obj, bitMask)         			(obj |= bitMask)
#define     	BIT_CLR(obj, bitMask)         			(obj &= (~bitMask))

#define     	BIT_GET(obj, XX_CODE)        			((obj >> XX_CODE) & (0x01))

#define     	ERR_SET(obj, ERR_CODE)         			BIT_SET(obj, ((u16)0x1 << ERR_CODE))
#define     	ERR_CLR(obj, ERR_CODE)         			BIT_CLR(obj, ((u16)0x1 << ERR_CODE))

#define     	STAT_SET(obj, STAT_CODE)        		BIT_SET(obj, ((u16)0x1 << STAT_CODE))
#define     	STAT_CLR(obj, STAT_CODE)        		BIT_CLR(obj, ((u16)0x1 << STAT_CODE))



#define 		U16_MAX                       			65530
#define 		S16_MAX                       			32760
#define 		U8_MAX                        			250
#define 		S8_MAX                        			120



//**********************************有符号*********************************************************************
typedef       	int64_t  								s64;
typedef       	int32_t  								s32;
typedef       	int16_t  								s16;
typedef       	int8_t   								s8;

typedef const  int64_t  								sc64;           /*!< Read Only	*/
typedef const  int32_t  								sc32;           /*!< Read Only	*/
typedef const  int16_t  								sc16;           /*!< Read Only	*/
typedef const  int8_t   								sc8;            /*!< Read Only	*/

typedef __IO  	int64_t  								vs64;
typedef __IO  	int32_t  								vs32;
typedef __IO  	int16_t  								vs16;
typedef __IO  	int8_t   								vs8;

typedef __I   	int64_t  								vsc64;           /*!< Read Only	*/
typedef __I   	int32_t  								vsc32;           /*!< Read Only	*/
typedef __I   	int16_t  								vsc16;           /*!< Read Only	*/
typedef __I   	int8_t   								vsc8;            /*!< Read Only	*/

//**********************************无符号*********************************************************************
typedef       	uint64_t 								u64;
typedef       	uint32_t 								u32;
typedef       	uint16_t 								u16;
typedef       	uint8_t  								u8;

typedef const  uint64_t 								uc64;    		/*!< Read Only	*/
typedef const  uint32_t 								uc32;    		/*!< Read Only	*/
typedef const  uint16_t 								uc16;    		/*!< Read Only	*/
typedef const  uint8_t  								uc8;     		/*!< Read Only	*/

typedef __IO  	uint64_t 								vu64;
typedef __IO  	uint32_t 								vu32;
typedef __IO  	uint16_t 								vu16;
typedef __IO  	uint8_t  								vu8;

typedef __I   	uint64_t 								vuc64;           /*!< Read Only  */
typedef __I   	uint32_t 								vuc32;           /*!< Read Only  */
typedef __I   	uint16_t 								vuc16;           /*!< Read Only  */
typedef __I   	uint8_t  								vuc8;            /*!< Read Only  */

//系统运行状态
typedef enum
{
	DS_LOST = 0,		// 丢失
	DS_INIT,	       	// 初始化
	DS_CLOSING ,       	// 关闭中
	DS_SHUT_DOWN,      	// 关机状态 3
	DS_ERR,            	// 错误状态
    DS_BOOTING,        	// 装载中  5
    DS_WORK,           	// 工作状态
	DS_UPDATE_MODE,		// 升级模式
	DS_ENG_MODE,       	// 工程模式 engineering mode
}DevState_E;


//开关类型
typedef enum
{
	ST_OFF=0,
	ST_ON,
	ST_NULL,//进行取反
}SwitchType_E;

//开关的对象
typedef enum
{
	SO_KEY=0,  		//按键
	SO_CONSOLE,    	//面板
	SO_PARA,     	//并机
	SO_DCAC,    	//逆变充电激活
	SO_MPPT,    	//MPPT充电激活
}SwitchObject_E;

//操作的对象
typedef enum
{
	OO_CHG=0,  		//充电
	OO_DISCHG,    	//放电
	OO_ALL,    		//充放电
	OO_PARA_IN,    	//并网
}OperaObject_E;

//输入输出状态
typedef enum
{
	IOS_CLOSING=0,  	//关闭中
	IOS_SHUT_DOWN,  	//关闭
	IOS_PROTE,      	//保护  需要关机清除
	IOS_ERR,        	//错误  可以移除输入清除
	IOS_STARTING,   	//启动中
	IOS_WORK,       	//工作
}InOutState_E;

//步骤
typedef enum
{
	STEP_FORWARD = (u8)0xfd,//上一步
	STEP_NEXT = (u8)0xfe,//下一步
	STEP_END = (u8)0xff,//结束
}Step_E;

//模块对象
typedef enum
{
	MO_CONSOLE=0,
	MO_BMS,
	MO_MPPT,
	MO_DCAC,
}ModuleObject_E;

typedef union
{
	struct
	{
		u8 ucObj;
		u8 ucParam;
	}tTaskParam;
	u16 usTaskInParam;
}TaskInParam_U;

#pragma pack(1)
typedef struct 
{
	u8 	obj;
	u32	cmd;
}tSysSetParam;
#pragma pack()

#endif /* __MAIN_H */


