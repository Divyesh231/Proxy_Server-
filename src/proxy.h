#ifndef PROXY_H
#define PROXY_H
#include <stddef.h>
#define PROXY_PORT 8080
#define REQUEST_SIZE 16384
#define RESPONSE_SIZE 8192
#define MAX_CACHEABLE_RESPONSE (512u * 1024u)
typedef struct { char method[16]; char host[256]; char port[6]; char path[4096]; int is_connect; } HttpRequest;
int parse_http_request(const char *raw_request, HttpRequest *request);
int forward_request(int client_fd, const char *raw_request, const HttpRequest *request);
int cache_get(const char *key, char **data, size_t *length);
void cache_put(const char *key, const char *data, size_t length);
void cache_free_copy(char *data);
int is_blocked(const char *host);
void log_event(const char *event, const char *client, const char *host, const char *detail);
#endif
