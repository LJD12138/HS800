/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\ComFunc
 * File    : function.h
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : 通用基础工具函数库头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 ************************************************************************************************************************/

#ifndef FUNCTION_H_
#define FUNCTION_H_

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
void     hex2ascii(const unsigned char *p_hex, char *p_ascii, int len);
int      sFunc_HexStrToHex(char ch);
void     sFunc_2HexStrTo1Hex(const char *p_hexstr, unsigned char *p_hex, int len);
bool     isGBK(u8 val);
bool     bFun_DataCompare(const uint8_t *p_src, const uint8_t *p_dst, uint8_t length);
bool     bFun_DataCompare1(const uint8_t *p_src, const uint8_t *p_dst, uint8_t length, uint8_t end_symbol);
void     Ui16ToUin8_P(uint8_t *p_bdata, uint16_t adata);
void     FloatToUin8_P(uint8_t *p_bdata, uint16_t adata);
void     Uin8ToUin16_P(const uint8_t *p_bdata, uint16_t *p_adata);
void     Uin8ToUin16_M(const uint8_t *p_bdata, uint16_t *p_adata);
int      sVulueTurn(int value);
bool     bFloatEqualJudge(float x, float y, float EPSILON);
u16      hex_to_int(const u8 *p_hex, u8 len);
bool     bFunc_CompareDataIsExist(const uint8_t *p_src, uint8_t data, uint16_t length);
uint16_t swap_bytes(uint16_t value);
void     bFunc_FindMinMax(const u16 *p_arr, u16 n, u16 *p_max, u16 *p_min);
u8       ucFunc_PositTableU16(const u16 *p_buff, u8 array_len, u16 dat);
u16      usFunc_PositTableU32(const u32 *p_buff, u16 array_len, u32 dat);
u16      usFunc_SwapU16(u16 input);
u32      ulFunc_SwapU32(u32 input);
u64      u64Func_SwapU64(u64 input);
bool     bFunc_SwapU16Array(u8 *p_dst, const u8 *p_src, u16 reg_size);
u8       ucFunc_ReverseBits(u8 x);
void     vFunc_GetMaxMin(u8 num, const s16 *p_array, s16 *p_min, s16 *p_max);
float    fFunc_Fabs(float num1, float num2);
u32      ulFunc_Pow(u8 m, u8 n);
void     vFunc_CycleGetNextNum(u8 *p_num, const u8 *p_num_max);
u32      ulFunc_GetLe32(const u8 *p_data);
void     vFunc_FastMemCpy(void *p_dst, const void *p_src, size_t len);

#endif  /* 1 */

#ifdef __cplusplus
}
#endif

#endif  /* FUNCTION_H_ */

