/* xutil.h
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

#ifndef __XUTIL_H__
#define __XUTIL_H__

#include <stddef.h>
#include <sys/types.h>

#ifndef offsetof
#  define offsetof(TYPE, MEMBER) ((unsigned long)&((TYPE *)0)->MEMBER)
#endif

#define streq(s1, s2) (!strcmp((s1), (s2)))

#define strcaseeq(s1, s2) (!strcasecmp((s1), (s2)))

void *xmalloc(unsigned int size);
void *xcalloc(unsigned int numb, unsigned int size);
void *xrealloc(void *old, unsigned int size);
void xfree(void *ptr);
size_t xread(int fd, void *buf, size_t size);
size_t xwrite(int fd, const char *buf, size_t size);
void xskipwhitespace(const char *str);
char *xstrdup(const char *s);
char *xstrcpy(char *dst, const char *str);
size_t xstrlcpy(char *dst, const char *str, size_t len);
unsigned long int xstroul(const char *str, char **end, int base);
size_t to_bytes(unsigned char *d, const char *str, int base_from);
off_t get_file_len(const char *fn);
int file_read(const char *fn, char **dst, size_t *flen);
void daemonize(void);
void xsetnonblock(int fd);
int xset_tcp_reuseaddr(int fd);
int xset_tcp_keepalive(int fd);
int xset_tcp_nodelay(int fd, int val);
int xenable_tcp_nodelay(int fd);
int xdisable_tcp_nodelay(int fd);

#endif
