/*****************************************************************************************************************
*                                            IIC接口                                                             *
 *                                                                                                              *
*备注:不要在接口里面使用信号量,避免出现信号量嵌套导致锁死                                                           *
******************************************************************************************************************/
#include "i2c.h"
#include "check.h"

#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif

#define  	ACK     					1
#define  	NACK    					0

#if(boardIC_TYPE == boardIC_GD32F30X)
#define   		I2C_LOW(GPIO, PIN)   					GPIO_BC(GPIO) = (uint32_t)PIN  
#define   		I2C_HIGH(GPIO, PIN)  					GPIO_BOP(GPIO) = (uint32_t)PIN
#elif(boardIC_TYPE == boardIC_STM32H7XX)
#define   		I2C_LOW(GPIO, PIN)   					(HAL_GPIO_WritePin(GPIO, PIN, GPIO_PIN_RESET))
#define   		I2C_HIGH(GPIO, PIN)  					(HAL_GPIO_WritePin(GPIO, PIN, GPIO_PIN_SET))

static GPIO_InitTypeDef 		gpio_init_struct;
#elif(boardIC_TYPE == boardIC_STM32G4XX)
#endif


static void v_delay(u16 X)      //延时，可以降低 IIC的速度
{ 
	while(X--);
}


static void v_delay_low(u16 X)      //半延时
{ 
	X >>= 1; 
	while(X--);
}

/*****************************************************************************************************************
-----函数功能    设置scl为输出
-----说明(备注)  数据接收用的
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_scl_gpio_output(const I2cObj_T *p_i2c_obj)
{ 
	#if(boardIC_TYPE == boardIC_GD32F30X)
	gpio_init(p_i2c_obj->ulGPIO_PORT_SCL, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SCL);
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	gpio_init_struct.Pin = p_i2c_obj->ulGPIO_PIN_SCL;               /* PIN口 */
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_NOPULL;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;           /* 低速 */
	HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SCL, &gpio_init_struct);
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#Error(未定义)
	#endif
    
}

/*****************************************************************************************************************
-----函数功能    设置sda为输出
-----说明(备注)  数据接收用的
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_sda_gpio_output(const I2cObj_T *p_i2c_obj)
{  
	#if(boardIC_TYPE == boardIC_GD32F30X)
	gpio_init(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SDA);
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	gpio_init_struct.Pin = p_i2c_obj->ulGPIO_PIN_SDA;               /* PIN口 */
	gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;             /* 推完输出 */
	gpio_init_struct.Pull = GPIO_NOPULL;                     /* 不上下拉 */
	gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;           /* 低速 */
	HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SDA, &gpio_init_struct);
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#Error(未定义)
	#endif
}

/*****************************************************************************************************************
-----函数功能    设置sda为输入
-----说明(备注)  数据接收用的
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_sda_gpio_input(const I2cObj_T *p_i2c_obj)
{
	#if(boardIC_TYPE == boardIC_GD32F30X)
	 gpio_init(p_i2c_obj->ulGPIO_PORT_SDA, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, p_i2c_obj->ulGPIO_PIN_SDA);
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	gpio_init_struct.Pin = p_i2c_obj->ulGPIO_PIN_SDA;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(p_i2c_obj->ulGPIO_PORT_SDA, &gpio_init_struct);
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#Error(未定义)
	#endif
}

/*****************************************************************************************************************
-----函数功能    读取sda电平信号
-----说明(备注)  数据接收用的
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      1 or 0
******************************************************************************************************************/
__STATIC_INLINE bool b_i2c_read_sda_gpio(const I2cObj_T *p_i2c_obj)   
{
	#if(boardIC_TYPE == boardIC_GD32F30X)
	if((uint32_t)RESET != (GPIO_ISTAT(p_i2c_obj->ulGPIO_PORT_SDA)&(p_i2c_obj->ulGPIO_PIN_SDA)))
	#elif(boardIC_TYPE == boardIC_STM32H7XX)
	if((HAL_GPIO_ReadPin(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA)) != GPIO_PIN_RESET)
	#elif(boardIC_TYPE == boardIC_STM32G4XX)
	#Error(未定义)
	#endif
		return true;
	else
		return false;
}


/*****************************************************************************************************************
-----函数功能    发送开始信号
-----说明(备注)  none
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_start(const I2cObj_T *p_i2c_obj)
{
	v_i2c_set_sda_gpio_output(p_i2c_obj);	// SDA线输出，完成后会变高
	
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA); 
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay       (p_i2c_obj->usDelay);
 	I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA); 		//START:when CLK is high,DATA change form high to low 
	v_delay       (p_i2c_obj->usDelay);
	I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);		//钳住I2C总线，准备发送或接收数据
}	



/*****************************************************************************************************************
-----函数功能    发送停止信号
-----说明(备注)  ********重点BUG*********
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_stop(const I2cObj_T *p_i2c_obj)
{
	v_i2c_set_sda_gpio_output(p_i2c_obj);	// SDA线输出，完成后会变高
	
	I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay       (p_i2c_obj->usDelay);
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	v_delay       (p_i2c_obj->usDelay);
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}


/*****************************************************************************************************************
-----函数功能    Master设置Slave产生应答信号(产生 ACK 信号)
-----说明(备注)  此时SDA处于 接收状态
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_ack(const I2cObj_T *p_i2c_obj)
{
	v_i2c_set_sda_gpio_output  (p_i2c_obj);
	
	I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);    
	I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA); 		// SDA = 0----上一句无延时 且 最后一个位收到0，主机应答这里会有一个冲击（慢速时候）
	v_delay_low (p_i2c_obj->usDelay); 
	I2C_HIGH    (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);		//然后 SCL来个上升沿
	v_delay_low (p_i2c_obj->usDelay); 
	I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);	    //此句后无延时 且 收下一字节首位为0，会有一个冲击
}


/*****************************************************************************************************************
-----函数功能    Master设置Slave不产生应答信号(产生 NACK 信号)
-----说明(备注)  此时SDA处于 接收状态
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
__STATIC_INLINE void v_i2c_set_no_ack(const I2cObj_T *p_i2c_obj)
{
	v_i2c_set_sda_gpio_output(p_i2c_obj);	                                // SDA线输出，完成后会变高
	
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);		// 20180828 和后一句调换位置
	v_delay_low(p_i2c_obj->usDelay); 
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);		//然后 SCL来个上升沿
	v_delay_low(p_i2c_obj->usDelay); 
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
}


/*****************************************************************************************************************
-----函数功能    等待回复应答信号
-----说明(备注)  none
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      0:没错误   小于0:错误
******************************************************************************************************************/  
__STATIC_INLINE s8 c_i2c_wait_ack(const I2cObj_T *p_i2c_obj)
{
	vu32 ucErrTime = 0;
	
	v_i2c_set_sda_gpio_input(p_i2c_obj);  // SDA为输入
	
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  //发送 1 Byte 结束后已经把SDA置 1 这里可以
	v_delay_low(p_i2c_obj->usDelay);      	  
	I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
	v_delay_low(p_i2c_obj->usDelay); 
	
	while(b_i2c_read_sda_gpio(p_i2c_obj))	//
	{
		ucErrTime++;
		if(ucErrTime > 10000)
		{
			v_i2c_set_stop(p_i2c_obj);
			return -1;
		}
	}
	
	I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  //时钟输出0
	v_delay_low(p_i2c_obj->usDelay); 	
	
	return 1; 
} 					 	

/*****************************************************************************************************************
-----函数功能    写一个字节
-----说明(备注)  none
-----传入参数    p_i2c_obj:发送对象
				 txd:数据
-----输出参数    none
-----返回值      0:没错误   小于0:错误
******************************************************************************************************************/  
__STATIC_INLINE void v_i2c_write_byte(const I2cObj_T *p_i2c_obj, u8 txd)   //发送一个字节后，应该等到从机应答，
{                        
    u8 t; 
	
	#if(boardUSE_OS)
    taskENTER_CRITICAL();
	#endif
	{
		v_i2c_set_sda_gpio_output(p_i2c_obj);  // SDA线输出，完成后会变高  
		
		I2C_LOW     (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  //拉低时钟开始数据传输
		v_delay_low (p_i2c_obj->usDelay);
		for(t = 0; t < 8; t++)
		{                      
			if((txd & 0x80) == 0x80)	//从高位开始，把数据放在 SDA上
				I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
			else
				I2C_LOW(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
			txd <<= 1; 	  
			v_delay_low(p_i2c_obj->usDelay);
			I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  //SCL变高上升沿，提醒从机，SDA数据有效
			v_delay_low(p_i2c_obj->usDelay);  //延时，给从机时间读取数据
			I2C_LOW (p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  //SCL变低，再次可以改变SDA数据      
		}
	}
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
} 


/*****************************************************************************************************************
-----函数功能    读取一个字节
-----说明(备注)  读1个字节，ack=1时，发送ACK，ack=0，发送nACK
-----传入参数    p_i2c_obj:发送对象
				 ack:应答
-----输出参数    none
-----返回值      0:没错误   小于0:错误
******************************************************************************************************************/
__STATIC_INLINE u8 uc_i2c_read_byte(const I2cObj_T *p_i2c_obj, u8 ack)
{
	uint8_t i, receive=0;
	
	#if(boardUSE_OS)
	taskENTER_CRITICAL();
	#endif
	{
		v_i2c_set_sda_gpio_input(p_i2c_obj);  // SDA为输入，配置完后会变低
		
		for(i = 0; i < 8; i++)
		{
			I2C_LOW(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  // SCL = 0 让从机改变 SDA上的数据
			v_delay_low(p_i2c_obj->usDelay);			
			I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);  // SCL = 1 提示从机，主机要读数据，不能改变数据
			v_delay_low(p_i2c_obj->usDelay);
			receive <<= 1;  // 右移 1 位
			if(b_i2c_read_sda_gpio(p_i2c_obj))	//不能写 == 1，应该按库函数的规则写
				receive++;  // 就把最低位 变成 1
			v_delay_low(p_i2c_obj->usDelay);
		}			
		//******读取 8 bit 完成时，SCL = 1   SDA = x	
		if (ack == ACK)
			v_i2c_set_ack(p_i2c_obj); //发送ACK 
		else
			v_i2c_set_no_ack(p_i2c_obj);//发送nACK
	}
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
    return receive;
}


//**************************************************************************对外接口***********************************

/*****************************************************************************************************************
-----函数功能    i2c对象初始化
-----说明(备注)  none
-----传入参数    p_i2c_obj:发送对象
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void vI2C_ObjInit(I2cObj_T *p_i2c_obj)
{
    if(p_i2c_obj->AddrType == AddrType_7bit)
        p_i2c_obj->Addr = (p_i2c_obj->Addr << 1) & 0xFE;
	
	v_i2c_set_scl_gpio_output(p_i2c_obj);
	v_i2c_set_sda_gpio_output(p_i2c_obj);
	
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SCL, p_i2c_obj->ulGPIO_PIN_SCL);
    I2C_HIGH(p_i2c_obj->ulGPIO_PORT_SDA, p_i2c_obj->ulGPIO_PIN_SDA);
}



/*****************************************************************************************************************
-----函数功能    连续写数据(没有寄存器地址)
-----说明(备注)  适合TEA5767 等 ，没有寄存器，直接写入5个字节的器件
-----传入参数    p_i2c_obj:发送对象
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_WriteData(const I2cObj_T *p_i2c_obj, const u8 *buf, u16 len)    
{
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0 ) );
	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -1;  //应答超时
	}
	
	//发送数据
	for(int i = 0; i < len; i++)
	{
		v_i2c_write_byte(p_i2c_obj, buf[i] );  
		
		//等待ACK  等到应答超时则放弃后面的数据
		if(c_i2c_wait_ack(p_i2c_obj) < 0)					
		{
			return -2;		 
		}		
	}
	
	//发送停止，提示从机，数据发送完毕
    v_i2c_set_stop(p_i2c_obj);	
	
	return 1;
}


/*****************************************************************************************************************
-----函数功能    连续读数据(没有寄存器地址)
-----说明(备注)  适合TEA5767 等 ，没有寄存器，直接读入5个字节的器件
-----传入参数    p_i2c_obj:发送对象
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_ReadData(const I2cObj_T *p_i2c_obj, u8 *buf, u16 len)  
{
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址 + 读命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 1));
	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -1;  //应答超时
	}
	
	while(len)
	{
		if(len == 1)
			*buf = uc_i2c_read_byte(p_i2c_obj, NACK);//读数据,发送nACK 
		else 
			*buf = uc_i2c_read_byte(p_i2c_obj, ACK);//读数据,发送ACK
		
		len --;
		buf ++; 
	}
	
	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}



/*****************************************************************************************************************
-----函数功能    写数据(有寄存器地址)
-----说明(备注)  适合有寄存器的器件，EEPROM，传感器
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_WriteBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *buf, u8 len)
{
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -1;
	}
	
	//写寄存器地址
	v_i2c_write_byte(p_i2c_obj, reg_addr);	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -2;
	}
	
	for(int i = 0; i < len; i++)
	{
		//发送数据
		v_i2c_write_byte(p_i2c_obj, buf[ i ] );	
		
		//等待ACK  等到应答超时则放弃后面的数据
		if(c_i2c_wait_ack(p_i2c_obj) < 0)				
		{
			return -3;		 
		}		
	}  

	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    读数据(有寄存器地址)
-----说明(备注)  适合有寄存器的器件，EEPROM，传感器
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_ReadBytes(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *buf, u8 len)  
{
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0 ) );
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -1;		
	}
	
	//写寄存器地址
	v_i2c_write_byte(p_i2c_obj, reg_addr);
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -2;		
	}	
	
	v_i2c_set_start(p_i2c_obj);

	//发送器件地址 + 读命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr ) | 1 );
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -3;		
	}	
	
	while(len)
	{
		if(len == 1)
			*buf = uc_i2c_read_byte(p_i2c_obj, NACK);//读数据,发送nACK 
		else 
			*buf = uc_i2c_read_byte(p_i2c_obj, ACK);//读数据,发送ACK
		
		len --;
		buf ++; 
	}
	
	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}



/*****************************************************************************************************************
-----函数功能    写数据(有寄存器地址)
-----说明(备注)  适合有寄存器的器件，EEPROM，传感器
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_WriteBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, const u8 *buf, u8 len)
{
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -1;
	}
	
	//写寄存器高地址
	v_i2c_write_byte(p_i2c_obj, reg_addr>>8);	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -2;
	}
	//写寄存器低地址
	v_i2c_write_byte(p_i2c_obj, reg_addr%256);	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -2;
	}
	
	for(int i = 0; i < len; i++)
	{
		//发送数据
		v_i2c_write_byte(p_i2c_obj, buf[ i ] );	
		
		//等待ACK  等到应答超时则放弃后面的数据
		if(c_i2c_wait_ack(p_i2c_obj) < 0)				
		{
			return -3;		 
		}		
	}  

	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    读数据(有寄存器地址)
-----说明(备注)  适合有寄存器的器件，EEPROM，传感器
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_ReadBytes1(const I2cObj_T *p_i2c_obj, u16 reg_addr, u8 *buf, u8 len)  
{
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0 ) );
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -1;		
	}
	
	//写寄存器高地址
	v_i2c_write_byte(p_i2c_obj, reg_addr>>8);
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -2;		
	}
	//写寄存器低地址
	v_i2c_write_byte(p_i2c_obj, reg_addr%256);
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -2;		
	}	
	
	v_i2c_set_start(p_i2c_obj);

	//发送器件地址 + 读命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr ) | 1 );
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -3;		
	}	
	
	while(len)
	{
		if(len == 1)
			*buf = uc_i2c_read_byte(p_i2c_obj, NACK);//读数据,发送nACK 
		else 
			*buf = uc_i2c_read_byte(p_i2c_obj, ACK);//读数据,发送ACK
		
		len --;
		buf ++; 
	}
	
	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}


/*****************************************************************************************************************
-----函数功能    写数据(带寄存器地址和校验)
-----说明(备注)  AFE
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_WriteBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *buf, u8 len)
{
	u8 buff[p_i2c_obj->ucBuffLen];
	vu8 uc_crc_data = 0;
	
	//BUFF长度不够
	if(p_i2c_obj->ucBuffLen <= 2)
	{
		return -1;
	}
	
	//数据长度不够
	if(p_i2c_obj->ucBuffLen < (len+2))
	{
		return -2;
	}
	
	//组帧
	buff[0] = p_i2c_obj->Addr;
	buff[1] = reg_addr;
	memcpy(&buff[2], buf, len);
	
	//校验
	uc_crc_data = ucCheck_CRC8cal(buff, len+2);
	
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送识别ID
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -3;
	}
	
	//写寄存器地址
	v_i2c_write_byte(p_i2c_obj, reg_addr);	
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -4;
	}
	
	//发送数据
	for(int i = 0; i < len; i++)
	{
		v_i2c_write_byte(p_i2c_obj, buf[i] );	
		//等待ACK  等到应答超时则放弃后面的数据
		if(c_i2c_wait_ack(p_i2c_obj) < 0)				
		{
			return -5;		 
		}		
	}

	//发送校验
	v_i2c_write_byte(p_i2c_obj, uc_crc_data);	
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -6;
	}

	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    读数据(带寄存器地址和校验)
-----说明(备注)  AFE
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_ReadBytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *buf, u8 len)  
{
	u8 buff[p_i2c_obj->ucBuffLen];
	vu8 uc_crc_data = 0;
	
	//BUFF长度不够
	if(p_i2c_obj->ucBuffLen <= 4)
	{
		return -1;
	}
	
	//数据长度不够
	if(p_i2c_obj->ucBuffLen < (len+4))
	{
		return -2;
	}
	
	//组帧
	buff[0] = p_i2c_obj->Addr;
	buff[1] = reg_addr;
	buff[2] = len;
	buff[3] = p_i2c_obj->Addr | 0x01;
	
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0 ) );
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -3;		
	}
	
	//读寄存器地址
	v_i2c_write_byte(p_i2c_obj, reg_addr);	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -4;		
	}
	
	//读寄存器长度
	v_i2c_write_byte(p_i2c_obj, len);	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -5;		
	}
	
	v_i2c_set_start(p_i2c_obj);

	//发送器件地址 + 读命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr ) | 1 );
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -6;		
	}	
	
	for(int i = 0; i < len; i++)
	{
		buff[4+i] = uc_i2c_read_byte(p_i2c_obj, ACK);//读数据,发送ACK
	}
	
	//接收校验数据并发送NACK
	uc_crc_data = uc_i2c_read_byte(p_i2c_obj, NACK);//读数据,发送nACK 
	
	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	if(uc_crc_data == ucCheck_CRC8cal(buff, 4+len))
	{
		for(int i=0;i<len;i++)
		{
			buf[i]=buff[4+i];
		}
	}
	else
	{
		return -7;
	}
	
	return 1;
}


/*****************************************************************************************************************
-----函数功能    读数据(带寄存器地址和校验)
-----说明(备注)  读取两个字节
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_Read2BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, u8 *buf)  
{
	u8 buff[p_i2c_obj->ucBuffLen];
	vu8 uc_crc_data = 0;
	
	//BUFF长度不够
	if(p_i2c_obj->ucBuffLen <= 4)
	{
		return -1;
	}
	
	//数据长度不够
	if(p_i2c_obj->ucBuffLen < (2+4))
	{
		return -2;
	}
	
	//组帧
	buff[0] = p_i2c_obj->Addr;
	buff[1] = reg_addr;
	buff[2] = p_i2c_obj->Addr | 0x01;
	
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送器件地址+写命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0 ) );
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -3;		
	}
	
	//读寄存器地址
	v_i2c_write_byte(p_i2c_obj, reg_addr);	
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -4;		
	}
	
	v_i2c_set_start(p_i2c_obj);

	//发送器件地址 + 读命令
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr ) | 1 );
	//等待应答
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -6;		
	}	
	
	for(int i = 0; i < 2; i++)
	{
		buff[3+i] = uc_i2c_read_byte(p_i2c_obj, ACK);//读数据,发送ACK
	}
	
	//接收校验数据并发送NACK
	uc_crc_data = uc_i2c_read_byte(p_i2c_obj, NACK);//读数据,发送nACK 
	
	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	if(uc_crc_data == ucCheck_CRC8cal(buff, 5))
	{
		for(int i=0;i<2;i++)
		{
			buf[i]=buff[3+i];
		}
	}
	else
	{
		return -7;
	}
	
	return 1;
}

/*****************************************************************************************************************
-----函数功能    写数据(带寄存器地址和校验)
-----说明(备注)  AFE
-----传入参数    p_i2c_obj:发送对象
				 reg_addr:寄存器地址
				 buf:数据的指针
				 len:数据的字节2长度
-----输出参数    none
-----返回值      小于0:操作失败   等于0:等待    大于0:写入成功
******************************************************************************************************************/
s8 cI2C_Write1BytesCrc(const I2cObj_T *p_i2c_obj, u8 reg_addr, const u8 *buf)
{
	u8 buff[p_i2c_obj->ucBuffLen];
	vu8 uc_crc_data = 0;
	
	const u8 len = 1;
	
	//BUFF长度不够
	if(p_i2c_obj->ucBuffLen <= 2)
	{
		return -1;
	}
	
	//数据长度不够
	if(p_i2c_obj->ucBuffLen < (len+2))
	{
		return -2;
	}
	
	//组帧
	buff[0] = p_i2c_obj->Addr;
	buff[1] = reg_addr;
	memcpy(&buff[2], buf, len);
	
	//校验
	uc_crc_data = ucCheck_CRC8cal(buff, len+2);
	
	//开始信号
	v_i2c_set_start(p_i2c_obj);
	
	//发送识别ID
	v_i2c_write_byte(p_i2c_obj, (p_i2c_obj->Addr | 0));
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -3;
	}
	
	//写寄存器地址
	v_i2c_write_byte(p_i2c_obj, reg_addr);	
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -4;
	}
	
	//发送数据
	for(int i = 0; i < len; i++)
	{
		v_i2c_write_byte(p_i2c_obj, buf[i] );	
		//等待ACK  等到应答超时则放弃后面的数据
		if(c_i2c_wait_ack(p_i2c_obj) < 0)				
		{
			return -5;		 
		}		
	}

	//发送校验
	v_i2c_write_byte(p_i2c_obj, uc_crc_data);	
	//等待应答,应答超时，放弃写数据
	if(c_i2c_wait_ack(p_i2c_obj) < 0)	
	{
		return -6;
	}

	//发送停止，提示从机，数据发送完毕
	v_i2c_set_stop(p_i2c_obj);
	
	return 1;
}



