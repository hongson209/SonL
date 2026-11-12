#ifndef SON_COMMON_H
#define SON_COMMON_H

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Source location tracking
typedef struct {
    const char *filename;
    size_t line;
    size_t column;
} SonSourceLoc;

// Immutable string slice
typedef struct {
    const char *data;
    size_t length;
} SonStringSlice;

static inline SonStringSlice son_slice_from_cstr(const char *str) {
    SonStringSlice slice;
    slice.data = str;
    slice.length = str ? strlen(str) : 0;
    return slice;
}

static inline bool son_slice_eq(SonStringSlice a, SonStringSlice b) {
    if (a.length != b.length) return false;
    return memcmp(a.data, b.data, a.length) == 0;
}

static inline bool son_slice_eq_cstr(SonStringSlice a, const char *cstr) {
    size_t len = strlen(cstr);
    if (a.length != len) return false;
    return memcmp(a.data, cstr, len) == 0;
}

// Simple bump arena allocator for Stage 0 bootstrap compiler
typedef struct SonArenaBlock {
    struct SonArenaBlock *next;
    size_t capacity;
    size_t used;
    uint8_t memory[];
} SonArenaBlock;

typedef struct {
    SonArenaBlock *current;
    size_t default_block_size;
} SonArena;

void son_arena_init(SonArena *arena, size_t default_block_size);
void* son_arena_alloc(SonArena *arena, size_t size, size_t align);
void son_arena_free_all(SonArena *arena);

// Diagnostic reporting
void son_error_report(SonSourceLoc loc, const char *fmt, ...);

// Global SDK / Standard Library resolution
void son_init_sdk_root(const char *argv0);
const char* son_get_sdk_root(void);

#endif // SON_COMMON_H
