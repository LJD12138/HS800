#ifndef FILTRATION_H_
#define FILTRATION_H_
#include "main.h"

#ifdef __cplusplus
 extern "C" {
#endif 

#pragma pack(1)
typedef struct
{
	s32*   data;        //数据BUFF
	u8     Buff_Size;   //缓存器的大小
	u8     Max_Swing;   //最大振幅
	vu8    BuffUseSize; //缓存器被使用的大小
    vu8    CyclicCount; //循环次数
	s64    Sum;         //总和
	s32    DataOut;     //返回的数据
} FilterHandler_T ;
#pragma pack()

#pragma pack(1)
typedef struct
{
	float*   	data;        //数据BUFF
	u8     		Buff_Size;   //缓存器的大小
	float     	Max_Swing;   //最大振幅
	vu8    		BuffUseSize; //缓存器被使用的大小
    vu8    		CyclicCount; //循环次数
	float    	Sum;         //总和
	float    	DataOut;     //返回的数据
} fFilterHandler_T ;
#pragma pack()



//1. 结构体类型定义
/*
Q:过程噪声，Q增大，动态响应变快，收敛稳定性变坏
R:测量噪声，R增大，动态响应变慢，收敛稳定性变好

其中lastP的初值可以随便取，但是不能为0（为0的话卡尔曼滤波器就认为已经是最优滤波器了）
Q,R的值需要我们试出来，讲白了就是(买的破温度计有多破，以及你的超人力有多强)
 
R参数调整滤波后的曲线与实测曲线的相近程度，R越小越接近。
 
Q参数调滤波后的曲线平滑程度，Q越小越平滑。


参数作用及调整
Q
Q 值为过程预测 噪声，越小系统越容易收敛，我们对模型预测的值信任度越高；
但是太小则容易发散，如果 Q 为零，那么我们只相信预测值； Q 值越大我们对于预测的信任度就越低，
而对测量值的信任度就变高；如果 Q 值无穷大，那么我们只信任测量值。
 
P
R 值为测量噪声，太小太大都不一定合适。 R 太大，卡尔曼滤波响应会变慢，
因为它对新测量的值的信任度降低；越小系统收敛越快，但过小则容易出现震荡。
 
Q/R
测试时可以先将 Q 从小往大调整，将 R 从大往小调整；先固定一个值去调整另外一个值. 
在实际的工程应用中， 只要固定估计噪声和测量噪声的方差的比值Q/R， 计算的结果就是一样的， 
调整比值 Q/R  就可以改变滤波的效果，这个比值越大，越信任测量值。
 
*/
typedef struct 
{
    float LastP;//上次估算协方差 初始化值为0.02
    float Now_P;//当前估算协方差 初始化值为0
    float out;//卡尔曼滤波器输出 初始化值为0
    float Kg;//卡尔曼增益 初始化值为0
    float Q;//过程噪声协方差 初始化值为0.001
    float R;//观测噪声协方差 初始化值为0.543
}KFP_t;//Kalman Filter parameter
	 
s8 cFilter_CkeckDataStability(FilterHandler_T* tfilter, u16 num);
s8 cFilter_CkeckFloatDataStability(fFilterHandler_T* tfilter, float num);

u16   Filter_Limits(u16 new_Value);
bool  Filter_MedianValue( u16* datain , u16* dataout);
bool  Filter_Average(u16* datain, u16* dataout);

u16   usFilter_RecursionAverage(FilterHandler_T* tHandler, u16* data);

s32 lFilter_MadianAverage(FilterHandler_T* tHandler, s32* datain);
float fFilter_MadianAverage(fFilterHandler_T* tHandler, float* datain);

u16   Filter_LimitAverage(u16* data, u16* dataout);
u16   Filter_FirstOrder(u16* data);
bool  Filter_RecursionAverage1( u16*  datain, u16* dataout );
u16   Filter_ClearShake( u16* data );
u16   Filter_LimitClearShake( u16 * data  );
float Filter_Kalman(KFP_t *kfp,float input);

void  text(u8 num, u8 num2);

#ifdef __cplusplus
}
#endif	 

#endif


