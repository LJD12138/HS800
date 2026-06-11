#ifndef NTC_H
#define NTC_H

#include "main.h"
#include <math.h>

#define 		ntc10K_B3950_RES_TABLE_SIZE     		161
#define 		ntc10K_B3950_INDEX_ZERO_TEMP          	-40
extern const u32 ula10K_B3950_RES_TABLE[ntc10K_B3950_RES_TABLE_SIZE];

#define 		ntc100K_B3950_RES_TABLE_SIZE     		161
#define 		ntc100K_B3950_INDEX_ZERO_TEMP          	-40
extern const u32 ula100K_B3950_RES_TABLE[ntc100K_B3950_RES_TABLE_SIZE];

typedef struct
{
    double         		sys_vol;     		/* 电压 */
    u16   				volt_res;    		/* ntc分压电阻*/
    u16   				ntc_res;     		/* ntc额定电阻 */
    u16   				hex_x;      		/* ADC分辨率 -12Bit_4096 10Bit_1025 8Bit_256 */
    u16   				b_x;         		/* B值*/
}ntc_val_t;

extern void vNtc_Init(ntc_val_t *val, double sys_vol, u16 volt_res, u16 ntc_res, u16 hex_x, u16 b_x);
extern s16 sNtc_GetTempByRes(const u32 *buff, const s16 zero_index_temp, const u16 len, const u32 res);
extern double fNtc_CulcTempByAD(ntc_val_t *val,u16 adc_val);
#endif

