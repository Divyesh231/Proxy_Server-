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
