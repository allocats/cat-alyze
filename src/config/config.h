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
#define MAX_BUILD_DIR_LEN 64
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
    char* name;
    char** sources;
    char** flags;

    uint8_t source_count : 4;
    uint8_t flag_count : 4;

    char* output_dir;
    char* output_name;
} __attribute__((aligned(8))) Target;

typedef struct {
    uint8_t nest_count : 4;
    uint8_t target_count : 4;
    Target** targets; 

    char* compiler;
    char** default_flags;
    uint8_t default_flag_count;

    char* build_dir;
} __attribute__((aligned(8))) CatalyzeConfig;

Result push_default_flag(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);
Result push_flag(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);
Result push_source(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);

Result set_compiler(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);
Result set_build_dir(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);

Result set_single(Arena* arena, char** dest, const char* start, size_t* len);

Result set_name(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);
Result set_type(Arena* arena, CatalyzeConfig* config, TargetType type);
Result set_output_dir(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);
Result set_output_name(Arena* arena, CatalyzeConfig* config, const char* start, size_t* len);

Result find_config_file(Arena* arena, uint8_t* nest_count);
Result parse_config(Arena* arena);
void print_config(const CatalyzeConfig* config);

#endif // !CONFIG_H
