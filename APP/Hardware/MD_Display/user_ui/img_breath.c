/*******************************************************************************************************************************
 * Project : APP
 * Module  : G:\1-Baiku_Projects\25-HS800\1.software\HS803\APP\Hardware\MD_Display\user_ui
 * File    : img_breath.c
 * Date    : 2026-05-30
 * Author  : LJD(291483914@qq.com)
 * Desc    : 电量动效呼吸/位移图片显示组件源文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "img_breath.h"
#include <string.h>

#if (boardDISPLAY_EN)
//****************************************************Macros*******************************************************************//


//****************************************************Parameter Initialization************************************************//
/* 图片资源引用 */
extern const lv_image_dsc_t img_1;
extern const lv_image_dsc_t img_2; 
extern const lv_image_dsc_t img_3; 
extern const lv_image_dsc_t img_1_mirror_map; /* 由 ui_image_1.c 导出的水平翻转 const 数据，不占 RAM */

/* 内部控制管理结构体定义 */
typedef struct
{
    lv_obj_t              *pContainer;                             /* 全模式共用的容器 */
    lv_obj_t              *pImgLeft;                               /* 全模式共用的左侧图片 */
    lv_obj_t              *pImgRight;                              /* 全模式共用的右侧图片 */
    ImgAnimMode_E          eCurrentMode;                           /* 当前正在运行的动画模式 */
    ImgAnimPosConfig_T     posConfigs[IMG_ANIM_MODE_MAX];          /* 各模式初始坐标 */
    uint32_t               usCurrentPeriod;                        /* 当前动画周期 */
    bool                   bIsPaused;                              /* 暂停状态标志 */
} ImgAnimCtrl_T;

/* 全局唯一控制管理变量，存放在 RAM 中，符合规范使用 S_t 前缀大驼峰 */
static ImgAnimCtrl_T S_tImgAnimCtrl;

//****************************************************Function Declaration****************************************************//
static void v_exec_cb(void *p_var, const int32_t l_val);


/***********************************************************************************************************************
 * 函数功能    : 动画执行私有回调
 * 说明(备注)  : 定时任务刷新，计算并修改图片透明度及相对偏移位置
 * 传入参数    : p_var 控制结构体指针, l_val 当前帧进度值 (0~255)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
static void v_exec_cb(void *p_var, const int32_t l_val)
{
    ImgAnimCtrl_T *p_ctrl = (ImgAnimCtrl_T *)p_var;
    if (p_ctrl == NULL || p_ctrl->pImgLeft == NULL || p_ctrl->pImgRight == NULL)
    {
        return;
    }

    /* 1. 呼吸渐暗基础逻辑：改变图片控件的 Alpha 透明度 */
    lv_opa_t current_opa = (lv_opa_t)(255 - l_val);

    /* 2. 获取当前动画模式的专属初始坐标 */
    ImgAnimMode_E e_mode = p_ctrl->eCurrentMode;
    const ImgAnimPosConfig_T *p_pos = &p_ctrl->posConfigs[e_mode];

    /* 3. 按模式区分左右图片的不透明度：
     *    - 快充模式(CHARGE_FAST): 左侧 img_2 固定满不透明, 仅右侧 img_3 进行呼吸渐变
     *    - 其他模式: 左右两侧均参与呼吸渐变
     */
    if (e_mode == IMG_ANIM_MODE_CHARGE_FAST)
    {
        lv_obj_set_style_image_opa(p_ctrl->pImgLeft,  LV_OPA_COVER,  LV_PART_MAIN);
        lv_obj_set_style_image_opa(p_ctrl->pImgRight, current_opa,   LV_PART_MAIN);
    }
    else
    {
        lv_obj_set_style_image_opa(p_ctrl->pImgLeft,  current_opa, LV_PART_MAIN);
        lv_obj_set_style_image_opa(p_ctrl->pImgRight, current_opa, LV_PART_MAIN);
    }

    /* 4. 实现各模式的具体位移轨迹 */
    if (e_mode == IMG_ANIM_MODE_CHG_DISCHG)
    {
        /* 放电位移呼吸模式 */
        int32_t l_offset_y = (l_val * 8) / 255;

        lv_obj_set_pos(p_ctrl->pImgLeft, p_pos->lLeftX, p_pos->lLeftY - l_offset_y);
        lv_obj_set_pos(p_ctrl->pImgRight, p_pos->lRightX, p_pos->lRightY + l_offset_y);
    }
    else if (e_mode == IMG_ANIM_MODE_DISCHARGE)
    {
        /* 放电呼吸模式：保持在初始位置不变，仅受透明度变化影响 */
        lv_obj_set_pos(p_ctrl->pImgLeft, p_pos->lLeftX, p_pos->lLeftY);
        lv_obj_set_pos(p_ctrl->pImgRight, p_pos->lRightX, p_pos->lRightY);
    }
    else if (e_mode == IMG_ANIM_MODE_CHARGE_SLOW || e_mode == IMG_ANIM_MODE_CHARGE_FAST)
    {
        /* 充电模式：保持位置不动，仅呼吸渐变(快充仅右侧渐变) */
        lv_obj_set_pos(p_ctrl->pImgLeft, p_pos->lLeftX, p_pos->lLeftY);
        lv_obj_set_pos(p_ctrl->pImgRight, p_pos->lRightX, p_pos->lRightY);
    }
}

/***********************************************************************************************************************
 * 函数功能    : 初始化全模式共用的电量动效图片控件
 * 说明(备注)  : 创建容器和左右两个图片控件
 * 传入参数    : parent 父容器对象
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_Init(lv_obj_t *parent)
{
    if (parent == NULL)
    {
        return;
    }

    if (S_tImgAnimCtrl.pContainer != NULL)
    {
        lv_obj_delete(S_tImgAnimCtrl.pContainer);
        S_tImgAnimCtrl.pContainer = NULL;
        S_tImgAnimCtrl.pImgLeft = NULL;
        S_tImgAnimCtrl.pImgRight = NULL;
    }

    /* 1. 创建全模式共用容器 */
    S_tImgAnimCtrl.pContainer = lv_obj_create(parent);
    if (S_tImgAnimCtrl.pContainer == NULL)
    {
        return;
    }
    
    /* 去除边框背景设置为透明，设置占满整个父容器 */
    lv_obj_remove_style_all(S_tImgAnimCtrl.pContainer);
    lv_obj_set_size(S_tImgAnimCtrl.pContainer, lv_pct(100), lv_pct(100));
    lv_obj_add_flag(S_tImgAnimCtrl.pContainer, LV_OBJ_FLAG_HIDDEN); /* 默认隐藏 */

    /* 2. 创建左侧图片控件并载入默认原图 img_1 */
    S_tImgAnimCtrl.pImgLeft = lv_image_create(S_tImgAnimCtrl.pContainer);
    if (S_tImgAnimCtrl.pImgLeft != NULL)
    {
        lv_image_set_src(S_tImgAnimCtrl.pImgLeft, &img_1);
    }

    /* 3. 创建右侧图片控件并载入默认原图 img_1 */
    S_tImgAnimCtrl.pImgRight = lv_image_create(S_tImgAnimCtrl.pContainer);
    if (S_tImgAnimCtrl.pImgRight != NULL)
    {
        lv_image_set_src(S_tImgAnimCtrl.pImgRight, &img_1);
    }

    /* 4. 默认初始化各模式下初始坐标配置 */
    for (int i = 0; i < IMG_ANIM_MODE_MAX; i++)
    {
        S_tImgAnimCtrl.posConfigs[i].lLeftX = 0;
        S_tImgAnimCtrl.posConfigs[i].lLeftY = 0;
        S_tImgAnimCtrl.posConfigs[i].lRightX = 0;
        S_tImgAnimCtrl.posConfigs[i].lRightY = 0;
    }

    S_tImgAnimCtrl.eCurrentMode = IMG_ANIM_MODE_NONE;
    S_tImgAnimCtrl.usCurrentPeriod = 1000;
    S_tImgAnimCtrl.bIsPaused = false;
}

/***********************************************************************************************************************
 * 函数功能    : 配置指定动画模式下左右两个图片的相对偏移初始坐标
 * 说明(备注)  : 将坐标保存于结构体变量中
 * 传入参数    : e_mode 目标模式, p_config 初始位置结构体指针
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_SetPosConfig(ImgAnimMode_E e_mode, const ImgAnimPosConfig_T *p_config)
{
    if (e_mode < IMG_ANIM_MODE_MAX && p_config != NULL)
    {
        /* 保存配置坐标 */
        S_tImgAnimCtrl.posConfigs[e_mode] = *p_config;
        
        /* 若配置的正是当前运行中的模式，立即应用刷新位置 */
        if (S_tImgAnimCtrl.eCurrentMode == e_mode)
        {
            lv_obj_set_pos(S_tImgAnimCtrl.pImgLeft, p_config->lLeftX, p_config->lLeftY);
            lv_obj_set_pos(S_tImgAnimCtrl.pImgRight, p_config->lRightX, p_config->lRightY);
        }
    }
}

/***********************************************************************************************************************
 * 函数功能    : 切换电量动效显示的模式
 * 说明(备注)  : 实时修改左右侧图片显示资源及缩放旋转属性以契合对应模式
 * 传入参数    : e_mode 目标模式, us_period_ms 呼吸渐变周期时间(毫秒)
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_SetMode(ImgAnimMode_E e_mode, uint32_t us_period_ms)
{
    if (S_tImgAnimCtrl.pContainer == NULL || S_tImgAnimCtrl.pImgLeft == NULL || S_tImgAnimCtrl.pImgRight == NULL)
    {
        return;
    }

    S_tImgAnimCtrl.bIsPaused = false;

    /* 若切换至关闭，隐藏共用容器即可 */
    if (e_mode == IMG_ANIM_MODE_NONE)
    {
        S_tImgAnimCtrl.eCurrentMode = IMG_ANIM_MODE_NONE;
        lv_obj_add_flag(S_tImgAnimCtrl.pContainer, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    /* 记录工作状态 */
    S_tImgAnimCtrl.eCurrentMode = e_mode;
    S_tImgAnimCtrl.usCurrentPeriod = us_period_ms;
    
    /* 移除隐藏，显示容器 */
    lv_obj_clear_flag(S_tImgAnimCtrl.pContainer, LV_OBJ_FLAG_HIDDEN); 

    /* 将左右两侧图片摆放重置为对应模式的初始设定位置 */
    const ImgAnimPosConfig_T *p_pos = &S_tImgAnimCtrl.posConfigs[e_mode];
    lv_obj_set_pos(S_tImgAnimCtrl.pImgLeft, p_pos->lLeftX, p_pos->lLeftY);
    lv_obj_set_pos(S_tImgAnimCtrl.pImgRight, p_pos->lRightX, p_pos->lRightY);

    /* 模式切换重置时，两张图片均默认清除隐藏属性以准备渲染 */
    lv_obj_clear_flag(S_tImgAnimCtrl.pImgLeft, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(S_tImgAnimCtrl.pImgRight, LV_OBJ_FLAG_HIDDEN);

    if (e_mode == IMG_ANIM_MODE_CHG_DISCHG)
    {
        /* 放电位移呼吸模式：左边使用原图，右边旋转 180 度 */
        lv_image_set_src(S_tImgAnimCtrl.pImgLeft, &img_1);
        lv_image_set_src(S_tImgAnimCtrl.pImgRight, &img_1);

        lv_image_set_rotation(S_tImgAnimCtrl.pImgLeft, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgLeft, 256);
        
        lv_image_set_rotation(S_tImgAnimCtrl.pImgRight, 1800); /* 180.0度 */
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgRight, 256);  
    } 
    else if (e_mode == IMG_ANIM_MODE_DISCHARGE)
    {
        /* 放电呼吸模式：左边使用原图，右边使用编译时由 img_1 自动水平翻转生成的只读 const 镜像资源 */
        lv_image_set_src(S_tImgAnimCtrl.pImgLeft, &img_1);
        lv_image_set_src(S_tImgAnimCtrl.pImgRight, &img_1_mirror_map);

        lv_image_set_rotation(S_tImgAnimCtrl.pImgLeft, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgLeft, 256);
        
        lv_image_set_rotation(S_tImgAnimCtrl.pImgRight, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgRight, 256);
    }
    else if (e_mode == IMG_ANIM_MODE_CHARGE_SLOW)
    {
        /* 慢充模式：左边显示慢充图标 img_2，隐藏右侧 */
        lv_image_set_src(S_tImgAnimCtrl.pImgLeft, &img_2);
        lv_obj_add_flag(S_tImgAnimCtrl.pImgRight, LV_OBJ_FLAG_HIDDEN);

        lv_image_set_rotation(S_tImgAnimCtrl.pImgLeft, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgLeft, 256);
        lv_image_set_rotation(S_tImgAnimCtrl.pImgRight, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgRight, 256);
    }
    else if (e_mode == IMG_ANIM_MODE_CHARGE_FAST)
    {
        /* 快充模式：左边显示慢充图标 img_2，右边显示快充标识 img_3 */
        lv_image_set_src(S_tImgAnimCtrl.pImgLeft, &img_2);
        lv_image_set_src(S_tImgAnimCtrl.pImgRight, &img_3);

        lv_image_set_rotation(S_tImgAnimCtrl.pImgLeft, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgLeft, 256);
        lv_image_set_rotation(S_tImgAnimCtrl.pImgRight, 0);
        lv_image_set_scale_x(S_tImgAnimCtrl.pImgRight, 256);
    }

    /* 启动时手动调用首帧更新进行重定位及透明度归一 */
    v_exec_cb(&S_tImgAnimCtrl, 0);
}

/***********************************************************************************************************************
 * 函数功能    : 手动刷新动画步进
 * 说明(备注)  : 定时任务驱动，每周期递增进度以展现渐亮渐暗呼吸动效
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_ManualTick(void)
{
    static uint32_t s_us_progress = 0;
    static uint32_t s_ul_last_tick = 0;

    if (S_tImgAnimCtrl.pContainer == NULL || S_tImgAnimCtrl.pImgLeft == NULL || S_tImgAnimCtrl.pImgRight == NULL)
    {
        s_ul_last_tick = 0U;
        return;
    }
    if (S_tImgAnimCtrl.eCurrentMode == IMG_ANIM_MODE_NONE || S_tImgAnimCtrl.bIsPaused)
    {
        s_ul_last_tick = 0U;
        return;
    }
    uint32_t ul_now_tick = lv_tick_get();
    uint32_t ul_period = S_tImgAnimCtrl.usCurrentPeriod;
    uint32_t ul_elapsed;
    uint32_t ul_step;

    if (ul_period == 0U)
        ul_period = 800U;

    if (s_ul_last_tick == 0U)
    {
        s_ul_last_tick = ul_now_tick;
        return;
    }

    ul_elapsed = lv_tick_elaps(s_ul_last_tick);
    if (ul_elapsed == 0U)
        return;

    s_ul_last_tick = ul_now_tick;
    ul_step = (ul_elapsed * 256U) / ul_period;
    if (ul_step == 0U)
        ul_step = 1U;

    s_us_progress = (s_us_progress + ul_step) & 0xFFU;
    v_exec_cb(&S_tImgAnimCtrl, s_us_progress);
}

/***********************************************************************************************************************
 * 函数功能    : 停止当前所有动画并完全隐藏图片控件
 * 说明(备注)  : 内部直接切换至 NONE 模式
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_Stop(void)
{
    vImgAnim_SetMode(IMG_ANIM_MODE_NONE, 0);
}

/***********************************************************************************************************************
 * 函数功能    : 暂停当前正在运行的动画
 * 说明(备注)  : 画面将冻结在当前帧的透明度和位置
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_Pause(void)
{
    S_tImgAnimCtrl.bIsPaused = true;
}

/***********************************************************************************************************************
 * 函数功能    : 恢复被暂停的动画
 * 说明(备注)  : 重设当前保存的模式即可从原透明度/进度继续运行
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ************************************************************************************************************************/
void vImgAnim_Resume(void)
{
    if (S_tImgAnimCtrl.bIsPaused && S_tImgAnimCtrl.eCurrentMode != IMG_ANIM_MODE_NONE)
    {
        vImgAnim_SetMode(S_tImgAnimCtrl.eCurrentMode, S_tImgAnimCtrl.usCurrentPeriod); 
    }
}

#endif  //boardDISPLAY_EN
