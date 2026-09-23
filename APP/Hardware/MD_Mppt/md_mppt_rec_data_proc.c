/*******************************************************************************************************************************
 * Project : APP
 * Module  : APP\Hardware\MD_Mppt
 * File    : md_mppt_rec_data_proc.c
 * Date    : 2026-09-20
 * Author  : LJD(291483914@qq.com)
 * Desc    : MPPT 接收数据协议解包处理实现
 * -------------------------------------------------------
 * todo    :
 * 1. 无
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "MD_Mppt/md_mppt_rec_data_proc.h"

#if (boardMPPT_EN)
#include "MD_Mppt/md_mppt_rec_task.h"
#include "MD_Mppt/md_mppt_prot_frame.h"
#include "MD_Mppt/md_mppt_task.h"
#include "Print/print_task.h"
#include "function.h"

/***********************************************************************************************************************
 * 函数功能    : 处理 MPPT 接收到的数据
 * 说明(备注)  : 校验 Modbus 响应并更新输入电压、电流、功率与工作模式
 * 传入参数    : proto_rx: 接收协议对象指针, proto_tx: 发送协议对象指针
 * 输出参数    : 无
 * 返回值      : 1: 成功; 负数: 校验失败
 ************************************************************************************************************************/
s8 c_mppt_rec_proc_data(ModbusProtoRx_t *proto_rx, ModbusProtoTx_t *proto_tx)
{
    if (uPrint.tFlag.bMpptRecTask)
    {
        sMyPrint("bMpptRecTask:接收地址%d:", proto_tx->usRegAddr);
        for (int i = 0; i < proto_rx->ucValidLen; i++)
            sMyPrint("%x ", proto_rx->ucpValidData[i]);
        sMyPrint("\r\n");
    }

    if (proto_rx->ucCmd == modbusREAD_MULTI_REG || proto_rx->ucCmd == modbusREAD_MULTI_BIT)
    {
        if (proto_rx->ucCharLen != proto_tx->ucCharLen)
            return -1;
    }
    else if (proto_rx->ucCmd == modbusWRITE_MULTI_REG)
    {
        if (proto_rx->usRegAddr != proto_tx->usRegAddr || proto_rx->usRegSize != proto_tx->usRegSize)
            return -2;
    }
    else if (proto_rx->ucCmd == modbusWRITE_SINGLE_REG || proto_rx->ucCmd == modbusWRITE_SINGLE_BIT)
    {
        if (proto_rx->usRegAddr != proto_tx->usRegAddr)
            return -3;
    }

    switch (proto_tx->usRegAddr)
    {
        case mpptREG_ADDR_SET_PV_CHG_PWR:
        {
            if (proto_rx->ucValidLen != 2 || proto_rx->ucpValidData == NULL)
                return -10;

            if (tpMpptTask->tReplyBuff.buff == NULL)
                return -11;

            uint16_t us_in_pwr = ((uint16_t)proto_rx->ucpValidData[0] << 8) | proto_rx->ucpValidData[1];
            tMpptRx.usMaxInPwr = us_in_pwr * 10;
        }
        break;

        case mpptREG_ADDR_GET_PARAM1:
        {
            MpptParam_T t_param;

            if (proto_rx->ucCharLen != sizeof(t_param))
                return -20;

            if (proto_rx->ucpValidData == NULL)
                return -21;

            /* 装载参数 */
            bFunc_SwapU16Array((u8*)&t_param, proto_rx->ucpValidData, proto_rx->ucCharLen / 2);

            if (t_param.usInState != 0)
                bMppt_SetDevState(DS_WORK);
            else
                bMppt_SetDevState(DS_SHUT_DOWN);

            tMpptRx.uInType         = (MpptInType_U)t_param.usInState;
            tMpptRx.uErrCode.usCode = t_param.usErrCode;

            tMpptRx.usInVolt = t_param.usInVolt;
            tMpptRx.usInCurr = t_param.usInCurr * 10;
            tMpptRx.usInPwr  = t_param.usInPwr * 10;

            if (tMppt.eDevState == DS_WORK)
                tMpptRx.sMaxTemp = sMpptMaxTemp;
            else
                tMpptRx.sMaxTemp = 25;
        }
        break;

        default:
            return -99;
    }

    return 1;
}

#endif  /* boardMPPT_EN */
