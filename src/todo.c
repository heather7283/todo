#include <unistd.h>

#include "utils.h"

int main(int argc, char **argv) {
    const char *tmpfile_path = get_tmpfile_path();

    int fd = open_and_edit(tmpfile_path, "sussus amogus\n");
    if (fd < 0) {
        goto err;
    }
    unlink(tmpfile_path);

    sleep(60);

    return 0;

err:
    return 1;
}
