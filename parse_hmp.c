/* parse_hmp.c
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
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>

static char* find_last_occurrence(const char *buf, const char *str) {
    char *last_occurrence = NULL;
    const char *current_pos = buf;

    while ((current_pos = strstr(current_pos, str)) != NULL) {
        last_occurrence = (char *)current_pos;
        current_pos++;
    }

    return last_occurrence;
}

int hmp_gpa2hva(const char *buf, uint64_t *hva)
{
    char *hva_str = find_last_occurrence(buf, "0x");
    if (!hva_str) {
        hva_str = find_last_occurrence(buf, "0X");
    }

    if (hva_str) {
        hva_str += 2;
        *hva = strtoul(hva_str, NULL, 16);
        return 0;
    }
    return -1;
}
