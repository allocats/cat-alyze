#ifndef CHAR_MAP_H
#define CHAR_MAP_H

#include <stdint.h>

#define IS_ALPHA(c) (char_map[(unsigned char)(c)] & 1)
#define IS_WHITESPACE(c) (char_map[(unsigned char)(c)] & 2)
#define IS_IDENTIFIER(c) (char_map[(unsigned char)(c)] & 4)
#define IS_DELIM(c) (char_map[(unsigned char)(c)] & 8)

static const uint8_t char_map[256] = {
    ['0' ... '9'] = 1,
    ['a' ... 'z'] = 1,
    ['A' ... 'Z'] = 1,

    ['_'] = 1,
    ['-'] = 1,
    ['/'] = 1,
    ['.'] = 1,
    ['*'] = 1,
    ['='] = 1,

    [' '] = 2, 
    ['\t'] = 2, 
    ['\n'] = 2,

    [':'] = 4,

    ['{'] = 8,
    ['}'] = 8,
};

#endif // !CHAR_MAP_H
