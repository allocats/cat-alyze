#ifndef CONFIG_H
#define CONFIG_H

#include "../utils/arena.h"
#include "../utils/result.h"

#include <stdint.h>

#define MAX_PATH 128

typedef struct {
    char compiler[64];
    char flags[32][128];
    uint16_t flag_count;

    char sources[256][128];
    uint8_t source_count;

    char build_dir[128];
    char output_dir[128];
    char output_name[64];
} CatalyzeConfig;

Result find_config_file(Arena* arena);
Result parse_config(Arena* arena);

#endif // !CONFIG_H
