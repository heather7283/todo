#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include "utils.h"
#include "db.h"
#include "getopt.h"

#include "actions/add.h"
#include "actions/check.h"
#include "actions/dismiss.h"
#include "actions/delete.h"
#include "actions/list.h"

static const char help[] =
    "Usage:\n"
    "    todo [-d DB_PATH] [-h] ACTION ARGS...\n"
    "\n"
    "Actions:\n"
    "  Add a new task:\n"
    "    todo add deadline DEADLINE TITLE [BODY]\n"
    "    todo add periodic CRON_EXPR TITLE [BODY]\n"
    "    todo add idle TITLE [BODY]\n"
    "  Show brief summary of active tasks:\n"
    "    todo check\n"
    "  Dismiss a periodic task until it fires again:\n"
    "    todo dismiss ID\n"
    "  List all tasks (optionally specify fields to print):\n"
    "    todo list [FIELD1,FIELD2,FIELD3...]\n"
    "  Delete a task:\n"
    "    todo delete ID\n"
;

typedef int (*action_func_t)(int argc, char **argv);

static const struct {
    const char *const name;
    const action_func_t action;
} actions[] = {
    {"add", action_add},
    {"check", action_check},
    {"dismiss", action_dismiss},
    {"delete", action_delete},
    {"list", action_list},
};

static action_func_t match_action(const char *input) {
    action_func_t match_func = NULL;
    int matches = 0;
    char matches_str[256];
    char *matches_str_pos = matches_str;
    char *const matches_str_end = matches_str + ARRAY_SIZE(matches_str);
    const size_t input_len = strlen(input);

    for (unsigned long i = 0; i < ARRAY_SIZE(actions); i++) {
        const char *action_name = actions[i].name;
        action_func_t action_func = actions[i].action;

        if (STRNEQ(action_name, input, input_len)) {
            match_func = action_func;
            matches += 1;
            matches_str_pos = stpecpy(matches_str_pos, matches_str_end, action_name);
            matches_str_pos = stpecpy(matches_str_pos, matches_str_end, ", ");
        }
    }

    if (matches < 1) {
        LOG("action %s is not found", input);
    } else if (matches > 1) {
        matches_str_pos[-2] = '\0'; /* remove trailing ", " */
        LOG("action name %s is ambiguous between %s", input, matches_str);
    }
    return match_func;
}

int main(int argc, char **argv) {
    int rc = 0;
    const char *db_path = NULL;

    int opt;
    while ((opt = getopt(argc, argv, "d:h")) != -1) {
        switch (opt) {
        case 'd':
            db_path = optarg;
            break;
        case 'h':
            print_help_and_exit(help, stdout, 0);
            break;
        default:
            return 1;
        }
    }
    argc = argc - optind;
    argv = &argv[optind];
    if (argc < 1) {
        LOG("no action provided");
        return 1;
    }

    action_func_t action = match_action(argv[0]);
    if (action == NULL) {
        return 1;
    }

    if (db_init(db_path) < 0) {
        rc = 1;
        goto out;
    };

    rc = action(argc, argv);

out:
    db_cleanup();
    return rc;
}

