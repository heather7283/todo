#ifndef SRC_UTILS_H
#define SRC_UTILS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#define LOG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define ELOG(fmt, ...) fprintf(stderr, fmt ": %s\n", ##__VA_ARGS__, strerror(errno))

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))
#define STREQ(a, b) (strcmp((a), (b)) == 0)

#define STR(s) #s
#define STR2(s) STR(s)

#define ANSI_DIM "\033[2m"
#define ANSI_BOLD "\033[1m"
#define ANSI_RED "\033[31m"
#define ANSI_GREEN "\033[32m"
#define ANSI_YELLOW "\033[33m"
#define ANSI_RESET "\033[0m"

/* Returns value of envvar name or fallback if envvar name is unset */
const char *getenv_or(const char *name, const char *fallback);

/* Opens filename, write contents of template to it and invokes EDITOR on it. Returns fd. */
int open_and_edit(const char *filename, const char *template);

/* Returns filename suitable for a temporary file in TMPDIR or /tmp */
const char *get_tmpfile_path(void);

/* Convert string to int while without all the strtol() bullshit */
bool str_to_int64(const char *str, int64_t *res);

/* Print message help to stream and exit with code rc */
void print_help_and_exit(const char *help, FILE *stream, int rc);

/* Formats unix seconds into human readable time string */
const char *format_seconds(time_t seconds);

/* Formats number of seconds into days, hours, minutes */
const char *format_timediff(time_t diff);

#endif /* #ifndef SRC_UTILS_H */

