#include "init.h"

#include "build.h"
#include "core.h"

#include "../config/config.h"
#include "../utils/macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static inline void init_err(const char* msg) {
    printf("\e[1mError:\e[0m %s\n", msg);
    exit(1);
}

static inline void create_dir() {
    make_dir("src");
}

static inline void create_config(const char* name) {
    FILE* fptr = fopen("config.cat", "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        init_err("Failed to write to config.cat");
    }

    write_config(fptr, name);
    fclose(fptr);
}

static inline void create_main() {
    FILE* fptr = fopen("src/main.c", "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        init_err("Failed to write to main.c");
    }

    fprintf(fptr, "#include <stdio.h>\n\nint main(int argc, char* argv[]) {\n\tprintf(\"Hello world!\");\n}");

    fclose(fptr);
} 

void init_project() {
    char cwd[MAX_NAME_LEN];
    getcwd(cwd, MAX_NAME_LEN);

    const char* name = strrchr(cwd, '/');
    name = name ? name + 1 : cwd;

    create_dir();
    create_config(name);
    create_main();
}
