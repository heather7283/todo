#ifndef SRC_UTILS_H
#define SRC_UTILS_H

#define LOG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#define ELOG(fmt, ...) fprintf(stderr, fmt ": %s\n", ##__VA_ARGS__, strerror(errno))

/* Returns value of envvar name or fallback if envvar name is unset */
const char *getenv_or(const char *name, const char *fallback);

/* Opens filename, write contents of template to it and invokes EDITOR on it. Returns fd. */
int open_and_edit(const char *filename, const char *template);

/* Returns filename suitable for a temporary file in TMPDIR or /tmp */
const char *get_tmpfile_path(void);

#endif /* #ifndef SRC_UTILS_H */

