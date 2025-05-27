#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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

int xasprintf(char **strp, const char *fmt, ...) {
	va_list args, args_copy;

	va_start(args, fmt);

	va_copy(args_copy, args);
	int length = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);

    if (length < 0) {
        return -1;
    }
    *strp = check_alloc(malloc(length + 1));

	int ret = vsnprintf(*strp, length + 1, fmt, args);

	va_end(args);

	return ret;
}

