/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Application
 * File    : main.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 系统全局类型、通用宏、运行状态及跨模块公用接口头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus

//****************************************************Includes******************************************************************//
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include "board_config.h"
#include "gd32f50x.h"

//****************************************************Macros********************************************************************//
#define			mainINIT_BOOT_PARAM_FLAG				0x00000000U	//初始化BOOT参数
#define			mainINIT_FINISH_FLAG					0x88888888U	//系统初始化完成
#define			mainUPDATE_FLAG							0xAAAAAAAAU	//升级
#define			mainDISPLAY_FLAG						0xAAAABBBBU	//显示
#define			mainLOW_POWER_FLAG						0xBBBBCCCCU	//进入低功耗
#define			mainINIT_APP_PARAM_FLAG					0xEFEFFEFEU	//初始化APP参数

#define			RANGE(val, min, max)					(((val) < (min)) ? false : (((val) > (max)) ? false : true))	//在范围内为true,包含min和max
#define			RANGE_NO(val, min, max)					(((val) < (min)) ? true  : (((val) > (max)) ? true  : false))	//在范围外为true
#define			RANGE_M(val, max)						(((val) > (max)) ? false : true)	//在范围内为true,包含max
#define			LIMIT(X, min, MAX)						(((X) <= (min)) ? (min) : (((X) >= (MAX)) ? (MAX) : (X)))
#define			LIMIT_MAX(X, MAX)						(((X) < (MAX)) ? (X) : (MAX))
#define			LIMIT_MIN(X, MIN)						(((X) > (MIN)) ? (X) : (MIN))

#define			MSEC(TIME)								((TIME) / tickTime)	//ms转成实际的tick时间
#define			MAX2(a, b)								(((a) > (b)) ? (a) : (b))
#define			MAX3(a, b, c)							(((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c)))
#define			MIN2(a, b)								(((a) < (b)) ? (a) : (b))
#define			MIN3(a, b, c)							(((a) < (b)) ? (((a) < (c)) ? (a) : (c)) : (((b) < (c)) ? (b) : (c)))
#define			DIFFER(val1, val2)						(((val1) <= (val2)) ? ((val2) - (val1)) : ((val1) - (val2)))

#define			wb(addr, value)							(*((u8  volatile *) (addr)) = (value))
#define			rb(addr)								(*((u8  volatile *) (addr)))
#define			whw(addr, value)						(*((u16 volatile *) (addr)) = (value))
#define			rhw(addr)								(*((u16 volatile *) (addr)))
#define			ww(addr, value)							(*((u32 volatile *) (addr)) = (value))
#define			ARRAYNUM(arr_name)						((uint32_t)(sizeof(arr_name) / sizeof(*(arr_name))))

#define			uchar									unsigned char
#define			uint									unsigned int

#ifndef __ALIGNED
#define			__ALIGNED(x)							__attribute__((aligned(x)))
#endif  /* __ALIGNED */

#define			BIT_SET(obj, bitMask)					((obj) |= (bitMask))
#define			BIT_CLR(obj, bitMask)					((obj) &= (~(bitMask)))
#define			BIT_GET(obj, XX_CODE)					(((obj) >> (XX_CODE)) & 0x01)

#define			ERR_SET(obj, ERR_CODE)					BIT_SET(obj, ((u64)0x1 << (ERR_CODE)))
#define			ERR_CLR(obj, ERR_CODE)					BIT_CLR(obj, ((u64)0x1 << (ERR_CODE)))

#define			STAT_SET(obj, STAT_CODE)				BIT_SET(obj, ((u64)0x1 << (STAT_CODE)))
#define			STAT_CLR(obj, STAT_CODE)				BIT_CLR(obj, ((u64)0x1 << (STAT_CODE)))

#define			mainARRAY_SIZE(a)						(sizeof(a) / sizeof((a)[0]))

#define			U16_MAX									65530
#define			S16_MAX									32760
#define			U8_MAX									250
#define			S8_MAX									120

#if (boardUSE_OS)
#define			mainENTER_CRITICAL()					taskENTER_CRITICAL()
#define			mainEXIT_CRITICAL()						taskEXIT_CRITICAL()
#else
#define     	mainENTER_CRITICAL()
#define     	mainEXIT_CRITICAL()
#endif  //boardUSE_OS

//****************************************************Types*********************************************************************//
typedef int64_t                         		s64;
typedef int32_t                         		s32;
typedef int16_t                         		s16;
typedef int8_t                          		s8;

typedef const int64_t                   		sc64;
typedef const int32_t                   		sc32;
typedef const int16_t                   		sc16;
typedef const int8_t                    		sc8;

typedef __IO int64_t                    		vs64;
typedef __IO int32_t                    		vs32;
typedef __IO int16_t                    		vs16;
typedef __IO int8_t                     		vs8;

typedef __I int64_t                     		vsc64;
typedef __I int32_t                     		vsc32;
typedef __I int16_t                     		vsc16;
typedef __I int8_t                      		vsc8;

typedef uint64_t                        		u64;
typedef uint32_t                        		u32;
typedef uint16_t                        		u16;
typedef uint8_t                         		u8;

typedef const uint64_t                  		uc64;
typedef const uint32_t                  		uc32;
typedef const uint16_t                  		uc16;
typedef const uint8_t                   		uc8;

typedef __IO uint64_t                   		vu64;
typedef __IO uint32_t                   		vu32;
typedef __IO uint16_t                   		vu16;
typedef __IO uint8_t                    		vu8;

typedef __I uint64_t                    		vuc64;
typedef __I uint32_t                    		vuc32;
typedef __I uint16_t                    		vuc16;
typedef __I uint8_t                     		vuc8;

//系统运行状态
typedef enum
{
	DS_LOST = 0,		//丢失
	DS_INIT,			//初始化
	DS_CLOSING,			//关闭中
	DS_SHUT_DOWN,		//关机状态 3
	DS_ERR,				//错误状态
	DS_BOOTING,			//装载中 5
	DS_WORK,			//工作状态
	DS_UPDATE_MODE,		//升级模式
	DS_ENG_MODE,		//工程模式 engineering mode
}DevState_E;

//开关类型
typedef enum
{
	ST_OFF = 0,			//关
	ST_ON,				//开
	ST_NULL,			//进行取反
}SwitchType_E;

//开关的对象
typedef enum
{
	SO_KEY = 0,			//按键
	SO_CONSOLE,			//面板
	SO_PARA,			//并机
	SO_DCAC,			//逆变充电激活
	SO_MPPT,			//MPPT充电激活
}SwitchObject_E;

//操作的对象
typedef enum
{
	OO_CHG = 0,			//充电
	OO_DISCHG,			//放电
	OO_ALL,				//充放电
	OO_PARA_IN,			//并网
}OperaObject_E;

//输入输出状态
typedef enum
{
	IOS_CLOSING = 0,	//关闭中
	IOS_SHUT_DOWN,		//关闭
	IOS_PROTE,			//保护 需要关机清除
	IOS_ERR,			//错误 可以移除输入清除
	IOS_STARTING,		//启动中
	IOS_WORK,			//工作
}InOutState_E;

//步骤
typedef enum
{
	STEP_FORWARD = (u8)0xfd,//上一步
	STEP_NEXT = (u8)0xfe,	//下一步
	STEP_END = (u8)0xff,	//结束
}Step_E;

//模块对象
typedef enum
{
	MO_DEFAULT = 0,		//当前连接设备
	MO_CONSOLE,			//主控
	MO_BMS,				//电池
	MO_MPPT,			//光伏
	MO_DCAC,			//逆变
	MO_MGMT_AC,			//MEGMEET_IC_TYPE_AC
	MO_MGMT_DC,			//MEGMEET_IC_TYPE_DC
	MO_INVAILD,			//超范围
}ModuleObject_E;

typedef union
{
	struct
	{
		u8				ucObj;
		u8				ucParam;
	}tTaskParam;
	u16					usTaskInParam;
}TaskInParam_U;

#pragma pack(1)
typedef struct
{
	u8					obj;				//对象
	u32					cmd;				//命令
}SysSetParam_T;
typedef SysSetParam_T tSysSetParam;
#pragma pack()

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  /* __MAIN_H */
