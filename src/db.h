#ifndef SRC_DB_H
#define SRC_DB_H

#include <sqlite3.h>

#define SQL_ELOG(fmt, ...) fprintf(stderr, fmt ": %s\n", ##__VA_ARGS__, sqlite3_errmsg(db))

#define STMT_BIND(stmt, type, name, ...) \
    sqlite3_bind_##type((stmt), sqlite3_bind_parameter_index((stmt), (name)), ##__VA_ARGS__)

enum todo_item_type {
    IDLE = 1,
    DEADLINE = 2,
    PERIODIC = 3,
};

extern struct sqlite3 *db;

int db_init(const char *db_path);
void db_cleanup(void);

#endif /* #ifndef SRC_DB_H */

