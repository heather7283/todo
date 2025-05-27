#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "actions/list.h"
#include "db.h"
#include "utils.h"
#include "xmalloc.h"
#include "getopt.h"

static const char help[] =
    "Usage:\n"
    "    todo list [FIELD1,FIELD2,FIELD3...]\n"
;

#define MAX_FIELD_LIST_LEN 1024
static const char *build_field_list(char *raw_list) {
    const char *allowed_fields[] = {
        "id", "title", "created_at", "deleted_at", "type",
        "deadline",
        "cron_expr", "prev_trigger", "next_trigger", "dismissed"
    };

    static char result_list[MAX_FIELD_LIST_LEN];
    char *result_pos = result_list;
    char *result_end = result_list + sizeof(result_list);

    char *token = strtok(raw_list, ",");
    while (token != NULL) {
        for (unsigned long i = 0; i < ARRAY_SIZE(allowed_fields); i++) {
            if (strcmp(token, allowed_fields[i]) == 0) {
                result_pos = stpecpy(result_pos, result_end, allowed_fields[i]);
                result_pos = stpecpy(result_pos, result_end, ",");
                if (result_pos == NULL) {
                    LOG("field list is too long");
                    return NULL;
                }
            }
        }

        token = strtok(NULL, ",");
    }

    if (result_pos > result_list && *(result_pos - 1) == ',') {
        *(result_pos - 1) = '\0';
    }

    return result_list;
}

int action_list(int argc, char **argv) {
    int ret;
    enum todo_item_type type = TODO_ITEM_TYPE_ANY;

    optreset = 1;
    optind = 0;
    int opt;
    while ((opt = getopt(argc, argv, ":ht:")) != -1) {
        switch (opt) {
        case 't':
            /* TODO: move to helper function? */
            if (STREQ(optarg, "deadline")) {
                type = DEADLINE;
            } else if (STREQ(optarg, "periodic")) {
                type = PERIODIC;
            } else if (STREQ(optarg, "idle")) {
                type = IDLE;
            } else {
                LOG("invalid entry type: %s\n", optarg);
                print_help_and_exit(help, stderr, 1);
            }
            break;
        case 'h':
            print_help_and_exit(help, stdout, 0);
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

    const char *sql_field_list = NULL;
    if (argc < 2) {
        sql_field_list = "id,type,title";
    } else if (argc == 2) {
        sql_field_list = build_field_list(argv[1]);
        if (sql_field_list == NULL) {
            goto err;
        }
    } else if (argc > 2) {
        LOG("too many arguments\n");
        print_help_and_exit(help, stderr, 1);
    }

    /* I hate this. I hate this so much */
    const char sql_select[] = "SELECT ";
    const char sql_from[] = " FROM todo_items";
    const char sql_where[] = " WHERE";
    const char sql_and[] = " AND";
    const char sql_deleted[] = " deleted_at IS NULL";
    const char sql_type[] = " type == $type";
    const char sql_semicolon[] = ";";

    char *sql = NULL;
    sqlite3_stmt *sql_stmt = NULL;
    if (type == TODO_ITEM_TYPE_ANY) {
        xasprintf(&sql, "%s%s%s%s%s%s", sql_select, sql_field_list, sql_from, sql_where,
                  sql_deleted, sql_semicolon);
    } else {
        xasprintf(&sql, "%s%s%s%s%s%s%s%s", sql_select, sql_field_list, sql_from, sql_where,
                  sql_deleted, sql_and, sql_type, sql_semicolon);
    }

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare statement");
        return 1;
    }

    STMT_BIND(sql_stmt, int64, "$type", type);

    while ((ret = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        int n_cols = sqlite3_column_count(sql_stmt);
        const char *val = NULL;

        for (int i = 0; i < n_cols - 1; i++) {
            val = (char *)sqlite3_column_text(sql_stmt, i);
            printf("%s\t", val ? val : "");
        }
        val = (char *)sqlite3_column_text(sql_stmt, n_cols - 1);
        printf("%s\n", val ? val : "");
    }
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to list entries");
        goto err;
    }

    return 0;

err:
    return 1;
}

