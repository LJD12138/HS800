#ifndef I2C_H_
#define I2C_H_

#include "board_config.h"

typedef enum 
{
	AddrType_7bit,
    AddrType_8bit,
}I2C_AddrType;


typedef struct  
{
	#if(boardIC_TYPE == boardIC_GD32F30X)
    vu8              Addr;
    I2C_AddrType     AddrType;
    vu32             ulGPIO_PORT_SCL;
    vu32             ulGPIO_PIN_SCL;
    vu32             ulGPIO_PORT_SDA;
    vu32             ulGPIO_PIN_SDA;
    vu16             usDelay;
	vu8              ucBuffLen;
	vu8              ucLostCnt;
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	vu8              Addr;
    I2C_AddrType     AddrType;
    GPIO_TypeDef*    ulGPIO_PORT_SCL;
    vu32             ulGPIO_PIN_SCL;
    GPIO_TypeDef*    ulGPIO_PORT_SDA;
    vu32             ulGPIO_PIN_SDA;
    vu16             usDelay;
	vu8              ucBuffLen;
	vu8              ucLostCnt;
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#Error(Œ¥∂®“Â)
	#endif
}I2cObj_T;

void vI2C_ObjInit(I2cObj_T *p_i2c_obj);
s8 cI2C_WriteData(const I2cObj_T *p_i2c_obj, const u8 *buf, u16 len);
s8 cI2C_ReadData(const I2cObj_T *p_i2c_obj, u8 *buf, u16 len);
s8 cI2C_WriteBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *buf, u8 len);
s8 cI2C_ReadBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *buf, u8 len);
s8 cI2C_WriteBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *buf, u8 len);
s8 cI2C_ReadBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *buf, u8 len);
s8 cI2C_WriteBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *buf, u8 len);
s8 cI2C_ReadBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *buf, u8 len);
s8 cI2C_Read2BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *buf);

#endif

