/* libvirt_client.c
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
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <string.h>
#include <fnmatch.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "xutil.h"
#include "defs.h"
#include "log.h"
#include "client.h"

typedef enum {
    VIR_DOMAIN_QEMU_MONITOR_COMMAND_DEFAULT = 0,
    VIR_DOMAIN_QEMU_MONITOR_COMMAND_HMP     = (1 << 0), /* cmd is in HMP */
} virDomainQemuMonitorCommandFlags;

typedef void* virDomainPtr;
typedef void* virConnectPtr;

virConnectPtr (*virConnectOpen)(const char *name);
int (*virConnectClose)(virConnectPtr conn);
virDomainPtr (*virDomainLookupByName)(virConnectPtr conn, const char *name);
int (*virDomainFree)(virDomainPtr domain);
int (*virDomainQemuMonitorCommand)(virDomainPtr domain, const char *cmd, char **result, unsigned int flags);

void *libvirt_handle = NULL;
void *libvirt_qemu_handle = NULL;
virDomainPtr domain = NULL;
virConnectPtr domain_conn = NULL;

FILE *mem_file = NULL;

#define CHECK_FUNC(f) if (!f) { pr_err("Error loading function: %s\n", dlerror()); return -1; }

static int libvirt_dlopen()
{
    libvirt_handle = dlopen("libvirt.so.0", RTLD_NOW);
    if (!libvirt_handle) {
        pr_err("Error loading libvirt.so.0, %s\n", dlerror());
        return -1;
    }

    libvirt_qemu_handle = dlopen("libvirt-qemu.so.0", RTLD_NOW);
    if (!libvirt_qemu_handle) {
        pr_err("Error loading libvirt-qemu.so.0, %s\n", dlerror());
        dlclose(libvirt_handle);
        libvirt_handle = NULL;
        return -1;
    }

    return 0;
}

static int libvirt_dlsym()
{
    virConnectOpen = dlsym(libvirt_handle, "virConnectOpen");
    virConnectClose = dlsym(libvirt_handle, "virConnectClose");
    virDomainLookupByName = dlsym(libvirt_handle, "virDomainLookupByName");
    virDomainFree = dlsym(libvirt_handle, "virDomainFree");
    virDomainQemuMonitorCommand = dlsym(libvirt_qemu_handle, "virDomainQemuMonitorCommand");

    CHECK_FUNC(virConnectOpen);
    CHECK_FUNC(virConnectClose);
    CHECK_FUNC(virDomainLookupByName);
    CHECK_FUNC(virDomainFree);
    CHECK_FUNC(virDomainQemuMonitorCommand);

    return 0;
}

int libvirt_client_init(char *guest_name)
{
    if (libvirt_dlopen()) {
        return -1;
    }

    if (libvirt_dlsym()) {
        return -1;
    }


    if (!domain_conn)
        domain_conn = virConnectOpen("qemu:///system");

    if (!domain_conn) {
        pr_err("Failed to open connection to qemu:///system");
        return -1;
    }

    if (!domain)
        domain = virDomainLookupByName(domain_conn, guest_name);

    if (!domain) {
        pr_err("Failed to find the domain: %s", guest_name);
        virConnectClose(domain_conn);
        domain_conn = NULL;
        return -1;
    }

    return 0;
}

int libvirt_client_uninit()
{
    if (domain) {
        virDomainFree(domain);
        domain = NULL;
    }

    if (domain_conn) {
        virConnectClose(domain_conn);
        domain_conn = NULL;
    }

    if (libvirt_handle) {
        dlclose(libvirt_handle);
        libvirt_handle = NULL;
    }

    if (libvirt_qemu_handle) {
        dlclose(libvirt_qemu_handle);
        libvirt_qemu_handle = NULL;
    }

    return 0;
}

static unsigned long get_line_value(const char *line, const char *key)
{
    const char *start = strstr(line, key);
    if (start) {
        start += strlen(key);
        while (*start == ' ' || *start == '=') start++;
        return strtoul(start, NULL, 16);
    }
    return 0;
}

int libvirt_get_registers(uint64_t *idtr, uint64_t *cr3, uint64_t *cr4)
{
    virDomainQemuMonitorCommandFlags flag = VIR_DOMAIN_QEMU_MONITOR_COMMAND_HMP;
    char *hmp_response;
    char hmp_command[64] = {0};

    snprintf(hmp_command, sizeof(hmp_command), "info registers");
    if (virDomainQemuMonitorCommand(domain, hmp_command, &hmp_response, flag) < 0) {
        pr_err("Failed to send QMP command: %s", hmp_command);
        return -1;
    }

    char *line = strtok(hmp_response, "\n");
    while (line != NULL) {
        if (strstr(line, "IDT")) {
            *idtr = get_line_value(line, "IDT");
        }

        if (strstr(line, "CR3")) {
            *cr3 = get_line_value(line, "CR3");
        }

        line = strtok(NULL, "\n");  // Next line
    }
    *cr4 = 0;

    free(hmp_response);
    return 0;
}

static void* libvirt_dump_phy_memory(uint64_t start_addr, ssize_t size)
{
    void *buffer = NULL;
    char *hmp_response;
    virDomainQemuMonitorCommandFlags flag = VIR_DOMAIN_QEMU_MONITOR_COMMAND_HMP;
    char hmp_command[64] = {0};

    size = roundup(size, 4);
    buffer = malloc(1024 + size * 2);

    // https://qemu-project.gitlab.io/qemu/system/monitor.html
    snprintf(hmp_command, sizeof(hmp_command), "xp /%" PRIu64 "xw 0x%zx", size / 4, start_addr);
    if (virDomainQemuMonitorCommand(domain, hmp_command, &hmp_response, flag) < 0) {
        pr_err("Failed to send QMP command: %s", hmp_command);
        return NULL;
    }

    uint32_t values[4];
    uint32_t *data = (uint32_t *)buffer;
    int i = 0;
    char *line = strtok(hmp_response, "\n");
    while (line != NULL) {
        int num = sscanf(line, "%*s 0x%x 0x%x 0x%x 0x%x",
                &values[0], &values[1], &values[2], &values[3]);
        for (int j = 0; j < num && j < (int)(sizeof(values) / sizeof(values[0])); j++) {
            data[i++] = values[j];
        }
        line = strtok(NULL, "\n");  // Next line
    }

    free(hmp_response);

    return buffer;
}

int libvirt_readmem_part(uint64_t addr, uint8_t *buffer, size_t size)
{
    uint8_t *buf = libvirt_dump_phy_memory(addr, size);
    if (!buf) {
        return -1;
    }
    memcpy(buffer, buf, size);
    free(buf);
    return 0;
}

int libvirt_readmem(uint64_t addr, void *buffer, size_t size)
{
    int step = 4096;
    uint8_t *buf = (uint8_t *)buffer;
    size_t left = size % step;

    for (size_t i = 0; i < size / step; i++) {
        if (libvirt_readmem_part(addr, buf, step) != 0) {
            return -1;
        }
        addr += step;
        buf += step;
    }

    if (left > 0) {
        if (libvirt_readmem_part(addr, buf, left) != 0) {
            return -1;
        }
    }

    return 0;
}

int file_client_init(char *path)
{
    if (mem_file)
        return 0;

    mem_file = fopen(path, "rb");
    if (!mem_file) {
        pr_err("fopen error");
        return -1;
    }
    return 0;
}

int file_client_uninit( )
{
    if (mem_file) {
        fclose(mem_file);
    }
    mem_file = NULL;
    return 0;
}

int file_readmem(uint64_t addr, void *buffer, size_t size)
{
    if (fseek(mem_file, addr, SEEK_SET) != 0) {
        pr_err("fseek error");
        return -1;
    }

    size_t bytesRead = fread(buffer, 1, size, mem_file);

    if (bytesRead != size) {
        if (feof(mem_file)) {
            printf("eof to read: %zu\n", bytesRead);
            return bytesRead;
        } else {
            pr_err("read bytes error");
            return -1;
        }
    }

    return 0;
}

int file_get_registers(uint64_t *idtr, uint64_t *cr3, uint64_t *cr4)
{
    *cr3 = 0x0000000019872000;
    *idtr = 0xffffffffff528000;
    *cr4 = 0;
    return 0;
}
