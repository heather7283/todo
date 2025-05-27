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

static const char help[] =
    "Usage:\n"
    "    todo [-d DB_PATH] [-h] ACTION ARGS...\n"
    "\n"
    "Actions:\n"
    "    todo add - create a todo item\n"
    "    todo check - list brief summary of all todo items\n"
;

int main(int argc, char **argv) {
    int rc = 0;
    const char *db_path = NULL;

    int opt;
    while ((opt = getopt(argc, argv, ":d:h")) != -1) {
        switch (opt) {
        case 'd':
            db_path = optarg;
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
    if (argc < 1) {
        LOG("no action provided\n");
        print_help_and_exit(help, stderr, 1);
    }

    action_func_t action = NULL;
    for (unsigned long i = 0; i < ARRAY_SIZE(actions); i++) {
        if (STREQ(argv[0], actions[i].name)) {
            action = actions[i].action;
        }
    }
    if (action == NULL) {
        LOG("unknown action: %s\n", argv[0]);
        print_help_and_exit(help, stderr, 1);
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

