#ifndef BUZ_TASK_H
#define BUZ_TASK_H

#include "board_config.h"

#if(boardBUZ_EN)

typedef enum
{
	Null = 0,
	SHORT_1, 			//操作提示音(长按触发)
	SHORT_2, 			//操作失败   
	SHORT_3, 			//准备打开时的错误
	LONG_1,  			//重要操作提示:开关提示音
	LONG_2, 			//设备丢失:逆变器丢失
	LONG_3,  			//运行中的错误:运行导致的高低温报警,过压过流
	LONG_5,
}Buzz_E;

void bBuz_TaskInit(void);
bool bBuz_Tweet(Buzz_E type);

#if(boardLOW_POWER)
void vBuz_EnterLowPower(void);
void vBuz_ExitLowPower(void);
#endif  //boardLOW_POWER

#endif  //boardBUZ_EN

#endif  //BUZ_TASK_H



