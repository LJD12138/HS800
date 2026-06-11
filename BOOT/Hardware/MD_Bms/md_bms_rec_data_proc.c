#include "MD_Bms/md_bms_rec_data_proc.h"

#if(boardBMS_EN)
#include "MD_Bms/md_bms_rec_task.h"
#include "MD_Bms/md_bms_task.h"
#include "Print/print_task.h"
#include "Baiku/baiku_proto.h"

//****************************************************函数声明****************************************************//


/***********************************************************************************************************************
-----函数功能    处理接收到的数据
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      0:没有错误  其他有错误
************************************************************************************************************************/
s8 c_bms_rec_proc_data(BaikuProtoRx_t* proto)
{
	if(uPrint.tFlag.bBmsRecTask)
	{
		sMyPrint("bBmsRecTask:指令:0x%x, 数据:",proto->ucCmd);
		for(int i = 0; i < proto->ucValidLen; i++)
			sMyPrint("%x ",proto->ucpValidData[i]);
		sMyPrint("\r\n");
	}
	
	switch (proto->ucCmd)
    {
		default:
			return -99;
	}
	
   return 1; 

}
#endif  //boardBMS_EN
