#include "lexer.h"
#include "whisker/stringd/stringd.h"
#include "whisker/macros.h"

#include <assert.h>
#include <immintrin.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>

Lexer* create_lexer(char* buffer, size_t max_len) {
    assert(buffer && max_len > 0);

    Lexer* lexer = malloc(sizeof(*lexer));
    if (LIKELY(lexer != NULL)) {
        lexer -> mapped_allocation = false;
    } else {
        lexer = mmap(NULL, sizeof(*lexer), PROT_READ | PROT_WRITE, MAP_PRIVATE, 0, 0);
        lexer -> mapped_allocation = true;

        if (UNLIKELY(lexer == MAP_FAILED)) {
            fprintf(stderr, "Out of memory!\n");
            return NULL;
        }
    }

    lexer -> buffer = buffer;
    lexer -> cursor = lexer -> buffer;
    lexer -> len = ws_strnlen(buffer, max_len);

    return lexer;
}

inline void destroy_lexer(Lexer* lexer) {
    assert(lexer);

    free(lexer -> buffer);
    if (LIKELY(lexer -> mapped_allocation == false)) {
        free(lexer);
        return;
    }
    munmap(lexer, lexer -> len);
}

static inline void parse_chunk(char* cursor, const char* end, uint64_t mask_64, char** word_start, size_t* word_len, bool* in_word, size_t* words) {
    char* p = cursor;
    uint64_t mask = mask_64;
    size_t* count = words;

    if (UNLIKELY(*in_word)) {
        if (UNLIKELY(mask == 0)) {
            *word_len += 64;
            return;
        } 

        uint8_t index = __builtin_ctzll(mask);
        *word_len += index;
        // printf("IN WORD: %.*s | %zu\n", (int)(*word_len), *word_start, *word_len);
        (*count)++;

        *in_word = false;

        p += index + 1;
        if (LIKELY(index + 1 < 64)) {
            mask >>= (index + 1);
        } else {
            mask = 0;
        }
    }

    while (mask != 0) {
        uint8_t index = __builtin_ctzll(mask);

        if (UNLIKELY(index > 0)) {
            *word_start = p;
            *word_len = index;
            // printf("WORD: %.*s | %zu\n", (int)(*word_len), *word_start, *word_len);
            (*count)++;
        }

        p += index + 1;
        if (LIKELY(index + 1 < 64)) {
            mask >>= (index + 1);
        } else {
            mask = 0;
        }
    }

    size_t remaining = end - p;
    if (UNLIKELY(remaining > 0)) {
        *in_word = true;
        *word_start = p;
        *word_len = remaining;
    }
}

static inline void lexer_parse_avx2(Lexer* lexer) {
    char* cursor = lexer -> cursor;
    const char* end = lexer -> buffer + lexer -> len;

    bool in_word = false;
    char* word_start;
    size_t word_len = 0;

    size_t words = 0;

    const __m256i ws_space = _mm256_set1_epi8(' ');
    const __m256i ws_tab = _mm256_set1_epi8('\t');
    const __m256i ws_nl = _mm256_set1_epi8('\n');
    const __m256i ws_cr = _mm256_set1_epi8('\r');

    while (LIKELY((size_t) (end - cursor) >= 64)) {
        __m256i chunk = _mm256_load_si256((const __m256i*) cursor);
        __m256i chunk2 = _mm256_load_si256((const __m256i*) (cursor + 32));

        __m256i m_space = _mm256_cmpeq_epi8(chunk, ws_space);
        __m256i m_tab = _mm256_cmpeq_epi8(chunk, ws_tab);
        __m256i m_nl = _mm256_cmpeq_epi8(chunk, ws_nl);
        __m256i m_cr = _mm256_cmpeq_epi8(chunk, ws_cr);

        __m256i m_space_2 = _mm256_cmpeq_epi8(chunk2, ws_space);
        __m256i m_tab_2 = _mm256_cmpeq_epi8(chunk2, ws_tab);
        __m256i m_nl_2 = _mm256_cmpeq_epi8(chunk2, ws_nl);
        __m256i m_cr_2 = _mm256_cmpeq_epi8(chunk2, ws_cr);

        __m256i ws_1 = _mm256_or_si256(m_space, m_tab);
        ws_1 = _mm256_or_si256(ws_1, m_nl);
        ws_1 = _mm256_or_si256(ws_1, m_cr);

        __m256i ws_2 = _mm256_or_si256(m_space_2, m_tab_2);
        ws_2 = _mm256_or_si256(ws_2, m_nl_2);
        ws_2 = _mm256_or_si256(ws_2, m_cr_2);

        uint64_t mask1 = _mm256_movemask_epi8(ws_1);
        uint64_t mask2 = _mm256_movemask_epi8(ws_2);

        uint64_t mask = mask1 | (mask2 << 32);

        if (UNLIKELY(mask == 0xffffffffffffffff)) {
            cursor += 64;
            continue;
        }

        parse_chunk(cursor, cursor + 64, mask, &word_start, &word_len, &in_word, &words);
        cursor += 64;
    }

    printf("Count: %zu\n", words);
}

static inline void lexer_parse_generic(Lexer* lexer) {
    fprintf(stderr, "AVX2 not supported! Closing...");
    destroy_lexer(lexer);
    exit(1);
}

static void* resolver_lexer(void) {
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx2")) {
        return lexer_parse_avx2;
    } else {
        return lexer_parse_generic;
    }
}

void lexer_parse(Lexer* lexer) __attribute__((ifunc("resolver_lexer")));

void lex(Lexer* lexer) {
    lexer_parse(lexer);
}
