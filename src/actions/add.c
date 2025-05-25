#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "actions/add.h"
#include "db.h"
#include "utils.h"
#include "getopt.h"
#include "ccronexpr.h"

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

static int add_periodic(int argc, char **argv) {
    int ret;
    const char *cron_expr_str = NULL;
    const char *title_str = NULL;
    const char *body_str = NULL;

    if (argc < 2) {
        LOG("not enough arguments\n");
        print_help_and_exit(stderr, 1);
    } else if (argc == 2) {
        cron_expr_str = argv[0];
        title_str = argv[1];
    } else if (argc == 3) {
        cron_expr_str = argv[0];
        title_str = argv[1];
        body_str = argv[2];
    } else {
        LOG("too many arguments\n");
        print_help_and_exit(stderr, 1);
    }

    cron_expr cron_expr;
    const char *cron_error = NULL;
    cron_parse_expr(cron_expr_str, &cron_expr, &cron_error);
    if (cron_error != NULL) {
        LOG("failed to parse %s as cron expression: %s", cron_expr_str, cron_error);
        return 1;
    }

    time_t created_at = time(NULL);
    time_t next_trigger = cron_next(&cron_expr, created_at);
    if (next_trigger == ((time_t)-1)) {
        LOG("failed to calculate next trigger time for entry");
        return 1;
    }

    const char sql[] =
        "INSERT INTO todo_items (title, body, created_at, type,"
                                "cron_expr, prev_trigger, next_trigger, iteration, dismissed)"
        "VALUES ($title, $body, $created_at, $type,"
                "$cron_expr, $prev_trigger, $next_trigger, $iteration, $dismissed);"
    ;
    sqlite3_stmt *sql_stmt = NULL;

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    STMT_BIND(sql_stmt, text, "$title", title_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, text, "$body", body_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, int64, "$created_at", created_at);
    STMT_BIND(sql_stmt, text, "$type", "periodic", -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, text, "$cron_expr", cron_expr_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, int64, "$prev_trigger", 0);
    STMT_BIND(sql_stmt, int64, "$next_trigger", next_trigger);
    STMT_BIND(sql_stmt, int64, "$iteration", 0);
    STMT_BIND(sql_stmt, int64, "$dismissed", 0);

    ret = sqlite3_step(sql_stmt);
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to insert a new db entry");
        goto err;
    }

    sqlite3_finalize(sql_stmt);

    return 0;

err:
    sqlite3_finalize(sql_stmt);
    return 1;
}

static int add_deadline(int argc, char **argv) {
    int ret;
    const char *deadline_str = NULL;
    const char *title_str = NULL;
    const char *body_str = NULL;

    if (argc < 2) {
        LOG("not enough arguments\n");
        print_help_and_exit(stderr, 1);
    } else if (argc == 2) {
        deadline_str = argv[0];
        title_str = argv[1];
    } else if (argc == 3) {
        deadline_str = argv[0];
        title_str = argv[1];
        body_str = argv[2];
    } else {
        LOG("too many arguments\n");
        print_help_and_exit(stderr, 1);
    }

    int64_t deadline;
    if (!str_to_int64(deadline_str, &deadline)) {
        return 1;
    }

    const char sql[] =
        "INSERT INTO todo_items (title, body, created_at, type, deadline)"
        "VALUES ($title, $body, $created_at, $type, $deadline);"
    ;
    sqlite3_stmt *sql_stmt = NULL;

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    time_t created_at = time(NULL);

    STMT_BIND(sql_stmt, text, "$title", title_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, text, "$body", body_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, int64, "$created_at", created_at);
    STMT_BIND(sql_stmt, text, "$type", "deadline", -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, int64, "$deadline", deadline);

    ret = sqlite3_step(sql_stmt);
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to insert a new db entry");
        goto err;
    }

    sqlite3_finalize(sql_stmt);

    return 0;

err:
    sqlite3_finalize(sql_stmt);
    return 1;
}

static int add_idle(int argc, char **argv) {
    int ret;
    const char *title_str = NULL;
    const char *body_str = NULL;

    if (argc < 1) {
        LOG("not enough arguments\n");
        print_help_and_exit(stderr, 1);
    } else if (argc == 1) {
        title_str = argv[0];
    } else if (argc == 2) {
        title_str = argv[0];
        body_str = argv[1];
    } else {
        LOG("too many arguments\n");
        print_help_and_exit(stderr, 1);
    }

    const char sql[] =
        "INSERT INTO todo_items (title, body, created_at, type)"
        "VALUES ($title, $body, $created_at, $type);"
    ;
    sqlite3_stmt *sql_stmt = NULL;

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    time_t created_at = time(NULL);

    STMT_BIND(sql_stmt, text, "$title", title_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, text, "$body", body_str, -1, SQLITE_STATIC);
    STMT_BIND(sql_stmt, int64, "$created_at", created_at);
    STMT_BIND(sql_stmt, text, "$type", "idle", -1, SQLITE_STATIC);

    ret = sqlite3_step(sql_stmt);
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to insert a new db entry");
        goto err;
    }

    sqlite3_finalize(sql_stmt);

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
        rc = add_deadline(argc - 1, argv + 1);
    } else if (STREQ(argv[0], "periodic")) {
        rc = add_periodic(argc - 1, argv + 1);
    } else {
        LOG("invalid item type: %s\n", argv[0]);
        print_help_and_exit(stderr, 1);
    }

    return rc;
}

