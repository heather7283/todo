#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "actions/add.h"
#include "db.h"
#include "utils.h"
#include "getopt.h"

static void print_help_and_exit(FILE *stream, int rc) {
    const char *help =
        "Usage:\n"
        "    todo add idle TITLE [BODY]\n"
        "    todo add deadline DEADLINE TITLE [BODY]\n"
        "    todo add periodic CRONEXPR TITLE [BODY]\n"
    ;

    fputs(help, stream);
    exit(rc);
}

static int add_idle(int argc, char **argv) {
    int ret;
    const char *title = NULL;
    const char *body = NULL;

    if (argc < 1) {
        LOG("not enough arguments\n");
        print_help_and_exit(stderr, 1);
    } else if (argc == 1) {
        title = argv[0];
    } else if (argc == 2) {
        title = argv[0];
        body = argv[1];
    }

    const char sql[] =
        "INSERT INTO todo_items (title, body, created_at, completed_at, type)"
        "VALUES ($title, $body, $created_at, $completed_at, $type);"
    ;
    sqlite3_stmt *sql_stmt = NULL;

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    time_t created_at = time(NULL);

    STMT_BIND(sql_stmt, text, "$title", title, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, text, "$body", body, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, int64, "$created_at", created_at);
    STMT_BIND(sql_stmt, text, "$type", "idle", -1, SQLITE_STATIC);

    ret = sqlite3_step(sql_stmt);
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to insert a new db entry");
        goto err;
    }

    return 0;

err:
    sqlite3_finalize(sql_stmt);
    return 1;
}

int action_add(int argc, char **argv) {
    int rc = 0;

    optreset = 1;
    optind = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":h")) != -1) {
        switch (opt) {
        case 'h':
            print_help_and_exit(stdout, 0);
            break;
        case '?':
            LOG("unknown option: %c\n", optopt);
            print_help_and_exit(stderr, 1);
            break;
        case ':':
            LOG("missing arg for %c\n", optopt);
            print_help_and_exit(stderr, 1);
            break;
        default:
            LOG("error while parsing command line options\n");
            print_help_and_exit(stderr, 1);
            break;
        }
    }
    argc = argc - optind;
    argv = &argv[optind];

    if (argc < 1) {
        LOG("item type is not specified\n");
        print_help_and_exit(stderr, 1);
    }

    if (STREQ(argv[0], "idle")) {
        rc = add_idle(argc - 1, argv + 1);
    } else if (STREQ(argv[0], "deadline")) {
        LOG("TODO");
        rc = 1;
    } else if (STREQ(argv[0], "periodic")) {
        LOG("TODO");
        rc = 1;
    } else {
        LOG("invalid item type: %s\n", argv[0]);
        print_help_and_exit(stderr, 1);
    }

    return rc;
}

