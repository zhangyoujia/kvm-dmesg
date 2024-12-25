/* mem.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "log.h"
#include "xutil.h"
#include "mem.h"

static proc_mem_t *proc_mem = NULL;

int mem_init(pid_t pid, uint64_t hva_base)
{
    int fd;
    char mem_path[32];
    if (proc_mem && proc_mem->mem_fd > 0)
        return 0;

    snprintf(mem_path, sizeof(mem_path), "/proc/%d/mem", pid);
    fd = open(mem_path, O_RDONLY);
    if (fd == -1) {
        pr_err("Failed to open /proc/%d/mem", pid);
        return -1;
    }

    if (!proc_mem) {
        proc_mem = (proc_mem_t *)xmalloc(sizeof(proc_mem_t));
    }

    proc_mem->mem_fd = fd;
    proc_mem->hva_base = hva_base;

    return 0;
}

int mem_uninit()
{
    if (!proc_mem) {
        return 0;
    }
    if (proc_mem->mem_fd > 0) {
        close(proc_mem->mem_fd);
    }
    xfree(proc_mem);
    return 0;
}

int mem_read(uint64_t addr, void *buffer, size_t size)
{
    if (!proc_mem || proc_mem->mem_fd <= 0)
        return -1;

    if (lseek(proc_mem->mem_fd, proc_mem->hva_base + addr, SEEK_SET) == -1) {
        pr_err("Failed to seek to the specified memory address");
        return -1;
    }

    ssize_t bytes_read = xread(proc_mem->mem_fd, buffer, size);
    if (bytes_read == -1) {
        pr_err("Failed to read memory");
        return -1;
    }

    return 0;
}
