#include "proxy.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define CACHE_ENTRIES 16
#define CACHE_TTL_SECONDS 60
typedef struct { char *key; char *data; size_t length; time_t stored_at; } CacheEntry;
static CacheEntry entries[CACHE_ENTRIES];
static pthread_mutex_t cache_lock = PTHREAD_MUTEX_INITIALIZER;
int cache_get(const char *key, char **data, size_t *length) {
    int found = 0;
    time_t now = time(NULL);
    pthread_mutex_lock(&cache_lock);

    for (int i = 0; i < CACHE_ENTRIES; i++) {
        if (!entries[i].key)
            continue;

        if (now - entries[i].stored_at > CACHE_TTL_SECONDS) {
            free(entries[i].key);
            free(entries[i].data);
            memset(&entries[i], 0, sizeof(entries[i]));
            continue;
        }

        if (strcmp(entries[i].key, key) == 0) {
            char *copy = malloc(entries[i].length);

            if (copy) {
                memcpy(copy, entries[i].data, entries[i].length);
                *data = copy;
                *length = entries[i].length;
                found = 1;
            }

            break;
        }
    }

    pthread_mutex_unlock(&cache_lock);
    return found;
} 
