#include "lv_port_disp.h"

#if(boardDISPLAY)

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_timer.h"
#include "lv_port_tft_disp.h"
#include "lv_port_tft_gpio.h"
#include <stdbool.h>

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#include "semphr.h"

/*创建信号量句柄 */
SemaphoreHandle_t DispSemaphoreBinary = NULL;
#endif  //boardUSE_OS


/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
    #warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
    #define MY_DISP_HOR_RES    320
#endif

#ifndef MY_DISP_VER_RES
    #warning Please define or replace the macro MY_DISP_VER_RES with the actual screen height, default value 240 is used for now.
    #define MY_DISP_VER_RES    240
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*------------------------------------
     * Create a display and set a flush_cb
     * -----------------------------------*/
    lv_display_t * disp = lv_display_create(MY_DISP_HOR_RES, MY_DISP_VER_RES);
    lv_display_set_flush_cb(disp, disp_flush);

    /* Example 1
     * One buffer for partial rendering*/
//    static lv_color_t buf_1_1[MY_DISP_HOR_RES * 10];                          /*A buffer for 10 rows*/
//    lv_display_set_buffers(disp, buf_1_1, NULL, sizeof(buf_1_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 2
     * Two buffers for partial rendering
     * In flush_cb DMA or similar hardware should be used to update the display in the background.*/
    __align(4) static lv_color_t buf_2_1[MY_DISP_HOR_RES * 100];
    __align(4) static lv_color_t buf_2_2[MY_DISP_HOR_RES * 100];
    lv_display_set_buffers(disp, buf_2_1, buf_2_2, sizeof(buf_2_1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* Example 3
     * Two buffers screen sized buffer for double buffering.
     * Both LV_DISPLAY_RENDER_MODE_DIRECT and LV_DISPLAY_RENDER_MODE_FULL works, see their comments*/
//    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];
//    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];
//    lv_display_set_buffers(disp, buf_3_1, buf_3_2, sizeof(buf_3_1), LV_DISPLAY_RENDER_MODE_DIRECT);

}

/**********************
 *   STATIC FUNCTIONS
 **********************/

/*Initialize your display and the required peripherals.*/
static void disp_init(void)
{
	/* 创建二进制信号量 */
    DispSemaphoreBinary = xSemaphoreCreateBinary(); 
	xSemaphoreGive(DispSemaphoreBinary);
	
    /*You code here*/
	vTFT_DispIfaceInit();
	
	#if(tftDISP_SPI_SELECT == 2)
	MX_SPI4_DMA_Init();
	#endif
	
//	gtim_timx_int_init(19999, 9);
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
__INLINE void disp_enable_update(u8 index)
{
	#if(boardUSE_OS)
	if(index == 1)
	{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
		xSemaphoreGiveFromISR(DispSemaphoreBinary,&xHigherPriorityTaskWoken);
	}
	else
		xSemaphoreGive(DispSemaphoreBinary);
    
	#else
	disp_flush_enabled = true;
	#endif
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

/*Flush the content of the internal buffer the specific area on the display.
 *`px_map` contains the rendered image as raw pixel map and it should be copied to `area` on the display.
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_display_flush_ready()' has to be called when it's finished.*/
static void disp_flush(lv_display_t * disp_drv, const lv_area_t * area, uint8_t * px_map)
{
	#if(boardUSE_OS)
	if(DispSemaphoreBinary != NULL) 
    {
		if(xSemaphoreTake(DispSemaphoreBinary, ( TickType_t)100) == pdPASS) 
		{
			vTFT_FastDrawColor(area->x1,area->y1,area->x2,area->y2,(u16*)px_map);

			lv_display_flush_ready(disp_drv);
		}
		else
		{
			disp_enable_update(0);
		}
	}
	#else
	if(disp_flush_enabled) 
	{
        /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
		vTFT_FastDrawColor(area->x1,area->y1,area->x2,area->y2,(u16*)px_map);
    }

    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    lv_display_flush_ready(disp_drv);
	#endif
}

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
