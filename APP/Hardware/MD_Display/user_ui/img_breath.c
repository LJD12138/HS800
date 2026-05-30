/**
 * @file img_breath.c
 * @brief 图像呼吸动画实现
 * @note  负责对指定图像对象做透明度渐变刷新，实现呼吸灯效果；其中 img_out 版本会自动定位 EEZ UI 对象。
 */

#include "img_breath.h"
#include "screens.h"  /* 包含screens.h以使用objects_t类型 */

/* 默认配置参数 */
#define BREATH_DEFAULT_STEP_MS      20     /* 默认刷新步进：20ms */

/***********************************************************************************************************************
-----函数功能    呼吸动画定时器回调
-----说明(备注)  每次定时器触发时只更新一次图像透明度，并在到达上下限时切换方向。
-----传入参数    p_timer:定时器对象
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
static void ImgBreath_TimerCallback(lv_timer_t *p_timer)
{
    ImgBreath_T *p_breath = (ImgBreath_T *)lv_timer_get_user_data(p_timer);
    
    if ((p_breath == NULL) || (p_breath->pObj == NULL) || (!p_breath->bEnabled))
    {
        return;
    }
    
    /* 根据半个呼吸周期计算步数 */
    u16 us_total_steps = p_breath->usPeriodMs / p_breath->usStepMs / 2; /* 半周期步数 */
    lv_opa_t t_opa_range = p_breath->tMaxOpa - p_breath->tMinOpa;
    lv_opa_t t_step;
    
    if (us_total_steps == 0)
    {
        us_total_steps = 1;
    }
    
    t_step = t_opa_range / us_total_steps;
    /* 确保步进值足够大，让呼吸变化更明显 */
    if (t_step < 50)
    {
        t_step = 50;  /* 限制最小步进值，避免变化过慢 */
    }
    
    /* 按当前方向推进透明度 */
    if (p_breath->bIncreasing)
    {
        /* 增加方向：暗 -> 明 */
        if (p_breath->tCurrentOpa + t_step >= p_breath->tMaxOpa)
        {
            p_breath->tCurrentOpa = p_breath->tMaxOpa;
            p_breath->bIncreasing = false;  /* 到达上限后切换方向 */
        }
        else
        {
            p_breath->tCurrentOpa += t_step;
        }
    }
    else
    {
        /* 减少方向：明 -> 暗 */
        if (p_breath->tCurrentOpa <= p_breath->tMinOpa + t_step)
        {
            p_breath->tCurrentOpa = p_breath->tMinOpa;
            p_breath->bIncreasing = true;   /* 到达下限后切换方向 */
        }
        else
        {
            p_breath->tCurrentOpa -= t_step;
        }
    }
    
    /* 刷新图像透明度 */
    lv_obj_set_style_img_opa(p_breath->pObj, p_breath->tCurrentOpa, LV_PART_MAIN);
}

/***********************************************************************************************************************
-----函数功能    启动指定图像的呼吸动画
-----说明(备注)  启动前会先停止旧定时器，避免重复创建动画任务。
-----传入参数    p_breath:呼吸动画控制结构体  p_img:目标图像对象  us_period_ms:呼吸周期（毫秒），0使用默认值  t_min_opa:最小不透明度  t_max_opa:最大不透明度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void ImgBreath_Start(ImgBreath_T *p_breath, lv_obj_t *p_img, u16 us_period_ms, lv_opa_t t_min_opa, lv_opa_t t_max_opa)
{
    if ((p_breath == NULL) || (p_img == NULL))
    {
        return;
    }
    
    /* 先清理旧动画，保证状态可重复进入 */
    if (p_breath->pTimer != NULL)
    {
        lv_timer_del(p_breath->pTimer);
        p_breath->pTimer = NULL;
    }
    
    /* 配置动画参数 */
    p_breath->pObj = p_img;
    p_breath->usPeriodMs = us_period_ms;
    p_breath->usStepMs = BREATH_DEFAULT_STEP_MS;
    p_breath->tMinOpa = t_min_opa;
    p_breath->tMaxOpa = t_max_opa;
    p_breath->tCurrentOpa = t_min_opa;  /* 从暗开始 */
    p_breath->bIncreasing = true;       /* 初始方向为增加 */
    p_breath->bEnabled = true;
    
    /* 设置初始透明度 */
    lv_obj_set_style_img_opa(p_breath->pObj, p_breath->tCurrentOpa, LV_PART_MAIN);
    
    /* 创建刷新定时器 */
    p_breath->pTimer = lv_timer_create(ImgBreath_TimerCallback, p_breath->usStepMs, p_breath);
    
    if (p_breath->pTimer == NULL)
    {
        p_breath->bEnabled = false;
    }
}

/***********************************************************************************************************************
-----函数功能    停止指定图像的呼吸动画
-----说明(备注)  删除定时器并释放动画状态，便于后续重新启动。
-----传入参数    p_breath:呼吸动画控制结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void ImgBreath_Stop(ImgBreath_T *p_breath)
{
    if (p_breath == NULL)
    {
        return;
    }
    
    /* 删除定时器并释放动画状态 */
    if (p_breath->pTimer != NULL)
    {
        lv_timer_del(p_breath->pTimer);
        p_breath->pTimer = NULL;
    }
    
    p_breath->bEnabled = false;
    p_breath->pObj = NULL;
}

/***********************************************************************************************************************
-----函数功能    暂停图像呼吸动画
-----说明(备注)  仅暂停定时器刷新，不释放动画状态。
-----传入参数    p_breath:呼吸动画控制结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void ImgBreath_Pause(ImgBreath_T *p_breath)
{
    if (p_breath == NULL)
    {
        return;
    }
    
    if (p_breath->pTimer != NULL)
    {
        lv_timer_pause(p_breath->pTimer);
    }
    
    p_breath->bEnabled = false;
}

/***********************************************************************************************************************
-----函数功能    恢复图像呼吸动画
-----说明(备注)  恢复已暂停的定时器，并重新使能动画。
-----传入参数    p_breath:呼吸动画控制结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void ImgBreath_Resume(ImgBreath_T *p_breath)
{
    if (p_breath == NULL)
    {
        return;
    }
    
    if (p_breath->pTimer != NULL)
    {
        lv_timer_resume(p_breath->pTimer);
    }
    
    p_breath->bEnabled = true;
}

/***********************************************************************************************************************
-----函数功能    获取 EEZ UI 中的 img_out 图像对象
-----说明(备注)  遍历 main 页面子对象，按图像源匹配 img_out。
-----传入参数    none
-----输出参数    none
-----返回值      img_out图像对象指针，未找到时返回NULL
************************************************************************************************************************/
static lv_obj_t *ImgBreath_GetImgOutObj(void)
{
    /* main 页面对象由 EEZ UI 在 screens.c 中创建 */
    extern objects_t objects;
    
    if (objects.main == NULL)
    {
        return NULL;
    }
    
    /* 通过图像源定位 img_out 对象 */
    extern const lv_image_dsc_t img_out;
    
    uint32_t child_count = lv_obj_get_child_cnt(objects.main);
    for (uint32_t i = 0; i < child_count; i++)
    {
        lv_obj_t *child = lv_obj_get_child(objects.main, i);
        if (child != NULL)
        {
            /* 只检查图像对象 */
            if (lv_obj_check_type(child, &lv_image_class))
            {
                const lv_image_dsc_t *img_src = lv_image_get_src(child);
                if (img_src == &img_out)
                {
                    return child;
                }
            }
        }
    }
    
    return NULL;
}

/***********************************************************************************************************************
-----函数功能    启动 img_out 图像呼吸动画
-----说明(备注)  自动获取 EEZ UI 中的 img_out 对象后再启动呼吸动画。
-----传入参数    p_breath:呼吸动画控制结构体  us_period_ms:呼吸周期（毫秒）  t_min_opa:最小不透明度  t_max_opa:最大不透明度
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void ImgBreath_StartImgOut(ImgBreath_T *p_breath, u16 us_period_ms, lv_opa_t t_min_opa, lv_opa_t t_max_opa)
{
    if (p_breath == NULL)
        return;
    
    lv_obj_t *p_img_out = ImgBreath_GetImgOutObj();

    /* 找到对象后再启动动画 */
    if (p_img_out != NULL)
        ImgBreath_Start(p_breath, p_img_out, us_period_ms, t_min_opa, t_max_opa);
}

/***********************************************************************************************************************
-----函数功能    停止 img_out 图像呼吸动画
-----说明(备注)  停止动画后将 img_out 设为完全透明，避免残留显示。
-----传入参数    p_breath:呼吸动画控制结构体
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void ImgBreath_StopImgOut(ImgBreath_T *p_breath)
{
    if (p_breath == NULL)
        return;
    
    /* 先停止动画 */
    ImgBreath_Stop(p_breath);
    
    lv_obj_t *p_img_out = ImgBreath_GetImgOutObj();

    /* 停止后隐藏图像 */
    if (p_img_out != NULL)
        lv_obj_set_style_img_opa(p_img_out, LV_OPA_TRANSP, LV_PART_MAIN);
}
