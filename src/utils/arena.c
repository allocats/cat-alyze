#include "arena.h"

#include <assert.h>
#include <immintrin.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define UNLIKELY(x) __builtin_expect(x, 0)
#define LIKELY(x) __builtin_expect(x, 1)

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
    ArenaBlock* block = (ArenaBlock*) malloc(total_size);
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

    while (copy_size >= 32) {
        _mm256_store_si256((__m256i*) new_ptr, _mm256_load_si256((const __m256i*) old_ptr));

        new_ptr += 32;
        old_ptr += 32;
        copy_size -= 32;
    }

    size_t zero_size = new_size - old_size;
    __m256i zeros = _mm256_setzero_si256();
    while (zero_size >= 32) {
        _mm256_store_si256((__m256i*) new_ptr, zeros);

        new_ptr += 32;
        zero_size -= 32;
    }

    while (zero_size > 0) {
        *new_ptr++ = 0;
        zero_size--;
    }

    return result;
}

void* arena_memset(void* ptr, int value, size_t len) {
    char* p = (char*) ptr;
    char char_value = (char) value;

    while (len > 0 && ((uintptr_t) p & 31)) {
        *p++ = char_value;
        len--;
    }

    __m256i byte_value = _mm256_set1_epi8(char_value);
    while (len >= 32) {
        _mm256_store_si256((__m256i*) p, byte_value);

        p += 32;
        len -= 32;
    }

    while (len > 0) {
        *p++ = char_value;
        len--;
    }

    return ptr;
}

void* arena_memcpy(void* dest, const void* src, size_t len) {
    char* d = dest;
    const char* s = src;

    while (len >= 32) {
        _mm256_storeu_si256((__m256i*) d, _mm256_loadu_si256((const __m256i*) s));

        len -= 32;
        d += 32;
        s += 32;
    }

    while (len--) {
        *d++ = *s++;
    }

    return dest;
}

char* arena_strdup(ArenaAllocator* arena, const char* str) {
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
