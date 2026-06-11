#ifndef BOOT_INFO_H_
#define BOOT_INFO_H_

#include "main.h"
#include "board_config.h"
#include "Sys/sys_task.h"

#if(boardEASY_FLASH)
#include "easyflash.h"
#endif

extern const char tBootMemParamStr[];
extern const char tBootVerInfoStr[];
extern const char tBootParamStr[];

//APP状态
typedef enum
{
	AS_NULL = 0,		//未选择
    AS_FINISH,			//刚升级完成
	AS_OK,				//当前是完整的
	AS_ERASE,			//已经擦除
}AppState_E;

#pragma pack(1)  //一个字节对齐
typedef struct
{
	char        saVersion[32];    // 软件版本
    char        saBuildDate[32];  // 程序编译日期
    char        saBuildTime[32];  // 程序编译时间
}VerInfo_T;
#pragma pack()

#pragma pack(4)  //一个字节对齐
typedef struct
{
	vu32        		ulCmd;             	// 0xAAAA_AAAA需要升级,其他不需要升级
	AppState_E			eAppState;			// APP状态
	vu8					ucAppFaultCnt;		// APP启动失败计数 
}BootParam_T;
#pragma pack()

#pragma pack(2)
typedef struct
{
	VerInfo_T        	tVerInfo;
	BootParam_T			tParam;
}BootMemParam_T;
#pragma pack()
extern BootMemParam_T  	tBootMemParam;

#if(boardEASY_FLASH)
extern const ef_env default_env_set[];
#endif

SysTaskId_E eBoot_InfoInit(bool init);
s8 cBoot_MemParamInit(const char* id_str);
s16 cBoot_UpdateMemParam(const char* id_str);
s16 cBoot_GetMemParam(const char* id_str);
u16 usBoot_GetMemParamSize(void);
bool bBoot_CmdExist(u32 cmd);

#if(boardUPDATE)
s8 cBoot_CtrlUpdate(bool en, AppState_E state);
#endif

#endif  















