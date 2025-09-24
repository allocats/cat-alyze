#include "init.h"

#include "core.h"

#include "../config/config.h"
#include "../utils/macros.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static inline Result create_dir() {
    size_t size = 32;
    char cmd[size];
    snprintf(cmd, size, "mkdir -p src");

    if (system(cmd) != 0) {
        return err("Mkdir failed");
    }

    return ok(NULL);
}

static inline Result create_config(const char* name) {
    FILE* fptr = fopen("config.cat", "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        return err("Failed to write to config.cat");
    }

    write_config(fptr, name);

    fclose(fptr);
    return ok(NULL);
}

static inline Result create_main() {
    FILE* fptr = fopen("src/main.c", "w");
    if (fptr == NULL) {
        fclose(fptr);
        return err("Failed to write to main.c");
    }

    fprintf(fptr, "#include <stdio.h>\n\nint main(int argc, char* argv[]) {\n\tprintf(\"Hello world!\");\n}");

    fclose(fptr);
    return ok(NULL);
} 

Result init_project() {
    char cwd[MAX_NAME_LEN];
    getcwd(cwd, MAX_NAME_LEN);

    const char* name = strrchr(cwd, '/');
    name = name ? name + 1 : cwd;

    Result result = create_dir();
    if (UNLIKELY(IS_ERR(result))) {
        return err(ERR_MSG(result));
    }

    result = create_config(name);
    if (UNLIKELY(IS_ERR(result))) {
        return err(ERR_MSG(result));
    }

    result = create_main();
    if (UNLIKELY(IS_ERR(result))) {
        return err(ERR_MSG(result));
    }

    return ok(NULL);
}
