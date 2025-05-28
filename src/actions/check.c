#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "actions/check.h"
#include "db.h"
#include "utils.h"
#include "xmalloc.h"
#include "ccronexpr.h"

#define DEADLINE_THRESH_YELLOW (60 * 60 * 24 * 30) /* a month */
#define DEADLINE_THRESH_RED (60 * 60 * 24 * 7) /* a week */

#define MINUTE (60)
#define HOUR (MINUTE * 60)
#define DAY (HOUR * 24)
#define WEEK (DAY * 7)

static int check_idle(const struct todo_item *item, int alignment) {
    int64_t id = item->id;
    const char *title = item->title;

    printf(ANSI_DIM "%*li: \"%s\"" ANSI_RESET "\n", alignment, id, title);

    return 0;
}

static int check_deadline(const struct todo_item *item, int alignment) {
    int64_t id = item->id;
    const char *title = item->title;
    time_t created_at = item->created_at;
    time_t deadline = item->as.deadline.deadline;

    time_t now = time(NULL); /* TODO: call time() once */
    time_t diff = deadline - now;

    const char *date_str = format_seconds(deadline);

    const char *color_str = NULL;
    if (diff < 0) { /* deadline has passed, I'm cooked */
        color_str = ANSI_BOLD ANSI_RED;
        printf("%s%*li: \"%s\" due %s (%s ago)" ANSI_RESET "\n",
               color_str, alignment, id, title, date_str, format_timediff(diff));
    } else {
        /* TODO: make the thresholds configurable? */
        float progress = ((float)now - (float)created_at) / ((float)deadline - (float)created_at);
        if (progress >= 0.9 || diff <= (3 * DAY)) {
            color_str = ANSI_BOLD ANSI_RED ANSI_BLINK;
        } else if (progress >= 0.75 || diff <= WEEK) {
            color_str = ANSI_RED;
        } else if (progress >= 0.5 && diff < (DAY * 31)) {
            color_str = ANSI_YELLOW;
        } else {
            color_str = ANSI_GREEN;
        }

        printf("%s%*li: \"%s\" due %s (in %s)" ANSI_RESET "\n",
               color_str, alignment, id, title, date_str, format_timediff(diff));
    }

    return 0;
}

static int check_periodic(const struct todo_item *item, int alignment) {
    /* Ok this one is tricky.
     *
     * If current time is greater than next_trigger, I should display the entry,
     * set it dismissed flag to false, set prev_trigger to current time,
     * and finally update next_trigger using the cron expression.
     */
    int64_t id = item->id;
    const char *title = item->title;
    const char *cron_expression = item->as.periodic.cron_expr;
    bool dismissed = item->as.periodic.dismissed;
    time_t prev_trigger = item->as.periodic.prev_trigger;
    time_t next_trigger = item->as.periodic.next_trigger;

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
            "WHERE id == $id;"
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
            SQL_ELOG("failed to update db entry %li", id);
            /* TODO: handle error */
            return 1;
        }

        sqlite3_finalize(sql_stmt);
    }

    if (!dismissed) {
        time_t now = time(NULL); /* TODO: call time() once */
        time_t diff = next_trigger - now;

        printf("%*li: \"%s\" %s (next %s, in %s)\n",
               alignment, id, title, cron_expression,
               format_seconds(next_trigger), format_timediff(diff));
    }

    return 0;
}

static int compare_items(const void *a, const void *b) {
    const struct todo_item *first = a;
    const struct todo_item *second = b;

    if (first->type != second->type) {
        return first->type > second->type;
    }

    switch (first->type) {
    case IDLE:
        return first->created_at > second->created_at;
    case DEADLINE:
        return first->as.deadline.deadline < second->as.deadline.deadline;
    case PERIODIC:
        return first->as.periodic.next_trigger < second->as.periodic.next_trigger;
    default:
        return 0; /* unreachable, make compiler shut up */
    }
}

static int count_digits(uint64_t number) {
    if (number == 0) {
        return 1;
    }

    int count = 0;
    while (number != 0) {
        number /= 10;
        count++;
    }
    return count;
}

int action_check(int argc, char **argv) {
    int ret;
    sqlite3_stmt *sql_stmt = NULL, *sql_count_stmt = NULL;
    int64_t items_count;
    struct todo_item *items = NULL;

    /* TODO: remove the count query and use linked list for this (or sort on sql side) */
    const char sql_count[] = "SELECT COUNT(*) FROM todo_items WHERE deleted_at IS NULL;";

    ret = sqlite3_prepare_v2(db, sql_count, -1, &sql_count_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    if ((ret = sqlite3_step(sql_count_stmt)) == SQLITE_ROW) {
        items_count = sqlite3_column_int64(sql_count_stmt, 0);
    } else {
        SQL_ELOG("failed to query entry count");
        goto err;
    }

    items = xcalloc(items_count, sizeof(*items));

    const char sql[] =
        /*       0     1          2    3        4         5            6            7         8 */
        "SELECT id,title,created_at,type,deadline,cron_expr,prev_trigger,next_trigger,dismissed "
        "FROM todo_items WHERE deleted_at IS NULL;";

    ret = sqlite3_prepare_v2(db, sql, -1, &sql_stmt, NULL);
    if (ret != SQLITE_OK) {
        SQL_ELOG("failed to prepare sql statement");
        goto err;
    }

    int max_digits = 0;
    int i = 0;
    while ((ret = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        items[i].id = sqlite3_column_int64(sql_stmt, 0);
        items[i].title = xstrdup((char *)sqlite3_column_text(sql_stmt, 1));
        items[i].created_at = sqlite3_column_int64(sql_stmt, 2);
        items[i].type = sqlite3_column_int64(sql_stmt, 3);

        switch (items[i].type) {
        case IDLE: {
            break;
        }
        case DEADLINE: {
            items[i].as.deadline.deadline = sqlite3_column_int64(sql_stmt, 4);
            break;
        }
        case PERIODIC: {
            items[i].as.periodic.cron_expr = xstrdup((char *)sqlite3_column_text(sql_stmt, 5));
            items[i].as.periodic.prev_trigger = sqlite3_column_int64(sql_stmt, 6);
            items[i].as.periodic.next_trigger = sqlite3_column_int64(sql_stmt, 7);
            items[i].as.periodic.dismissed = sqlite3_column_int64(sql_stmt, 8);
            break;
        }
        default: {
            LOG("got invalid item type from db??? wtf");
            goto err;
        }
        }

        int d = count_digits(items[i].id);
        max_digits = (d > max_digits) ? d : max_digits;
        i += 1;
    }
    if (ret != SQLITE_DONE) {
        SQL_ELOG("failed to list entries");
        goto err;
    }

    qsort(items, items_count, sizeof(items[0]), compare_items);

    for (int i = 0; i < items_count; i++) {
        switch (items[i].type) {
        case IDLE: {
            check_idle(&items[i], max_digits);
            break;
        }
        case DEADLINE: {
            check_deadline(&items[i], max_digits);
            break;
        }
        case PERIODIC: {
            check_periodic(&items[i], max_digits);
            free(items[i].as.periodic.cron_expr);
            break;
        }
        default: {
            LOG("got invalid item type from db??? wtf");
            goto err;
        }
        }

        free(items[i].title);
        free(items[i].body);
    }

    free(items);
    sqlite3_finalize(sql_count_stmt);
    sqlite3_finalize(sql_stmt);
    return 0;

err:
    return 1;
}

