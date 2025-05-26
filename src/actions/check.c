#include <string.h>
#include <time.h>

#include "actions/add.h"
#include "db.h"
#include "utils.h"
#include "getopt.h"
#include "ccronexpr.h"

#define DEADLINE_THRESH_YELLOW (60 * 60 * 24 * 30) /* a month */
#define DEADLINE_THRESH_RED (60 * 60 * 24 * 7) /* a week */

static const char help[] =
    "Usage:\n"
    "    todo check takes no command line arguments\n"
;

static int check_idle(int id, const char *title) {
    printf(ANSI_DIM "%4i: \"%s\"" ANSI_RESET "\n", id, title);

    return 0;
}

static int check_deadline(int id, const char *title, time_t deadline) {
    time_t now = time(NULL); /* TODO: call time() once */
    time_t diff = deadline - now;

    char date_str[64];
    struct tm *deadline_tm = localtime(&deadline);
    strftime(date_str, sizeof(date_str), "%a %d %b %Y %H:%M:%S", deadline_tm);

    if (diff < 0) { /* deadline has passed, I'm cooked */
        printf(ANSI_BOLD ANSI_RED "%4i: \"%s\" due %s (%s ago)" ANSI_RESET "\n",
               id, title, date_str, format_timediff(diff));
    } else if (diff < DEADLINE_THRESH_RED) {
        printf(ANSI_RED "%4i: \"%s\" due %s (in %s)" ANSI_RESET "\n",
               id, title, date_str, format_timediff(diff));
    } else if (diff < DEADLINE_THRESH_YELLOW) {
        printf(ANSI_YELLOW "%4i: \"%s\" due %s (in %s)" ANSI_RESET "\n",
               id, title, date_str, format_timediff(diff));
    } else {
        printf(ANSI_GREEN "%4i: \"%s\" due %s (in %s)" ANSI_RESET "\n",
               id, title, date_str, format_timediff(diff));
    }

    return 0;
}

static int check_periodic(int id, const char *title, const char *cron_expression,
                          time_t prev_trigger, time_t next_trigger, bool dismissed) {
    /* Ok this one is tricky.
     *
     * If current time is greater than next_trigger, I should display the entry,
     * set it dismissed flag to false, set prev_trigger to current time,
     * and finally update next_trigger using the cron expression.
     */
    time_t now = time(NULL);

    if (now > next_trigger) {
        const char *ccron_error = NULL;
        cron_expr ccron_expr;
        cron_parse_expr(cron_expression, &ccron_expr, &ccron_error);
        if (ccron_error != NULL) {
            /* TODO: handle error */
        }

        prev_trigger = now;
        next_trigger = cron_next(&ccron_expr, now);
        dismissed = false;

        const char sql[] =
            "UPDATE todo_items SET "
                "prev_trigger = $prev_trigger, "
                "next_trigger = $next_trigger, "
                "dismissed = $dismissed "
            "WHERE id = $id;"
        ;
        sqlite3_stmt *sql_stmt = NULL;

        int ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
        if (ret != SQLITE_OK) {
            SQL_ELOG("failed to prepare statement");
            /* TODO: handle error */
            return 1;
        }

        STMT_BIND(sql_stmt, int64, "$prev_trigger", prev_trigger);
        STMT_BIND(sql_stmt, int64, "$next_trigger", next_trigger);
        STMT_BIND(sql_stmt, int64, "$dismissed", dismissed);
        STMT_BIND(sql_stmt, int64, "$id", id);

        ret = sqlite3_step(sql_stmt);
        if (ret != SQLITE_DONE) {
            SQL_ELOG("failed to update db entry %i", id);
            /* TODO: handle error */
            return 1;
        }

        sqlite3_finalize(sql_stmt);
    }

    if (!dismissed) {
        time_t now = time(NULL); /* TODO: call time() once */
        time_t diff = next_trigger - now;

        char next_str[64];
        struct tm *next_tm = localtime(&next_trigger);
        strftime(next_str, sizeof(next_str), "%a %d %b %Y %H:%M:%S", next_tm);

        printf("%4i: \"%s\" %s (next %s, in %s)\n",
               id, title, cron_expression, next_str, format_timediff(diff));
    }

    return 0;
}

int action_check(int argc, char **argv) {
    int ret;

    const char sql[] =
        /*       0     1    2        3         4            5            6         7 */
        "SELECT id,title,type,deadline,cron_expr,prev_trigger,next_trigger,dismissed "
        "FROM todo_items;";
    sqlite3_stmt *sql_stmt = NULL;

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    while ((ret = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        int64_t id = sqlite3_column_int64(sql_stmt, 0);
        const char *title = (char *)sqlite3_column_text(sql_stmt, 1);
        const char *type = (char *)sqlite3_column_text(sql_stmt, 2);

        if (STREQ(type, "idle")) {
            check_idle(id, title);
        } else if (STREQ(type, "deadline")) {
            time_t deadline = sqlite3_column_int64(sql_stmt, 3);

            check_deadline(id, title, deadline);
        } else if (STREQ(type, "periodic")) {
            const char *cron_expr = (char *)sqlite3_column_text(sql_stmt, 4);
            time_t prev_trigger = sqlite3_column_int64(sql_stmt, 5);
            time_t next_trigger = sqlite3_column_int64(sql_stmt, 6);
            bool dismissed = sqlite3_column_int64(sql_stmt, 7);

            check_periodic(id, title, cron_expr, prev_trigger, next_trigger, dismissed);
        }
    }
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to list entrie");
        goto err;
    }

    return 0;

err:
    sqlite3_finalize(sql_stmt);
    return 1;
}

