/* log.h
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

#ifndef __LOG_H__
#define __LOG_H__

#define LOGLEVEL_ERROR      0
#define LOGLEVEL_WARNING    1
#define LOGLEVEL_INFO       2
#define LOGLEVEL_DEBUG      3
#define LOGLEVEL_MAX        4

void log_init(int level);

void pr_err(const char *err, ...);
void pr_warning(const char *err, ...);
void pr_info(const char *err, ...);
void __pr_debug(const char *debug, ...);
#define pr_debug(fmt, ...)                      \
    do {                                \
        __pr_debug("(%s) %s:%d: " fmt, __FILE__,    \
            __func__, __LINE__, ##__VA_ARGS__); \
    } while (0)

#endif
