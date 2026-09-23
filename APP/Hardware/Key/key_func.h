/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\Key
 * File    : key_func.h
 * Date    : 2026-09-21
 * Author  : LJD(291483914@qq.com)
 * Desc    : 按键业务动作分发处理头文件
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef KEY_FUNC_H_
#define KEY_FUNC_H_

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

//****************************************************Includes******************************************************************//
#include "main.h"

#if (boardKEY_EN)
void vKey_ProcKeyFunc(uint8_t *p_uc_key_tri_type_buff);
#endif  /* boardKEY_EN */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* KEY_FUNC_H_ */

