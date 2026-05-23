#include "elog_init.h"
#include "..\plugins\flash\elog_flash.h"
#include "elog.h"

void vElog_Init(void)
{
/********************elog_code_begin**************************/	
 
	/* 初始化 EasyLogger */
	elog_init();
	/* 设置每个级别的日志输出格式 */
	//输出日志级别信息和日志TAG
	elog_set_fmt(ELOG_LVL_ERROR, ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_WARN, ELOG_FMT_TIME);
	elog_set_fmt(ELOG_LVL_INFO, ELOG_FMT_TIME);
	//使用颜色API开启输出
	elog_set_text_color_enabled(true);
	/* initialize EasyLogger Flash plugin */
	
	#ifdef EF_USING_LOG
	elog_flash_init();
	#endif // EF_USING_LOG
	
	/* 启动 EasyLogger */
	elog_start();
/*********************elog_code_end***************************/		
 
///********************elog_test_begin**************************/	
//	log_a("Hello EasyLogger!");
//    log_e("Hello EasyLogger!");
//    log_w("Hello EasyLogger!");
//    log_i("Hello EasyLogger!");
//    log_d("Hello EasyLogger!");
//    log_v("Hello EasyLogger!");
///********************elog_test_end**************************/	

}

