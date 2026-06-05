#include "SEGGER_RTT.h" 
#include "board_config.h"

#if (boardIC_TYPE == boardIC_GD32F30X)
//标准库需要的支持函数(ARMCC编译器需要,ARMClang已内置定义)
struct __FILE 
{ 
	int handle; 
}; 

void _ttywrch(int ch)
{
    ch = ch;
}

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
    x = x; 
}
#endif

//重定义fputc函数 ////串口1
/* retarget the C library printf function to the USART */
int fputc(int ch, FILE *f)
{
	SEGGER_RTT_printf(0,"%d",ch); 
    return ch;
}

/*-------------------------------------------------------------------------------------*/

/*-------------------------------------------------------------------------------------*/


