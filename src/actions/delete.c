#include <stdio.h>

#include "actions/delete.h"
#include "db.h"
#include "utils.h"
#include "getopt.h"

static const char help[] =
    "Usage:\n"
    "    todo delete [-dh] ID\n"
    "\n"
    "Command line arguments:\n"
    "    -d  Really delete instead of soft deleting\n"
    "    -h  Print this message and exit\n"
;

int action_delete(int argc, char **argv) {
    bool soft_delete = true;

    optreset = 1;
    optind = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":hd")) != -1) {
        switch (opt) {
        case 'h':
            print_help_and_exit(help, stdout, 0);
            break;
        case 'd':
            soft_delete = false;
            break;
        case '?':
            LOG("unknown option: %c\n", optopt);
            print_help_and_exit(help, stderr, 1);
            break;
        case ':':
            LOG("missing arg for %c\n", optopt);
            print_help_and_exit(help, stderr, 1);
            break;
        default:
            LOG("error while parsing command line options\n");
            print_help_and_exit(help, stderr, 1);
            break;
        }
    }
    argc = argc - optind;
    argv = &argv[optind];

    if (argc < 1) {
        LOG("id is not specified\n");
        print_help_and_exit(help, stderr, 1);
    }

    int64_t id;
    if (!str_to_int64(argv[0], &id)) {
        return 1;
    }

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

        STMT_BIND(sql_stmt, int64, "$id", id);
        STMT_BIND(sql_stmt, int64, "$deleted_at", time(NULL));
    } else {
        sql = "DELETE FROM todo_items WHERE id == $id;";

        ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
        if (ret != SQLITE_OK) {
            SQL_ELOG("failed to prepare sql statement");
            return 1;
        }

        STMT_BIND(sql_stmt, int64, "$id", id);
    }

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

