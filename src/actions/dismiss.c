#include <stdio.h>

#include "actions/dismiss.h"
#include "db.h"
#include "utils.h"

static const char help[] =
    "Usage:\n"
    "    todo dismiss ID\n"
;

int action_dismiss(int argc, char **argv) {
    if (argc < 2) {
        LOG("id is not specified\n");
        print_help_and_exit(help, stderr, 1);
    }

    int64_t id;
    if (!str_to_int64(argv[1], &id)) {
        return 1;
    }

    /* MAKE SURE type == 3 CORRESPONDS TO THE PERIODIC VALUE OF todo_item_type ENUM !!! */
    const char sql[] = "UPDATE todo_items SET dismissed = 1 WHERE id == $id AND type == 3;";
    sqlite3_stmt *sql_stmt = NULL;

    int ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        return 1;
    }

    STMT_BIND(sql_stmt, int64, "$id", id);

    ret = sqlite3_step(sql_stmt);
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to update db entry");
        return 1;
    }

    sqlite3_finalize(sql_stmt);

    return 0;
}

