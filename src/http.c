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