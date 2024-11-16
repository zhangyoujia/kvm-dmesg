/* printk.h
 *
 * Copyright (C) 2024 Ray Lee
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PRINTK_H__
#define __PRINTK_H__

#include <stdint.h>

struct log {
    uint64_t ts_nsec;
    uint16_t len;
    uint16_t text_len;
    uint16_t dict_len;
    uint8_t facility;
    uint8_t flags:5;
    uint8_t level:3;
};

typedef struct {
    long long counter;
} atomic_long_t;

struct dev_printk_info {
    char subsystem[16];
    char device[48];
};

struct printk_info {
    uint64_t seq;
    uint64_t ts_nsec;
    uint16_t text_len;
    uint8_t facility;
    uint8_t flags : 5;
    uint8_t level : 3;
    uint32_t caller_id;
    struct dev_printk_info dev_info;
};

struct prb_data_blk_lpos {
    unsigned long begin;
    unsigned long next;
};

struct prb_desc {
    atomic_long_t state_var;
    struct prb_data_blk_lpos text_blk_lpos;
};

struct prb_desc_ring {
    unsigned int count_bits;
    struct prb_desc *descs;
    struct printk_info *infos;
    atomic_long_t head_id;
    atomic_long_t tail_id;
};

struct prb_data_ring {
    unsigned int size_bits;
    char *data;
    atomic_long_t head_lpos;
    atomic_long_t tail_lpos;
};

struct printk_ringbuffer {
    struct prb_desc_ring desc_ring;
    struct prb_data_ring text_data_ring;
    atomic_long_t fail;
};

struct prb_map {
    char *prb;

    char *desc_ring;
    unsigned long desc_ring_count;
    char *descs;
    char *infos;

    char *text_data_ring;
    unsigned long text_data_ring_size;
    char *text_data;
};

void dump_lockless_record_log();

#endif
