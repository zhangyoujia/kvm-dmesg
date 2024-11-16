/* version.c
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

#include "version.h"

#define vquote(x) #x
#define vexp(x) vquote(x)

#define VERSION_TEXT (vexp(MAJOR_VERSION) "." vexp(MINOR_VERSION) "." vexp(BUGFIX_VERSION) EXTRA_VERSION)
#define VERSION_ID ((MAJOR_VERSION) * 10000 + (MINOR_VERSION) * 100 + (BUGFIX_VERSION))

const char* get_version_text()
{
    return VERSION_TEXT;
}
