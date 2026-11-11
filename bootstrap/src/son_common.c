#include "../include/son_common.h"
#include <stdarg.h>

void son_arena_init(SonArena *arena, size_t default_block_size) {
    arena->current = NULL;
    arena->default_block_size = default_block_size ? default_block_size : (64 * 1024);
}

void* son_arena_alloc(SonArena *arena, size_t size, size_t align) {
    if (align == 0) align = sizeof(void*);
    
    // Check if current block has enough capacity
    if (arena->current) {
        size_t current_addr = (size_t)(arena->current->memory + arena->current->used);
        size_t aligned_addr = (current_addr + align - 1) & ~(align - 1);
        size_t padding = aligned_addr - current_addr;
        
        if (arena->current->used + padding + size <= arena->current->capacity) {
            arena->current->used += padding + size;
            return (void*)aligned_addr;
        }
    }

    // Allocate new block
    size_t block_capacity = arena->default_block_size;
    if (size > block_capacity) {
        block_capacity = size + align;
    }

    SonArenaBlock *new_block = (SonArenaBlock*)malloc(sizeof(SonArenaBlock) + block_capacity);
    if (!new_block) {
        fprintf(stderr, "FATAL: Out of memory in bootstrap arena allocator\n");
        exit(1);
    }

    new_block->capacity = block_capacity;
    new_block->used = 0;
    new_block->next = arena->current;
    arena->current = new_block;

    size_t current_addr = (size_t)new_block->memory;
    size_t aligned_addr = (current_addr + align - 1) & ~(align - 1);
    size_t padding = aligned_addr - current_addr;
    new_block->used = padding + size;

    return (void*)aligned_addr;
}

void son_arena_free_all(SonArena *arena) {
    SonArenaBlock *curr = arena->current;
    while (curr) {
        SonArenaBlock *next = curr->next;
        free(curr);
        curr = next;
    }
    arena->current = NULL;
}

void son_error_report(SonSourceLoc loc, const char *fmt, ...) {
    fprintf(stderr, "%s:%zu:%zu: error: ", loc.filename ? loc.filename : "<input>", loc.line, loc.column);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
