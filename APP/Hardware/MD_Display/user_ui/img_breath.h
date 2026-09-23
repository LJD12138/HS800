/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Display\user_ui
 * File    : img_breath.h
 * Date    : 2026-05-30
 * Author  : LJD(291483914@qq.com)
 * Desc    : 电量动效呼吸/位移图片显示组件头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef __IMG_BREATH_H
#define __IMG_BREATH_H

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "board_config.h"

#if (boardDISPLAY_EN)
#include "lvgl.h"
//****************************************************Macros********************************************************************//


//****************************************************Types*********************************************************************//
/* 电量动画模式枚举 */
typedef enum {
	IMG_ANIM_MODE_NONE = 0,	/* 关闭所有动效并隐藏 */
	IMG_ANIM_MODE_CHARGE_SLOW,	/* 慢充模式：仅显示 img_2 */
	IMG_ANIM_MODE_CHARGE_FAST,	/* 快充模式：左边 img_2，右边 img_3 */
	IMG_ANIM_MODE_DISCHARGE,	/* 放电动画 OUT 模式：左边 img_1，右边 img_1_mirror 镜像呼吸渐亮 */
	IMG_ANIM_MODE_CHG_DISCHG,	/* 放电动画 IN-OUT 模式：左边原图，右边旋转180度位移呼吸 */
	IMG_ANIM_MODE_MAX
}ImgAnimMode_E;

/* 动效图片初始配置结构体 */
typedef struct {
	int32_t				lLeftX;				/* 左侧图片初始 X 坐标 */
	int32_t				lLeftY;				/* 左侧图片初始 Y 坐标 */
	int32_t				lRightX;			/* 右侧图片初始 X 坐标 */
	int32_t				lRightY;			/* 右侧图片初始 Y 坐标 */
}ImgAnimPosConfig_T;


//****************************************************Extern********************************************************************//
void vImgAnim_Init(lv_obj_t *parent);
void vImgAnim_SetPosConfig(ImgAnimMode_E e_mode, const ImgAnimPosConfig_T *p_config);
void vImgAnim_SetMode(ImgAnimMode_E e_mode, uint32_t us_period_ms);
void vImgAnim_Stop(void);
void vImgAnim_Pause(void);
void vImgAnim_Resume(void);
void vImgAnim_ManualTick(void);

#endif  /* boardDISPLAY_EN */

#ifdef __cplusplus
}
#endif  //boardDISPLAY_EN

#endif /* __IMG_BREATH_H */
