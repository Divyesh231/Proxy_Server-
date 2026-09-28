#include "proxy.h"
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

static int connect_destination(const char *host, const char *port)
{
    struct addrinfo hints, *list = NULL, *item;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hints, &list) != 0)
        return -1;

    int fd = -1;

    for (item = list; item; item = item->ai_next) {
        fd = socket(item->ai_family, item->ai_socktype, item->ai_protocol);

        if (fd < 0)
            continue;

        int flags = fcntl(fd, F_GETFL, 0);

        if (flags < 0 ||
            fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            close(fd);
            fd = -1;
            continue;
        }

        int result = connect(
            fd,
            item->ai_addr,
            item->ai_addrlen
        );

        if (result < 0 && errno == EINPROGRESS) {
            fd_set write_set;
            FD_ZERO(&write_set);
            FD_SET(fd, &write_set);

            struct timeval connect_timeout = {5, 0};

            result = select(
                fd + 1,
                NULL,
                &write_set,
                NULL,
                &connect_timeout
            );

            if (result > 0) {
                int socket_error = 0;
                socklen_t error_length = sizeof(socket_error);

                if (getsockopt(
                        fd,
                        SOL_SOCKET,
                        SO_ERROR,
                        &socket_error,
                        &error_length
                    ) < 0 ||
                    socket_error != 0) {
                    result = -1;
                } else {
                    result = 0;
                }
            } else {
                result = -1;
            }
        }

        if (result == 0 && fcntl(fd, F_SETFL, flags) == 0)
            break;

        close(fd);
        fd = -1;
    }

    freeaddrinfo(list);

    if (fd >= 0) {
        struct timeval t = {15, 0};

        setsockopt(
            fd,
            SOL_SOCKET,
            SO_RCVTIMEO,
            &t,
            sizeof(t)
        );

        setsockopt(
            fd,
            SOL_SOCKET,
            SO_SNDTIMEO,
            &t,
            sizeof(t)
        );
    }

    return fd;
}

static int send_all(int fd, const char *p, size_t n)
{
    size_t done = 0;

    while (done < n) {
        ssize_t sent = send(fd, p + done, n - done, 0);

        if (sent <= 0)
            return -1;

        done += (size_t)sent;
    }

    return 0;
}

static void tunnel(int client, int remote)
{
    char b[RESPONSE_SIZE];

    for (;;) {
        fd_set set;

        FD_ZERO(&set);
        FD_SET(client, &set);
        FD_SET(remote, &set);

        int maxfd = client > remote ? client : remote;

        if (select(
                maxfd + 1,
                &set,
                NULL,
                NULL,
                NULL
            ) <= 0)
            break;

        int src = FD_ISSET(client, &set) ? client : remote;
        int dst = src == client ? remote : client;

        ssize_t n = recv(src, b, sizeof(b), 0);

        if (n <= 0 ||
            send_all(dst, b, (size_t)n) != 0)
            break;
    }
}