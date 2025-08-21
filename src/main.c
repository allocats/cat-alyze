#include <stdio.h>

#include "utils/arena.h"
#include "utils/result.h"

#include "config/config.h"

static Arena arena = {0};

int main(int argc, char *argv[]) {
    Result config_file = find_config_file(&arena);

    if (IS_OK(config_file)) {
        printf("config.cat found!\n");
    } else  {
        printf("%s", ERR_MSG(config_file));
    }

    arena_free(&arena);
}
