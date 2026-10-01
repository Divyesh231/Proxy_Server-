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