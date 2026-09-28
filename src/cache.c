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

void cache_put(const char *key, const char *data, size_t length)
{
    pthread_mutex_lock(&cache_lock);

    int slot = -1;
    time_t oldest = 0;

    for (int i = 0; i < CACHE_ENTRIES; i++) 
    {
        if (entries[i].key && strcmp(entries[i].key, key) == 0) 
        {
            slot = i;
            break;
        }

        if (!entries[i].key) 
        {
            slot = i;
            break;
        }

        if (slot < 0 || entries[i].stored_at < oldest) 
        {
            slot = i;
            oldest = entries[i].stored_at;
        }
    }
    if (slot >= 0) 
    {
        char *new_key = strdup(key);
        char *new_data = malloc(length);
        if (new_key && new_data) 
        {
            memcpy(new_data, data, length);

            free(entries[slot].key);
            free(entries[slot].data);

            entries[slot] = (CacheEntry){
                new_key,
                new_data,
                length,
                time(NULL)
            };
        } 
        else 
        {
            free(new_key);
            free(new_data);
        }
    }

    pthread_mutex_unlock(&cache_lock);
}

void cache_free_copy(char *data)
{
    free(data);
}