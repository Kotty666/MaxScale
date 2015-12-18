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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdint.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include "pp_log.h"
#include "pp_io.h"

using std::cerr;
using std::cout;
using std::endl;
using std::ios;
using std::ostream;
using std::string;
using std::vector;

namespace
{

#ifndef STRERROR_BUFLEN
const size_t STRERROR_BUFLEN = 256;
#endif
const size_t LISTEN_BACKLOG = 5;

const char USAGE[] = "usage: %s -p <port>\n";
const char SOCKET_PATH[] = "/tmp/plugin_process.sock";

struct OPTIONS
{
    OPTIONS()
        : socket_path(SOCKET_PATH)
        {}

    string socket_path;
};

void exit_with_usage(const char* name)
{
    fprintf(stderr, USAGE, name);
    exit(EXIT_FAILURE);
}


string strerror(int e)
{
    char buffer[STRERROR_BUFLEN];

    return strerror_r(e, buffer, STRERROR_BUFLEN);
}

}

namespace
{

void serve_client(int sd)
{
    bool stop = false;

    while (!stop)
    {
        void* pdata;
        uint32_t len;

        if (pp_recv(sd, &pdata, &len) == 0)
        {
            if (pp_send(sd, pdata, len) == 0)
            {
                MXS_NOTICE("Received %u, \tsend %u.", len, len);
            }
            else
            {
                MXS_ERROR("Could not send data to client, bailing out.");
                stop = true;
            }
        }
        else
        {
            MXS_ERROR("Could not receive data from client, bailing out.");
            stop = true;
        }
    }
}

void* client_thread(void* parg)
{
    cout << "Client thread!" << endl;

    int* psd = static_cast<int*>(parg);
    int sd = *psd;
    delete psd;

    try
    {
        serve_client(sd);
    }
    catch (const std::exception& x)
    {
        MXS_ERROR("Serving client failed with exception: %s", x.what());
    }

    close(sd);

    return 0;
}

int accept_client(int sfd)
{
    MXS_NOTICE("Starting accept loop.");

    bool ok = true;

    while (ok)
    {
        sockaddr_un address;
        socklen_t len;

        MXS_NOTICE("Accepting");
        int sd = accept(sfd, reinterpret_cast<sockaddr*>(&address), &len);
        MXS_NOTICE("Accepted");

        if (sd != -1)
        {
            pthread_t thread;
            int* parg = new int(sd);

            int error = pthread_create(&thread, 0, client_thread, parg);

            if (error)
            {
                MXS_ERROR("Failed to create a client thread: %s", strerror(error).c_str());
                ok = false;
            }
        }
        else
        {
            MXS_ERROR("Could not accept client: %s", strerror(errno).c_str());
            ok = false;
        }
    }

    // TODO: Clean up existing threads.

    MXS_NOTICE("Leaving accept loop.");

    return -1;
}

int run(const OPTIONS& options)
{
    int rc = -1;
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);

    if (sfd != -1)
    {
        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strcpy(addr.sun_path, options.socket_path.c_str());

        unlink(SOCKET_PATH);

        if (bind(sfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != -1)
        {
            if (listen(sfd, LISTEN_BACKLOG) != -1)
            {
                rc = accept_client(sfd);
            }
            else
            {
                MXS_ERROR("Could not listen on socket: %s", strerror(errno).c_str());
            }
        }
        else
        {
            MXS_ERROR("Could not bind to socket: %s", strerror(errno).c_str());
        }

        close(sfd);
    }
    else
    {
        MXS_ERROR("Could not create socket: %s", strerror(errno).c_str());
    }

    return rc;
}

}

int main(int argc, char* argv[])
{
    ios::sync_with_stdio();

    OPTIONS options;
    int opt;

    while ((opt = getopt(argc, argv, "s:")) != -1)
    {
        switch (opt)
        {
        case 's':
            if (strlen(optarg) > sizeof(sockaddr_un::sun_path) - 1)
            {
                cerr << "error: Too long socket path: " << optarg << endl;
                exit(EXIT_FAILURE);
            }

            options.socket_path = optarg;
            break;

        default:
            exit_with_usage(argv[0]);
        }
    }

    int rc = -1;

    try
    {
        rc = run(options);
    }
    catch (const std::exception& x)
    {
        cerr << "error: Unhandled exception caught: " << x.what() << endl;
    }

    return (rc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
