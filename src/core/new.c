#include "new.h"

#include "core.h"
#include "build.h"

#include "../config/config.h"
#include "../utils/macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void new_err(const char* msg) {
    printf("\033[1mError:\033[0m %s\n", msg);
    exit(1);
}

static inline void create_dir(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char cmd[size];
    snprintf(cmd, size, "%s/src", name);

    make_dir(cmd);
}

static inline void create_config(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char path[size];
    snprintf(path, size, "%s/config.cat", name);

    FILE* fptr = fopen(path, "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        new_err("Failed to write to config.cat");
    }

    write_config(fptr, name);
    fclose(fptr);
}

static inline void create_main(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char path[size];
    snprintf(path, size, "%s/src/main.c", name);

    FILE* fptr = fopen(path, "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        new_err("Failed to write to main.c");
    }

    fprintf(fptr, "#include <stdio.h>\n\nint main(int argc, char* argv[]) {\n\tprintf(\"Hello world!\");\n}");
    fclose(fptr);
} 

void new_project(const char* name) {
    if (UNLIKELY(strlen(name) > MAX_NAME_LEN)) {
        new_err("Project name too long");
    }

    create_dir(name);
    create_config(name);
    create_main(name);
}
