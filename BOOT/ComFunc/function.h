#ifndef FUNCTION_H_
#define FUNCTION_H_

#include "main.h"

void hex2ascii(unsigned char* hex, char* ascii, int len);
int sFunc_HexStrToHex(char ch);
void sFunc_2HexStrTo1Hex(char* hexstr, unsigned char* hex, int len);
bool isGBK(u8 val);
bool bFun_DataCompare(uint8_t *src, uint8_t *dst, uint8_t length);
bool bFun_DataCompare1(uint8_t *src, uint8_t *dst, uint8_t length, uint8_t end_symbol);
void Ui16ToUin8_P(uint8_t *bdata,uint16_t adata);
void FloatToUin8_P(uint8_t *bdata,uint16_t adata);
void Uin8ToUin16_P(uint8_t *bdata,uint16_t adata);
void Uin8ToUin16_M(uint8_t *bdata,uint16_t *adata);
int sVulueTurn(int value);
bool bFloatEqualJudge( float x, float y, float EPSILON);
u16 hex_to_int(u8* hex ,u8 len);
bool bFunc_CompareDataIsExist(uint8_t *src, uint8_t data);
void bFunc_FindMinMax(u16 *arr, u16 n, u16 *max, u16 *min);
u8 ucFunc_PositTableU16(const u16 *buff,u8 array_len,u16 dat);
u16 usFunc_PositTableU32(const u32 *buff,u16 array_len,u32 dat);
u16 usFunc_SwapU16(u16 input);
bool bFunc_SwapU16Array(u8 *dst, u8 *src, u16 reg_size);
u8 ucFunc_ReverseBits(u8 x);
void vFunc_GetMaxMin(u8 Num, s16 *Array, s16 *Min, s16 *Max);
float fFunc_Fabs(float num1, float num2);
u32 ulFunc_Pow(u8 m,u8 n);
void vFunc_GetCoefficient(const float *x, float *y, int n, float *a, float *dt);
void vFunc_CycleGetNextNum(u8* num, const u8* num_max);
#endif




