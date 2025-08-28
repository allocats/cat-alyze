#ifndef CONFIG_H
#define CONFIG_H

#include "../utils/arena.h"
#include "../utils/result.h"

#include <stdint.h>

#define MAX_NAME_LEN 64 
#define MAX_PATH 128
#define MAX_COMPILER_LEN 64
#define MAX_FLAGS 32
#define MAX_FLAG_LEN 128
#define MAX_SOURCES 128
#define MAX_SOURCE_LEN 128
#define MAX_BUILD_DIR_LEN 128
#define MAX_OUTPUT_DIR_LEN 128
#define MAX_OUTPUT_NAME_LEN 128
#define MAX_TARGETS 16

typedef enum {
    Executable,
    Debug,
    Test,
    StaticLib,
    SharedLib
} TargetType;

typedef struct {
    TargetType type;
    char name[MAX_NAME_LEN];
    char sources[MAX_SOURCES][MAX_SOURCE_LEN];
    char flags[MAX_FLAGS][MAX_FLAG_LEN];

    uint8_t source_count : 4;
    uint8_t flag_count : 4;

    char output_dir[MAX_OUTPUT_DIR_LEN];
    char output_name[MAX_OUTPUT_NAME_LEN];
} Target;

typedef struct {
    uint8_t nest_count : 4;
    uint8_t target_count : 4;

    char compiler[MAX_COMPILER_LEN];
    char default_flags[MAX_FLAGS][MAX_FLAG_LEN];
    uint8_t default_flag_count;
    char build_dir[MAX_BUILD_DIR_LEN];
    Target targets[MAX_TARGETS]; 
} CatalyzeConfig;

Result find_config_file(Arena* arena, uint8_t* nest_count);
Result parse_config(Arena* arena);
void print_config(const CatalyzeConfig* config);

#endif // !CONFIG_H
