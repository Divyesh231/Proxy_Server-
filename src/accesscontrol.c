#include "proxy.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static int equal_case_insensitive(const char *a, const char *b) {
    while (*a && *b && tolower((unsigned char)*a) == tolower((unsigned char)*b)) {
        a++;
        b++;
    }

    return *a == '\0' && *b == '\0';
}

int is_blocked(const char *host) {
    FILE *file = fopen("config/blocked_domains.txt", "r");

    if (!file)
        return 0;

    char rule[256];

    while (fgets(rule, sizeof(rule), file)) {
        rule[strcspn(rule, "\r\n")] = '\0';
        char *start = rule;

        while (isspace((unsigned char)*start))
            start++;

        size_t n = strlen(start);

        while (n && isspace((unsigned char)start[n - 1]))
            start[--n] = '\0';

        if (!*start || *start == '#')
            continue;

        if (equal_case_insensitive(host, start)) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}
