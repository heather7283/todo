#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>

#include "utils.h"

const char *getenv_or(const char *name, const char *fallback) {
    const char *env = getenv(name);
    if (env == NULL) {
        env = fallback;
    }
    return env;
}

int open_and_edit(const char *filename, const char *template) {
    int fd = -1;

    do {
        fd = open(filename, O_RDWR | O_CREAT | O_TRUNC | O_CLOEXEC, S_IRUSR | S_IWUSR);
    } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        ELOG("failed to open %s", filename);
        goto err;
    }

    if (template != NULL) {
        size_t template_len = strlen(template);
        size_t written = 0;
        while (written < template_len) {
            ssize_t w = write(fd, template + written, template_len - written);
            if (w < 0 && errno != EINTR) {
                ELOG("failed to write template");
                goto err;
            }
            written += w;
        }
    }

    const char *editor = getenv_or("EDITOR", "nvim");
    pid_t pid;
    switch (pid = fork()) {
    case -1: /* error */
        ELOG("failed to fork child");
        goto err;
    case 0: /* child */
        execlp(editor, editor, filename, NULL);
        ELOG("failed to exec into %s", editor);
        exit(1);
    default: /* parent */
        break;
    }

    while (waitpid(pid, NULL, 0) < 0) {
        if (errno != EINTR) {
            ELOG("failed to wait for child with pid %d", pid);
            goto err;
        }
    };

    return fd;

err:
    if (fd > 0) {
        close(fd);
    }
    return -1;
}

const char *get_tmpfile_path(void) {
    static int n = 0;
    static char path[PATH_MAX];

    const char *tmpdir = getenv_or("TMPDIR", "/tmp");

    snprintf(path, sizeof(path), "%s/todo_tmpfile_%d_%d.txt", tmpdir, getpid(), n++);

    return path;
}

bool str_to_int64(const char *str, int64_t *res) {
    char *endptr = NULL;

    errno = 0;
    int64_t res_tmp = strtoll(str, &endptr, 10);

    if (errno == 0 && *endptr == '\0') {
        *res = res_tmp;
        return true;
    } else if (errno != 0) {
        ELOG("failed to convert %s to int64", str);
        return false;
    } else {
        LOG("failed to convert %s to int64: Invalid character %c", str, *endptr);
        return false;
    }
}

void print_help_and_exit(const char *help, FILE *stream, int rc) {
    fputs(help, stream);
    exit(rc);
}

const char *format_seconds(time_t seconds) {
    static char string[64];

    const struct tm *tm = localtime(&seconds);
    strftime(string, sizeof(string), "%a %d %b %Y %H:%M", tm);

    return string;
}

const char *format_timediff(time_t diff) {
    static char string[64];
    char *s = string;

    if (diff < 0) {
        diff = -diff;
    }
    int d = (diff / 86400);
    int h = (diff % 86400) / 3600;
    int m = (diff % 3600) / 60;

    if (d == 0 && h == 0 && m == 0) {
        return "<1m";
    }

    if (d != 0) {
        s += snprintf(s, sizeof(string) - (s - string), "%id", d);
    }
    if (h != 0) {
        s += snprintf(s, sizeof(string) - (s - string), "%s%ih", d != 0 ? " " : "", h);
    }
    if (m != 0) {
        s += snprintf(s, sizeof(string) - (s - string), "%s%im", h != 0 ? " " : "", m);
    }

    return string;
}

