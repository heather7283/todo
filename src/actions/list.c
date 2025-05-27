#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "actions/list.h"
#include "db.h"
#include "utils.h"

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

static int print_row(void *data, int argc, char **argv, char **column_names) {
    for (int i = 0; i < argc - 1; i++) {
        printf("%s\t", argv[i] ? argv[i] : "");
    }
    printf("%s\n", argv[argc - 1] ? argv[argc - 1] : "");
    return 0;
}

int action_list(int argc, char **argv) {
    int ret;

    const char *sql2 = NULL;
    if (argc < 2) {
        sql2 = "id,type,title";
    } else if (argc == 2) {
        sql2 = build_field_list(argv[1]);
        if (sql2 == NULL) {
            goto err;
        }
    } else if (argc > 2) {
        LOG("too many arguments\n");
        print_help_and_exit(help, stderr, 1);
    }

    const char sql1[] = "SELECT ";
    const char sql3[] = " FROM todo_items WHERE deleted_at IS NULL;";

    char sql[sizeof(sql1) + sizeof(sql3) + MAX_FIELD_LIST_LEN];
    snprintf(sql, sizeof(sql), "%s%s%s", sql1, sql2, sql3);

    ret = sqlite3_exec(db, sql, print_row, NULL, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to list entries");
        return 1;
    }

    return 0;

err:
    return 1;
}

