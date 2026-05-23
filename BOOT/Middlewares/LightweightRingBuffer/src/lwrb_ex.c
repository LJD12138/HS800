/**
 * \file            lwrb_ex.c
 * \brief           轻量级环形缓冲区 - 扩展函数
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

#if(LWRB_DEV)

/* 仅在开发模式启用时才编译 */

#define BUF_IS_VALID(b) ((b) != NULL && (b)->buff != NULL && (b)->size > 0)
#define BUF_MIN(x, y)   ((x) < (y) ? (x) : (y))

/**
 * \brief           向缓冲区写入数据，当没有足够空间容纳完整输入数据对象时使用覆盖功能。
 * \note            类似于\ref lwrb_write但会覆盖原有数据
 * \param[in]       buff: 缓冲区句柄
 * \param[in]       data: 要写入环形缓冲区的数据
 * \param[in]       btw: 要写入的字节数（Bytes To Write）
 * \return          写入缓冲区的字节数，始终返回btw
 * \note            功能主要分为两部分，总是先写入一些线性区域，然后
 *                      如果有更多数据要写入，则写入环绕区域。如果写指针超过读指针，读指针会被推进。
 *                      此操作既是读操作也是写操作。为保证线程安全，可能需要互斥锁，详见文档。
 */
lwrb_sz_t
lwrb_overwrite(lwrb_t* buff, const void* data, lwrb_sz_t btw) {
    lwrb_sz_t orig_btw = btw, max_cap;
    const uint8_t* d = data;

    if (!BUF_IS_VALID(buff) || data == NULL || btw == 0) {
        return 0;
    }

    /* 处理完整的输入数组 */
    max_cap = buff->size - 1; /* 缓冲区可容纳的最大容量 */
    if (btw > max_cap) {
        /*
         * 当要写入的数据大于缓冲区最大容量时，
         * 我们可以重置缓冲区并简单地写入输入缓冲区的最后部分。
         * 
         * 这里通过计算剩余长度，然后推进到输入缓冲区的末尾来实现
         */
        d += btw - max_cap; /* 推进数据指针 */
        btw = max_cap;      /* 限制要写入的数据量 */
        lwrb_reset(buff);   /* 重置缓冲区 */
    } else {
        /* 
         * 要写入的字节数小于容量
         * 我们最多需要执行一次跳过操作，
         * 但仅当空闲内存小于btw时才需要，否则跳过该操作
         * 只写入数据
         */
        lwrb_sz_t f = lwrb_get_free(buff);
        if (f < btw) {
            lwrb_skip(buff, btw - f);
        }
    }
    lwrb_write(buff, d, btw);
    return orig_btw;
}

/**
 * \brief           将一个环形缓冲区的数据移动到另一个，移动量不超过源缓冲区中的数据量或目标缓冲区中的空闲空间。
 * \param[in]       dest: 目标缓冲区句柄，复制的数据将写入其中
 * \param[in]       src:  源缓冲区句柄，复制的数据将来自其中。
 *                      操作完成后，源缓冲区的读指针会相应更新。
 * \return          写入目标缓冲区的字节数
 * \note            此操作对源缓冲区是读操作，成功时会更新读索引。
 *                  同时对目标缓冲区是写操作，可能会更新写索引。
 *                  为保证线程安全，可能需要互斥锁，详见文档。
 */
lwrb_sz_t
lwrb_move(lwrb_t* dest, lwrb_t* src) {
    lwrb_sz_t len_to_copy, len_to_copy_orig, src_full, dest_free;

    if (!BUF_IS_VALID(dest) || !BUF_IS_VALID(src)) {
        return 0;
    }
    src_full = lwrb_get_full(src);
    dest_free = lwrb_get_free(dest);
    len_to_copy = BUF_MIN(src_full, dest_free);
    len_to_copy_orig = len_to_copy;

    /* 已计算好可复制的长度。
        我们可以安全地假设循环内的操作会正确完成。 */
    while (len_to_copy > 0) {
        lwrb_sz_t max_seq_read, max_seq_write, op_len;
        const uint8_t* d_src;
        uint8_t* d_dst;

        /* 计算数据 */
        max_seq_read = lwrb_get_linear_block_read_length(src);
        max_seq_write = lwrb_get_linear_block_write_length(dest);
        op_len = BUF_MIN(max_seq_read, max_seq_write);
        op_len = BUF_MIN(len_to_copy, op_len);

        /* 获取地址 */
        d_src = lwrb_get_linear_block_read_address(src);
        d_dst = lwrb_get_linear_block_write_address(dest);

        /* 逐字节复制 */
        for (lwrb_sz_t i = 0; i < op_len; ++i) {
            *d_dst++ = *d_src++;
        }

        lwrb_advance(dest, op_len);
        lwrb_skip(src, op_len);
        len_to_copy -= op_len;
        if (op_len == 0) {
            /* 严重错误... */
            return 0;
        }
    }
    return len_to_copy_orig;
}

/**
 * \brief           从给定偏移量开始在数组中搜索*needle*。
 * 
 * \note            此函数不是线程安全的。 
 * 
 * \param           buff: 要在其中搜索needle的环形缓冲区
 * \param           bts: 要在缓冲区中搜索的常量字节数组序列
 * \param           len: \arg bts数组的长度
 * \param           start_offset: 缓冲区中的起始偏移量
 * \param           found_idx: 用于写入bts在数组中找到位置的变量指针
 *                      不能设置为`NULL`
 * \return          如果找到\arg bts返回`1`，否则返回`0`
 */
uint8_t
lwrb_find(const lwrb_t* buff, const void* bts, lwrb_sz_t len, lwrb_sz_t start_offset, lwrb_sz_t* found_idx) {
    lwrb_sz_t full = 0, r_ptr = 0, buff_r_ptr = 0, max_x = 0;
    uint8_t found = 0;
    const uint8_t* needle = bts;

    if (!BUF_IS_VALID(buff) || needle == NULL || len == 0 || found_idx == NULL) {
        return 0;
    }
    *found_idx = 0;

    full = lwrb_get_full(buff);
    /* 验证初始条件 */
    if (full < (len + start_offset)) {
        return 0;
    }

    /* 获取此搜索的实际缓冲区读指针 */
    buff_r_ptr = LWRB_LOAD(buff->r_ptr, memory_order_relaxed);

    /* for循环的最大次数是缓冲区已满大小 - 输入长度 - 缓冲区长度的起始偏移量 */
    max_x = full - len;
    for (lwrb_sz_t skip_x = start_offset; !found && skip_x <= max_x; ++skip_x) {
        found = 1; /* 默认找到 */

        /* 准备读取的起始点 */
        r_ptr = buff_r_ptr + skip_x;
        if (r_ptr >= buff->size) {
            r_ptr -= buff->size;
        }

        /* 在缓冲区中搜索 */
        for (lwrb_sz_t idx = 0; idx < len; ++idx) {
            if (buff->buff[r_ptr] != needle[idx]) {
                found = 0;
                break;
            }
            if (++r_ptr >= buff->size) {
                r_ptr = 0;
            }
        }
        if (found) {
            *found_idx = skip_x;
        }
    }
    return found;
}

/******************************************************************************
* 函 数 名 : lwrb_find_target_read
* 功    能 : 在缓存器中查找target字节,找到就读取needLen长度内容到outBuf中
* 参    数 : BuffHandler - 缓冲区句柄
*            target      - 要查找的字节
*            needLen     - 需读取字节数（含 target 本身）
*            outBuf      - 用户接收缓冲区（≥ needLen）
* 返 回 值 : 实际读取字节数（≤ needLen）；0 表示未找到或长度不足
******************************************************************************/
uint32_t lwrb_find_target_read(lwrb_t *BuffHandler,
                                uint8_t  target,
                                uint32_t needLen,
                                uint8_t *outBuf)
{
    if (BuffHandler == NULL || BuffHandler->buff == NULL ||
        outBuf == NULL || needLen == 0)
        return 0;

    uint32_t l_total_len = lwrb_get_full(BuffHandler);
    uint32_t l_buff_handle_size = BuffHandler->size;
    if (l_total_len == 0) return 0;

    /* 顺序查找 target */
    for (uint32_t i = 0; i < l_total_len; i++)
    {
        uint32_t l_index = (BuffHandler->r_ptr + i) % l_buff_handle_size;
		
        if (BuffHandler->buff[l_index] == target)
        {
            /* 检查剩余长度 */
            uint32_t target_after_len = l_total_len - i;
            if (target_after_len < needLen)
                return 0;

            /* -------- 1. 读取数据 -------- */
			//目标到数组最后的长度
            uint32_t l_target_to_end_len = l_buff_handle_size - l_index;
            if (l_target_to_end_len >= needLen)
            {
                memcpy(outBuf, &BuffHandler->buff[l_index], needLen);
            }
            else
            {
                memcpy(outBuf, &BuffHandler->buff[l_index], l_target_to_end_len);
                memcpy(outBuf + l_target_to_end_len, BuffHandler->buff, needLen - l_target_to_end_len);
            }

            /* -------- 2. 处理剩余数据 -------- */
			uint32_t l_remove_index = (l_index + needLen) % l_buff_handle_size;
            uint32_t l_remain_to_end_len = l_buff_handle_size - l_index;
            uint32_t l_remain_after_len = target_after_len - needLen;

            /* -------- 3. 移动剩余数据（使用 memmove） -------- */
            if (l_remain_to_end_len >= l_remain_after_len && l_remove_index > l_index)
            {
				if (l_remain_after_len)
					memmove(&BuffHandler->buff[l_index],
							&BuffHandler->buff[l_remove_index],
							l_remain_after_len);
				
				BuffHandler->w_ptr = ((l_buff_handle_size + BuffHandler->w_ptr) - needLen) % l_buff_handle_size;
            }
            else
            {
				if (l_remain_after_len)
				{
					memmove(&BuffHandler->buff[l_index],
							&BuffHandler->buff[l_remove_index],
							l_remain_to_end_len);
					
					memmove(BuffHandler->buff,
							&BuffHandler->buff[l_remove_index + l_remain_to_end_len],
							l_remain_after_len - l_remain_to_end_len);
					
					BuffHandler->w_ptr = ((l_buff_handle_size + l_remain_after_len - l_remain_to_end_len)) % l_buff_handle_size;
				}
				else
					BuffHandler->w_ptr = ((l_buff_handle_size + l_remain_after_len - l_remain_to_end_len) - 1 ) % l_buff_handle_size;
            }
            return needLen;
        }
    }

    return 0;
}

/**************************************************************************************
函数名   ：ZH_Buff_IsFull
功能     ：查Buffer是否已经存满
输入参数 ：*pBuffer：被查的Buffer对象
返回值   ：false代表未存满，true代表存满
完成时间 ：2017.09.01
作者     ：Kaven
版本     ：第一版
修改时间 ：
***************************************************************************************/
#if(LWRB_TEST)
static uint8_t uca_data[20] = {0};
static lwrb_t t_buff_handler;
void lwrb_test(void)
{
	lwrb_init(&t_buff_handler, uca_data, sizeof(uca_data));
	
	uint8_t data[10]={1,2,3,4,5,6,7,8,9,10};
	uint8_t data3[4]={11,12,13,14};
	uint8_t data2[10]={0};
	
	lwrb_write(&t_buff_handler, data, sizeof(data));

	
	memset(data2, 0, sizeof(data2));
	lwrb_find_target_read(&t_buff_handler, 5, 2, data2);
	
	lwrb_write(&t_buff_handler, data3, sizeof(data3));
	
	memset(data2, 0, sizeof(data2));
	lwrb_read(&t_buff_handler, data2, 10);
	
	lwrb_write(&t_buff_handler, data, sizeof(data));
	
	memset(data2, 0, sizeof(data2));
	lwrb_read(&t_buff_handler, data2, 5);
	
	lwrb_write(&t_buff_handler, data3, sizeof(data3));
	
	memset(data2, 0, sizeof(data2));
	lwrb_find_target_read(&t_buff_handler, 9, 2, data2);
}
#endif  //LWRB_TEST

#endif /* defined(LWRB_DEV) */
