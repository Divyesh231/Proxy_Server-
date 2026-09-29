#include "proxy.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

static void trim(char *text) {
    char *start = text;
    while (isspace((unsigned char)*start)) start++;
    if (start != text) memmove(text, start, strlen(start) + 1);
    size_t length = strlen(text);
    while (length && isspace((unsigned char)text[length - 1])) text[--length] = '\0';
}
static int copy_part(char *dst, size_t cap, const char *src, size_t n) 
{ 
  if (!n || n >= cap)
    return -1; 
  memcpy(dst, src, n);
  dst[n] = '\0'; 
  return 0; 
}
static int parse_authority(char *authority, HttpRequest *request,
                           const char *default_port)
{
    if (!authority[0] || strchr(authority, '@'))
        return -1;

    char *port = NULL;

    if (authority[0] == '[') {
        char *close = strchr(authority, ']');

        if (!close)
            return -1;

        if (close[1] == ':') {
            port = close + 2;
            *close = '\0';

            memmove(authority, authority + 1, strlen(authority));
        }
        else if (close[1] == '\0') {
            *close = '\0';

            memmove(authority, authority + 1, strlen(authority));
        }
        else {
            return -1;
        }
    }
    else {
        char *colon = strrchr(authority, ':');

        if (colon && strchr(authority, ':') == colon) {
            *colon = '\0';
            port = colon + 1;
        }
    }

    if (copy_part(request->host,
                  sizeof(request->host),
                  authority,
                  strlen(authority)) != 0)
        return -1;

    if (!port || !*port)
        port = (char *)default_port;

    char *end = NULL;
    long value = strtol(port, &end, 10);

    if (!*port || !end || *end || value < 1 || value > 65535)
        return -1;

    snprintf(request->port,
             sizeof(request->port),
             "%ld",
             value);

    return 0;
}
int parse_http_request(const char *raw_request, HttpRequest *request) {
    memset(request, 0, sizeof(*request));

    char method[16], target[4096], version[16];
    const char *line_end = strstr(raw_request, "\r\n");

    if (!line_end)
        return -1;

    size_t line_len = (size_t)(line_end - raw_request);

    if (line_len >= 4200)
        return -1;

    char line[4200];
    memcpy(line, raw_request, line_len);
    line[line_len] = '\0';

    if (sscanf(line, "%15s %4095s %15s", method, target, version) != 3 ||
        strncmp(version, "HTTP/1.", 7) != 0)
        return -1;

    snprintf(request->method, sizeof(request->method), "%s", method);

    if (strcmp(method, "CONNECT") == 0) {
        char authority[512];

        if (copy_part(authority, sizeof(authority), target, strlen(target)) != 0)
            return -1;

        request->is_connect = 1;
        return parse_authority(authority, request, "443");
    }

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
        return -2;

    if (strncmp(target, "http://", 7) == 0) {
        const char *start = target + 7;
        const char *path = strpbrk(start, "/?");
        size_t auth_len = path ? (size_t)(path - start) : strlen(start);

        char authority[512];

        if (copy_part(authority, sizeof(authority), start, auth_len) != 0 ||
            parse_authority(authority, request, "80") != 0)
            return -1;

        if (!path)
            strcpy(request->path, "/");
        else if (*path == '?')
            snprintf(request->path, sizeof(request->path), "/%s", path);
        else
            snprintf(request->path, sizeof(request->path), "%s", path);

    } else if (target[0] == '/') {
        snprintf(request->path, sizeof(request->path), "%s", target);

        const char *cursor = line_end + 2;
        int found = 0;

        while (*cursor && !(cursor[0] == '\r' && cursor[1] == '\n')) {
            const char *end = strstr(cursor, "\r\n");

            if (!end)
                return -1;

            if ((size_t)(end - cursor) >= 5 &&
                strncasecmp(cursor, "Host:", 5) == 0) {

                char authority[512];

                if (copy_part(
                        authority,
                        sizeof(authority),
                        cursor + 5,
                        (size_t)(end - cursor - 5)) != 0)
                    return -1;

                trim(authority);

                if (parse_authority(authority, request, "80") != 0)
                    return -1;

                found = 1;
                break;
            }

            cursor = end + 2;
        }

        if (!found)
            return -1;

    } else {
        return -1;
    }

    return request->host[0] && request->path[0] ? 0 : -1;
}