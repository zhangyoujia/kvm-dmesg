/* mem.h
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
#ifndef __MEM_H__
#define __MEM_H__

typedef struct {
    int mem_fd;
    uint64_t hva_base;
} proc_mem_t;

int mem_init(pid_t pid, uint64_t hva_base);
int mem_uninit();
int mem_read(uint64_t addr, void *buffer, size_t size);

#endif
