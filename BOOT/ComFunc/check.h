#ifndef CHECK_H_
#define CHECK_H_

#include "main.h"

//¿€º”∫Õ
uint8_t ucCheck_Sum(uint8_t *str, int str_length);
uint8_t ucCheck_SumReflect(uint8_t *str, int str_length);

//CRC8
uint8_t ucCheck_GetCrc8Tab(uint8_t *buf, uint16_t len);
uint8_t ucCheck_CRC8cal(uint8_t *p, uint8_t counter);
uint8_t ucCheck_GetCrc8(uint8_t init, const uint8_t *data, uint16_t length, uint8_t poly, bool reflection);

//CRC16
uint16_t usCheck_CRC_CCITT(uint16_t init,  const uint8_t* data, uint16_t length);
uint16_t usCheck_Crc16(uint16_t init, uint8_t* data, uint16_t length);
uint16_t usCheck_GetModbusCrc16(uint8_t *data, uint32_t len);
uint16_t usCheck_MsbDataGetCrc16(uint8_t* buf, int len, uint16_t crc);
uint16_t usCheck_LsbDataGetCrc16(unsigned char* buf, int len, uint16_t crc);
uint16_t usCheck_CRC16(uint8_t *puchMsg, uint8_t usDataLen);
uint16_t usCheck_GetCrc16Tab(uint8_t *buf, uint16_t len);

//CRC32
uint32_t ulCheck_GetCRC32(uint32_t init, uint8_t* data, uint32_t length);

#endif


