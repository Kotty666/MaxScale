#ifndef PP_LOG_H
#define PP_LOG_H
/*
 * This file is distributed as part of the MariaDB Corporation MaxScale. It is free
 * software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright MariaDB Corporation Ab 2013-2015
 *
 */

#include <stdio.h>

#ifndef STRERROR_BUFLEN
#define STRERROR_BUFLEN 512
#endif

#define MXS_ERROR(format, ...)  fprintf(stderr, "error : " format "\n", ##__VA_ARGS__)
#define MXS_NOTICE(format, ...) fprintf(stderr, "notice: " format "\n", ##__VA_ARGS__)

#endif
