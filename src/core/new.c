#include "new.h"

#include "core.h"

#include "../config/config.h"
#include "../utils/macros.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static inline Result create_dir(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char cmd[size];
    size_t offset = snprintf(cmd, size, "mkdir -p ");
    offset += snprintf(cmd + offset, size - offset, "%s/src", name);

    if (system(cmd) != 0) {
        return err("Mkdir failed");
    }

    return ok(NULL);
}

static inline Result create_config(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char path[size];
    snprintf(path, size, "%s/config.cat", name);

    FILE* fptr = fopen(path, "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        return err("Failed to write to config.cat");
    }

    write_config(fptr, name);

    fclose(fptr);
    return ok(NULL);
}

static inline Result create_main(const char* name) {
    size_t size = 32 + MAX_NAME_LEN;
    char path[size];
    snprintf(path, size, "%s/src/main.c", name);

    FILE* fptr = fopen(path, "w");
    if (UNLIKELY(fptr == NULL)) {
        fclose(fptr);
        return err("Failed to write to main.c");
    }

    fprintf(fptr, "#include <stdio.h>\n\nint main(int argc, char* argv[]) {\n\tprintf(\"Hello world!\");\n}");

    fclose(fptr);
    return ok(NULL);
} 

Result new_project(const char* name) {
    if (strlen(name) > MAX_NAME_LEN) {
        return err("Project name too long");
    }

    Result result = create_dir(name);
    if (UNLIKELY(IS_ERR(result))) {
        return err(ERR_MSG(result));
    }

    result = create_config(name);
    if (UNLIKELY(IS_ERR(result))) {
        return err(ERR_MSG(result));
    }

    result = create_main(name);
    if (UNLIKELY(IS_ERR(result))) {
        return err(ERR_MSG(result));
    }

    return ok(NULL);
}
