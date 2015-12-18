#ifndef PP_IO_H
#define PP_IO_H
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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int pp_send(int sd, const void* pdata, uint32_t len);

int pp_recv(int sd, void** ppdata, uint32_t* plen);

#ifdef __cplusplus
}
#endif

#endif
