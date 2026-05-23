/*****************************************************************************************************************
*                                                                                                                *
 *                                         自定义输出函数                                                        *
*                                                                                                                *
******************************************************************************************************************/
#include "Print/print_api.h"
#include "Print/print_iface.h"   
#include "Print/print_task.h"
#include "Sys/sys_task.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"
#endif  //boardUSE_OS

//****************************************************局部宏定义**************************************************//
//开启实时Print输出,则有部分打印会丢失,反之会因为等待输出而影响输出任务的实时性.建议开启此定义
#define          printREAL_TIME_OUT
#define          printLOG_BUFF_SIZE					256

//****************************************************参数初始化**************************************************//
static char log_buf[printLOG_BUFF_SIZE];

#if(boardUSE_OS)
/*创建信号量句柄 */
SemaphoreHandle_t MyPrintSemaphoreMutex = NULL;

#ifdef printREAL_TIME_OUT
const int delay_value = 0;
#else
const int delay_value = 100;
#endif 
#endif  //boardUSE_OS

#if(boardEASY_LOGGER == 0)
int (*log_e)(const char *fmt, ...) = sMyPrintErr;
int (*log_w)(const char *fmt, ...) = sMyPrintWarn;
int (*log_i)(const char *fmt, ...) = sMyPrintTips;
#endif

/***********************************************************************************************************************
-----函数功能    MyPrint函数参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
s8 c_print_start_check(const char* str)
{
	// 如果输入为空,返回错误
    if (str == NULL || tSysInfo.uInit.tFinish.bIF_Print == 0) 
        return -1;
    
    // 如果没有开启输出,返回错误
    if((boardPRINT_IFACE == 0) && (boardSEGGER == 0))
        return -2;
    
    #if( boardPRINT_IFACE )
    if(tPrintTxBuff.buff == NULL)
        return -3;
    #endif  //boardPRINT_IFACE
	
	#if(boardUSE_OS)
	if(MyPrintSemaphoreMutex == NULL)
		return -4;
	#endif  //boardUSE_OS
	
	return 1;
}

/***********************************************************************************************************************
-----函数功能    MyPrint函数参数初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vPrint_MyPrintParamInit(void)
{
	#if(boardUSE_OS)
    /* 创建互斥信号量 */
    MyPrintSemaphoreMutex = xSemaphoreCreateMutex();
	#endif  //boardUSE_OS
}

/***********************************************************************************************************************
-----函数功能    封装的输出函数
-----说明(备注)  支持%d，%o, %x，%s，%c，%f（只打印6位数字）
                 该函数用到互斥量,所以不能在中断中使用
-----传入参数    需要输出的数据
-----输出参数    none
-----返回值      打印字符的个数,负数为错误
************************************************************************************************************************/
int sMyPrint(const char* str, ...)
{
	// 初始化变量
    int len = 0;
	
	s8 c_ret = c_print_start_check(str);
	if(c_ret <= 0)
		return c_ret;
	
	#if(boardUSE_OS)
	// 获取互斥量
    if(xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
	#endif  //boardUSE_OS
    {
		va_list args;            // 定义va_list类型指针，用于存储参数的地址
		va_start(args, str);     // 初始化pArgs
		
		/* ① 先算到底需要多少字节 */
		len = vsnprintf(NULL, 0, str, args);   /* C99 支持 */
		if(len < 0) /* 格式串本身非法 */          			
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -4; 
		}   
		if(len > printLOG_BUFF_SIZE) /* ② 自定义上限 512 KB */	
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -5; 
		}  
		
		//转换
		va_start(args, str);            /* 必须重新初始化 */
		len = vsprintf(log_buf, str, args);
		va_end(args);  // 结束取参数
		
		//存储到缓存区
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, log_buf, len);
		#endif
        
		#if(boardUSE_OS)
        // 释放互斥量
        xSemaphoreGive(MyPrintSemaphoreMutex); 
		#endif  //boardUSE_OS
        
        // 开始发送数据
        #if(boardPRINT_IFACE)
        bPrint_SendDataToUsart();
        #endif
	}

    return len;
}

/***********************************************************************************************************************
-----函数功能    封装的输出错误函数
-----说明(备注)  支持%d，%o, %x，%s，%c，%f
-----传入参数    需要输出的数据
-----输出参数    none
-----返回值      打印字符的个数,负数为错误
************************************************************************************************************************/
int sMyPrintErr(const char* str, ...)
{
	// 初始化变量
    int len = 0;
	int len_str = 0;
	
	s8 c_ret = c_print_start_check(str);
	if(c_ret <= 0)
		return c_ret;
	
	#if(boardUSE_OS)
	// 获取互斥量
    if(xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
	#endif  //boardUSE_OS
	{
		va_list args;            // 定义va_list类型指针，用于存储参数的地址
		va_start(args, str);     // 初始化pArgs
		
		/* ① 先算到底需要多少字节 */
		len = vsnprintf(NULL, 0, str, args);   /* C99 支持 */
		if(len < 0) /* 格式串本身非法 */          			
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -4; 
		}   
		if(len > printLOG_BUFF_SIZE) /* ② 自定义上限 512 KB */	
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -5; 
		}  
		
		char err1[] = "\033[31;";
		len_str = strlen(err1);
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, err1, len_str);
		#endif
		
		//转换
		va_start(args, str);            /* 必须重新初始化 */
		len = vsprintf(log_buf, str, args);
		va_end(args);  // 结束取参数
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, log_buf, len);
		#endif
		len += len_str;
		
		char err2[] = "\033[0m \r\n";
		len_str = strlen(err2);
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, err2, len_str);
		#endif
		len += len_str;
		
		#if(boardUSE_OS)
        // 释放互斥量
        xSemaphoreGive(MyPrintSemaphoreMutex); 
		#endif  //boardUSE_OS
		
		//开始发送数据
		#if(boardPRINT_IFACE)
		bPrint_SendDataToUsart();
		#endif
    }
	//等待信号量释放超时
    return len;
}


/***********************************************************************************************************************
-----函数功能    封装的输出警告函数
-----说明(备注)  支持%d，%o, %x，%s，%c，%f
-----传入参数    需要输出的数据
-----输出参数    none
-----返回值      打印字符的个数,负数为错误
************************************************************************************************************************/
int sMyPrintWarn(const char* str, ...)
{
	// 初始化变量
    int len = 0;
	int len_str = 0;
	
    s8 c_ret = c_print_start_check(str);
	if(c_ret <= 0)
		return c_ret;
	
	#if(boardUSE_OS)
	// 获取互斥量
    if(xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
	#endif  //boardUSE_OS
	{
		va_list args;            // 定义va_list类型指针，用于存储参数的地址
		va_start(args, str);     // 初始化pArgs
		
		/* ① 先算到底需要多少字节 */
		len = vsnprintf(NULL, 0, str, args);   /* C99 支持 */
		if(len < 0) /* 格式串本身非法 */          			
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -4; 
		}   
		if(len > printLOG_BUFF_SIZE) /* ② 自定义上限 512 KB */	
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -5; 
		}  
		
		char err1[] = "\033[33;";
		len_str = strlen(err1);
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, err1, len_str);
		#endif
		
		//转换
		va_start(args, str);            /* 必须重新初始化 */
		len = vsprintf(log_buf, str, args);
		va_end(args);  // 结束取参数
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, log_buf, len);
		#endif
		len += len_str;
		
		char err2[] = "\033[0m \r\n";
		len_str = strlen(err2);
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, err2, len_str);
		#endif
		len += len_str;
		
		#if(boardUSE_OS)
        // 释放互斥量
        xSemaphoreGive(MyPrintSemaphoreMutex); 
		#endif  //boardUSE_OS
		
		//开始发送数据
		#if(boardPRINT_IFACE)
		bPrint_SendDataToUsart();
		#endif
    }
	//等待信号量释放超时
    return len;
}


/***********************************************************************************************************************
-----函数功能    封装的输出提示函数
-----说明(备注)  支持%d，%o, %x，%s，%c，%f
-----传入参数    需要输出的数据
-----输出参数    none
-----返回值      打印字符的个数,负数为错误
************************************************************************************************************************/
int sMyPrintTips(const char* str, ...)
{
	// 初始化变量
    int len = 0;
	int len_str = 0;
	
    s8 c_ret = c_print_start_check(str);
	if(c_ret <= 0)
		return c_ret;
	
	#if(boardUSE_OS)
	// 获取互斥量
    if(xSemaphoreTake(MyPrintSemaphoreMutex, pdMS_TO_TICKS(delay_value)) == pdPASS)
	#endif  //boardUSE_OS
	{
		va_list args;            // 定义va_list类型指针，用于存储参数的地址
		va_start(args, str);     // 初始化pArgs
		
		/* ① 先算到底需要多少字节 */
		len = vsnprintf(NULL, 0, str, args);   /* C99 支持 */
		if(len < 0) /* 格式串本身非法 */          			
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -4; 
		}   
		if(len > printLOG_BUFF_SIZE) /* ② 自定义上限 512 KB */	
		{ 
			va_end(args);
			
			#if(boardUSE_OS)
			xSemaphoreGive(MyPrintSemaphoreMutex);
			#endif  //boardUSE_OS
			
			return -5; 
		}  
		
		char err1[] = "\033[32;";
		len_str = strlen(err1);
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, err1, len_str);
		#endif
		
		//转换
		va_start(args, str);            /* 必须重新初始化 */
		len = vsprintf(log_buf, str, args);
		va_end(args);  // 结束取参数
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, log_buf, len);
		#endif
		len += len_str;
		
		char err2[] = "\033[0m \r\n";
		len_str = strlen(err2);
		#if(boardPRINT_IFACE)
		lwrb_write(&tPrintTxBuff, err2, len_str);
		#endif
		len += len_str;
		
		#if(boardUSE_OS)
        // 释放互斥量
        xSemaphoreGive(MyPrintSemaphoreMutex); 
		#endif  //boardUSE_OS
		
		//开始发送数据
		#if(boardPRINT_IFACE)
		bPrint_SendDataToUsart();
		#endif
    }
	//等待信号量释放超时
    return len;
}

