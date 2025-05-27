#ifndef SRC_DB_H
#define SRC_DB_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sqlite3.h>

#define SQL_ELOG(fmt, ...) fprintf(stderr, fmt ": %s\n", ##__VA_ARGS__, sqlite3_errmsg(db))

#define STMT_BIND(stmt, type, name, ...) \
    sqlite3_bind_##type((stmt), sqlite3_bind_parameter_index((stmt), (name)), ##__VA_ARGS__)

enum todo_item_type {
    IDLE = 1,
    DEADLINE = 2,
    PERIODIC = 3,
    TODO_ITEM_TYPE_ANY,
};

struct todo_item {
    int64_t id;
    char *title;
    char *body;
    time_t created_at;
    time_t deleted_at;
    enum todo_item_type type;
    union {
        struct {
            time_t deadline;
        } deadline;
        struct {
            char *cron_expr;
            time_t prev_trigger, next_trigger;
            bool dismissed;
        } periodic;
    } as;
};

extern struct sqlite3 *db;

int db_init(const char *db_path);
void db_cleanup(void);

#endif /* #ifndef SRC_DB_H */

