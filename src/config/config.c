#include "config.h"

#include "lexer.h"
#include "../utils/macros.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void config_err(const char* msg) {
    printf("\033[1mError:\033[0m %s\n", msg);
    exit(1);
}

static char* find_config_file(ArenaAllocator* arena, uint8_t* nest_count) {
    static const char* path_templates[] = {
        "./config.cat",
        "../config.cat", 
        "../../config.cat",
        "../../../config.cat",
        "../../../../config.cat",
        "../../../../../config.cat",
        "../../../../../../config.cat",
        "../../../../../../../config.cat",
        "../../../../../../../../config.cat",
        "../../../../../../../../../config.cat",
        "../../../../../../../../../../config.cat",
        "../../../../../../../../../../../config.cat",
        "../../../../../../../../../../../../config.cat",
        "../../../../../../../../../../../../../config.cat",
        "../../../../../../../../../../../../../../config.cat",
        NULL
    };
    
    for (int i = 0; path_templates[i]; i++) {
        FILE* fptr = fopen(path_templates[i], "r");
        if (fptr) {
            fclose(fptr);
            *nest_count = i;
            
            size_t path_len = strlen(path_templates[i]);
            char* result_path = arena_alloc(arena, path_len + 1);
            if (UNLIKELY(!result_path)) {
                config_err("Arena allocation failed");
            }

            strcpy(result_path, path_templates[i]);
            return result_path;
        }
    }
    
    config_err("config.cat not found");
    exit(1);
}

CatalyzeConfig* parse_config(ArenaAllocator* arena) { 
    uint8_t nest_count = 0;
    char* path = find_config_file(arena, &nest_count);

    int fd = open(path, O_RDONLY);
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
    ssize_t bytes_read = read(fd, buffer, st.st_size);
    if (UNLIKELY(bytes_read != st.st_size)) {
        close(fd);
        config_err("Failed to read config.cat");
    }

    close(fd);
    buffer[st.st_size] = '\0';

    return lexer_parse(arena, buffer, st.st_size, nest_count);
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
    printf("  nest_count: %u\n", config->nest_count);

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
