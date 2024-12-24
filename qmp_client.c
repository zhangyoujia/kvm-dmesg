/* qmp_client.h
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
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>
#include <stdint.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "xutil.h"
#include "log.h"

static int qmp_fd;

#define MAX_PATH_LEN 512

#define QMP_GREETING            "{\"QMP\":"
#define QMP_ENTER_COMMAND_MODE  "{ \"execute\": \"qmp_capabilities\" }"
#define QMP_COMMAND_MODE_OK     "{\"return\": {}}\r\n"

#define QMP_COMMAND_INFO_REGS   "{\"execute\": \"human-monitor-command\", \"arguments\": {\"command-line\": \"info registers\"}}"
#define QMP_COMMAND_XP          "{\"execute\": \"human-monitor-command\", \"arguments\": {\"command-line\": \"xp /%" PRIu64 "xb 0x%zx\"}}"

static char* get_absolute_path(const char *file_path)
{
    char *abs_path = (char *)xmalloc(MAX_PATH_LEN);
    if (!abs_path) {
        return NULL;
    }

    if (realpath(file_path, abs_path) == NULL) {
        xfree(abs_path);
        return NULL;
    }

    return abs_path;
}

static ino_t get_inode_from_socket(const char *socket_path)
{
    char line[MAX_PATH_LEN];
    char inode[16];
    char *path = (char *)xmalloc(MAX_PATH_LEN);
    ino_t ino = -1;

    if (!path) {
        return -1;
    }

    char *abs_path = get_absolute_path(socket_path);
    if (!abs_path) {
        xfree(path);
        return -1;
    }

    FILE *fp = fopen("/proc/net/unix", "r");
    if (!fp) {
        perror("Failed to open /proc/net/unix");
        xfree(path);
        xfree(abs_path);
        return -1;
    }

    // Skip the first line (headers)
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%*s %*s %*s %*s %*s %*s %s %s", inode, path) != 2) {
            continue;
        }

        if (strcmp(abs_path, path) == 0) {
            ino = atoi(inode);
            break;
        }
    }

    fclose(fp);
    xfree(path);
    xfree(abs_path);
    return ino;
}

static pid_t find_pid_by_inode(ino_t target_inode)
{
    DIR *dir;
    struct dirent *entry;
    char *path = (char *)xmalloc(MAX_PATH_LEN);
    char *fd_target = (char *)xmalloc(MAX_PATH_LEN * 2);
    pid_t pid = -1;

    if (!path || !fd_target) {
        xfree(path);
        xfree(fd_target);
        return -1;
    }

    dir = opendir("/proc");
    if (dir == NULL) {
        perror("Failed to opendir /proc");
        xfree(path);
        xfree(fd_target);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        snprintf(path, MAX_PATH_LEN, "/proc/%s/fd", entry->d_name);

        DIR *fd_dir = opendir(path);
        if (!fd_dir)
            continue;

        char pid_dname[MAX_PATH_LEN] = {0};
        snprintf(pid_dname, sizeof(pid_dname), "%s", entry->d_name);

        while ((entry = readdir(fd_dir)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;

            snprintf(fd_target, MAX_PATH_LEN * 2, "/proc/%s/fd/%s", pid_dname, entry->d_name);

            char target[MAX_PATH_LEN];
            ssize_t len = readlink(fd_target, target, MAX_PATH_LEN - 1);
            if (len == -1) {
                continue;
            }
            target[len] = '\0';

            if (strncmp(target, "socket:[", 8) == 0) {
                ino_t inode = atoi(&target[8]);

                if (inode == target_inode) {
                    pid = atoi(pid_dname);
                    break;
                }
            }
        }
        closedir(fd_dir);
        if (pid != -1)
            break;
    }

    closedir(dir);
    xfree(path);
    xfree(fd_target);
    return pid;
}

pid_t qmp_get_pid(char *sock_path)
{
    ino_t inode = get_inode_from_socket(sock_path);
    if (inode <= 0) {
        return -1;
    }

    return find_pid_by_inode(inode);
}

static int qmp_read(int fd, void *buf, size_t *len)
{
    struct pollfd pfd;
    size_t tread = 0, nread;
    int r;

    pfd.fd = fd;
    pfd.events = POLLIN;

    while ((r = poll(&pfd, 1, 5)) != 0) {

        if (r == -1)
            return -1;

        if (pfd.revents & POLLIN) {
            /* read at max 1k at a time */
            nread = xread(fd, buf, 1024);
            tread += nread;
            buf += nread;
        }
    }

    *len = tread;
    return 0;
}

static int qmp_establish_conn(char *sock_path)
{
    int s;
    struct sockaddr_un saddr;
    size_t path_len;
    size_t nread;

    char buf[512];

    path_len = strlen(sock_path);
    if (path_len == 0) {
        return -1;
    }

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        return -1;
    }

    memset(&saddr, 0, sizeof(struct sockaddr_un));
    saddr.sun_family = AF_UNIX;

    memcpy(saddr.sun_path, sock_path, path_len);
    saddr.sun_path[path_len] = '\0';

    qmp_fd = s;

    /* connect */
    if (connect(qmp_fd, (struct sockaddr *) &saddr,
                sizeof(struct sockaddr_un)) == -1) {
        pr_err("Failed to connect to '%s' ('%s')", sock_path, strerror(errno));
        close(qmp_fd);

        return -1;
    }

    xsetnonblock(qmp_fd);

    memset(buf, 0, sizeof(buf));

    if (qmp_read(qmp_fd, buf, &nread) == -1) {
        return -1;
    }

    if (nread == 0 && strncasecmp(buf, QMP_GREETING, strlen(QMP_GREETING))) {
        pr_err("Failed to get QMP greeting message");
        return -1;
    }

    return 0;
}

static int qmp_negotiate()
{
    size_t cmd_len, nwrite, nread;
    char buf[512];


    cmd_len = strlen(QMP_ENTER_COMMAND_MODE);
    nwrite = xwrite(qmp_fd, QMP_ENTER_COMMAND_MODE, cmd_len);
    if (nwrite == 0) {
        goto err_exit;
    }

    memset(buf, 0, sizeof(buf));
    if (qmp_read(qmp_fd, buf, &nread) == -1) {
        goto err_exit;
    }

    if (strncasecmp(buf, QMP_COMMAND_MODE_OK, nread)) {
        goto err_exit;
    }

    return 0;

err_exit:
    pr_err("Failed to enter in command mode");
    return -1;
}

int qmp_client_init(char *sock_path)
{
    int r;

    r = qmp_establish_conn(sock_path);
    if (r == -1) {
        pr_err("Unable to talk to qemu monitor");
        goto err_exit;
    }


    r = qmp_negotiate();
    if (r == -1) {
        pr_err("Failed to negotiate qmp commands");
        goto err_exit;
    }

    return 0;

err_exit:
	return -1;
}

int qmp_client_uninit()
{
    if (close(qmp_fd) == -1) {
        return -1;
    }

    return 0;
}

/*
 * json looks like
 *
 * {"return": "RAX=ffffffff8101c9a0 RBX=ffffffff818e2880
 *      RCX=ffffffff818550e0 RDX=0000000000000000\r\n
 */
static int qmp_populate_reg(const char *buf, const char *reg_name, uint64_t *reg)
{
	char *start;
	start = strstr(buf, reg_name);

	if (!start) {
		return -1;
	}

	while (start && *start) {
		if (*start == '=') {
			start++;
			break;
		}
		start++;
	}

	if (sscanf(start, "%lx", reg) != 1) {
		return -1;
	}

	return 0;
}

int qmp_get_registers(uint64_t *idtr, uint64_t *cr3, uint64_t *cr4)
{
	size_t nread, nwrite, cmd_len_regs;
	char *buf;

	cmd_len_regs = strlen(QMP_COMMAND_INFO_REGS);

	nwrite = xwrite(qmp_fd, QMP_COMMAND_INFO_REGS, cmd_len_regs);
	if (nwrite != cmd_len_regs) {
		return -1;
	}

	buf = xmalloc(8192);

	if (qmp_read(qmp_fd, buf, &nread) == -1 || nread <= 0) {
		pr_err("Failed to get read");
		goto err_exit;
	}

	if (qmp_populate_reg(buf, "CR3", cr3) == -1) {
		pr_err("Failed to get register CR3");
		goto err_exit;
	}

	if (qmp_populate_reg(buf, "CR4", cr4) == -1) {
		pr_err("Failed to get register CR4");
		goto err_exit;
	}

	if (qmp_populate_reg(buf, "IDT", idtr) == -1) {
		pr_err("Failed to get register IDT");
		goto err_exit;
	}

	xfree(buf);
	return 0;

err_exit:
    xfree(buf);
	return -1;
}

int qmp_populate_mem(char *input, size_t len, uint8_t *buffer, size_t size)
{
    char line[128];
    int line_index = 0;
    uint8_t values[8] = {0};
    size_t pos = 0;

    const char *return_start = "\"return\": \"";
    char *start = strstr(input, return_start);

    if (start == NULL) {
        pr_err("Can not found start");
        return -1;
    }
    start += strlen(return_start);

    while (*start != '\"' && start < input + len && pos < size) {
        if (*start == '\\' && *(start + 1) == 'r') {
            line[line_index] = '\0';

            int num = sscanf(line, "%*s 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx 0x%hhx",
                    &values[0], &values[1],
                    &values[2], &values[3],
                    &values[4], &values[5],
                    &values[6], &values[7]);
            for (int i = 0 ; i < num; i++) {
                buffer[pos++] = values[i];
            }

            line_index = 0;
            start += 2;
        } else if (*start == '\\' && *(start + 1) == 'n') {
            start += 2;
        } else {
            line[line_index++] = *start;
            start++;
        }
    }

    return 0;
}

static int qmp_readmem_part(uint64_t addr, uint8_t *buffer, size_t size)
{
	size_t nread, nwrite, cmd_len_regs;
    char cmd[256] = {0};
	char *buf;

    if (size <= 0) {
        pr_warning("Invalid input size: %ld", size);
        return size;
    }

    snprintf(cmd, sizeof(cmd), QMP_COMMAND_XP, size, addr);
	cmd_len_regs = strlen(cmd);

	nwrite = xwrite(qmp_fd, cmd, cmd_len_regs);
	if (nwrite != cmd_len_regs) {
		return -1;
	}

    size_t buf_len = 1024 + size * 8;
    buf= xmalloc(buf_len);

    if (qmp_read(qmp_fd, buf, &nread) == -1 || nread <= 0) {
		pr_err("Failed to get read");
		goto err_exit;
	}

    qmp_populate_mem(buf, buf_len, buffer, size);

    xfree(buf);
	return 0;

err_exit:
    xfree(buf);
	return -1;
}

int qmp_readmem(uint64_t addr, void *buffer, size_t size)
{
    int step = 4096;
    uint8_t *buf = (uint8_t *)buffer;
    size_t left = size % step;

    for (size_t i = 0; i < size / step; i++) {
        if (qmp_readmem_part(addr, buf, step) != 0) {
            return -1;
        }
        addr += step;
        buf += step;
    }

    if (left > 0) {
        if (qmp_readmem_part(addr, buf, left) != 0) {
            return -1;
        }
    }

    return 0;
}
