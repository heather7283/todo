#include <stdio.h>

#include "actions/delete.h"
#include "db.h"
#include "utils.h"

static const char help[] =
    "Usage:\n"
    "    todo delete ID\n"
;

int action_delete(int argc, char **argv) {
    if (argc < 2) {
        LOG("id is not specified\n");
        print_help_and_exit(help, stderr, 1);
    }

    int64_t id;
    if (!str_to_int64(argv[1], &id)) {
        return 1;
    }

    const char sql[] = "DELETE FROM todo_items WHERE id == $id;";
    sqlite3_stmt *sql_stmt = NULL;

    int ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        return 1;
    }

    STMT_BIND(sql_stmt, int64, "$id", id);

    ret = sqlite3_step(sql_stmt);
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to delete db entry");
        return 1;
    }
    if (sqlite3_changes(db) < 1) {
        LOG("entry with id %li was not found", id);
        return 1;
    }

    sqlite3_finalize(sql_stmt);

    return 0;
}

