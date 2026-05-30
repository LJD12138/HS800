/**
 * @file img_breath.h
 * @brief Image Breathing Animation - Header
 * @author User
 * @version 1.0
 */

#ifndef __IMG_BREATH_H
#define __IMG_BREATH_H

#include "lvgl.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Image Breathing 控制结构体 */
typedef struct
{
    lv_obj_t * pObj;           /* 目标图像对象 */
    lv_timer_t * pTimer;       /* 动画定时器 */
    u16 usPeriodMs;            /* 呼吸周期（毫秒） */
    u16 usStepMs;              /* 定时器间隔（毫秒） */
    lv_opa_t tMinOpa;          /* 最小不透明度 */
    lv_opa_t tMaxOpa;          /* 最大不透明度 */
    lv_opa_t tCurrentOpa;      /* 当前不透明度 */
    bool bIncreasing;          /* 当前是否为增加方向 */
    bool bEnabled;             /* 动画使能标志 */
} ImgBreath_T;


void ImgBreath_Start(ImgBreath_T *p_breath, lv_obj_t *p_img, u16 us_period_ms, lv_opa_t t_min_opa, lv_opa_t t_max_opa);
void ImgBreath_Stop(ImgBreath_T *p_breath);
void ImgBreath_Pause(ImgBreath_T *p_breath);
void ImgBreath_Resume(ImgBreath_T *p_breath);
void ImgBreath_StartImgOut(ImgBreath_T *p_breath, u16 us_period_ms, lv_opa_t t_min_opa, lv_opa_t t_max_opa);
void ImgBreath_StopImgOut(ImgBreath_T *p_breath);

#ifdef __cplusplus
}
#endif

#endif /* __IMG_BREATH_H */
