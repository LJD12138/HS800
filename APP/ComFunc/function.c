#include "function.h"
#include <math.h>
#include <ctype.h>

// 将16进制数组转换为ASCII码数组
void hex2ascii(unsigned char* hex, char* ascii, int len) {
    for (int i = 0; i < len; i++) {
        sprintf(&ascii[i * 2], "%02X", hex[i]);
    }
}

/*****************************************************************************************************************
-----函数功能    Hex字符转为十进制
-----说明(备注)  将16进制的字符转换为十进制数字,把字符F转化为15
-----传入参数    ch:字符
-----输出参数    none
-----返回值      字符对应的hex
******************************************************************************************************************/
int sFunc_HexStrToHex(char ch) 
{
    if (isdigit(ch)) {
        return (ch - '0');
    } else {
        ch = toupper(ch);
        return (ch - 'A' + 10);
    }
}

/*****************************************************************************************************************
-----函数功能    把两个Hex字符分别转为Hex并拼接成一个字节
-----说明(备注)  将16进制字符数组转换为16进制数组,把字符EF转化为0xEF
-----传入参数    
				 hexstr:需要被转化的字符串
				 hex:转化后hex数据
				 len:转化后hex数据的长度
-----输出参数    none
-----返回值      none
******************************************************************************************************************/
void sFunc_2HexStrTo1Hex(char* hexstr, unsigned char* hex, int len) 
{
    for (int i = 0; i < len; i++) {
        hex[i] = (sFunc_HexStrToHex(hexstr[i * 2]) << 4) | sFunc_HexStrToHex(hexstr[i * 2 + 1]);
    }
}



bool isGBK(u8 val) {
    // 判断一个整数是否是GBK内码  //|| (val >= 0x40 && val <= 0xfe && val != 0x7f)
    if ((val >= 0x81 && val <= 0xfe) ) 
	{
        return true;
    }
    return false;
}


/***********************************************************************************************************************
-----函数功能    数据比对
-----说明(备注)  none
-----传入参数    
				 src: source data pointer
				 dst: destination data pointer
				 length: the compare data length
-----输出参数    none
-----返回值      true:比对一致    false:失败
************************************************************************************************************************/
bool bFun_DataCompare(uint8_t *src, uint8_t *dst, uint8_t length)
{
    while(length--) {
        if(*src++ != *dst++) {
            return false;
        }
    }
    return true;
}


/***********************************************************************************************************************
-----函数功能    数据比对
-----说明(备注)  none
-----传入参数    
				 src: 源数据 source
				 dst: 目标数据 destination
				 length: 字节长度
                 end_symbol:结束符号
-----输出参数    none
-----返回值      true:比对一致    false:失败
************************************************************************************************************************/
bool bFun_DataCompare1(uint8_t *src, uint8_t *dst, uint8_t length, uint8_t end_symbol)
{
    while(length--) {
        if(*src++ != *dst++) {
            return false;
        }
    }
    return true;
}


//
void Ui16ToUin8_P(uint8_t *bdata,uint16_t adata)
{
    //把一个16位数复制给一个8位的指针变量，这个8位指针变量的两个连续地址用
    //于存放16位数的高低位，输入16位，输出8位指针。
    *bdata = (uint8_t)(adata >>8 &0xFF); //前面(uint8_t)为强制类型转换
    *(bdata+1) = (uint8_t)(adata & 0xFF);
}

//数组为大端 高位在前,低位在后
void Uin8ToUin16_P(uint8_t *bdata,uint16_t adata)
{
    adata = *bdata;
	adata = (adata<<8)|*(bdata+1) ;
}

//数组为小端 低位在前,高位在后
void Uin8ToUin16_M(uint8_t *bdata,uint16_t *adata)
{
    *adata = *(bdata+1);
	*adata = (*adata<<8)|(*(bdata)) ;
}

//
void FloatToUin8_P(uint8_t *bdata,uint16_t adata)
{
	uint8_t num[4]={0};
	float dataf_temp=0;
	dataf_temp = (((float)adata)/1000);
    memcpy(num,&dataf_temp,4);
	for(int i=0;i<4;i++)
	{
		*bdata = num[3-i];
		bdata++;
	}
}

int sVulueTurn(int value)
{
	value = ~value + 1;
	return value;
}



bool bFloatEqualJudge( float x, float y, float EPSILON)
{
    /* 判断方法：
     *  1. 两个浮点数在二进制上各个数据位完全相同
     *  2. 小于某个极小的模糊因子，根据精度要求设置
     *  3. 转为整型用位异或判断数据位
     */
    if( x >= y 
     || fabs( x - y ) < EPSILON 
     || ( *( (uint32_t*)&x ) ^ *( (uint32_t*)&y ) ) == 1 )
    {
        return true;
    }
    else
    {
        return false;
    }
}

u16 hex_to_int(u8* hex ,u8 len) 
{
    u16 result16=0;
	u8 result=0;
    for(u8 i=0;i<len;i++)
	{
		if (hex[i] >= '0' && hex[i] <= '9') {
			result = hex[i] - '0';
		}
		else if (hex[i] >= 'a' && hex[i] <= 'f') {
			result = hex[i] - 'a' + 10;
		}
		else if (hex[i] >= 'A' && hex[i] <= 'F') {
			result = hex[i] - 'A' + 10;
		}
		result16 = result16 << 4;
		result16 = result16|result;
	}
    return result16;
}


/***********************************************************************************************************************
-----函数功能    对比数据是否存在
-----说明(备注)  none
-----传入参数    src:源数据指针
                 data:要对比的数据
-----输出参数    none
-----返回值      true:数据存在,反之不存在
************************************************************************************************************************/
bool bFunc_CompareDataIsExist(uint8_t *src, uint8_t data)
{
	u8 length = sizeof(*src);
	
    while(length--) 
	{
        if(*src++ == data) 
		{
            return true;
        }
    }
    return false;
}


/***********************************************************************************************************************
-----函数功能    字节序转换
-----说明(备注)  用于在大端（Big-Endian）和小端（Little-Endian）字节序之间进行转换的函数
-----传入参数    src:源数据指针
                 data:要对比的数据
-----输出参数    none
-----返回值      true:数据存在,反之不存在
************************************************************************************************************************/
uint16_t swap_bytes(uint16_t value) 
{ 
	return (value >> 8) | (value << 8); 
}



void bFunc_FindMinMax(u16 *arr, u16 n, u16 *max, u16 *min) {
    // 假设数组至少有一个元素，初始化最大值和最小值为数组的第一个元素
    *max = arr[0];
    *min = arr[0];

    // 遍历数组中的所有元素
    for (int i = 1; i < n; i++) {
        // 如果当前元素大于当前最大值，则更新最大值
        if (arr[i] > *max) {
            *max = arr[i];
        }
        // 如果当前元素小于当前最小值，则更新最小值
        if (arr[i] < *min) {
            *min = arr[i];
        }
    }
}

/*****************************************************************************************************************
-----函数功能    定位表格位置(二分法)
-----说明(备注)  none
-----传入参数    ch:字符
-----输出参数    none
-----返回值      字符对应的hex
******************************************************************************************************************/
u8 ucFunc_PositTableU16(const u16 *buff,u8 array_len,u16 dat)
{
	vu8 begin = 0;
	vu8 end = array_len -1;
	vu8 middle = 0;
	vu8 i = 0;
	
	if(dat>=buff[end]) 
		return end;//大于尾
	else if(dat<=buff[begin]) 
		return begin;//小于或等于头
	
	while(begin<end)
	{
		middle=(begin+end)/2;//从一半开始
		if(dat==buff[middle]) 
			break;//完成
		if(dat>buff[middle] && dat<buff[middle +1]) 
			break;//1度范围
		if(dat<buff[middle]) 
			end=middle;	//小于往上查询
		else 		    //大于往下查询
			begin=middle ;
		
		if(i++>array_len ) break ;//查询完毕没有
	}
	
	if(begin>end) 
		return 0;
	
	return middle ;	
}


/*****************************************************************************************************************
-----函数功能    定位表格位置(二分法)
-----说明(备注)  none
-----传入参数    ch:字符
-----输出参数    none
-----返回值      字符对应的hex
******************************************************************************************************************/
u16 usFunc_PositTableU32(const u32 *buff,u16 array_len,u32 dat)
{
	vu16 begin = 0;
	vu16 end = array_len -1;
	vu16 middle = 0;
	vu16 i = 0;
	
	//NTC 大到小
	if(buff[begin] > buff[end])
	{
		if(dat <= buff[end]) 
			return end;//小于尾
		else if(dat >= buff[begin]) 
			return begin;//大于头
		
		while(begin < end)
		{
			middle = (begin+end)/2;//从一半开始
			if(dat == buff[middle]) 
				break;//完成
			if(dat < buff[middle] && dat > buff[middle + 1]) 
				break;//1度范围
			if(dat > buff[middle]) 
				end = middle;	//小于往上查询
			else 		    //大于往下查询
				begin = middle ;
			
			if(i++ > array_len ) 
				break ;//查询完毕没有
		}
	}
	//PTC 小到大
	else
	{
		if(dat >= buff[end]) 
			return end;//大于尾
		else if(dat <= buff[begin]) 
			return begin;//小于或等于头
		
		while(begin<end)
		{
			middle = (begin + end) / 2;//从一半开始
			if(dat == buff[middle]) 
				break;//完成
			if(dat > buff[middle] && dat < buff[middle + 1]) 
				break;//1度范围
			if(dat < buff[middle]) 
				end = middle;	//小于往上查询
			else 		    //大于往下查询
				begin = middle ;
			
			if(i++ > array_len ) 
				break ;//查询完毕没有
		}
	}
	
	if(begin>end) 
			return 0;
	
	return middle ;	
}

/***********************************************************************************************************************
-----函数功能    字节序转换
-----说明(备注)  用于在大端（Big-Endian）和小端（Little-Endian）字节序之间进行转换的函数
-----传入参数    src:源数据指针
                 data:要对比的数据
-----输出参数    none
-----返回值      true:数据存在,反之不存在
************************************************************************************************************************/
u16 usFunc_SwapU16(u16 input) 
{ 
	return (input >> 8) | (input << 8); 
}
u32 ulFunc_SwapU32(u32 input)
{
    u16 l16 = input & 0xFFFF;
    u16 h16 = (input >> 16) & 0xFFFF;
    return (((u32)usFunc_SwapU16(l16) << 16) & 0xFFFF0000)  | usFunc_SwapU16(h16);
}
u64 u64Func_SwapU64(u64 input)
{
    u32 l32 = input & 0xFFFFFFFF;
    u32 h32 = (input >> 32) & 0xFFFFFFFF;
    return (((u64)ulFunc_SwapU32(l32) << 32) & 0xFFFFFFFF00000000)  | ulFunc_SwapU32(h32);
}


/***********************************************************************************************************************
-----函数功能    转换
-----说明(备注)  用于在大端（Big-Endian）和小端（Little-Endian）字节序之间进行转换的函数
-----传入参数    src: 源数据 source
				 dst: 目标数据 destination
				 size:寄存器的长度
-----输出参数    none
-----返回值      true:转换成功,反之失败
************************************************************************************************************************/
bool bFunc_SwapU16Array(u8 *dst, u8 *src, u16 reg_size) 
{
    for (u16 i = 0; i < reg_size; i++) 
	{
        dst[i*2] = src[(i*2)+1];
		dst[(i*2)+1] = src[i*2];
    }
	return true;
}

/***********************************************************************************************************************
-----函数功能    蝶式交换
-----说明(备注)  原始：b7 b6 b5 b4 b3 b2 b1 b0 -> 交换后：b6 b7 b4 b5 b2 b3 b0 b1
-----传入参数    x
-----输出参数    none
-----返回值      x
************************************************************************************************************************/
u8 ucFunc_ReverseBits(u8 x) 
{
    x = (x & 0xF0) >> 4 | (x & 0x0F) << 4;  // 交换高4位与低4位
    x = (x & 0xCC) >> 2 | (x & 0x33) << 2;  // 交换每4位中的高2位与低2位
    x = (x & 0xAA) >> 1 | (x & 0x55) << 1;  // 交换每2位中的高位与低位
    return x;
}


/***********************************************************************************************************************
-----函数功能	获取最大最小值
-----说明(备注)	none
-----传入参数	Num：需要用于比较的数据数量;
				Temp：指向存放数据的数组的指针；
				Min：指向存放最小数据的指针；
				Max：指向存放最大数据的指针。
-----输出参数	none
-----返回值		x
************************************************************************************************************************/
void vFunc_GetMaxMin(u8 Num, s16 *Array, s16 *Min, s16 *Max)
{
	*Min = Array[0];
	*Max = Array[0];
	for(int i=1; i<Num; i++)
	{
		if(Array[i] < *Min)
		{
			*Min = Array[i];
		}
		else if(Array[i] > *Max)
		{
			*Max = Array[i];
		}
	}
}

/***********************************************************************************************************************
-----函数功能	浮点绝对值
-----说明(备注)	none
-----传入参数	none
-----输出参数	none
-----返回值		x
************************************************************************************************************************/
float fFunc_Fabs(float num1, float num2)
{
	if(num1 > num2)
		return (num1 - num2);
	else
		return (num2 - num1);
}

/***********************************************************************************************************************
-----函数功能    m^n函数
-----说明(备注)  none
-----传入参数    m n
-----输出参数    none
-----返回值     	返回值:m^n次方.
************************************************************************************************************************/
u32 ulFunc_Pow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}

/***********************************************************************************************************************
*函数功能：线性回归算法求得斜率
*入口参数：x:存放自变量x的n个值的数组首地址.
*入口参数：y:存放与自变量x的n个值对应的随机变量观测值的数组首地址.
*入口参数：n:观测点数.
*出口参数：a:长度为2的数组,其中a[0]存放回归系数b,   a[1]存放回归系数a.
*出口参数：dt:长度为6的数组,dt[0]为偏差平方和, dt[1]为平均标准偏差, dt[2]为回归平方和,   
                         dt[3]为最大偏差,   dt[4]为最小偏差,     dt[5]为偏差平均值.
*说    明：斜率越趋近于0越平稳，大于0说明斜率向高走，小于0说明斜率向低走
************************************************************************************************************************/
void vFunc_GetCoefficient(const float *x, float *y, int n, float *a, float *dt)
{
    int  i;
    float  xx, yy, e, f, q, u, p, umax, umin, s;
    
    xx = 0.0f;   yy = 0.0f;
    for (i = 0; i <= n - 1; i++)
    {
        xx = xx + x[i] / n;
        yy = yy + y[i] / n;
    }
    e = 0.0f;   f = 0.0f;
    for (i = 0; i <= n - 1; i++)
    {
        q = x[i] - xx;   e = e + q   *   q;
        f = f + q   *   (y[i] - yy);
    }
    a[1] = f / e;   a[0] = yy - a[1] * xx;
    q = u = p = 0.0f;
    umax = 0.0f;   umin = 1.0e+5f;
    for (i = 0; i <= n - 1; i++)
    {
        s = a[1] * x[i] + a[0];
        q = q + (y[i] - s)   *   (y[i] - s);
        p = p + (s - yy)   *   (s - yy);
        e = fabs(y[i] - s);
        if (e   >   umax)   umax = e;
        if (e   <   umin)   umin = e;
        u = u + e / n;
    }
    dt[1] = sqrt(q / n);
    dt[0] = q;   dt[2] = p;
    dt[3] = umax;   dt[4] = umin;   dt[5] = u;
}


/***********************************************************************************************************************
-----函数功能    循环获取下一个序号,返回的值
-----说明(备注)  none
-----传入参数    num:输入需要获取数据的指针   num_max:输入的最大值的指针
-----输出参数    none
-----返回值      none
************************************************************************************************************************/
void vFunc_CycleGetNextNum(u8* num, const u8* num_max)
{
	if(*num < *num_max) (*num)++;
	else
		*num = 0;
}
