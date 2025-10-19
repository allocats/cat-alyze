#include "config.h"

#include "lexer.h"
#include "../utils/macros.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void config_err(const char* msg) {
    fprintf(stderr, "\033[1mError:\033[0m %s\n%s\n", msg, strerror(errno));
    exit(1);
}

static PathMap* find_config_file(ArenaAllocator* arena) {
    const PathMap path_templates[] = {
        { "./config.cat", 12 },
        { "../config.cat", 13 },
        { "../../config.cat", 16 },
        { "../../../config.cat", 19 },
        { "../../../../config.cat", 22 },
        { "../../../../../config.cat", 25 },
        { "../../../../../../config.cat", 28 },
        { "../../../../../../../config.cat", 31 },
        { "../../../../../../../../config.cat", 34 },
        { "../../../../../../../../../config.cat", 37 },
        { "../../../../../../../../../../config.cat", 40 },
        { "../../../../../../../../../../../config.cat", 43 },
        { "../../../../../../../../../../../../config.cat", 46 },
        { "../../../../../../../../../../../../../config.cat", 49 },
        { "../../../../../../../../../../../../../../config.cat", 52 },
        { NULL, 0 }
    };
    
    for (int i = 0; path_templates[i].path != NULL; i++) {
        FILE* fptr = fopen(path_templates[i].path, "r");
        if (fptr) {
            fclose(fptr);
            
            size_t path_len = path_templates[i].len;
            char* result_path = arena_alloc(arena, path_len + 1);
            if (UNLIKELY(!result_path)) {
                config_err("Arena allocation failed");
            }

            strcpy(result_path, path_templates[i].path);
            
            PathMap* result = arena_alloc(arena, sizeof(*result));
            result -> path = result_path;
            result -> len = path_len - 10;

            return result;
        }
    }
    
    config_err("config.cat not found");
    exit(1);
}

CatalyzeConfig* parse_config(ArenaAllocator* arena) { 
    PathMap* map = find_config_file(arena);

    int fd = open(map -> path, O_RDONLY);
    if (UNLIKELY(fd == -1)) {
        config_err("Failed to open config.cat");
    }

    struct stat st;
    int result = fstat(fd, &st);
    if (UNLIKELY(result == -1)) {
        close(fd);
        config_err("Failed to get stats about config.cat");
    }

    char* buffer = arena_alloc(arena, st.st_size + 1);
    ssize_t bytes_read = 0;
    while (LIKELY(bytes_read < st.st_size)) {
        ssize_t n = read(fd, buffer + bytes_read, st.st_size - bytes_read);

        if (UNLIKELY(n <= 0)) {
            if (n == 0) break;
            if (n == -1) {
                if (errno == EINTR) continue;
                close(fd);
                config_err("Failed to read config.cat");
            }
        }

        bytes_read += n;
    }

    close(fd);
    buffer[st.st_size] = '\0';

    const char* prefix = map -> path;
    char* end = strrchr(prefix, '/');
    *(end + 1) = 0;

    return lexer_parse(arena, buffer, st.st_size, prefix, map -> len);
}

static const char* target_type_to_string(TargetType type) {
    switch (type) {
        case Executable: return "Executable";
        case Debug: return "Debug";
        case Test: return "Test";
        default: return "Unknown";
    }
}

void print_target(const Target* target, int indent) {
    printf("%*sTarget {\n", indent, "");
    printf("%*s  name: %s\n", indent, "", target->name ? target->name : "(null)");
    printf("%*s  type: %s\n", indent, "", target_type_to_string(target->type));
    printf("%*s  output_dir: %s\n", indent, "", target->output_dir ? target->output_dir : "(null)");
    printf("%*s  output_name: %s\n", indent, "", target->output_name ? target->output_name : "(null)");
    printf("%*s  flag_count: %u\n", indent, "", target->flag_count);
    printf("%*s  flags: [\n", indent, "");

    for (uint8_t i = 0; i < target->flag_count; i++) {
        printf("%*s    [%u]: %s\n", indent, "", i, target->flags[i] ? target->flags[i] : "(null)");
    }

    printf("%*s  ]\n", indent, "");

    printf("%*s  source_count: %u\n", indent, "", target->source_count);
    printf("%*s  sources: [\n", indent, "");

    for (uint8_t i = 0; i < target->source_count; i++) {
        printf("%*s    [%u]: %s\n", indent, "", i, target->sources[i] ? target->sources[i] : "(null)");
    }

    printf("%*s  ]\n", indent, "");
    printf("%*s}\n", indent, "");
}

void print_catalyze_config(const CatalyzeConfig* config) {
    printf("CatalyzeConfig {\n");
    printf("  compiler: %s\n", config->compiler ? config->compiler : "(null)");
    printf("  build_dir: %s\n", config->build_dir ? config->build_dir : "(null)");
    printf("  prefix: %s\n", config->prefix);

    printf("  flag_count: %u\n", config->default_flag_count);
    printf("  flags: [\n");

    for (uint8_t i = 0; i < config->default_flag_count; i++) {
        printf("    [%u]: %s\n", i, config->default_flags[i] ? config->default_flags[i] : "(null)");
    }

    printf("  ]\n");
    printf("  target_count: %u\n", config->target_count);
    printf("  targets: [\n");

    for (uint8_t i = 0; i < config->target_count; i++) {
        printf("    [%u]:\n", i);
        print_target(&config->targets[i], 4);
    }

    printf("  ]\n");
    printf("}\n");
}
