#ifndef LEXER_H
#define LEXER_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    char* buffer;
    char* cursor;
    size_t len;
    bool mapped_allocation;
} Lexer;

Lexer* create_lexer(char* buffer, size_t max_len);
void destroy_lexer(Lexer* lexer);

void lex(Lexer* lexer);

#endif // !LEXER_H
