#include "arena.h"

#include <assert.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h> 
#include <stdio.h>
#include <string.h>

#define UNLIKELY(x) __builtin_expect(x, 0)
#define LIKELY(x) __builtin_expect(x, 1)

#define AVX2_CHUNK(p, n) (p + (32 * n))

inline size_t align_size(size_t size) {
    return (size + sizeof(void*) - 1) & ~(sizeof(void*) - 1);
}

inline void init_arena(ArenaAllocator* arena, size_t default_capacity) {
    assert(arena);
    arena -> start = NULL;
    arena -> end = NULL;
    arena -> default_capacity = default_capacity == 0 ? ARENA_DEFAULT_CAPACITY : align_size(default_capacity);
}

static __attribute__((noinline)) ArenaBlock* new_block(size_t default_capacity, size_t size) {
    size_t capacity = default_capacity;

    while (UNLIKELY(size > capacity * sizeof(uintptr_t))) {
        capacity *= 2;
    }

    size_t bytes = capacity * sizeof(uintptr_t);
    size_t total_size = sizeof(ArenaBlock) + bytes;
    ArenaBlock* block = (ArenaBlock*) aligned_alloc(32, total_size);
    assert(block);

    block -> next = NULL;
    block -> usage =  0;
    block -> capacity = bytes;

    return block;
}

static inline void free_block(ArenaBlock* block) {
    free(block);
}

inline void* arena_alloc(ArenaAllocator* arena, size_t size) {
    size = align_size(size);

    if (UNLIKELY(arena -> end == NULL && arena -> start == NULL)) {
        arena -> end = new_block(arena -> default_capacity, size);
        arena -> start = arena -> end;

        void* result = (char*) arena -> end -> data + arena -> end -> usage;
        arena -> end -> usage += size;
        return result;
    }

    while (UNLIKELY(arena -> end -> usage + size > arena -> end -> capacity && arena -> end -> next != NULL)) {
        arena -> end = arena -> end -> next;
    } 

    if (UNLIKELY(arena -> end -> usage + size > arena -> end -> capacity)) {
            ArenaBlock* block = new_block(arena -> default_capacity, size);
            arena -> end -> next = block;
            arena -> end = block;
    }

    void* result = (char*) arena -> end -> data + arena -> end -> usage;
    arena -> end -> usage += size;
    return result;
}

void* arena_realloc(ArenaAllocator* arena, void* ptr, size_t old_size, size_t new_size) {
    if (UNLIKELY(new_size <= old_size)) {
        return ptr;
    }

    void* result = arena_alloc(arena, new_size);
    char* new_ptr = (char*) result;
    char* old_ptr = (char*) ptr;
    size_t copy_size = old_size;

    while (copy_size > 0 && ((uintptr_t) new_ptr & 31)) {
        *new_ptr++ = *old_ptr++;
        copy_size--;
    }

    while (copy_size >= 128) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 0)));
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 1), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 1)));
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 2), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 2)));
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 3), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 3)));

        new_ptr += 128;
        old_ptr += 128;
        copy_size -= 128;
    }

    while (copy_size >= 96) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 0)));
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 1), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 1)));
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 2), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 2)));

        new_ptr += 96;
        old_ptr += 96;
        copy_size -= 96;
    }

    while (copy_size >= 64) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 0)));
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 1), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 1)));

        new_ptr += 64;
        old_ptr += 64;
        copy_size -= 64;
    }

    while (copy_size >= 32) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), _mm256_load_si256((const __m256i*) AVX2_CHUNK(old_ptr, 0)));

        new_ptr += 32;
        old_ptr += 32;
        copy_size -= 32;
    }

    size_t zero_size = new_size - old_size;
    __m256i zeros = _mm256_setzero_si256();
    while (zero_size >= 128) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), zeros);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 1), zeros);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 2), zeros);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 3), zeros);

        new_ptr += 128;
        zero_size -= 128;
    }

    while (zero_size >= 96) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), zeros);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 1), zeros);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 2), zeros);

        new_ptr += 96;
        zero_size -= 96;
    }

    while (zero_size >= 64) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), zeros);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 1), zeros);

        new_ptr += 64;
        zero_size -= 64;
    }

    while (zero_size >= 32) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(new_ptr, 0), zeros);

        new_ptr += 32;
        zero_size -= 32;
    }

    while (zero_size > 0) {
        *new_ptr++ = 0;
        zero_size--;
    }

    return result;
}

inline void* arena_memset(void* ptr, int value, size_t len) {
    char* p = (char*) ptr;
    char char_value = (char) value;

    while (len > 0 && ((uintptr_t) p & 31)) {
        *p++ = char_value;
        len--;
    }

    __m256i byte_value = _mm256_set1_epi8(char_value);

    while (len >= 128) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 0), byte_value);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 1), byte_value);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 2), byte_value);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 3), byte_value);

        p += 128;
        len -= 128;
    }

    while (len >= 96) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 0), byte_value);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 1), byte_value);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 2), byte_value);

        p += 96;
        len -= 96;
    }

    while (len >= 64) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 0), byte_value);
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 1), byte_value);

        p += 64;
        len -= 64;
    }

    while (len >= 32) {
        _mm256_store_si256((__m256i*) AVX2_CHUNK(p, 0), byte_value);

        p += 32;
        len -= 32;
    }

    while (len > 0) {
        *p++ = char_value;
        len--;
    }

    return ptr;
}

inline void* arena_memcpy(void* dest, const void* src, size_t len) {
    char* d = dest;
    const char* s = src;


    while (len >= 128) {
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 0), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 0)));
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 1), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 1)));
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 2), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 2)));
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 3), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 3)));

        len -= 128;
        d += 128;
        s += 128;
    }

    while (len >= 96) {
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 0), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 0)));
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 1), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 1)));
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 2), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 2)));

        len -= 96;
        d += 96;
        s += 96;
    }

    while (len >= 64) {
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 0), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 0)));
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 1), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 1)));

        len -= 64;
        d += 64;
        s += 64;
    }

    while (len >= 32) {
        _mm256_storeu_si256((__m256i*) AVX2_CHUNK(d, 0), _mm256_loadu_si256((const __m256i*) AVX2_CHUNK(s, 0)));

        len -= 32;
        d += 32;
        s += 32;
    }

    while (len--) {
        *d++ = *s++;
    }

    return dest;
}

inline char* arena_strdup(ArenaAllocator* arena, const char* str) {
    size_t len = strlen(str);
    char* duplicate = (char*) arena_alloc(arena, len + 1);

    arena_memcpy(duplicate, str, len + 1);
    duplicate[len] = '\0';

    return duplicate;
}

inline void arena_reset(ArenaAllocator* arena) {
    for (ArenaBlock* block = arena -> start; block != NULL; block = block -> next) {
        block -> usage = 0;
    }
    arena -> end = arena -> start;
}

void arena_free(ArenaAllocator* arena) {
    ArenaBlock* block = arena -> start;

    while (block != NULL) {
        ArenaBlock* previous = block;
        block = block -> next;
        free_block(previous);
    }

    arena -> start = NULL;
    arena -> end = NULL;
}

size_t total_capacity(ArenaAllocator* arena) {
    ArenaBlock* current = arena -> start;
    size_t total = 0;

    while (current != NULL) {
        total += current -> capacity;
        current = current -> next;
    }
    
    return total;
}

size_t total_usage(ArenaAllocator* arena) {
    ArenaBlock* current = arena -> start;
    size_t total = 0;

    while (current != NULL) {
        total += current -> usage;
        current = current -> next;
    }
    
    return total;
}
