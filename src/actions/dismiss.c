#include <stdio.h>

#include "actions/dismiss.h"
#include "db.h"
#include "utils.h"

static const char help[] =
    "Usage:\n"
    "    todo dismiss ID...\n"
;

int action_dismiss(int argc, char **argv) {
    int rc = 0;

    if (argc < 2) {
        LOG("id is not specified");
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

    char **ids = &argv[1];
    char *id_str;
    while ((id_str = *(ids++)) != NULL) {
        int64_t id;
        if (!str_to_int64(id_str, &id)) {
            rc = 1;
            goto loop_continue;
        }

        STMT_BIND(sql_stmt, int64, "$id", id);

        ret = sqlite3_step(sql_stmt);
        if (ret != SQLITE_DONE) {
            SQL_ELOG("failed to update db entry");
            rc = 1;
            goto loop_continue;
        }
        if (sqlite3_changes(db) < 1) {
            LOG("periodic entry with id %li does not exist", id);
            rc = 1;
            goto loop_continue;
        }

    loop_continue:
        sqlite3_clear_bindings(sql_stmt);
        sqlite3_reset(sql_stmt);
    }

    sqlite3_finalize(sql_stmt);

    return rc;
}

