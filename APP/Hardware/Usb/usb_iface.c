#include "Usb/usb_iface.h"

#if(boardUSB_EN)
I2cObj_T  		tUSB_IC1_I2C;
I2cObj_T  		tUSB_IC2_I2C;

/*****************************************************************************************************************
-----函数功能    DC相关IO初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
******************************************************************************************************************/

static void v_usb_gpio_init(void)
{
    rcu_periph_clock_enable(usbPD_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(usbPD_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, usbPD_EN_PIN);
	gpio_output_options_set(usbPD_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, usbPD_EN_PIN);
	#else
    gpio_init(usbPD_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, usbPD_EN_PIN);
	#endif
    usbPD_EN_OFF();

    rcu_periph_clock_enable(usbPD2_EN_RCU);
	#if (boardIC_TYPE == boardIC_GD32F50X)
	gpio_mode_set(usbPD2_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, usbPD2_EN_PIN);
	gpio_output_options_set(usbPD2_EN_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL0, usbPD2_EN_PIN);
	#else
    gpio_init(usbPD2_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, usbPD2_EN_PIN);
	#endif
    usbPD2_EN_OFF();
	
	// rcu_periph_clock_enable(usbPOWER_EN_RCU);
	// gpio_init(usbPOWER_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ,usbPOWER_EN_PIN);
    // usbPOWER_EN_OFF();

    // rcu_periph_clock_enable(usbA_EN_RCU);
	// gpio_init(usbA_EN_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ,usbA_EN_PIN);
    // usbA_EN_OFF();

	//PD100W   IIC初始化
	rcu_periph_clock_enable(usbIC1_SCL_RCU);
	rcu_periph_clock_enable(usbIC1_SDA_RCU);
	tUSB_IC1_I2C.ulGPIO_PORT_SCL = usbIC1_SCL_PORT;
    tUSB_IC1_I2C.ulGPIO_PIN_SCL  = usbIC1_SCL_PIN;
    tUSB_IC1_I2C.ulGPIO_PORT_SDA = usbIC1_SDA_PORT;
    tUSB_IC1_I2C.ulGPIO_PIN_SDA  = usbIC1_SDA_PIN;
    tUSB_IC1_I2C.Addr = 0x3C;
    tUSB_IC1_I2C.AddrType = AddrType_7bit;
    tUSB_IC1_I2C.usDelay = 150;
    vI2C_ObjInit(&tUSB_IC1_I2C);
	
	// USB充电 IIC初始化
	rcu_periph_clock_enable(usbIC2_SCL_RCU);
	rcu_periph_clock_enable(usbIC2_SDA_RCU);
	tUSB_IC2_I2C.ulGPIO_PORT_SCL = usbIC2_SCL_PORT;
    tUSB_IC2_I2C.ulGPIO_PIN_SCL  = usbIC2_SCL_PIN;
    tUSB_IC2_I2C.ulGPIO_PORT_SDA = usbIC2_SDA_PORT;
    tUSB_IC2_I2C.ulGPIO_PIN_SDA  = usbIC2_SDA_PIN;
    tUSB_IC2_I2C.Addr = 0x3C;
    tUSB_IC2_I2C.AddrType = AddrType_7bit;
    tUSB_IC2_I2C.usDelay = 150;
    vI2C_ObjInit(&tUSB_IC2_I2C);
}


/***********************************************************************************************************************
-----函数功能    LED初始化
-----说明(备注)  none
-----传入参数    none
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vUsb_IfaceInit(void)
{
	v_usb_gpio_init();
}

#endif  //boardUSB_EN

















