#include "filtration.h"
#include "board_config.h"
#include "function.h"
#if(boardUSE_OS)
#include "freertos.h"
#include "task.h"
#endif


int cmp_int(const void *a, const void *b)
{
    return (*(int*)a - *(int*)b);   // 升序
}

int cmp_float(const void *a, const void *b)
{
    return (*(float*)a - *(float*)b);   // 升序
}

/**************************************************************************************
函数名   Filter_Limits_Filtering 数据稳定检测
功能     ：检测数据是否处于稳定状态
输入参数 ：new_Value 输入的值
返回     ：true:已经稳定   false:未稳定
优缺点   ：none
***************************************************************************************/
s8 cFilter_CkeckDataStability(FilterHandler_T* tfilter, u16 num)
{
	tfilter->data[tfilter->CyclicCount] = num;
	tfilter->CyclicCount++;
	if(tfilter->CyclicCount >= tfilter->Buff_Size)
	{
		//长度要减一,要不指针数组会越界
		for(int i = 0; i < (tfilter->CyclicCount - 1) ; i++)  
		{
			tfilter->Sum +=  abs(tfilter->data[i] - tfilter->data[i+1]);
		}
		tfilter->CyclicCount = 0;
		if(tfilter->Sum <= tfilter->Max_Swing)  //振幅合格
		{
			tfilter->Sum = 0;
			return 1;
		}
		else   //振幅超标
		{
			tfilter->Sum = 0;
			return -1;
		}			
	}
	return 0;
}

s8 cFilter_CkeckFloatDataStability(fFilterHandler_T* tfilter, float num)
{
	tfilter->data[tfilter->CyclicCount] = num;
	tfilter->CyclicCount++;
	if(tfilter->CyclicCount >= tfilter->Buff_Size)
	{
		//长度要减一,要不指针数组会越界
		for(int i = 0; i < (tfilter->CyclicCount - 1) ; i++)  
		{
			tfilter->Sum +=  fFunc_Fabs(tfilter->data[i] , tfilter->data[i+1]);
		}
		tfilter->CyclicCount = 0;
		if(tfilter->Sum <= tfilter->Max_Swing)  //振幅合格
		{
			tfilter->Sum = 0;
			return 1;
		}
		else   //振幅超标
		{
			tfilter->Sum = 0;
			return -1;
		}			
	}
	return 0;
}


/**************************************************************************************
函数名   Filter_Limits_Filtering 限幅滤波
功能     ：每次采新值时判断：若本次值与上次值之差<=A，则本次有效；若本次值与上次值之差>A，
           本次无效，用上次值代替本次。
输入参数 ：new_Value 输入的AD值
返回     ：  
优缺点   ：克服脉冲干扰，无法抑制周期性干扰，平滑度差。
***************************************************************************************/
#define		LF_MAX_SWING_VALUE	      10    //最大振幅
u16 LF_LastValue=0;
u16 Filter_Limits(u16 new_value)
{	
	if( abs( new_value - LF_LastValue ) > LF_MAX_SWING_VALUE )  //abs()取绝对值函数
	{
		if(new_value > LF_LastValue)
		{
			new_value += LF_MAX_SWING_VALUE;
		}
		else 
		{
			new_value -= LF_MAX_SWING_VALUE;
		}
		
		LF_LastValue = new_value;
		
		return LF_LastValue;		
	}
			
	return new_value;
}



/**************************************************************************************
函数名   Filter_MedianValueFilter 中位值滤波
功能     ：连续采样N次，按大小排列
输入参数 ：datain 输入的AD值；   dataout 过滤后的AD值
返回     ：true：采集够数量的数据，已经得出中位数，反之为False   
优缺点   ：克服波动干扰，对温度等变化缓慢的被测参数有良好的滤波效果，
           对速度等快速变化的参数不宜
***************************************************************************************/
#define   MV_Max_Buff_Num    11
vu8 MV_CyclicCount = 0;      //
vu16 MV_value_buf[MV_Max_Buff_Num];
bool Filter_MedianValue( u16* datain , u16* dataout)
{
	vu16  temp;
	MV_value_buf[MV_CyclicCount] = *datain;
	MV_CyclicCount ++ ;
	if(MV_CyclicCount >= MV_Max_Buff_Num)
	{
		for( u8 j = 0; j < ( MV_Max_Buff_Num - 1 ); j++ )
			for( u8 i = 0; i < ( MV_Max_Buff_Num - j ); i++ )
			  if( MV_value_buf[ i ] > MV_value_buf[ i+1 ] )
			  {
				   temp = MV_value_buf[ i ];
				   MV_value_buf[ i ] = MV_value_buf[ i+1 ];
				   MV_value_buf[ i+1 ] = temp;
			  }
		*dataout = MV_value_buf[( MV_Max_Buff_Num-1 ) / 2 ];
	    MV_CyclicCount = 0;
		return true;
	}
	return false ;
}



/**************************************************************************************
函数名   Filter_MedianValueFilter 平均数滤波
功能     ：连续采样N次，取平均
输入参数 ：datain 输入的AD值；   dataout 过滤后的AD值
返回     ：true：采集够数量的数据，已经得出平均数，反之为False   
优缺点   ：N较大时平滑度高，灵敏度低；  N较小时平滑度低，灵敏度高
***************************************************************************************/
#define   AF_Sum_Num   10
u32  AF_Sum = 0;
u8   AF_SumCount=0;
bool Filter_Average(u16* datain, u16* dataout)
{
	AF_Sum += *datain;
	AF_SumCount ++ ;
	if( AF_SumCount >= AF_Sum_Num )
	{
		*dataout = (u16)( AF_Sum / AF_Sum_Num );
		AF_SumCount = 0;
		return true;
	}
	return false ;
}
/**************************************************************************************
函数名   Filter_RecursionAverageFilter 递推平均滤波
功能     ：连续采样N次，取平均，取N个采样值形成队列，先进先出
输入参数 ：datain 输入的AD值；
返回     ：平均后的AD值   
优缺点   ：对周期性干扰抑制性好，平滑度高， 适用于高频振动系统
         ：灵敏度低，RAM占用较大，脉冲干扰严重
***************************************************************************************/
u16 usFilter_RecursionAverage(FilterHandler_T* tHandler, u16* data)
{
	vu16  RE_CyclicSum = 0;
	//优化后
	tHandler->data[tHandler->CyclicCount] = *data ; //存储数据
	
	tHandler->CyclicCount++;
	
	if(tHandler->BuffUseSize < tHandler->CyclicCount) //记录存储个数
		tHandler->BuffUseSize ++;
	
	if( tHandler->Buff_Size <= tHandler->CyclicCount )   
		tHandler->CyclicCount = 0;
	
	for( u8 n = 0; n< tHandler->BuffUseSize; n++)  //累加
	{
		RE_CyclicSum += tHandler->data[ n ];
	}
	
	tHandler->DataOut = RE_CyclicSum / tHandler->BuffUseSize;
	
	return tHandler->DataOut;  //返回平均值
}


/**************************************************************************************
函数名   Filter_MadianAverageFilter  中位平均滤波 (验证过是对的)
功能     ：融合了中位值，平均值的优点 消除脉冲干扰
输入参数 ：datain 输入的AD值；   dataout 过滤后的AD值
返回     ：true：采集够数量的数据，已经得出中位数，反之为False  
优缺点   ：计算速度慢，RAM占用大
***************************************************************************************/
s32 lFilter_MadianAverage(FilterHandler_T* tHandler, s32* datain)
{
	#if(boardUSE_OS)
    taskENTER_CRITICAL();
	#endif
	
	tHandler->data[ tHandler->CyclicCount ] = *datain;
	tHandler->CyclicCount++;
	if( tHandler->CyclicCount >= tHandler->Buff_Size )  //数据满足
	{
		//排序
		qsort(tHandler->data,tHandler->Buff_Size,sizeof(int),cmp_int);
		
		for(u8 n = 1; n < tHandler->Buff_Size - 1; n++ )  //去除最大最小值
		{
			tHandler->Sum += tHandler->data[ n ];
		}
	
		tHandler->DataOut =  (u16)(tHandler->Sum/(tHandler->Buff_Size-2));   //求出平均值
		tHandler->CyclicCount = 0 ;
		tHandler->Sum = 0;
	}
	else 
	{
		//初始化采集时候,输入等于输出
		if(tHandler->DataOut == 0) 
			tHandler->DataOut = *datain;  
	}
	
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	return tHandler->DataOut;
}

float fFilter_MadianAverage(fFilterHandler_T* tHandler, float* datain)
{
	#if(boardUSE_OS)
    taskENTER_CRITICAL();
	#endif
	
	tHandler->data[ tHandler->CyclicCount ] = *datain;
	tHandler->CyclicCount++;
	if( tHandler->CyclicCount >= tHandler->Buff_Size )  //数据满足
	{
		//排序
		qsort(tHandler->data,tHandler->Buff_Size,sizeof(float),cmp_float);
		
		for(u8 n = 1; n < tHandler->Buff_Size - 1; n++ )  //去除最大最小值
		{
			tHandler->Sum += tHandler->data[ n ];
		}
	
		tHandler->DataOut = tHandler->Sum/(tHandler->Buff_Size-2);   //求出平均值
		tHandler->CyclicCount = 0 ;
		tHandler->Sum = 0;
	}
	else 
	{
		//初始化采集时候,输入等于输出
		if(tHandler->DataOut == 0) 
			tHandler->DataOut = *datain;  
	}
	
	#if(boardUSE_OS)
	taskEXIT_CRITICAL();
	#endif
	
	return tHandler->DataOut;
}

/**************************************************************************************
函数名   Filter_MedianValue 限幅平均滤波
功能     ：每次采样数据先限幅后送入队列
输入参数 ：datain 输入的AD值；   dataout 过滤后的AD值
返回     ：true：采集够数量的数据，已经得出中位数，反之为False   
优缺点   ：融合限幅、均值、队列的优点 ， 消除脉冲干扰，占RAM较多
***************************************************************************************/
#define    LA_Max_Swing_Value     10    //最大振幅
#define    LA_Max_Buff_Num        12

vu32 LA_Sum = 0 ;
u16  LA_Lastvalue = 0;
vu8  LA_CyclicCount = 0;      //
u16  LA_value_buf[LA_Max_Buff_Num];

u16 Filter_LimitAverage(u16* data, u16* dataout)
{
	if( abs ( *data - LA_Lastvalue ) < LA_Max_Swing_Value ) //储存符合要求的数据
	{
		LA_value_buf[ LA_CyclicCount++ ] = *data;
	}
	if( LA_CyclicCount >= LA_Max_Buff_Num )
	{
		for(u8 count = 0; count < LA_Max_Buff_Num; count++ )  //累加
		{
			LA_Sum += LA_value_buf[ count ];
		}
		LA_CyclicCount = 0;
		LA_Lastvalue = *dataout = (u16)(LA_Sum/LA_Max_Buff_Num); //求平均
		return true;
	}
	return false;
}


/**************************************************************************************
函数名   Filter_MedianValueFilter 限幅平均滤波
功能     ：每次采样数据先限幅后送入队列   本次滤波结果=（1-a）* 本次采样 + a * 上次结果
输入参数 ：datain 输入的AD值；   
返回     ：过滤后的AD值  
优缺点   : 良好一直周期性干扰，适用波动频率较高场合
***************************************************************************************/

#define			FirstOrderNum		30  /*为加快程序处理速度，取a=0~100*/
u16 FO_Lastvalue;
u16 Filter_FirstOrder(u16* data)
{
	return ((( 100 - FirstOrderNum ) * FO_Lastvalue + FirstOrderNum * ( *data )) / 100 );
}



/**************************************************************************************
函数名   Filter_RecursionAverageFilterPlus 加权递推平均滤波
功能     ：对递推平均滤波的改进，不同时刻的数据加以不同权重，通常越新的数据权重越大，
           这样灵敏度高，但平滑度低。
输入参数 ：datain 输入的AD值；dataout 滤波后的值
返回     ：true：采集够数量的数据，已经得出中位数，反之为False   
优缺点   ：适用有较大滞后时间常数和采样周期短的系统，对滞后时间常数小，
           采样周期长、变化慢的信号不能迅速反应其所受干扰。
***************************************************************************************/
#define    RA1_Max_Buff_Num        12  
  
vu16 RA_Sum1=0;
vu8  RA_CyclicCount1 = 0; 
vu16 RA_value_buf1[ RA1_Max_Buff_Num ] = {0};
const u8 coe[RA1_Max_Buff_Num] = { 1,2,3,4,5,6,7,8,9,10,11,12 };/*?coe数组为加权系数表?*/
const u8 sum_coe = { 1+2+3+4+5+6+7+8+9+10+11+12 };

bool Filter_RecursionAverage1( u16*  datain, u16* dataout )
{
	RA_value_buf1[RA_CyclicCount1] = *datain;
	RA_CyclicCount1++;
	if(RA_CyclicCount1 >= RA1_Max_Buff_Num)
	{
		for(int count = 0; count < RA1_Max_Buff_Num; count++ )
		{
			RA_Sum1+=RA_value_buf1[count]*coe[count];
		}
		*dataout = (u16) (RA_Sum1 / sum_coe) ;
		RA_CyclicCount1 = 0;
		return true;
	}
	
	return false ;
}

/**************************************************************************************
函数名   : Filter_ClearShakeFilter 清除抖动滤波
功能     ：避免临界值附近的跳动，计数器溢出时若采到干扰值则无法滤波
输入参数 ：datain 输入的AD值
返回     ：滤波后的值 
优缺点   ：对变化慢的信号滤波效果好，变化快的不好
***************************************************************************************/

#define  CS_CyclicCountMax  	5

u16  CS_LastValue = 0;
vu8  CS_CyclicCount = 0; 

u16 Filter_ClearShake( u16* data )
{
	u16	new_value;
	new_value = *data;
	if(CS_LastValue != new_value)
	{
		CS_CyclicCount++;
		if( CS_CyclicCount >= CS_CyclicCountMax )
		{
			CS_LastValue = new_value;
			CS_CyclicCount = 0;
		}
	}
	return CS_LastValue;
}

/**************************************************************************************
函数名   : Filter_LimitClearShakeFilter 限幅消抖滤波
功能     ：先限幅 后消抖
输入参数 ：data 输入的AD值
返回     ：滤波后的值 
优缺点   ：融合了限幅、消抖的优点 避免引入干扰值，对快速变化的信号不宜
***************************************************************************************/
#define    LCS_Max_Swing_Value     10    //最大振幅
#define    LCS_CyclicCountMax  	    12

u16  LCS_LastValue = 0;
vu8  LCS_CyclicCount = 0; 

u16 Filter_LimitClearShake( u16 * data  )
{
	u16 new_value;
	new_value = *data ;
	if(LCS_LastValue != new_value)
	{
		if( abs ( LCS_LastValue - new_value ) < LCS_Max_Swing_Value )
		{
			LCS_CyclicCount++;
			if( LCS_CyclicCount >= LCS_CyclicCountMax )
			{
				LCS_CyclicCount = 0;
				LCS_LastValue = new_value;
			}
		}
	}
	return LCS_LastValue;
}


/**
  ******************************************************************************
  * @brief  卡尔曼滤波器 函数
  * @param  inData - 输入值
  * @return 滤波后的值
  * @note   r值固定，q值越大，代表越信任测量值，q值无穷大，代表只用测量值。
  *                  q值越小，代表越信任模型预测值，q值为0，代表只用模型预测值。
  *         q:过程噪声，q增大，动态响应变快，收敛稳定性变坏；反之。控制误差 
  *         r:测量噪声，r增大，动态响应变慢，收敛稳定性变好；反之。控制响应速度
  ******************************************************************************
  */
unsigned long KalmanFilter(unsigned long inData)
{
    static float  kalman = 0; //上次卡尔曼值(估计出的最优值)
    static float  p = 10;
    float  q = 0.001; //q:过程噪声
    float  r = 0.001; //r:测量噪声
    float  kg = 0; //kg:卡尔曼增益
 
    p += q;
    kg = p / ( p + r ); //计算卡尔曼增益
    kalman = kalman + (kg * (inData - kalman)); //计算本次滤波估计值
    p = (1 - kg) * p; //更新测量方差
    
    return (unsigned long)kalman; //返回估计值
}




//2. 以高度为例 定义卡尔曼结构体并初始化参数
//KFP KFP_height={0.02,0,0,0,0.001,0.543};

/**
 *卡尔曼滤波器
 *@param KFP *kfp 卡尔曼结构体参数
 *   float input 需要滤波的参数的测量值（即传感器的采集值）
 *@return 滤波后的参数（最优值）
*/
float Filter_Kalman(KFP_t *kfp,float input)
{
     //预测协方差方程：k时刻系统估算协方差 = k-1时刻的系统协方差 + 过程噪声协方差
     kfp->Now_P = kfp->LastP + kfp->Q;
     //卡尔曼增益方程：卡尔曼增益 = k时刻系统估算协方差 / （k时刻系统估算协方差 + 观测噪声协方差）
     kfp->Kg = kfp->Now_P / (kfp->Now_P + kfp->R);
     //更新最优值方程：k时刻状态变量的最优值 = 状态变量的预测值 + 卡尔曼增益 * （测量值 - 状态变量的预测值）
     kfp->out = kfp->out + kfp->Kg * (input -kfp->out);//因为这一次的预测值就是上一次的输出值
     //更新协方差方程: 本次的系统协方差付给 kfp->LastP 威下一次运算准备。
     kfp->LastP = (1-kfp->Kg) * kfp->Now_P;
     return kfp->out;
}









