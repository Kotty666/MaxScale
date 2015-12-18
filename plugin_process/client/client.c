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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "pp_io.h"
#include "pp_log.h"


void run(int sd, uint32_t min_len, uint32_t max_len, int count)
{
    MXS_NOTICE("Writing %d messages between %u and %u.", count, min_len, max_len);

    for (int i = 0; i < count; ++i)
    {
        uint32_t req_len = min_len + ((double) random() / RAND_MAX) * (max_len - min_len);

        char* prequest = malloc(req_len);

        if (prequest)
        {
            for (uint32_t j = 0; j < req_len; ++j)
            {
                prequest[j] = j;
            }

            if (pp_send(sd, prequest, req_len) == 0)
            {
                char* presponse;
                uint32_t res_len;

                if (pp_recv(sd, (void**) &presponse, &res_len) == 0)
                {
                    MXS_NOTICE("Sent %u, \treceived %u.", req_len, res_len);

                    free(presponse);
                }
                else
                {
                    MXS_ERROR("Could not receive response.");
                }
            }
            else
            {
                MXS_ERROR("Could not send request.");
            }

            free(prequest);
        }
        else
        {
            MXS_ERROR("Could not allocated %d bytes.", req_len);
        }
    }
}

int main(int argc, char* argv[])
{
    char buf[STRERROR_BUFLEN];
    const char* socket_path = getenv("PP_SOCKET_PATH");

    if (socket_path)
    {
        int sd = socket(AF_UNIX, SOCK_STREAM, 0);

        if (sd != -1)
        {
            struct sockaddr_un address;
            memset(&address, 0, sizeof(address));
            address.sun_family = AF_UNIX;
            strncpy(address.sun_path, socket_path, sizeof(address.sun_path) - 1);
            address.sun_path[sizeof(address.sun_path) - 1] = 0;

            if (connect(sd, (struct sockaddr*) &address, sizeof(address)) == 0)
            {
                MXS_NOTICE("Connected to plugin process. Note, there'll be notice per thread.");

                run(sd, 10240, 2560000, 20);
                close(sd);
            }
            else
            {
                MXS_ERROR("Could not connect to plugin process. "
                          "Restart of MaxScale is needed for recovery.");
                close(sd);
            }
        }
        else
        {
            MXS_ERROR("Could not create plugin process socket: %s. "
                      "Restart of MaxScale is needed for recovery.",
                      strerror_r(errno, buf, sizeof(buf)));
        }
    }
    else
    {
        MXS_NOTICE("No PP_SOCKET_PATH environment variable defined. "
                   "Plugin process behaviour will not be simulated. "
                   "Restart of MaxScale is needed for recovery.");
    }
}
