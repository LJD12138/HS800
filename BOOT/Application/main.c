/*!
#define flashAPP_START			0x5000		//APP加载地址

1.宏 flashAPP_START 是APP的起始地址，而APP程序的起始是栈顶指针。

2.为什么加载地址不是0x8005000?
	因为选择了从Flash启动，所以Flash被映射到了0x00000000。所以访问0x080050000和0x5000效果是一样的。

3.所以如果正确写入了APP，则 地址 (__IO uint32_t *)flashAPP_START 处的栈顶指针必然在该型号单片机的SRAM大小范围内。
	96K Sram地址范围为0x2000 0000 - 0x2001 7FFF，64K Sram地址范围为0x2000 0000 - 0x2000 FFFF。

4.(0x20000000 == (((__IO uint32_t)flashAPP_START) & 0x2FFE0000)) 表示的是从地址0x5000取出栈顶指针，通过运算检查栈顶地址是否落在0x2000 0000 - 0x2001 7FFF。
	64K的检查为(0x20000000 == (((__IO uint32_t)flashAPP_START) & 0x2FFF0000))。相当于读出的值直接与Sram范围值比较。

5.跳转到APP不是跳转栈顶指针，而是跳转Reset_Handler，而Reset_Handler在栈顶指针后，所以需要偏移32位。

6.application为一个void类型的函数指针，并将其指向Reset_Handler的地址。application()表示执行指针指向的函数，即Reset_Handler函数。

7.跳转前关闭中断，防止 
	1）已经跳转到APP了但来了一个中断并落在了Bootloader， 
	2）中断向量表已偏移但来了中断，就会落在错误的地方
	
	
需要特别注意的是，如果使能了Systick等内核级别的中断，用__disable_irq(); 之类的屏蔽中断的语句是无法关闭的，需要去停止Systick的运行。
最优的是在bootloader跳转前关闭所有用到的资源以及中断，在APP中重新配置所需要的，以免在Bootloader中用到的中断在APP中没有用到，
而缺少对应的中断处理导致中断死循环或HardFault。APP跳转Bootloader亦应如此。
*/

#include "main.h"
#include "board_config.h"
#include "systick.h"
#include "gpio_init.h"
#include "timer_task.h"

#include "Sys/sys_task.h"
#include "Led/led_task.h"

#if(boardKEY_EN)
#include "Key/key_task.h"
#endif  //boardKEY_EN

#if(boardPRINT_IFACE)
#include "Print/print_task.h"
#endif  //boardPRINT_IFACE

#if(boardDISPLAY_EN)
#include "MD_Display/md_display_task.h"
#endif  //boardDISPLAY_EN

#if(boardUPDATE)
#include "Update/update_main.h"
#endif  //boardUPDATE

#include "MD_Bms/md_bms_prot_frame.h"

//****************************************************参数初始化**************************************************//


//****************************************************函数声明****************************************************//


/*****************************************************************************************************************
-----函数功能    Boot主程序
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
*****************************************************************************************************************/
int main(void)
{
	
    vGPIO_Init();       //IO口初始化
	
    vSys_TickConfig();   //系统tick
	
	vBoard_SysInit();
	
	vBoard_StartTask(NULL);

    while(1) 
	{
		if(bSystick_10MsFlag == true
			#if(boardUPDATE)
			|| tpSysTask->ucID == STI_UPDATE
			#endif  //boardUPDATE
		)
		{
			vSys_Task(NULL);
		}
		
		if(bSystick_10MsFlag == true)
		{
			bSystick_10MsFlag = false;
			
			#if(boardKEY_EN)
			vKey_Task(NULL);
			#endif  //boardKEY_EN
			
			#if(boardLED_EN)
			vLed_Task(NULL);
			#endif  //boardLED_EN
		}

		if(bSystick_100MsFlag)
		{
			bSystick_100MsFlag = false;
			
			vTimer_Task();
			
			vAdc_Task(NULL);//测试
			
			#if(boardDISPLAY_EN)
			vDisp_Task(NULL);
			#endif
		}
		
		#if(boardPRINT_IFACE)
		vPrint_Task(NULL);
		#endif
    }
}

