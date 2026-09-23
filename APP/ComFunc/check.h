/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\ComFunc
 * File    : check.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 数据校验算法库头文件，提供累加和、CRC8、CRC16、CRC32等常用校验计算接口
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef CHECK_H_
#define CHECK_H_

#ifdef __cplusplus
extern "C" {
#endif

//****************************************************Includes******************************************************************//
#include "main.h"

#if (1)
//****************************************************Macros********************************************************************//

//****************************************************Globals*******************************************************************//

//****************************************************Types*********************************************************************//

//****************************************************Extern********************************************************************//
uint8_t  ucCheck_Sum(const uint8_t *p_str, int str_length);
uint8_t  ucCheck_SumReflect(const uint8_t *p_str, int str_length);

uint8_t  ucCheck_CRC8cal(const uint8_t *p_data, uint8_t counter);
uint8_t  ucCheck_GetCrc8(uint8_t init, const uint8_t *p_data, uint16_t length, uint8_t poly, bool reflection);

uint16_t usCheck_CRC_CCITT(uint16_t init, const uint8_t *p_data, uint16_t length);
uint16_t usCheck_Crc16(uint16_t init, const uint8_t *p_data, uint16_t length);
uint16_t usCheck_GetModbusCrc16(const uint8_t *p_data, uint32_t len);
uint16_t usCheck_MsbDataGetCrc16(const uint8_t *p_buf, int len, uint16_t crc);
uint16_t usCheck_LsbDataGetCrc16(const unsigned char *p_buf, int len, uint16_t crc);
uint16_t usCheck_CRC16(const uint8_t *p_msg, uint16_t us_data_len);
uint16_t usCheck_GetCrc16Tab(const uint8_t *p_buf, uint16_t len);

uint32_t ulCheck_GetCRC32(uint32_t init, const uint8_t *p_data, uint32_t length);
uint32_t ulCheck_Crc32Update(uint32_t crc_state, const uint8_t *p_data, uint16_t len);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* CHECK_H_ */


