#include <stdio.h>

#include "actions/delete.h"
#include "db.h"
#include "utils.h"
#include "getopt.h"

static const char help[] =
    "Usage:\n"
    "    todo delete [-dh] ID...\n"
    "\n"
    "Command line arguments:\n"
    "    -d  Really delete instead of soft deleting\n"
    "    -h  Print this message and exit\n"
;

int action_delete(int argc, char **argv) {
    int rc = 0;
    bool soft_delete = true;

    optreset = 1;
    optind = 0;
    int opt;
    while ((opt = getopt(argc, argv, "hd")) != -1) {
        switch (opt) {
        case 'h':
            print_help_and_exit(help, stdout, 0);
        case 'd':
            soft_delete = false;
            break;
        default:
            return 1;
        }
    }
    argc = argc - optind;
    argv = &argv[optind];

    if (argc < 1) {
        LOG("id is not specified");
        return 1;
    }
    char **ids = &argv[0];

    const char *sql = NULL;
    sqlite3_stmt *sql_stmt = NULL;
    int ret;

    if (soft_delete) {
        sql = "UPDATE todo_items SET deleted_at = $deleted_at WHERE id == $id;";

        ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
        if (ret != SQLITE_OK) {
            SQL_ELOG("failed to prepare sql statement");
            return 1;
        }
    } else {
        sql = "DELETE FROM todo_items WHERE id == $id;";

        ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
        if (ret != SQLITE_OK) {
            SQL_ELOG("failed to prepare sql statement");
            return 1;
        }
    }

    time_t deleted_at = time(NULL);
    char *id_str;
    while ((id_str = *(ids++)) != NULL) {
        int64_t id;
        if (!str_to_int64(id_str, &id)) {
            rc = 1;
            goto loop_continue;
        }

        if (soft_delete) {
            STMT_BIND(sql_stmt, int64, "$id", id);
            STMT_BIND(sql_stmt, int64, "$deleted_at", deleted_at);
        } else {
            STMT_BIND(sql_stmt, int64, "$id", id);
        }

        ret = sqlite3_step(sql_stmt);
        if (ret != SQLITE_DONE) {
            SQL_ELOG("failed to delete db entry");
            rc = 1;
            goto loop_continue;
        }
        if (sqlite3_changes(db) < 1) {
            LOG("entry with id %li was not found", id);
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

