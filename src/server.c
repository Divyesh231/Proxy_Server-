#include "proxy.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct { 
    int fd;
    char address[INET6_ADDRSTRLEN]; 
} ClientInfo;

static int contains_ci(const char *text, const char *needle) {
    size_t n = strlen(needle);
    for (const char *p = text; *p; p++) if (strncasecmp(p, needle, n) == 0) return 1;
    return 0;
}

static int send_error(
    int fd,
    int status,
    const char *reason,
    const char *message
)
{
    char response[512];

    int size = snprintf(
        response,
        sizeof(response),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: %zu\r\n"
        "\r\n"
        "%s",
        status,
        reason,
        strlen(message),
        message
    );

    return size > 0
        ? (int)send(fd, response, (size_t)size, 0)
        : -1;
}

static void *handle_client(void *argument) {
    ClientInfo *client = argument;
    int fd = client->fd;

    char client_address[INET6_ADDRSTRLEN];
    snprintf(client_address, sizeof(client_address), "%s", client->address);
    free(client);

    struct timeval timeout = {15, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    char raw[REQUEST_SIZE + 1];
    size_t used = 0;

    while (used < REQUEST_SIZE) {
        ssize_t n = recv(fd, raw + used, REQUEST_SIZE - used, 0);

        if (n <= 0)
            break;

        used += (size_t)n;
        raw[used] = '\0';

        if (strstr(raw, "\r\n\r\n"))
            break;
    }

    raw[used] = '\0';

    if (!strstr(raw, "\r\n\r\n")) {
        send_error(
            fd,
            used >= REQUEST_SIZE ? 431 : 408,
            used >= REQUEST_SIZE ? "Request Header Fields Too Large" : "Request Timeout",
            "Incomplete or oversized request headers"
        );

        log_event(
            "BAD_REQUEST",
            client_address,
            "-",
            "Incomplete or oversized request"
        );

        close(fd);
        return NULL;
    }

    HttpRequest request;
    int parsed = parse_http_request(raw, &request);

    if (parsed != 0) {
        send_error(
            fd,
            parsed == -2 ? 501 : 400,
            parsed == -2 ? "Not Implemented" : "Bad Request",
            parsed == -2 ? "This proxy supports GET, HEAD, and CONNECT" : "Malformed HTTP request"
        );

        log_event(
            "BAD_REQUEST",
            client_address,
            "-",
            "Malformed or unsupported request"
        );

        close(fd);
        return NULL;
    }

    if (is_blocked(request.host)) {
        send_error(
            fd,
            403,
            "Forbidden",
            "Destination blocked by proxy policy"
        );

        log_event(
            "BLOCKED",
            client_address,
            request.host,
            request.path
        );

        close(fd);
        return NULL;
    }