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