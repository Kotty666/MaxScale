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

#include "pp_io.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "pp_log.h"

namespace
{

#ifndef STRERROR_BUFLEN
const size_t STRERROR_BUFLEN = 256;
#endif

const size_t DEFAULT_READ_BUFSIZE = 1024;

int pp_write(int sd, const void* pdata, uint32_t len)
{
    uint32_t n_written = 0;
    ssize_t sz = 0;
    const char* p = static_cast<const char*>(pdata);

    do
    {
        sz = write(sd, p, len - n_written);

        if (sz >= 0)
        {
            p += sz;
            n_written += sz;
        }
    }
    while ((sz > 0) && (n_written != len));

    return sz < 0 ? sz : n_written;
}

int pp_receive(int sd, char* pdata, uint32_t bufsize, void** ppdata, uint32_t* plen)
{
    int rv = -1;

    char errbuf[STRERROR_BUFLEN];
    char* p = pdata;
    size_t n_read = 0;

    const uint32_t header_len = sizeof(uint32_t);
    uint32_t data_len = 0;
    ssize_t sz;

    do
    {
        sz = read(sd, p, bufsize - n_read);

        if (sz > 0)
        {
            p += sz;
            n_read += sz;
        }
    }
    while ((sz > 0) && (n_read < header_len));

    if (sz < 0)
    {
        MXS_ERROR("Failed when reading header from server: %s", strerror_r(errno, errbuf, sizeof(errbuf)));
    }
    else if (n_read < header_len)
    {
        MXS_ERROR("Could not read header from server, needed %u, got %lu.", header_len, n_read);
    }
    else
    {
        data_len = ntohl(*(uint32_t*) pdata);
        uint32_t message_len = header_len + data_len;

        if (n_read < message_len)
        {
            char* tmp = static_cast<char*>(realloc(pdata, message_len));

            if (tmp)
            {
                pdata = tmp;
                p = pdata + n_read;

                uint32_t n_remaining = message_len - n_read;

                do
                {
                    sz = read(sd, p, n_remaining);

                    if (sz > 0)
                    {
                        p += sz;
                        n_remaining -= sz;
                    }
                }
                while ((sz > 0) && (n_remaining > 0));

                if (sz < 0)
                {
                    MXS_ERROR("Failed when reading data from server: %s",
                              strerror_r(errno, errbuf, sizeof(errbuf)));
                }
                else if (n_remaining > 0)
                {
                    MXS_ERROR("Could not read data from server, needed %u, got %u.",
                              data_len, data_len - n_remaining);
                }
                else
                {
                    rv = 0;
                }
            }
            else
            {
                MXS_ERROR("Could not allocate memory.");
            }
        }
        else if (n_read > message_len)
        {
            MXS_ERROR("Received more data %lu than expected %u.", n_read, message_len);
        }
        else
        {
            rv = 0;
        }
    }

    if (rv == 0)
    {
        *ppdata = pdata;
        *plen = data_len;
    }
    else
    {
        free(pdata);
    }

    return rv;
}

}


extern int pp_send(int sd, const void* pdata, uint32_t len)
{
    int rv = -1;
    char errbuf[STRERROR_BUFLEN];
    uint32_t dlen = htonl(len);

    // Not that efficient to write with two writes, but this is for worstcase anyway.
    ssize_t sz = pp_write(sd, &dlen, sizeof(dlen));

    if (sz < 0)
    {
        MXS_ERROR("Failed when writing header to plugin process: %s",
                  strerror_r(errno, errbuf, sizeof(errbuf)));
    }
    else if (sz < static_cast<ssize_t>(sizeof(dlen)))
    {
        MXS_ERROR("Failed to write complete header; attempted: %lu, wrote: %ld.",
                  sizeof(uint32_t), sz);
    }
    else
    {
        sz = pp_write(sd, pdata, len);

        if (sz < 0)
        {
            MXS_ERROR("Failed when writing data to plugin process: %s",
                      strerror_r(errno, errbuf, sizeof(errbuf)));
        }
        else if (sz < len)
        {
            MXS_ERROR("Failed to write all data; attempted: %u, wrote: %ld.", len, sz);
        }
        else
        {
            rv = 0;
        }
    }

    return rv;
}

int pp_recv(int sd, void** ppdata, uint32_t* plen)
{
    int rv = -1;

    size_t bufsize = DEFAULT_READ_BUFSIZE;
    char* pdata = static_cast<char*>(malloc(bufsize));

    if (pdata)
    {
        rv = pp_receive(sd, pdata, bufsize, ppdata, plen);
    }
    else
    {
        MXS_ERROR("Could not allocate memory.");
    }

    return rv;
}
