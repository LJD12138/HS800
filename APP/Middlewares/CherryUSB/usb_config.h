/***********************************************************************************************************************
 * Project : APP
 * Module  : APP\Middlewares\CherryUSB
 * File    : usb_config.h
 * Date    : 2026-10-08
 * Author  : LJD(291483914@qq.com)
 * Desc    : CherryUSB协议栈配置、功能裁剪开关及 Keil Configuration Wizard 向导定义头文件
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 * 原始版权: Copyright (c) 2022, sakumisu (SPDX-License-Identifier: Apache-2.0)
 ************************************************************************************************************************/
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

// <<< Use Configuration Wizard in Context Menu >>>
//=============================================================================================================
//===================================================USB公共配置===============================================
//=============================================================================================================

//-------------------------------------------------------------------
//             USB调试打印输出宏(默认重定向到printf)
#define CONFIG_USB_PRINTF(...) printf(__VA_ARGS__)
//-------------------------------------------------------------------
//             <o0> 调试打印等级
//                                                <0=> USB_DBG_ERROR(仅错误)
//                                                <1=> USB_DBG_WARNING(警告及以上)
//                                                <2=> USB_DBG_INFO(信息及以上)
//                                                <3=> USB_DBG_LOG(全部日志)
#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL            2              //USB_DBG_INFO(信息)
#endif  /* CONFIG_USB_DBG_LEVEL */
//-------------------------------------------------------------------
//             <c1> 打印带颜色
//                                                <i> 勾选为开启
#define CONFIG_USB_PRINTF_COLOR_ENABLE
//             </c>
//-------------------------------------------------------------------
//             <o0> 使用DMA时的数据对齐大小(字节)
//                                                <4-64><i> 需为2的幂次
#ifndef CONFIG_USB_ALIGN_SIZE
#define CONFIG_USB_ALIGN_SIZE            4
#endif  /* CONFIG_USB_ALIGN_SIZE */
//-------------------------------------------------------------------
//             无Cache数据段属性(本MCU内部标准SRAM无Cache,无需分段)
#define USB_NOCACHE_RAM_SECTION

//=============================================================================================================
//===================================================USB设备栈配置=============================================
//=============================================================================================================

//-------------------------------------------------------------------
//             <o0> Ep0收发缓冲区长度(字节)
//                                                <64-4096><i>
#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN  512
#endif  /* CONFIG_USBDEV_REQUEST_BUFFER_LEN */
//-------------------------------------------------------------------
//             <c0> Setup包调试日志打印
//                                                <i> 勾选为开启
//#define CONFIG_USBDEV_SETUP_LOG_PRINT
//             </c>
//-------------------------------------------------------------------
//             <c0> Ep0输入数据直接使用用户缓冲区(不拷贝)
//                                                <i> 勾选为开启
//                                                <i> 注意:用户缓冲区必须按CONFIG_USB_ALIGN_SIZE对齐
//#define CONFIG_USBDEV_EP0_INDATA_NO_COPY
//             </c>
//-------------------------------------------------------------------
//             <c0> 描述符合法性检查
//                                                <i> 勾选为开启
//#define CONFIG_USBDEV_DESC_CHECK
//             </c>
//-------------------------------------------------------------------
//             <c0> USB测试模式
//                                                <i> 勾选为开启
//#define CONFIG_USBDEV_TEST_MODE
//             </c>
//-------------------------------------------------------------------
//<h>          MSC(大容量存储)配置
//-------------------------------------------------------------------
//             <o0> 最大逻辑单元数(LUN)
//                                                <1-16><i>
#ifndef CONFIG_USBDEV_MSC_MAX_LUN
#define CONFIG_USBDEV_MSC_MAX_LUN        1
#endif  /* CONFIG_USBDEV_MSC_MAX_LUN */
//-------------------------------------------------------------------
//             <o0> MSC读写缓冲区大小(字节)
//                                                <512-16384><i>
#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE    512
#endif  /* CONFIG_USBDEV_MSC_MAX_BUFSIZE */
//-------------------------------------------------------------------
//             <s> 厂商字符串
#ifndef CONFIG_USBDEV_MSC_MANUFACTURER_STRING
#define CONFIG_USBDEV_MSC_MANUFACTURER_STRING  ""
#endif  /* CONFIG_USBDEV_MSC_MANUFACTURER_STRING */
//-------------------------------------------------------------------
//             <s> 产品字符串
#ifndef CONFIG_USBDEV_MSC_PRODUCT_STRING
#define CONFIG_USBDEV_MSC_PRODUCT_STRING  ""
#endif  /* CONFIG_USBDEV_MSC_PRODUCT_STRING */
//-------------------------------------------------------------------
//             <s> 版本字符串
#ifndef CONFIG_USBDEV_MSC_VERSION_STRING
#define CONFIG_USBDEV_MSC_VERSION_STRING  "0.01"
#endif  /* CONFIG_USBDEV_MSC_VERSION_STRING */
//-------------------------------------------------------------------
//             <c0> MSC读写改为主循环轮询
//                                                <i> 勾选后需在while(1)中调用usbd_msc_polling
//#define CONFIG_USBDEV_MSC_POLLING
//             </c>
//-------------------------------------------------------------------
//             <c0> MSC读写改为线程处理
//                                                <i> 勾选为开启
//#define CONFIG_USBDEV_MSC_THREAD
//             </c>
//-------------------------------------------------------------------
//             <o0> MSC线程优先级
//                                                <0-31><i> 仅MSC线程方式有效
#ifndef CONFIG_USBDEV_MSC_PRIO
#define CONFIG_USBDEV_MSC_PRIO           4
#endif  /* CONFIG_USBDEV_MSC_PRIO */
//-------------------------------------------------------------------
//             <o0> MSC线程栈大小(字节)
//                                                <512-65535><i> 仅MSC线程方式有效
#ifndef CONFIG_USBDEV_MSC_STACKSIZE
#define CONFIG_USBDEV_MSC_STACKSIZE      2048
#endif  /* CONFIG_USBDEV_MSC_STACKSIZE */
//</h>         MSC(大容量存储)配置
//-------------------------------------------------------------------
//<h>          RNDIS(USB虚拟网卡)配置
//-------------------------------------------------------------------
//             <o0> RNDIS响应缓冲区大小(字节)
//                                                <156-2048><i>
#ifndef CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE
#define CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE  156
#endif  /* CONFIG_USBDEV_RNDIS_RESP_BUFFER_SIZE */
//-------------------------------------------------------------------
//             <o0> RNDIS以太网最大帧大小(字节)
//                                                <1580-16384><i> 必须为(1536+44)的整数倍
#ifndef CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE
#define CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE  1580
#endif  /* CONFIG_USBDEV_RNDIS_ETH_MAX_FRAME_SIZE */
//-------------------------------------------------------------------
//             RNDIS厂商ID
#ifndef CONFIG_USBDEV_RNDIS_VENDOR_ID
#define CONFIG_USBDEV_RNDIS_VENDOR_ID    0x0000ffff
#endif  /* CONFIG_USBDEV_RNDIS_VENDOR_ID */
//-------------------------------------------------------------------
//             <s> RNDIS厂商描述字符串
#ifndef CONFIG_USBDEV_RNDIS_VENDOR_DESC
#define CONFIG_USBDEV_RNDIS_VENDOR_DESC  "CherryUSB"
#endif  /* CONFIG_USBDEV_RNDIS_VENDOR_DESC */
//-------------------------------------------------------------------
//             <c1> 使用LwIP协议栈
//                                                <i> 勾选为开启
#define CONFIG_USBDEV_RNDIS_USING_LWIP
//             </c>
//</h>         RNDIS(USB虚拟网卡)配置

//=============================================================================================================
//===================================================USB主机栈配置=============================================
//=============================================================================================================

//-------------------------------------------------------------------
//             <o0> 最大根HUB(主机端口)数
//                                                <1-8><i>
#define CONFIG_USBHOST_MAX_RHPORTS       1
//-------------------------------------------------------------------
//             <o0> 最大外部HUB数
//                                                <0-8><i>
#define CONFIG_USBHOST_MAX_EXTHUBS       1
//-------------------------------------------------------------------
//             <o0> 外部HUB最大下行端口数
//                                                <1-8><i>
#define CONFIG_USBHOST_MAX_EHPORTS       4
//-------------------------------------------------------------------
//             <o0> 设备最大接口数
//                                                <1-32><i>
#define CONFIG_USBHOST_MAX_INTERFACES    8
//-------------------------------------------------------------------
//             <o0> 接口最大备用设置数
//                                                <1-8><i>
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS  8
//-------------------------------------------------------------------
//             <o0> 接口最大端点数
//                                                <1-16><i>
#define CONFIG_USBHOST_MAX_ENDPOINTS     4
//-------------------------------------------------------------------
//             <o0> 最大CDC_ACM(虚拟串口)类实例数
//                                                <0-16><i>
#define CONFIG_USBHOST_MAX_CDC_ACM_CLASS  4
//-------------------------------------------------------------------
//             <o0> 最大HID类实例数
//                                                <0-16><i>
#define CONFIG_USBHOST_MAX_HID_CLASS     4
//-------------------------------------------------------------------
//             <o0> 最大MSC类实例数
//                                                <0-16><i>
#define CONFIG_USBHOST_MAX_MSC_CLASS     2
//-------------------------------------------------------------------
//             <o0> 最大Audio类实例数
//                                                <0-8><i>
#define CONFIG_USBHOST_MAX_AUDIO_CLASS   1
//-------------------------------------------------------------------
//             <o0> 最大Video类实例数
//                                                <0-8><i>
#define CONFIG_USBHOST_MAX_VIDEO_CLASS   1
//-------------------------------------------------------------------
//             <o0> 设备名长度(字节)
//                                                <16-255><i>
#define CONFIG_USBHOST_DEV_NAMELEN       16
//-------------------------------------------------------------------
//             <o0> PSC线程(集线器/枚举处理)优先级
//                                                <0-31><i>
#ifndef CONFIG_USBHOST_PSC_PRIO
#define CONFIG_USBHOST_PSC_PRIO          0
#endif  /* CONFIG_USBHOST_PSC_PRIO */
//-------------------------------------------------------------------
//             <o0> PSC线程栈大小(字节)
//                                                <512-65535><i>
#ifndef CONFIG_USBHOST_PSC_STACKSIZE
#define CONFIG_USBHOST_PSC_STACKSIZE     2048
#endif  /* CONFIG_USBHOST_PSC_STACKSIZE */
//-------------------------------------------------------------------
//             <c0> 获取字符串描述符
//                                                <i> 勾选为开启
//#define CONFIG_USBHOST_GET_STRING_DESC
//             </c>
//-------------------------------------------------------------------
//             <c0> 使能MS OS描述符(WinUSB)
//                                                <i> 勾选为开启
//#define CONFIG_USBHOST_MSOS_ENABLE
//             </c>
//             MS OS厂商请求码
#ifndef CONFIG_USBHOST_MSOS_VENDOR_CODE
#define CONFIG_USBHOST_MSOS_VENDOR_CODE  0x00
#endif  /* CONFIG_USBHOST_MSOS_VENDOR_CODE */
//-------------------------------------------------------------------
//             <o0> Ep0最大传输缓冲区长度(字节)
//                                                <64-4096><i>
#ifndef CONFIG_USBHOST_REQUEST_BUFFER_LEN
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN  512
#endif  /* CONFIG_USBHOST_REQUEST_BUFFER_LEN */
//-------------------------------------------------------------------
//             <o0> 控制传输超时时间(毫秒)
//                                                <1-65535><i>
#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
#define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT  500
#endif  /* CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT */
//-------------------------------------------------------------------
//             <o0> MSC传输超时时间(毫秒)
//                                                <1-65535><i>
#ifndef CONFIG_USBHOST_MSC_TIMEOUT
#define CONFIG_USBHOST_MSC_TIMEOUT       5000
#endif  /* CONFIG_USBHOST_MSC_TIMEOUT */
//-------------------------------------------------------------------
//             <o0> RNDIS最大接收缓冲大小(字节)
//                                                <2048-16384><i> 影响USB性能,取决于TCP接收窗口(TCP_WND)大小
//                                                <i> 可改为2K~16K,须大于TCP接收窗口以避免溢出
#ifndef CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE  2048
#endif  /* CONFIG_USBHOST_RNDIS_ETH_MAX_RX_SIZE */
//-------------------------------------------------------------------
//             <o0> RNDIS最大发送缓冲大小(字节)
//                                                <2048-16384><i> lwip单次不支持多pbuf,增大此值无性能提升
#ifndef CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE  2048
#endif  /* CONFIG_USBHOST_RNDIS_ETH_MAX_TX_SIZE */
//-------------------------------------------------------------------
//             <o0> CDC_NCM最大接收缓冲大小(字节)
//                                                <2048-16384><i> 影响USB性能,取决于TCP接收窗口(TCP_WND)大小
//                                                <i> 可改为2K~16K,须大于TCP接收窗口以避免溢出
#ifndef CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE  2048
#endif  /* CONFIG_USBHOST_CDC_NCM_ETH_MAX_RX_SIZE */
//-------------------------------------------------------------------
//             <o0> CDC_NCM最大发送缓冲大小(字节)
//                                                <2048-16384><i> lwip单次不支持多pbuf,增大此值无性能提升
#ifndef CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE  2048
#endif  /* CONFIG_USBHOST_CDC_NCM_ETH_MAX_TX_SIZE */
//-------------------------------------------------------------------
//             <o0> ASIX最大接收缓冲大小(字节)
//                                                <2048-16384><i> 影响USB性能,取决于TCP接收窗口(TCP_WND)大小
//                                                <i> 可改为2K~16K,须大于TCP接收窗口以避免溢出
#ifndef CONFIG_USBHOST_ASIX_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_ASIX_ETH_MAX_RX_SIZE  2048
#endif  /* CONFIG_USBHOST_ASIX_ETH_MAX_RX_SIZE */
//-------------------------------------------------------------------
//             <o0> ASIX最大发送缓冲大小(字节)
//                                                <2048-16384><i> lwip单次不支持多pbuf,增大此值无性能提升
#ifndef CONFIG_USBHOST_ASIX_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_ASIX_ETH_MAX_TX_SIZE  2048
#endif  /* CONFIG_USBHOST_ASIX_ETH_MAX_TX_SIZE */
//-------------------------------------------------------------------
//             <o0> RTL8152最大接收缓冲大小(字节)
//                                                <2048-16384><i> 影响USB性能,取决于TCP接收窗口(TCP_WND)大小
//                                                <i> 可改为2K~16K,须大于TCP接收窗口以避免溢出
#ifndef CONFIG_USBHOST_RTL8152_ETH_MAX_RX_SIZE
#define CONFIG_USBHOST_RTL8152_ETH_MAX_RX_SIZE  2048
#endif  /* CONFIG_USBHOST_RTL8152_ETH_MAX_RX_SIZE */
//-------------------------------------------------------------------
//             <o0> RTL8152最大发送缓冲大小(字节)
//                                                <2048-16384><i> lwip单次不支持多pbuf,增大此值无性能提升
#ifndef CONFIG_USBHOST_RTL8152_ETH_MAX_TX_SIZE
#define CONFIG_USBHOST_RTL8152_ETH_MAX_TX_SIZE  2048
#endif  /* CONFIG_USBHOST_RTL8152_ETH_MAX_TX_SIZE */
//-------------------------------------------------------------------
//             <c1> 蓝牙HCI H4传输模式
//                                                <i> 勾选为开启
#define CONFIG_USBHOST_BLUETOOTH_HCI_H4
//             </c>
//-------------------------------------------------------------------
//             <c0> 蓝牙HCI日志
//                                                <i> 勾选为开启
//#define CONFIG_USBHOST_BLUETOOTH_HCI_LOG
//             </c>
//-------------------------------------------------------------------
//             <o0> 蓝牙HCI发送缓冲大小(字节)
//                                                <512-16384><i>
#ifndef CONFIG_USBHOST_BLUETOOTH_TX_SIZE
#define CONFIG_USBHOST_BLUETOOTH_TX_SIZE  2048
#endif  /* CONFIG_USBHOST_BLUETOOTH_TX_SIZE */
//-------------------------------------------------------------------
//             <o0> 蓝牙HCI接收缓冲大小(字节)
//                                                <512-16384><i>
#ifndef CONFIG_USBHOST_BLUETOOTH_RX_SIZE
#define CONFIG_USBHOST_BLUETOOTH_RX_SIZE  2048
#endif  /* CONFIG_USBHOST_BLUETOOTH_RX_SIZE */

//=============================================================================================================
//=================================================USB设备端口配置=============================================
//=============================================================================================================

//-------------------------------------------------------------------
//             <o0> 设备最大总线数
//                                                <1-8><i> 目前除hpm外总线数必须为1
#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS            1
#endif  /* CONFIG_USBDEV_MAX_BUS */
//-------------------------------------------------------------------
//             <o0> 设备最大端点数
//                                                <1-16><i>
#ifndef CONFIG_USBDEV_EP_NUM
#define CONFIG_USBDEV_EP_NUM             4
#endif  /* CONFIG_USBDEV_EP_NUM */
//-------------------------------------------------------------------
//             <c0> 高速模式
//                                                <i> 芯片支持高速且需以高速初始化时勾选,相关IP将按此配置内部/外部高速PHY
//#define CONFIG_USB_HS
//             </c>
//-------------------------------------------------------------------
//             <c0> FSDEV PMA访问宽度
//                                                <i> 可为1或2,不同芯片存在差异
//#define CONFIG_USBDEV_FSDEV_PMA_ACCESS  2
//             </c>
//-------------------------------------------------------------------
//             DWC2 RX全局FIFO大小(单位:字4字节)
//             公式: (5*控制端点数+8)+((最大USB包长/4)+1状态信息)+(2*OUT端点数)+1全局NAK
#define CONFIG_USB_DWC2_RXALL_FIFO_SIZE  (512 / 4)
//             DWC2 IN端点0发送FIFO大小(单位:字,最大包长/4)
#define CONFIG_USB_DWC2_TX0_FIFO_SIZE    (64 / 4)
//             DWC2 IN端点1发送FIFO大小(单位:字,最大包长/4)
#define CONFIG_USB_DWC2_TX1_FIFO_SIZE    (256 / 4)
//             DWC2 IN端点2发送FIFO大小(单位:字,最大包长/4)
#define CONFIG_USB_DWC2_TX2_FIFO_SIZE    (64 / 4)
//             DWC2 IN端点3发送FIFO大小(单位:字,不小于该端点最大包长,CDC通知端点MPS=64)
#define CONFIG_USB_DWC2_TX3_FIFO_SIZE    (64 / 4)
//             DWC2 IN端点4~8发送FIFO(默认0,需用时取消注释并按最大包长/4修改)
// #define CONFIG_USB_DWC2_TX4_FIFO_SIZE (0 / 4)
// #define CONFIG_USB_DWC2_TX5_FIFO_SIZE (0 / 4)
// #define CONFIG_USB_DWC2_TX6_FIFO_SIZE (0 / 4)
// #define CONFIG_USB_DWC2_TX7_FIFO_SIZE (0 / 4)
// #define CONFIG_USB_DWC2_TX8_FIFO_SIZE (0 / 4)
//-------------------------------------------------------------------
//             <c0> DWC2 DMA方式使能
//                                                <i> 勾选为开启
//#define CONFIG_USB_DWC2_DMA_ENABLE
//             </c>
//-------------------------------------------------------------------
//             <c0> 使用全志MUSB IP
//                                                <i> 勾选为开启
//#define CONFIG_USB_MUSB_SUNXI
//             </c>

//=============================================================================================================
//=================================================USB主机端口配置=============================================
//=============================================================================================================

//-------------------------------------------------------------------
//             <o0> 主机最大总线数
//                                                <1-8><i>
#ifndef CONFIG_USBHOST_MAX_BUS
#define CONFIG_USBHOST_MAX_BUS           1
#endif  /* CONFIG_USBHOST_MAX_BUS */
//-------------------------------------------------------------------
//             <o0> 主机最大管道(通道)数
//                                                <1-30><i>
#ifndef CONFIG_USBHOST_PIPE_NUM
#define CONFIG_USBHOST_PIPE_NUM          10
#endif  /* CONFIG_USBHOST_PIPE_NUM */
//-------------------------------------------------------------------
//<h>          EHCI配置
//-------------------------------------------------------------------
//             EHCI HCCR寄存器偏移
#define CONFIG_USB_EHCI_HCCR_OFFSET      (0x0)
//             <o0> 帧列表大小
//                                                <256=> 256
//                                                <512=> 512
//                                                <1024=> 1024
#define CONFIG_USB_EHCI_FRAME_LIST_SIZE  1024
//             EHCI QH数量(等于主机管道数)
#define CONFIG_USB_EHCI_QH_NUM           CONFIG_USBHOST_PIPE_NUM
//             <o0> QTD数量
//                                                <1-8><i>
#define CONFIG_USB_EHCI_QTD_NUM          3
//             <o0> ITD数量
//                                                <1-64><i>
#define CONFIG_USB_EHCI_ITD_NUM          20
//-------------------------------------------------------------------
//             <c0> HCOR保留区访问禁止
//                                                <i> 勾选为开启
//#define CONFIG_USB_EHCI_HCOR_RESERVED_DISABLE
//             </c>
//             <c0> CONFIGFLAG标志使能
//                                                <i> 勾选为开启
//#define CONFIG_USB_EHCI_CONFIGFLAG
//             </c>
//             <c0> 同步(ISO)传输支持
//                                                <i> 勾选为开启
//#define CONFIG_USB_EHCI_ISO
//             </c>
//             <c0> EHCI与OHCI联合使用
//                                                <i> 勾选为开启
//#define CONFIG_USB_EHCI_WITH_OHCI
//             </c>
//</h>         EHCI配置
//-------------------------------------------------------------------
//             OHCI HCOR寄存器偏移
#define CONFIG_USB_OHCI_HCOR_OFFSET      (0x0)
//-------------------------------------------------------------------
//             XHCI HCCR寄存器偏移
#define CONFIG_USB_XHCI_HCOR_OFFSET      (0x0)
//-------------------------------------------------------------------
//             DWC2主机非周期发送FIFO大小(最大非周期USB包长/4)
// #define CONFIG_USB_DWC2_NPTX_FIFO_SIZE (512 / 4)
//             DWC2主机周期发送FIFO大小(最大周期USB包长/4)
// #define CONFIG_USB_DWC2_PTX_FIFO_SIZE (1024 / 4)
//             DWC2主机RX全局FIFO大小: (最大USB包长/4)+1状态信息+1传输完成+每个Bulk/Control端点1个NAK/NYET处理位置
// #define CONFIG_USB_DWC2_RX_FIFO_SIZE ((1012 - CONFIG_USB_DWC2_NPTX_FIFO_SIZE - CONFIG_USB_DWC2_PTX_FIFO_SIZE))
//-------------------------------------------------------------------
//             <c0> 使用全志MUSB IP
//                                                <i> 勾选为开启
//#define CONFIG_USB_MUSB_SUNXI
//             </c>

// <<< end of configuration section >>>

#endif  /* CHERRYUSB_CONFIG_H */
