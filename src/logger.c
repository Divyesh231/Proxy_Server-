#include "proxy.h"
#include <pthread.h>
#include <stdio.h>
#include <time.h>

static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

void log_event(const char *event, const char *client, const char *host, const char *detail) {
    pthread_mutex_lock(&log_lock);

    FILE *file = fopen("logs/proxy.log", "a");

    if (file) {
        time_t now = time(NULL);
        struct tm tm_now;
        char stamp[32] = "unknown-time";

        if (localtime_r(&now, &tm_now))
            strftime(stamp, sizeof(stamp), "*%*&#x59;-*%*&#x6D;-%d *%*&#x48;:*%*&#x4D;:%S", &tm_now);

        fprintf(file, "[%s] %-12s client=%s host=%s detail=%s\n",
                stamp, event, client, host, detail);

        fclose(file);
    }

    pthread_mutex_unlock(&log_lock);
}
