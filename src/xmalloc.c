#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xmalloc.h"

static void *check_alloc(void *ptr) {
    if (ptr == NULL) {
        fprintf(stderr, "memory allocation failed, buy more ram lol\n");
        fflush(stderr);
        abort();
    }

    return ptr;
}

void *xmalloc(size_t size) {
    return check_alloc(malloc(size));
}

void *xcalloc(size_t n, size_t size) {
    return check_alloc(calloc(n, size));
}

void *xrealloc(void *ptr, size_t size) {
    return check_alloc(realloc(ptr, size));
}

char *xstrdup(const char *s) {
    return (s == NULL) ? NULL : check_alloc(strdup(s));
}

