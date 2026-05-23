#ifndef __HT1621_H
#define __HT1621_H

#include "main.h"
#include "i2c.h"

/* Backlight control macros */
#define     	dispLIGHT_POWER_RCU      				RCU_GPIOC
#define     	dispLIGHT_POWER_PORT     				GPIOC
#define     	dispLIGHT_POWER_PIN      				GPIO_PIN_8
#define     	dispLIGHT_POWER_ON()    				GPIO_BOP(dispLIGHT_POWER_PORT) = dispLIGHT_POWER_PIN
#define     	dispLIGHT_POWER_OFF()    				GPIO_BC(dispLIGHT_POWER_PORT)  = dispLIGHT_POWER_PIN

/* HT1621 I2C SDA macros */
#define     	dispHT_SDA_RCU      					RCU_GPIOC
#define     	dispHT_SDA_PORT     					GPIOC
#define     	dispHT_SDA_PIN      					GPIO_PIN_9
#define     	dispHT_SDA_H()      					GPIO_BOP(dispHT_SDA_PORT) = dispHT_SDA_PIN
#define     	dispHT_SDA_L()      					GPIO_BC(dispHT_SDA_PORT)  = dispHT_SDA_PIN

/* HT1621 I2C SCL macros */
#define     	dispHT_SCL_RCU      					RCU_GPIOA
#define     	dispHT_SCL_PORT     					GPIOA
#define     	dispHT_SCL_PIN      					GPIO_PIN_8
#define     	dispHT_SCL_H()      					GPIO_BOP(dispHT_SCL_PORT) = dispHT_SCL_PIN
#define     	dispHT_SCL_L()      					GPIO_BC(dispHT_SCL_PORT)  = dispHT_SCL_PIN

/* HT1621 I2C read SDA macro */
#define     	dispHT_READ_SDA()   					gpio_input_bit_get(dispHT_SDA_PORT, dispHT_SDA_PIN)

 

#define  LCD_IC1_1A_ICON_COM_INDEX				3
#define  LCD_IC1_1B_ICON_COM_INDEX				2
#define  LCD_IC1_1C_ICON_COM_INDEX				0
#define  LCD_IC1_1D_ICON_COM_INDEX				0
#define  LCD_IC1_1E_ICON_COM_INDEX				1
#define  LCD_IC1_1F_ICON_COM_INDEX				2
#define  LCD_IC1_1G_ICON_COM_INDEX				1

#define LCD_IC1_1A_ICON_SEG_INDEX 				49  
#define LCD_IC1_1B_ICON_SEG_INDEX 				48 
#define LCD_IC1_1C_ICON_SEG_INDEX 				48 
#define LCD_IC1_1D_ICON_SEG_INDEX 				49 
#define LCD_IC1_1E_ICON_SEG_INDEX 				49 
#define LCD_IC1_1F_ICON_SEG_INDEX 				49 
#define LCD_IC1_1G_ICON_SEG_INDEX 				48 


#define LCD_IC1_2A_ICON_COM_INDEX				3
#define LCD_IC1_2B_ICON_COM_INDEX				2
#define LCD_IC1_2C_ICON_COM_INDEX				0
#define LCD_IC1_2D_ICON_COM_INDEX				0
#define LCD_IC1_2E_ICON_COM_INDEX				1
#define LCD_IC1_2F_ICON_COM_INDEX				2
#define LCD_IC1_2G_ICON_COM_INDEX				1

#define LCD_IC1_2A_ICON_SEG_INDEX 				47  
#define LCD_IC1_2B_ICON_SEG_INDEX 				46 
#define LCD_IC1_2C_ICON_SEG_INDEX 				46 
#define LCD_IC1_2D_ICON_SEG_INDEX 				47 
#define LCD_IC1_2E_ICON_SEG_INDEX 				47 
#define LCD_IC1_2F_ICON_SEG_INDEX 				47 
#define LCD_IC1_2G_ICON_SEG_INDEX 				46 


#define LCD_IC1_3A_ICON_COM_INDEX				3
#define LCD_IC1_3B_ICON_COM_INDEX				2
#define LCD_IC1_3C_ICON_COM_INDEX				0
#define LCD_IC1_3D_ICON_COM_INDEX				0
#define LCD_IC1_3E_ICON_COM_INDEX				1
#define LCD_IC1_3F_ICON_COM_INDEX				2
#define LCD_IC1_3G_ICON_COM_INDEX				1

#define LCD_IC1_3A_ICON_SEG_INDEX 				45 
#define LCD_IC1_3B_ICON_SEG_INDEX 				44 
#define LCD_IC1_3C_ICON_SEG_INDEX 				44 
#define LCD_IC1_3D_ICON_SEG_INDEX 				45
#define LCD_IC1_3E_ICON_SEG_INDEX 				45 
#define LCD_IC1_3F_ICON_SEG_INDEX 				45 
#define LCD_IC1_3G_ICON_SEG_INDEX 				44


#define LCD_IC1_4A_ICON_COM_INDEX				3
#define LCD_IC1_4B_ICON_COM_INDEX				2
#define LCD_IC1_4C_ICON_COM_INDEX				0
#define LCD_IC1_4D_ICON_COM_INDEX				0
#define LCD_IC1_4E_ICON_COM_INDEX				1
#define LCD_IC1_4F_ICON_COM_INDEX				2
#define LCD_IC1_4G_ICON_COM_INDEX				1

#define LCD_IC1_4A_ICON_SEG_INDEX 				43 
#define LCD_IC1_4B_ICON_SEG_INDEX 				42
#define LCD_IC1_4C_ICON_SEG_INDEX 				42 
#define LCD_IC1_4D_ICON_SEG_INDEX 				43 
#define LCD_IC1_4E_ICON_SEG_INDEX 				43 
#define LCD_IC1_4F_ICON_SEG_INDEX 				43
#define LCD_IC1_4G_ICON_SEG_INDEX 				42 


#define LCD_IC1_5A_ICON_COM_INDEX				0
#define LCD_IC1_5B_ICON_COM_INDEX				0
#define LCD_IC1_5C_ICON_COM_INDEX				2
#define LCD_IC1_5D_ICON_COM_INDEX				3
#define LCD_IC1_5E_ICON_COM_INDEX				2
#define LCD_IC1_5F_ICON_COM_INDEX				1
#define LCD_IC1_5G_ICON_COM_INDEX				1

#define LCD_IC1_5A_ICON_SEG_INDEX 				36  
#define LCD_IC1_5B_ICON_SEG_INDEX 				37 
#define LCD_IC1_5C_ICON_SEG_INDEX 				37 
#define LCD_IC1_5D_ICON_SEG_INDEX 				36 
#define LCD_IC1_5E_ICON_SEG_INDEX 				36 
#define LCD_IC1_5F_ICON_SEG_INDEX 				36 
#define LCD_IC1_5G_ICON_SEG_INDEX 				37


#define LCD_IC1_6A_ICON_COM_INDEX				0
#define LCD_IC1_6B_ICON_COM_INDEX				0
#define LCD_IC1_6C_ICON_COM_INDEX				2
#define LCD_IC1_6D_ICON_COM_INDEX				3
#define LCD_IC1_6E_ICON_COM_INDEX				2
#define LCD_IC1_6F_ICON_COM_INDEX				1
#define LCD_IC1_6G_ICON_COM_INDEX				1

#define LCD_IC1_6A_ICON_SEG_INDEX 				38  
#define LCD_IC1_6B_ICON_SEG_INDEX 			    39
#define LCD_IC1_6C_ICON_SEG_INDEX 				39 
#define LCD_IC1_6D_ICON_SEG_INDEX 				38 
#define LCD_IC1_6E_ICON_SEG_INDEX 				38 
#define LCD_IC1_6F_ICON_SEG_INDEX 				38 
#define LCD_IC1_6G_ICON_SEG_INDEX 				39 


#define LCD_IC1_7A_ICON_COM_INDEX				0
#define LCD_IC1_7B_ICON_COM_INDEX				0
#define LCD_IC1_7C_ICON_COM_INDEX				2
#define LCD_IC1_7D_ICON_COM_INDEX				3
#define LCD_IC1_7E_ICON_COM_INDEX				2
#define LCD_IC1_7F_ICON_COM_INDEX				1
#define LCD_IC1_7G_ICON_COM_INDEX				1

#define LCD_IC1_7A_ICON_SEG_INDEX 				40  
#define LCD_IC1_7B_ICON_SEG_INDEX 				41 
#define LCD_IC1_7C_ICON_SEG_INDEX 				41 
#define LCD_IC1_7D_ICON_SEG_INDEX 				40 
#define LCD_IC1_7E_ICON_SEG_INDEX 				40 
#define LCD_IC1_7F_ICON_SEG_INDEX 				40
#define LCD_IC1_7G_ICON_SEG_INDEX 				41 


#define LCD_IC1_8A_ICON_COM_INDEX				3
#define LCD_IC1_8B_ICON_COM_INDEX				2
#define LCD_IC1_8C_ICON_COM_INDEX				0
#define LCD_IC1_8D_ICON_COM_INDEX				0
#define LCD_IC1_8E_ICON_COM_INDEX				1
#define LCD_IC1_8F_ICON_COM_INDEX				2
#define LCD_IC1_8G_ICON_COM_INDEX				1

#define LCD_IC1_8A_ICON_SEG_INDEX 				35  
#define LCD_IC1_8B_ICON_SEG_INDEX 				34
#define LCD_IC1_8C_ICON_SEG_INDEX 				34 
#define LCD_IC1_8D_ICON_SEG_INDEX 				35 
#define LCD_IC1_8E_ICON_SEG_INDEX 				35
#define LCD_IC1_8F_ICON_SEG_INDEX 				35
#define LCD_IC1_8G_ICON_SEG_INDEX 				34 


#define LCD_IC1_9A_ICON_COM_INDEX				3
#define LCD_IC1_9B_ICON_COM_INDEX				2
#define LCD_IC1_9C_ICON_COM_INDEX				0
#define LCD_IC1_9D_ICON_COM_INDEX				0
#define LCD_IC1_9E_ICON_COM_INDEX				1
#define LCD_IC1_9F_ICON_COM_INDEX				2
#define LCD_IC1_9G_ICON_COM_INDEX				1

#define LCD_IC1_9A_ICON_SEG_INDEX 				27  
#define LCD_IC1_9B_ICON_SEG_INDEX 				26 
#define LCD_IC1_9C_ICON_SEG_INDEX 				26 
#define LCD_IC1_9D_ICON_SEG_INDEX 				27 
#define LCD_IC1_9E_ICON_SEG_INDEX 				27 
#define LCD_IC1_9F_ICON_SEG_INDEX 				27 
#define LCD_IC1_9G_ICON_SEG_INDEX 				26 


#define LCD_IC1_10A_ICON_COM_INDEX				3
#define LCD_IC1_10B_ICON_COM_INDEX				3
#define LCD_IC1_10C_ICON_COM_INDEX				1
#define LCD_IC1_10D_ICON_COM_INDEX				0
#define LCD_IC1_10E_ICON_COM_INDEX				1
#define LCD_IC1_10F_ICON_COM_INDEX				2
#define LCD_IC1_10G_ICON_COM_INDEX				2

#define LCD_IC1_10A_ICON_SEG_INDEX 				25  
#define LCD_IC1_10B_ICON_SEG_INDEX 				24 
#define LCD_IC1_10C_ICON_SEG_INDEX 				24 
#define LCD_IC1_10D_ICON_SEG_INDEX 				25 
#define LCD_IC1_10E_ICON_SEG_INDEX 				25 
#define LCD_IC1_10F_ICON_SEG_INDEX 				25 
#define LCD_IC1_10G_ICON_SEG_INDEX 				24 


#define LCD_IC1_11A_ICON_COM_INDEX				3
#define LCD_IC1_11B_ICON_COM_INDEX				3
#define LCD_IC1_11C_ICON_COM_INDEX				1
#define LCD_IC1_11D_ICON_COM_INDEX				0
#define LCD_IC1_11E_ICON_COM_INDEX				1
#define LCD_IC1_11F_ICON_COM_INDEX				2
#define LCD_IC1_11G_ICON_COM_INDEX				2

#define LCD_IC1_11A_ICON_SEG_INDEX 				23  
#define LCD_IC1_11B_ICON_SEG_INDEX 				22 
#define LCD_IC1_11C_ICON_SEG_INDEX 				22 
#define LCD_IC1_11D_ICON_SEG_INDEX 				23 
#define LCD_IC1_11E_ICON_SEG_INDEX 				23 
#define LCD_IC1_11F_ICON_SEG_INDEX 				23
#define LCD_IC1_11G_ICON_SEG_INDEX 				22 


#define LCD_IC1_12A_ICON_COM_INDEX				3
#define LCD_IC1_12B_ICON_COM_INDEX				2
#define LCD_IC1_12C_ICON_COM_INDEX				0
#define LCD_IC1_12D_ICON_COM_INDEX				0
#define LCD_IC1_12E_ICON_COM_INDEX				1
#define LCD_IC1_12F_ICON_COM_INDEX				2
#define LCD_IC1_12G_ICON_COM_INDEX				1

#define LCD_IC1_12A_ICON_SEG_INDEX 				32  
#define LCD_IC1_12B_ICON_SEG_INDEX 				31
#define LCD_IC1_12C_ICON_SEG_INDEX 				31
#define LCD_IC1_12D_ICON_SEG_INDEX 				32 
#define LCD_IC1_12E_ICON_SEG_INDEX 				32
#define LCD_IC1_12F_ICON_SEG_INDEX 				32
#define LCD_IC1_12G_ICON_SEG_INDEX 				31


#define LCD_IC1_13A_ICON_COM_INDEX				3
#define LCD_IC1_13B_ICON_COM_INDEX				3
#define LCD_IC1_13C_ICON_COM_INDEX				1
#define LCD_IC1_13D_ICON_COM_INDEX				0
#define LCD_IC1_13E_ICON_COM_INDEX				1
#define LCD_IC1_13F_ICON_COM_INDEX				2
#define LCD_IC1_13G_ICON_COM_INDEX				2

#define LCD_IC1_13A_ICON_SEG_INDEX 				30  
#define LCD_IC1_13B_ICON_SEG_INDEX 				29
#define LCD_IC1_13C_ICON_SEG_INDEX 				29
#define LCD_IC1_13D_ICON_SEG_INDEX 				30 
#define LCD_IC1_13E_ICON_SEG_INDEX 				30
#define LCD_IC1_13F_ICON_SEG_INDEX 				30
#define LCD_IC1_13G_ICON_SEG_INDEX 				29


#define LCD_IC1_14A_ICON_COM_INDEX				3
#define LCD_IC1_14B_ICON_COM_INDEX				3
#define LCD_IC1_14C_ICON_COM_INDEX				1
#define LCD_IC1_14D_ICON_COM_INDEX				0
#define LCD_IC1_14E_ICON_COM_INDEX				1
#define LCD_IC1_14F_ICON_COM_INDEX				2
#define LCD_IC1_14G_ICON_COM_INDEX				2

#define LCD_IC1_14A_ICON_SEG_INDEX 				12  
#define LCD_IC1_14B_ICON_SEG_INDEX 				13
#define LCD_IC1_14C_ICON_SEG_INDEX 				13
#define LCD_IC1_14D_ICON_SEG_INDEX 				12 
#define LCD_IC1_14E_ICON_SEG_INDEX 				12
#define LCD_IC1_14F_ICON_SEG_INDEX 				12
#define LCD_IC1_14G_ICON_SEG_INDEX 				13


#define LCD_IC1_15A_ICON_COM_INDEX				3
#define LCD_IC1_15B_ICON_COM_INDEX				3
#define LCD_IC1_15C_ICON_COM_INDEX				1
#define LCD_IC1_15D_ICON_COM_INDEX				0
#define LCD_IC1_15E_ICON_COM_INDEX				1
#define LCD_IC1_15F_ICON_COM_INDEX				2
#define LCD_IC1_15G_ICON_COM_INDEX				2

#define LCD_IC1_15A_ICON_SEG_INDEX 				14  
#define LCD_IC1_15B_ICON_SEG_INDEX 				15
#define LCD_IC1_15C_ICON_SEG_INDEX 				15
#define LCD_IC1_15D_ICON_SEG_INDEX 				14 
#define LCD_IC1_15E_ICON_SEG_INDEX 				14
#define LCD_IC1_15F_ICON_SEG_INDEX 				14
#define LCD_IC1_15G_ICON_SEG_INDEX 				15


#define LCD_IC1_16A_ICON_COM_INDEX				3
#define LCD_IC1_16B_ICON_COM_INDEX				3
#define LCD_IC1_16C_ICON_COM_INDEX				1
#define LCD_IC1_16D_ICON_COM_INDEX				0
#define LCD_IC1_16E_ICON_COM_INDEX				1
#define LCD_IC1_16F_ICON_COM_INDEX				2
#define LCD_IC1_16G_ICON_COM_INDEX				2

#define LCD_IC1_16A_ICON_SEG_INDEX 				16  
#define LCD_IC1_16B_ICON_SEG_INDEX 				17
#define LCD_IC1_16C_ICON_SEG_INDEX 				17
#define LCD_IC1_16D_ICON_SEG_INDEX 				16 
#define LCD_IC1_16E_ICON_SEG_INDEX 				16
#define LCD_IC1_16F_ICON_SEG_INDEX 				16
#define LCD_IC1_16G_ICON_SEG_INDEX 				17


#define LCD_IC1_17A_ICON_COM_INDEX				3
#define LCD_IC1_17B_ICON_COM_INDEX				3
#define LCD_IC1_17C_ICON_COM_INDEX				1
#define LCD_IC1_17D_ICON_COM_INDEX				0
#define LCD_IC1_17E_ICON_COM_INDEX				1
#define LCD_IC1_17F_ICON_COM_INDEX				2
#define LCD_IC1_17G_ICON_COM_INDEX				2

#define LCD_IC1_17A_ICON_SEG_INDEX 				18  
#define LCD_IC1_17B_ICON_SEG_INDEX 				19
#define LCD_IC1_17C_ICON_SEG_INDEX 				19
#define LCD_IC1_17D_ICON_SEG_INDEX 				18 
#define LCD_IC1_17E_ICON_SEG_INDEX 				18
#define LCD_IC1_17F_ICON_SEG_INDEX 				18
#define LCD_IC1_17G_ICON_SEG_INDEX 				19


#define LCD_IC1_18A_ICON_COM_INDEX				3
#define LCD_IC1_18B_ICON_COM_INDEX				2
#define LCD_IC1_18C_ICON_COM_INDEX				0
#define LCD_IC1_18D_ICON_COM_INDEX				0
#define LCD_IC1_18E_ICON_COM_INDEX				1
#define LCD_IC1_18F_ICON_COM_INDEX				2
#define LCD_IC1_18G_ICON_COM_INDEX				1

#define LCD_IC1_18A_ICON_SEG_INDEX 				7  
#define LCD_IC1_18B_ICON_SEG_INDEX 				8
#define LCD_IC1_18C_ICON_SEG_INDEX 				8
#define LCD_IC1_18D_ICON_SEG_INDEX 				7 
#define LCD_IC1_18E_ICON_SEG_INDEX 				7
#define LCD_IC1_18F_ICON_SEG_INDEX 				7
#define LCD_IC1_18G_ICON_SEG_INDEX 				8


#define LCD_IC1_19A_ICON_COM_INDEX				3
#define LCD_IC1_19B_ICON_COM_INDEX				2
#define LCD_IC1_19C_ICON_COM_INDEX				0
#define LCD_IC1_19D_ICON_COM_INDEX				0
#define LCD_IC1_19E_ICON_COM_INDEX				1
#define LCD_IC1_19F_ICON_COM_INDEX				2
#define LCD_IC1_19G_ICON_COM_INDEX				1

#define LCD_IC1_19A_ICON_SEG_INDEX 				9  
#define LCD_IC1_19B_ICON_SEG_INDEX 				10
#define LCD_IC1_19C_ICON_SEG_INDEX 				10
#define LCD_IC1_19D_ICON_SEG_INDEX 				9 
#define LCD_IC1_19E_ICON_SEG_INDEX 				9
#define LCD_IC1_19F_ICON_SEG_INDEX 				9
#define LCD_IC1_19G_ICON_SEG_INDEX 				10




//S1 
#define S1_ICON1_SEG_INDEX					0
#define S1_ICON1_COM_INDEX					0
//S2  
#define S2_ICON1_SEG_INDEX					0
#define S2_ICON1_COM_INDEX					1
//S3  
#define S3_ICON1_SEG_INDEX					0
#define S3_ICON1_COM_INDEX					2
//S4  
#define S4_ICON1_SEG_INDEX					0
#define S4_ICON1_COM_INDEX					3
//S5  
#define S5_ICON1_SEG_INDEX					1
#define S5_ICON1_COM_INDEX					3
//S6  
#define S6_ICON1_SEG_INDEX					1
#define S6_ICON1_COM_INDEX					0
//S7  
#define S7_ICON1_SEG_INDEX					1
#define S7_ICON1_COM_INDEX					1
//S8  
#define S8_ICON1_SEG_INDEX					1
#define S8_ICON1_COM_INDEX					2
//S9  
#define S9_ICON1_SEG_INDEX					46
#define S9_ICON1_COM_INDEX					3
//S10  
#define S10_ICON1_SEG_INDEX					44
#define S10_ICON1_COM_INDEX					3
//S11  
#define S11_ICON1_SEG_INDEX					42
#define S11_ICON1_COM_INDEX					3
//S12 
#define S12_ICON1_SEG_INDEX					48
#define S12_ICON1_COM_INDEX					3
//S13   
#define S13_ICON1_SEG_INDEX					33
#define S13_ICON1_COM_INDEX					3
//S14  
#define S14_ICON1_SEG_INDEX					41
#define S14_ICON1_COM_INDEX					3
//S15  
#define S15_ICON1_SEG_INDEX					37
#define S15_ICON1_COM_INDEX					3
//S16  
#define S16_ICON1_SEG_INDEX					34
#define S16_ICON1_COM_INDEX					3
//S17  
#define S17_ICON1_SEG_INDEX					13
#define S17_ICON1_COM_INDEX					0
//S18  
#define S18_ICON1_SEG_INDEX					33
#define S18_ICON1_COM_INDEX					0
//S19  
#define S19_ICON1_SEG_INDEX					26
#define S19_ICON1_COM_INDEX					3
//S20 
#define S20_ICON1_SEG_INDEX					21
#define S20_ICON1_COM_INDEX					0
//S21  
#define S21_ICON1_SEG_INDEX					22
#define S21_ICON1_COM_INDEX					0 
//S22  
#define S22_ICON1_SEG_INDEX					20
#define S22_ICON1_COM_INDEX					2
//S23  
#define S23_ICON1_SEG_INDEX					15
#define S23_ICON1_COM_INDEX					0
//S24  
#define S24_ICON1_SEG_INDEX					17
#define S24_ICON1_COM_INDEX					0 
//S25  
#define S25_ICON1_SEG_INDEX					20
#define S25_ICON1_COM_INDEX					1
//S26  
#define S26_ICON1_SEG_INDEX					20
#define S26_ICON1_COM_INDEX					0
//S27  
#define S27_ICON1_SEG_INDEX					19
#define S27_ICON1_COM_INDEX					0 
//S28  
#define S28_ICON1_SEG_INDEX					11
#define S28_ICON1_COM_INDEX					3
//S29  
#define S29_ICON1_SEG_INDEX					11
#define S29_ICON1_COM_INDEX					2
//S30  
#define S30_ICON1_SEG_INDEX					11
#define S30_ICON1_COM_INDEX					1 
//S31 
#define S31_ICON1_SEG_INDEX					11
#define S31_ICON1_COM_INDEX					0
//S32  
#define S32_ICON1_SEG_INDEX					6
#define S32_ICON1_COM_INDEX					0
//S33  
#define S33_ICON1_SEG_INDEX					6
#define S33_ICON1_COM_INDEX					1 
//S34  
#define S34_ICON1_SEG_INDEX					6
#define S34_ICON1_COM_INDEX					2
//S35  
#define S35_ICON1_SEG_INDEX					6
#define S35_ICON1_COM_INDEX					3
//S36 
#define S36_ICON1_SEG_INDEX					5
#define S36_ICON1_COM_INDEX					1 
//S37  
#define S37_ICON1_SEG_INDEX					5
#define S37_ICON1_COM_INDEX					2
//S38  
#define S38_ICON1_SEG_INDEX					5
#define S38_ICON1_COM_INDEX					3
//S39  
#define S39_ICON1_SEG_INDEX					8
#define S39_ICON1_COM_INDEX					3 
//S40  
#define S40_ICON1_SEG_INDEX					10
#define S40_ICON1_COM_INDEX					3
//S41  
#define S41_ICON1_SEG_INDEX					28
#define S41_ICON1_COM_INDEX					0
//S42 
#define S42_ICON1_SEG_INDEX					31
#define S42_ICON1_COM_INDEX					3
//S43  
#define S43_ICON1_SEG_INDEX					29
#define S43_ICON1_COM_INDEX					0
 

//T1- 
#define T1_ICON1_SEG_INDEX					33
#define T1_ICON1_COM_INDEX					1
//T2-  
#define T2_ICON1_SEG_INDEX					33
#define T2_ICON1_COM_INDEX					2
//T3-  
#define T3_ICON1_SEG_INDEX					2
#define T3_ICON1_COM_INDEX					3
//T4-  
#define T4_ICON1_SEG_INDEX					2
#define T4_ICON1_COM_INDEX					2
//T5-  
#define T5_ICON1_SEG_INDEX					2
#define T5_ICON1_COM_INDEX					1
//T6-  
#define T6_ICON1_SEG_INDEX					2
#define T6_ICON1_COM_INDEX					0
//T7-  
#define T7_ICON1_SEG_INDEX					3
#define T7_ICON1_COM_INDEX					0
//T8-  
#define T8_ICON1_SEG_INDEX					3
#define T8_ICON1_COM_INDEX					1
//T9-  
#define T9_ICON1_SEG_INDEX					3
#define T9_ICON1_COM_INDEX					2
//T10-  
#define T10_ICON1_SEG_INDEX					3
#define T10_ICON1_COM_INDEX					3
//T11-  
#define T11_ICON1_SEG_INDEX					4
#define T11_ICON1_COM_INDEX					3
//T12- 
#define T12_ICON1_SEG_INDEX					4
#define T12_ICON1_COM_INDEX					2
//T13-   
#define T13_ICON1_SEG_INDEX					4
#define T13_ICON1_COM_INDEX					1
//T14-  
#define T14_ICON1_SEG_INDEX					4
#define T14_ICON1_COM_INDEX					0
//T15-  
#define T15_ICON1_SEG_INDEX					28
#define T15_ICON1_COM_INDEX					1
//T16-  
#define T16_ICON1_SEG_INDEX					28
#define T16_ICON1_COM_INDEX					2
//T17-  
#define T17_ICON1_SEG_INDEX					28
#define T17_ICON1_COM_INDEX					3
//T18-  
#define T18_ICON1_SEG_INDEX					21
#define T18_ICON1_COM_INDEX					3
//T19-  
#define T19_ICON1_SEG_INDEX					21
#define T19_ICON1_COM_INDEX					2
//T20- 
#define T20_ICON1_SEG_INDEX					21
#define T20_ICON1_COM_INDEX					1


//P1 
#define P1_ICON1_SEG_INDEX					39
#define P1_ICON1_COM_INDEX					3
//P2
#define P2_ICON1_SEG_INDEX					24
#define P2_ICON1_COM_INDEX					0

 
 
extern I2cObj_T tLCD_HT1621_IIC;
 
extern unsigned char gsDisplayBuff[25];

 
void HT1621_IfaceInit(void);
void Ht1621Wr_Comd(unsigned char command,unsigned char ComData);
 
void FillLcdFromBuff(void);
void HT1621DispIcon(unsigned char mSegIndex,unsigned char mComIndex);
void HT1621DispIcon2(unsigned char mSegIndex,unsigned char mComIndex, bool sw);
void DisplayNum(signed short int mStartX,unsigned char mNum);
void FillLcdAll1(unsigned char mFillData);

#endif
