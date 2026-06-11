/**
 * \file            lwrb.c
 * \brief           轻量级环形缓冲区
 */

/*
 * Copyright (c) 2024 Tilen MAJERLE
 *
 * 特此授予任何获得本软件及相关文档文件（以下简称“软件”）副本的人
 * 免费许可，在不受限制的情况下处理本软件，包括但不限于使用、复制、修改、合并、
 * 出版、分发、再许可和/或出售软件副本的权利，
 * 以及允许向其提供软件的人做出上述行为，
 * 但须符合以下条件：
 *
 * 上述版权声明和本许可声明应包含在软件的所有副本或主要部分中。
 *
 * 本软件按“原样”提供，不提供任何形式的保证，
 * 无论是明示的还是暗示的，包括但不限于适销性、
 * 特定用途适用性和非侵权性的保证。在任何情况下，作者或版权持有人
 * 均不对任何索赔、损害或其他责任承担责任，
 * 无论是在合同诉讼、侵权行为或其他方面，
 * 源于、基于或与软件或软件的使用或其他交易有关。
 *
 * 本文件是LwRB（轻量级环形缓冲区库）的一部分。
 *
 * 作者：Tilen MAJERLE <tilen@majerle.eu>
 * 版本：v3.2.0
 */
#include "lwrb.h"

/* 内存设置和复制函数 */
#define BUF_MEMSET      memset
#define BUF_MEMCPY      memcpy

#define BUF_IS_VALID(b) ((b) != NULL && (b)->buff != NULL && (b)->size > 0)
#define BUF_MIN(x, y)   ((x) < (y) ? (x) : (y))
#define BUF_MAX(x, y)   ((x) > (y) ? (x) : (y))
#define BUF_SEND_EVT(b, type, bp)                                                                                      \
    do {                                                                                                               \
        if ((b)->evt_fn != NULL) {                                                                                     \
            (b)->evt_fn((void*)(b), (type), (bp));                                                                     \
        }                                                                                                              \
    } while (0)



/**
 * \brief           使用大小和缓冲区数据数组将缓冲区句柄初始化为默认值
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       buffdata: 用作缓冲区数据的内存指针
 * \param[in]       size: `buffdata`的大小（以字节为单位）
 *                      缓冲区可容纳的最大字节数为`size - 1`
 * \return          成功返回`1`，否则返回`0`
 */
uint8_t
lwrb_init(lwrb_t* buff, void* buffdata, lwrb_sz_t size) {
    if (buff == NULL || buffdata == NULL || size == 0) {
        return 0;
    }

    buff->evt_fn = NULL;
    buff->size = size;
    buff->buff = buffdata;
    LWRB_INIT(buff->w_ptr, 0);
    LWRB_INIT(buff->r_ptr, 0);
    return 1;
}

/**
 * \brief           检查缓冲区是否已初始化并可以使用
 * \param[in]       buff: 环形缓冲区实例
 * \return          就绪返回`1`，否则返回`0`
 */
uint8_t
lwrb_is_ready(lwrb_t* buff) {
    return BUF_IS_VALID(buff);
}

/**
 * \brief           释放缓冲区内存
 * \note            由于实现不使用动态分配，
 *                  只需将缓冲区句柄设置为`NULL`
 * \param[in]       buff: 环形缓冲区实例
 */
void
lwrb_free(lwrb_t* buff) {
    if (BUF_IS_VALID(buff)) {
        buff->buff = NULL;
    }
}

/**
 * \brief           为不同的缓冲区操作设置事件函数回调
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       evt_fn: 回调函数
 */
void
lwrb_set_evt_fn(lwrb_t* buff, lwrb_evt_fn evt_fn) {
    if (BUF_IS_VALID(buff)) {
        buff->evt_fn = evt_fn;
    }
}

/**
 * \brief           设置自定义缓冲区参数，可在事件函数中检索
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       arg: 自定义用户参数
 */
void
lwrb_set_arg(lwrb_t* buff, void* arg) {
    if (BUF_IS_VALID(buff)) {
        buff->arg = arg;
    }
}

/**
 * \brief           获取自定义缓冲区参数，先前通过\ref lwrb_set_arg设置
 * \param[in]       buff: 环形缓冲区实例
 * \return          先前通过\ref lwrb_set_arg设置的用户参数
 */
void*
lwrb_get_arg(lwrb_t* buff) {
    return buff != NULL ? buff->arg : NULL;
}

/**
 * \brief           向缓冲区写入数据。
 *                  将数据从`data`数组复制到缓冲区，并将写指针推进最多`btw`个字节。
 * 
 *                  如果缓冲区中可用内存较少，则复制的字节数较少。
 *                  用户必须检查函数的返回值并将其与
 *                  请求的写入长度进行比较，以确定是否已写入所有内容
 * 
 * \note            更高级的用法请使用\ref lwrb_write_ex
 *
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       data: 要写入缓冲区的数据指针
 * \param[in]       btw: 要写入的字节数
 * \return          写入缓冲区的字节数。
 *                      当返回值小于`btw`时，没有足够的内存可用
 *                      来复制完整的数据数组。
 */
lwrb_sz_t
lwrb_write(lwrb_t* buff, const void* data, lwrb_sz_t btw) {
    lwrb_sz_t written = 0;

    if (lwrb_write_ex(buff, data, btw, &written, 0)) {
        return written;
    }
    return 0;
}

/**
 * \brief           扩展写入功能
 * 
 * \param           buff: 环形缓冲区实例
 * \param           data: 要写入缓冲区的数据指针
 * \param           btw: 要写入的字节数
 * \param           bwritten: 输出指针，用于写入写入缓冲区的字节数
 * \param           flags: 可选标志。
 *                      \ref LWRB_FLAG_WRITE_ALL: 请求写入所有数据（最多btw）。
 *                          如果没有可用内存，将提前返回
 * \return          写入操作成功返回`1`，否则返回`0`
 */
uint8_t
lwrb_write_ex(lwrb_t* buff, const void* data, lwrb_sz_t btw, lwrb_sz_t* bwritten, uint16_t flags) {
    lwrb_sz_t tocopy = 0, free = 0, w_ptr = 0;
    const uint8_t* d_ptr = data;

    if (!BUF_IS_VALID(buff) || data == NULL || btw == 0) {
        return 0;
    }

    /* 计算可写入的最大字节数 */
    free = lwrb_get_free(buff);
    /* 如果没有内存，或者用户想要写入所有数据但空间不足，提前退出 */
    if (free == 0 || (free < btw && (flags & LWRB_FLAG_WRITE_ALL))) {
        return 0;
    }
    btw = BUF_MIN(free, btw);
    w_ptr = LWRB_LOAD(buff->w_ptr, memory_order_acquire);

    /* 步骤1：将数据写入缓冲区的线性部分 */
    tocopy = BUF_MIN(buff->size - w_ptr, btw);
    BUF_MEMCPY(&buff->buff[w_ptr], d_ptr, tocopy);
    d_ptr += tocopy;
    w_ptr += tocopy;
    btw -= tocopy;

    /* 步骤2：将数据写入缓冲区的开头（溢出部分） */
    if (btw > 0) {
        BUF_MEMCPY(buff->buff, d_ptr, btw);
        w_ptr = btw;
    }

    /* 步骤3：检查缓冲区末尾 */
    if (w_ptr >= buff->size) {
        w_ptr = 0;
    }

    /*
     * 将最终值写入实际运行的变量。
     * 这是为了确保没有读取操作可以访问中间数据
     */
    LWRB_STORE(buff->w_ptr, w_ptr, memory_order_release);

    BUF_SEND_EVT(buff, LWRB_EVT_WRITE, tocopy + btw);
    if (bwritten != NULL) {
        *bwritten = tocopy + btw;
    }
    return 1;
}

/**
 * \brief           从缓冲区读取数据。
 *                  将数据从`data`数组复制到缓冲区，并将读指针推进最多`btr`个字节。
 * 
 *                  如果缓冲区中的可用数据较少，则复制的字节数较少。
 * 
 * \note            更高级的用法请使用\ref lwrb_read_ex
 *
 * \param[in]       buff: 环形缓冲区实例
 * \param[out]      data: 用于复制缓冲区数据的输出内存指针
 * \param[in]       btr: 要读取的字节数
 * \return          读取并复制到数据数组的字节数
 */
lwrb_sz_t
lwrb_read(lwrb_t* buff, void* data, lwrb_sz_t btr) {
    lwrb_sz_t read = 0;

    if (lwrb_read_ex(buff, data, btr, &read, 0)) {
        return read;
    }
    return 0;
}

/**
 * \brief           扩展读取功能
 * 
 * \param           buff: 环形缓冲区实例
 * \param           data: 用于写入从缓冲区读取的数据的内存指针
 * \param           btr: 要读取的字节数
 * \param           bread: 输出指针，用于写入从缓冲区读取并写入到
 *                      输出`data`变量的字节数
 * \param           flags: 可选标志
 *                      \ref LWRB_FLAG_READ_ALL: 请求读取所有数据（最多btr）。
 *                          如果缓冲区中没有足够的字节，将提前返回
 * \return          读取操作成功返回`1`，否则返回`0`
 */
uint8_t
lwrb_read_ex(lwrb_t* buff, void* data, lwrb_sz_t btr, lwrb_sz_t* bread, uint16_t flags) {
    lwrb_sz_t tocopy = 0, full = 0, r_ptr = 0;
    uint8_t* d_ptr = data;

    if (!BUF_IS_VALID(buff) || data == NULL || btr == 0) {
        return 0;
    }

    /* 计算可读取的最大字节数 */
    full = lwrb_get_full(buff);
    if (full == 0 || (full < btr && (flags & LWRB_FLAG_READ_ALL))) {
        return 0;
    }
    btr = BUF_MIN(full, btr);
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_acquire);

    /* 步骤1：从缓冲区的线性部分读取数据 */
    tocopy = BUF_MIN(buff->size - r_ptr, btr);
    BUF_MEMCPY(d_ptr, &buff->buff[r_ptr], tocopy);
    d_ptr += tocopy;
    r_ptr += tocopy;
    btr -= tocopy;

    /* 步骤2：从缓冲区的开头读取数据（溢出部分） */
    if (btr > 0) {
        BUF_MEMCPY(d_ptr, buff->buff, btr);
        r_ptr = btr;
    }

    /* 步骤3：检查缓冲区末尾 */
    if (r_ptr >= buff->size) {
        r_ptr = 0;
    }

    /*
     * 将最终值写入实际运行的变量。
     * 这是为了确保没有写入操作可以访问中间数据
     */
    LWRB_STORE(buff->r_ptr, r_ptr, memory_order_release);

    BUF_SEND_EVT(buff, LWRB_EVT_READ, tocopy + btr);
    if (bread != NULL) {
        *bread = tocopy + btr;
    }
    return 1;
}

/**
 * \brief           从缓冲区读取而不改变读指针（仅预览）
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       skip_count: 读取数据前要跳过的字节数
 * \param[out]      data: 用于复制缓冲区数据的输出内存指针
 * \param[in]       btp: 要预览的字节数
 * \return          预览并写入输出数组的字节数
 */
lwrb_sz_t
lwrb_peek(const lwrb_t* buff, lwrb_sz_t skip_count, void* data, lwrb_sz_t btp) {
    lwrb_sz_t full = 0, tocopy = 0, r_ptr = 0;
    uint8_t* d_ptr = data;

    if (!BUF_IS_VALID(buff) || data == NULL || btp == 0) {
        return 0;
    }

    /*
     * 计算可读取的最大字节数
     * 并检查是否可以容纳
     */
    full = lwrb_get_full(buff);
    if (skip_count >= full) {
        return 0;
    }
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);
    r_ptr += skip_count;
    full -= skip_count;
    if (r_ptr >= buff->size) {
        r_ptr -= buff->size;
    }

    /* 检查跳过之后可读取的最大字节数 */
    btp = BUF_MIN(full, btp);
    if (btp == 0) {
        return 0;
    }

    /* 步骤1：从缓冲区的线性部分读取数据 */
    tocopy = BUF_MIN(buff->size - r_ptr, btp);
    BUF_MEMCPY(d_ptr, &buff->buff[r_ptr], tocopy);
    d_ptr += tocopy;
    btp -= tocopy;

    /* 步骤2：从缓冲区的开头读取数据（溢出部分） */
    if (btp > 0) {
        BUF_MEMCPY(d_ptr, buff->buff, btp);
    }
    return tocopy + btp;
}

/**
 * \brief           获取缓冲区中可用于写入操作的可用大小
 * \param[in]       buff: 环形缓冲区实例
 * \return          内存中的空闲字节数
 */
lwrb_sz_t
lwrb_get_free(const lwrb_t* buff) {
    lwrb_sz_t size = 0, w_ptr = 0, r_ptr = 0;

    if (!BUF_IS_VALID(buff)) {
        return 0;
    }

    /*
     * 通过原子访问将缓冲区指针复制到局部变量。
     *
     * 为确保线程安全（仅在单入口、单出口FIFO模式用例中），
     * 重要的是将缓冲区r和w值写入局部w和r变量。
     *
     * 局部变量将确保下面的if语句将始终使用相同的值，
     * 即使buff->w或buff->r在中断处理期间发生变化。
     *
     * 它们可能在加载操作期间发生变化，重要的是
     * 在这些赋值之后的if-else操作期间它们不会变化。
     *
     * lwrb_get_free仅用于写入目的，当处于FIFO模式时：
     * - buff->w指针不会被另一个进程/中断更改，因为我们现在处于写入模式
     * - buff->r指针可能被另一个进程更改。如果在buff->r已加载到局部变量后更改，
     *    缓冲区将看到“空闲大小”小于实际大小。这不是问题，应用程序可以
     *    再次尝试将更多数据写入在复制操作期间刚刚读取的剩余空闲内存中
     */
    w_ptr = LWRB_LOAD(buff->w_ptr, memory_order_relaxed);
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);

    if (w_ptr >= r_ptr) {
        size = buff->size - (w_ptr - r_ptr);
    } else {
        size = r_ptr - w_ptr;
    }

    /* 缓冲区空闲大小总是比实际大小小1 */
    return size - 1;
}

/**
 * \brief           获取缓冲区中当前可用的字节数
 * \param[in]       buff: 环形缓冲区实例
 * \return          准备好读取的字节数
 */
lwrb_sz_t
lwrb_get_full(const lwrb_t* buff) {
    lwrb_sz_t size = 0, w_ptr = 0, r_ptr = 0;

    if (!BUF_IS_VALID(buff)) {
        return 0;
    }

    /*
     * 将缓冲区指针复制到局部变量。
     *
     * 为确保线程安全（仅在单入口、单出口FIFO模式用例中），
     * 重要的是将缓冲区r和w值写入局部w和r变量。
     *
     * 局部变量将确保下面的if语句将始终使用相同的值，
     * 即使buff->w或buff->r在中断处理期间发生变化。
     *
     * 它们可能在加载操作期间发生变化，重要的是
     * 在这些赋值之后的if-else操作期间它们不会变化。
     *
     * lwrb_get_full仅用于读取目的，当处于FIFO模式时：
     * - buff->r指针不会被另一个进程/中断更改，因为我们现在处于读取模式
     * - buff->w指针可能被另一个进程更改。如果在buff->w已加载到局部变量后更改，
     *    缓冲区将看到“已满大小”小于实际大小。这不是问题，应用程序可以
     *    再次尝试从在复制操作期间刚刚写入的剩余已满内存中读取更多数据
     */
    w_ptr = LWRB_LOAD(buff->w_ptr, memory_order_relaxed);
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);

    if (w_ptr >= r_ptr) {
        size = w_ptr - r_ptr;
    } else {
        size = buff->size - (r_ptr - w_ptr);
    }
    return size;
}

/**
 * \brief           将缓冲区重置为默认值。缓冲区大小不会修改
 * \note            此函数不是线程安全的。
 *                      使用时，应用程序必须确保没有活动的读/写操作
 * \param[in]       buff: 环形缓冲区实例
 */
void
lwrb_reset(lwrb_t* buff) {
    if (BUF_IS_VALID(buff)) {
        LWRB_STORE(buff->w_ptr, 0, memory_order_release);
        LWRB_STORE(buff->r_ptr, 0, memory_order_release);
        BUF_SEND_EVT(buff, LWRB_EVT_RESET, 0);
    }
}

/**
 * \brief           获取缓冲区的线性地址以进行快速读取
 * \param[in]       buff: 环形缓冲区实例
 * \return          线性缓冲区起始地址
 */
void*
lwrb_get_linear_block_read_address(const lwrb_t* buff) {
    lwrb_sz_t ptr = 0;

    if (!BUF_IS_VALID(buff)) {
        return NULL;
    }
    ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);
    return &buff->buff[ptr];
}

/**
 * \brief           获取读取操作溢出前的线性块地址长度
 * \param[in]       buff: 环形缓冲区实例
 * \return          用于读取操作的线性缓冲区大小（以字节为单位）
 */
lwrb_sz_t
lwrb_get_linear_block_read_length(const lwrb_t* buff) {
    lwrb_sz_t len = 0, w_ptr = 0, r_ptr = 0;

    if (!BUF_IS_VALID(buff)) {
        return 0;
    }

    /*
     * 在操作期间使用临时值以防它们被更改。
     * 有关为什么这样做可行的更多信息，请参见lwrb_buff_free或lwrb_buff_full函数。
     */
    w_ptr = LWRB_LOAD(buff->w_ptr, memory_order_relaxed);
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);

    if (w_ptr > r_ptr) {
        len = w_ptr - r_ptr;
    } else if (r_ptr > w_ptr) {
        len = buff->size - r_ptr;
    } else {
        len = 0;
    }
    return len;
}

/**
 * \brief           跳过（忽略；推进读指针）缓冲区数据
 * 将数据标记为已在缓冲区中读取，并最多增加`len`字节的空闲内存
 *
 * \note            在流式传输（如DMA）结束时很有用
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       len: 要跳过并标记为已读取的字节数
 * \return          跳过的字节数
 */
lwrb_sz_t
lwrb_skip(lwrb_t* buff, lwrb_sz_t len) {
    lwrb_sz_t full = 0, r_ptr = 0;

    if (!BUF_IS_VALID(buff) || len == 0) {
        return 0;
    }

    full = lwrb_get_full(buff);
    len = BUF_MIN(len, full);
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_acquire);
    r_ptr += len;
    if (r_ptr >= buff->size) {
        r_ptr -= buff->size;
    }
    LWRB_STORE(buff->r_ptr, r_ptr, memory_order_release);
    BUF_SEND_EVT(buff, LWRB_EVT_READ, len);
    return len;
}

/**
 * \brief           获取缓冲区的线性地址以进行快速写入
 * \param[in]       buff: 环形缓冲区实例
 * \return          线性缓冲区起始地址
 */
void*
lwrb_get_linear_block_write_address(const lwrb_t* buff) {
    lwrb_sz_t ptr = 0;

    if (!BUF_IS_VALID(buff)) {
        return NULL;
    }
    ptr = LWRB_LOAD(buff->w_ptr, memory_order_relaxed);
    return &buff->buff[ptr];
}

/**
 * \brief           获取写入操作溢出前的线性块地址长度
 * \param[in]       buff: 环形缓冲区实例
 * \return          用于写入操作的线性缓冲区大小（以字节为单位）
 */
lwrb_sz_t
lwrb_get_linear_block_write_length(const lwrb_t* buff) {
    lwrb_sz_t len = 0, w_ptr = 0, r_ptr = 0;

    if (!BUF_IS_VALID(buff)) {
        return 0;
    }

    /*
     * 在操作期间使用临时值以防它们被更改。
     * 有关为什么这样做可行的更多信息，请参见lwrb_buff_free或lwrb_buff_full函数。
     */
    w_ptr = LWRB_LOAD(buff->w_ptr, memory_order_relaxed);
    r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);

    if (w_ptr >= r_ptr) {
        len = buff->size - w_ptr;
        /*
         * 当读指针为0时，
         * 最大长度少1，因为如果写入太多字节，
         * 缓冲区将再次被视为空（r == w）
         */
        if (r_ptr == 0) {
            /*
             * 不会溢出：
             * - 如果r不为0，则不调用该语句
             * - buff->size不能为0，并且如果r为0，则len大于0
             */
            --len;
        }
    } else {
        len = r_ptr - w_ptr - 1;
    }
    return len;
}

/**
 * \brief           推进缓冲区中的写指针。
 * 类似于跳过函数，但修改写指针而不是读指针
 *
 * \note            当硬件正在写入缓冲区并且应用程序需要增加
 *                      硬件写入缓冲区的字节数时很有用
 * \param[in]       buff: 环形缓冲区实例
 * \param[in]       len: 要推进的字节数
 * \return          为写入操作推进的字节数
 */
lwrb_sz_t
lwrb_advance(lwrb_t* buff, lwrb_sz_t len) {
    lwrb_sz_t free = 0, w_ptr = 0;

    if (!BUF_IS_VALID(buff) || len == 0) {
        return 0;
    }

    /* 在写回主结构之前使用局部变量 */
    free = lwrb_get_free(buff);
    len = BUF_MIN(len, free);
    w_ptr = LWRB_LOAD(buff->w_ptr, memory_order_acquire);
    w_ptr += len;
    if (w_ptr >= buff->size) {
        w_ptr -= buff->size;
    }
    LWRB_STORE(buff->w_ptr, w_ptr, memory_order_release);
    BUF_SEND_EVT(buff, LWRB_EVT_WRITE, len);
    return len;
}


