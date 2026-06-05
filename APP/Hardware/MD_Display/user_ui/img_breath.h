/***********************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS803\APP\Hardware\MD_Display\user_ui
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

/* ==========================================includes====================================*/
#include "board_config.h"

#if (boardDISPLAY_EN)
#include "lvgl.h"
/* ==========================================macros======================================*/


/* ==========================================types=======================================*/
/* 电量动画模式枚举 */
typedef enum {
    IMG_ANIM_MODE_NONE = 0,        /* 关闭所有动效并隐藏 */
    IMG_ANIM_MODE_CHARGE_SLOW,     /* 慢充模式：仅显示 img_2 */
    IMG_ANIM_MODE_CHARGE_FAST,     /* 快充模式：左边 img_2，右边 img_3 */
    IMG_ANIM_MODE_DISCHARGE,       /* 放电动画 OUT 模式：左边 img_1，右边 img_1_mirror 镜像呼吸渐亮 */
    IMG_ANIM_MODE_CHG_DISCHG,      /* 放电动画 IN-OUT 模式：左边原图，右边旋转180度位移呼吸 */
    IMG_ANIM_MODE_MAX
} ImgAnimMode_E;

/* 动效图片初始配置结构体 */
typedef struct {
    int32_t lLeftX;                /* 左侧图片初始 X 坐标 */
    int32_t lLeftY;                /* 左侧图片初始 Y 坐标 */
    int32_t lRightX;               /* 右侧图片初始 X 坐标 */
    int32_t lRightY;               /* 右侧图片初始 Y 坐标 */
} ImgAnimPosConfig_T;


/* ==========================================extern======================================*/
/**
 * 函数功能    : 初始化全模式共用的电量动效图片控件
 * 说明(备注)  : 创建容器和左右两个图片控件
 * 传入参数    : parent 父容器对象
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_Init(lv_obj_t *parent);

/**
 * 函数功能    : 配置指定动画模式下左右两个图片的相对偏移初始坐标
 * 说明(备注)  : 设置各模式下左右两个图片的初始摆放坐标
 * 传入参数    : e_mode 目标模式, p_config 初始位置结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_SetPosConfig(ImgAnimMode_E e_mode, const ImgAnimPosConfig_T *p_config);

/**
 * 函数功能    : 切换电量动效显示的模式
 * 说明(备注)  : 切换不同的动画表现形式
 * 传入参数    : e_mode 目标模式, us_period_ms 呼吸渐变周期时间(毫秒)
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_SetMode(ImgAnimMode_E e_mode, uint32_t us_period_ms);

/**
 * 函数功能    : 停止当前所有动画并完全隐藏图片控件
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_Stop(void);

/**
 * 函数功能    : 暂停当前正在运行的动画
 * 说明(备注)  : 画面将冻结在当前帧的透明度和位置
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_Pause(void);

/**
 * 函数功能    : 恢复被暂停的动画
 * 说明(备注)  : 无
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_Resume(void);

/**
 * 函数功能    : 手动刷新动画步进
 * 说明(备注)  : 在刷新界面周期性任务中被定时调用步进
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 */
void vImgAnim_ManualTick(void);

#endif  /* img_breath.h */

#ifdef __cplusplus
}
#endif  //boardDISPLAY_EN

#endif /* __IMG_BREATH_H */
