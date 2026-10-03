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

#if defined(LWRB_DEV)

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

#endif /* defined(LWRB_DEV) */
