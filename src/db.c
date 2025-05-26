#include <sys/wait.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <limits.h>

#include "db.h"
#include "utils.h"

struct sqlite3 *db = NULL;

static const char create_table_todo_items_stmt[] =
    "CREATE TABLE IF NOT EXISTS todo_items ("
        "id           INTEGER PRIMARY KEY,"
        "title        TEXT    NOT NULL UNIQUE,"
        "body         TEXT,"
        "created_at   INTEGER NOT NULL,"
        "completed_at INTEGER,"
        "type         TEXT    CHECK(type == 'deadline' OR type == 'idle' OR type == 'periodic'),"
        /* only for deadline type */
        "deadline     INTEGER CHECK((deadline IS NOT NULL) == (type == 'deadline')),"
        /* only for periodic type */
        "cron_expr    TEXT    CHECK((cron_expr IS NOT NULL) == (type == 'periodic')),"
        "prev_trigger INTEGER CHECK((prev_trigger IS NOT NULL) == (type == 'periodic')),"
        "next_trigger INTEGER CHECK((next_trigger IS NOT NULL) == (type == 'periodic')),"
        "dismissed    INTEGER CHECK((dismissed IS NOT NULL) == (type == 'periodic'))"
    ")";

static const char *get_default_db_path(void) {
    static char path[PATH_MAX];

    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    const char *home = getenv("HOME");
    if (xdg_data_home != NULL) {
        snprintf(path, sizeof(path), "%s/todo/db.sqlite3", xdg_data_home);
    } else if (home != NULL) {
        snprintf(path, sizeof(path), "%s/.local/share/todo/db.sqlite3", home);
    } else {
        LOG("both XDG_DATA_HOME and HOME are unset, unable to determine database path");
        return NULL;
    }

    return path;
}

int db_init(const char *db_path) {
    int rc = 0;

    db_path = (db_path == NULL) ? get_default_db_path() : db_path;
    if (db_path == NULL) {
        return -1;
    }

    if (access(db_path, F_OK) < 0) {
        pid_t pid = fork();
        switch (pid) {
        case -1: /* error */
            ELOG("failed to fork child");
            break;
        case 0: /* child */
            execlp("install", "install", "-Dm644", "/dev/null", db_path, NULL);
            ELOG("failed to exec install -Dm644 /dev/null %s", db_path);
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
    }

    rc = sqlite3_open(db_path, &db);
    if (rc != SQLITE_OK) {
        LOG("failed to open sqlite database at %s: %s", db_path, sqlite3_errstr(rc));
        goto err;
    }

    rc = sqlite3_exec(db, create_table_todo_items_stmt, NULL, NULL, NULL);
    if (rc != SQLITE_OK) {
        LOG("failed to create todo_items table: %s", sqlite3_errmsg(db));
        goto err;
    }

    return 0;

err:
    sqlite3_close(db);
    return -1;
}

void db_cleanup(void) {
    sqlite3_close(db);
    db = NULL;
}

