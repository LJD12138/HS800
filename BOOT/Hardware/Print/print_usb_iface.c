/*******************************************************************************************************************************
 * Project : BOOT
 * Module  : BOOT\Hardware\Usb
 * File    : print_usb_iface.c
 * Date    : 2026-09-30
 * Author  : LJD(291483914@qq.com)
 * Desc    : CherryUSB CDC-ACM 虚拟串口硬件驱动与通信接口实现
 * -------------------------------------------------------
 * todo    :
 * 1. none
 * -------------------------------------------------------
 * Copyright (c) 2026 -inc
 *******************************************************************************************************************************/

//****************************************************Includes******************************************************************//
#include "Print/print_usb_iface.h"

#if (boardUSB_EN || boardPRINT_IFACE == 7)
#include "usbd_core.h"
#include "usb_dc.h"
#include "usbd_cdc_acm.h"

#if (boardPRINT_IFACE)
#include "Print/print_prot_frame.h"
#include "lwrb.h"
#endif  /* boardPRINT_IFACE */

//****************************************************Macros********************************************************************//
#define			USBD_VID								0x28E9U	/* GigaDevice Vendor ID */
#define			USBD_PID								0x018AU	/* GD32 CDC ACM Product ID */
#define			USBD_MAX_POWER							100U	/* 100mA */
#define			USBD_LANGID_STRING						1033U	/* 英语 (美国) 0x0409 */

#define			CDC_MAX_MPS								64U		/* 全速 CDC 最大端点包长 */
#define         USB_CONFIG_SIZE                         (9U + CDC_ACM_DESCRIPTOR_LEN)

//****************************************************Parameter Initialization**************************************************//
static volatile bool s_b_usb_configured = false;
static volatile bool s_b_usb_tx_busy    = false;

static pfnUsbCdcRxCallback_T s_pfn_rx_callback = NULL;

__ALIGNED(4) static uint8_t s_uca_cdc_rx_buff[usbCDC_RX_BUF_SIZE];
__ALIGNED(4) static uint8_t s_uca_cdc_tx_buff[usbCDC_TX_BUF_SIZE];

/* USB 描述符配置 */
static const uint8_t s_uca_cdc_descriptor[] = {
    USB_DEVICE_DESCRIPTOR_INIT(USB_2_0, 0xEF, 0x02, 0x01, USBD_VID, USBD_PID, 0x0100, 0x01),
    USB_CONFIG_DESCRIPTOR_INIT(USB_CONFIG_SIZE, 0x02, 0x01, USB_CONFIG_BUS_POWERED, USBD_MAX_POWER),
    CDC_ACM_DESCRIPTOR_INIT(0x00, usbCDC_INT_EP, usbCDC_OUT_EP, usbCDC_IN_EP, CDC_MAX_MPS, 0x02),
    /* String0 语言描述符 */
    USB_LANGID_INIT(USBD_LANGID_STRING),
    /* String1 厂商描述符 "GigaDevice" */
    0x16, USB_DESCRIPTOR_TYPE_STRING,
    'G', 0x00, 'i', 0x00, 'g', 0x00, 'a', 0x00, 'D', 0x00,
    'e', 0x00, 'v', 0x00, 'i', 0x00, 'c', 0x00, 'e', 0x00,
    /* String2 产品描述符 "GD32 Virtual COM Port" */
    0x2E, USB_DESCRIPTOR_TYPE_STRING,
    'G', 0x00, 'D', 0x00, '3', 0x00, '2', 0x00, ' ', 0x00,
    'V', 0x00, 'i', 0x00, 'r', 0x00, 't', 0x00, 'u', 0x00, 'a', 0x00, 'l', 0x00, ' ', 0x00,
    'C', 0x00, 'O', 0x00, 'M', 0x00, ' ', 0x00, 'P', 0x00, 'o', 0x00, 'r', 0x00, 't', 0x00,
    /* String3 序列号描述符 "HS800-BOOT-001" */
    0x1E, USB_DESCRIPTOR_TYPE_STRING,
    'H', 0x00, 'S', 0x00, '8', 0x00, '0', 0x00, '0', 0x00, '-', 0x00,
    'B', 0x00, 'O', 0x00, 'O', 0x00, 'T', 0x00, '-', 0x00, '0', 0x00, '0', 0x00, '1', 0x00,
    0x00
};

static struct usbd_interface s_t_intf0;
static struct usbd_interface s_t_intf1;

//****************************************************Function Declaration******************************************************//
static void usbd_event_handler(uint8_t busid, uint8_t event);
static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes);
static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes);

static struct usbd_endpoint s_t_cdc_out_ep = {
    .ep_addr = usbCDC_OUT_EP,
    .ep_cb   = usbd_cdc_acm_bulk_out
};

static struct usbd_endpoint s_t_cdc_in_ep = {
    .ep_addr = usbCDC_IN_EP,
    .ep_cb   = usbd_cdc_acm_bulk_in
};

/***********************************************************************************************************************
 * 函数功能    : USB 设备底层时钟与硬件引脚初始化
 * 说明(备注)  : 由 CherryUSB usb_dc_init 内部弱函数回调
 * 传入参数    : busid: 总线编号
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void usb_dc_low_level_init(uint8_t busid)
{
    (void)busid;

    /* 1. 开启内部 IRC48M 48MHz 振荡器 */
    rcu_osci_on(RCU_IRC48M);
    while (SUCCESS != rcu_osci_stab_wait(RCU_IRC48M))
    {
    }

    /* 2. 选择 48MHz 时钟源为内部 IRC48M */
    rcu_ck48m_clock_config(RCU_CK48MSRC_IRC48M);

    /* 3. 使能 GPIOA 与 USBFS 模块时钟 */
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_USBFS);

    /* 4. 软断开重连：先通过通用推挽输出拉低 PA11/PA12 保持约 10ms，强制主机识别断开并重新枚举 */
    gpio_mode_set(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_output_options_set(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_LEVEL3, GPIO_PIN_11 | GPIO_PIN_12);
    gpio_bit_reset(GPIOA, GPIO_PIN_11 | GPIO_PIN_12);
    for (volatile uint32_t i = 0; i < 200000; i++)
    {
        __NOP();
    }

    /* 5. 配置 PA11 (DM) 和 PA12 (DP) 引脚为模拟模式 (关闭数字输入缓冲，直通内部 USB PHY) */
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_11 | GPIO_PIN_12);

    /* 6. 配置 USBFS 全局中断优先级并使能 */
    nvic_irq_enable((uint8_t)USBFS_IRQn, 2U, 0U);
}

/***********************************************************************************************************************
 * 函数功能    : USB 设备底层去初始化
 * 说明(备注)  : none
 * 传入参数    : busid: 总线编号
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void usb_dc_low_level_deinit(uint8_t busid)
{
    (void)busid;
    nvic_irq_disable((uint8_t)USBFS_IRQn);
    rcu_periph_clock_disable(RCU_USBFS);
}

/***********************************************************************************************************************
 * 函数功能    : USBFS 中断服务函数
 * 说明(备注)  : 将硬件中断派发至 CherryUSB 设备协议栈
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void USBFS_IRQHandler(void)
{
    extern void USBD_IRQHandler(uint8_t busid);
    USBD_IRQHandler(0);
}

/***********************************************************************************************************************
 * 函数功能    : USB 状态事件处理回调
 * 说明(备注)  : 处理连接、复位、挂起及配置成功事件
 * 传入参数    : busid: 总线编号, event: 事件类型
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void usbd_event_handler(uint8_t busid, uint8_t event)
{
    switch (event)
    {
        case USBD_EVENT_RESET:
            s_b_usb_configured = false;
            s_b_usb_tx_busy    = false;
            break;

        case USBD_EVENT_CONFIGURED:
            s_b_usb_configured = true;
            s_b_usb_tx_busy    = false;
            /* 启动首次 OUT 端点读取传输，准备接收上位机下发数据 */
            usbd_ep_start_read(busid, usbCDC_OUT_EP, s_uca_cdc_rx_buff, usbCDC_RX_BUF_SIZE);
            break;

        case USBD_EVENT_DISCONNECTED:
            s_b_usb_configured = false;
            s_b_usb_tx_busy    = false;
            break;

        case USBD_EVENT_RESUME:
            s_b_usb_configured = true;
            s_b_usb_tx_busy    = false;
            break;

        case USBD_EVENT_SUSPEND:
            /* 挂起不代表断开配置，清忙标记防止在途传输标志挂死 */
            s_b_usb_tx_busy    = false;
            break;

        default:
            break;
    }
}

/***********************************************************************************************************************
 * 函数功能    : USB CDC OUT 端点数据接收完成回调
 * 说明(备注)  : 将收到的数据推入 Print 环形缓冲区，并准备下一次接收
 * 传入参数    : busid: 总线编号, ep: 端点地址, nbytes: 实际接收字节数
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void usbd_cdc_acm_bulk_out(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)ep;

    if (nbytes > 0)
    {
        /* 执行用户注册的接收回调函数 */
        if (s_pfn_rx_callback != NULL)
        {
            s_pfn_rx_callback(s_uca_cdc_rx_buff, (uint16_t)nbytes);
        }

        #if (boardPRINT_IFACE)
        /* 桥接到系统 Print 接收环形缓冲区 */
        if (tpPrintProtoRx != NULL)
        {
            lwrb_write(&tpPrintProtoRx->tRxBuff, s_uca_cdc_rx_buff, nbytes);
        }
        #endif  /* boardPRINT_IFACE */
    }

    /* 准备下一次数据接收 */
    usbd_ep_start_read(busid, usbCDC_OUT_EP, s_uca_cdc_rx_buff, usbCDC_RX_BUF_SIZE);
}

/***********************************************************************************************************************
 * 函数功能    : USB CDC IN 端点数据发送完成回调
 * 说明(备注)  : 释放发送繁忙标志，整包情况处理 ZLP
 * 传入参数    : busid: 总线编号, ep: 端点地址, nbytes: 实际发送字节数
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
static void usbd_cdc_acm_bulk_in(uint8_t busid, uint8_t ep, uint32_t nbytes)
{
    (void)busid;
    (void)ep;
    (void)nbytes;

    /* CDC ACM 虚拟串口为流式协议，无需额外发送 ZLP，每次 IN 完成直接释放发送繁忙标志 */
    s_b_usb_tx_busy = false;
}

/***********************************************************************************************************************
 * 函数功能    : CDC 终端控制线状态变化回调
 * 说明(备注)  : 上位机打开/关闭/配置串口时触发，复位在途标志
 * 传入参数    : busid: 总线编号, intf: 接口编号, dtr: 数据终端就绪状态
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void usbd_cdc_acm_set_dtr(uint8_t busid, uint8_t intf, bool dtr)
{
    (void)busid;
    (void)intf;
    (void)dtr;
    s_b_usb_tx_busy = false;
}

/***********************************************************************************************************************
 * 函数功能    : 注册 CDC 数据接收回调
 * 说明(备注)  : 提供给外部应用直接获取接收数据流
 * 传入参数    : pfn_callback: 回调函数指针
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vUsbCdc_RegisterRxCallback(pfnUsbCdcRxCallback_T pfn_callback)
{
    s_pfn_rx_callback = pfn_callback;
}

/***********************************************************************************************************************
 * 函数功能    : 判断 USB 是否已完成枚举配置
 * 说明(备注)  : 返回 true 表示上位机已连接并完成枚举
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 已就绪, false: 未就绪
 ***********************************************************************************************************************/
bool bUsbCdc_IsConfigured(void)
{
    return s_b_usb_configured;
}

/***********************************************************************************************************************
 * 函数功能    : 判断 USB 发送端点是否处于繁忙状态
 * 说明(备注)  : 附带防死锁超时复位保护
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : true: 正在发送中, false: 空闲
 ***********************************************************************************************************************/
bool bUsbCdc_IsTxBusy(void)
{
    return s_b_usb_tx_busy;
}

/***********************************************************************************************************************
 * 函数功能    : USB CDC 定时节拍处理
 * 说明(备注)  : 在 SysTick (1ms) 中调用，看门狗超时监控发送繁忙状态
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vUsbCdc_Tick(void)
{
    static uint8_t s_uc_busy_ms = 0;

    if (s_b_usb_tx_busy)
    {
        s_uc_busy_ms++;
        if (s_uc_busy_ms >= 50)  /* 超过 50ms 强制解除 busy */
        {
            s_b_usb_tx_busy = false;
            s_uc_busy_ms = 0;
        }
    }
    else
    {
        s_uc_busy_ms = 0;
    }
}

/***********************************************************************************************************************
 * 函数功能    : 通过 USB CDC 虚拟串口发送数据
 * 说明(备注)  : 非阻塞发送，单包最大支持 usbCDC_TX_BUF_SIZE
 * 传入参数    : p_data: 发送数据指针, us_len: 发送长度
 * 输出参数    : 无
 * 返回值      : true: 成功提交发送, false: 发送失败或端点忙
 ***********************************************************************************************************************/
bool bUsbCdc_Send(const uint8_t *p_data, uint16_t us_len)
{
    if (!s_b_usb_configured || s_b_usb_tx_busy || p_data == NULL || us_len == 0)
    {
        return false;
    }

    if (us_len > usbCDC_TX_BUF_SIZE)
    {
        us_len = usbCDC_TX_BUF_SIZE;
    }

    memcpy(s_uca_cdc_tx_buff, p_data, us_len);
    s_b_usb_tx_busy = true;

    if (0 != usbd_ep_start_write(0, usbCDC_IN_EP, s_uca_cdc_tx_buff, us_len))
    {
        s_b_usb_tx_busy = false;
        return false;
    }

    return true;
}

/***********************************************************************************************************************
 * 函数功能    : 初始化 USB CDC 虚拟串口协议栈
 * 说明(备注)  : 完成描述符、接口、端点注册及 USB 控制器启动
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vUsbCdc_Init(void)
{
    usbd_desc_register(0, s_uca_cdc_descriptor);
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &s_t_intf0));
    usbd_add_interface(0, usbd_cdc_acm_init_intf(0, &s_t_intf1));
    usbd_add_endpoint(0, &s_t_cdc_out_ep);
    usbd_add_endpoint(0, &s_t_cdc_in_ep);

    usbd_initialize(0, USBFS_BASE, usbd_event_handler);
}

/***********************************************************************************************************************
 * 函数功能    : 去初始化 USB CDC 虚拟串口
 * 说明(备注)  : 关闭 USB 中断与外设时钟，重置状态标记
 * 传入参数    : 无
 * 输出参数    : 无
 * 返回值      : 无
 ***********************************************************************************************************************/
void vUsbCdc_DeInit(void)
{
    /* 完整去初始化: 软断开(SDIS)使主机看到干净断开,关内核中断,清FIFO,再关NVIC与时钟 */
    usb_dc_deinit(0);
    s_b_usb_configured = false;
    s_b_usb_tx_busy    = false;
}

#endif  /* boardUSB_EN || boardPRINT_IFACE == 7 */
