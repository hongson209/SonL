#include "../include/son_common.h"
#include <stdarg.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#endif

static char g_sdk_root[1024] = ".";

static bool dir_has_library(const char *dir) {
    char probe[1024];
    snprintf(probe, sizeof(probe), "%s/library/core/types.sonl", dir);
    FILE *f = fopen(probe, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    return false;
}

void son_init_sdk_root(const char *argv0) {
    // 1. Highest priority: explicit environment variable
    const char *env_home = getenv("SON_HOME");
    if (!env_home) env_home = getenv("SONLANG_HOME");
    if (env_home && env_home[0] != '\0' && dir_has_library(env_home)) {
        snprintf(g_sdk_root, sizeof(g_sdk_root), "%s", env_home);
        return;
    }

    // 2. Check current working directory
    if (dir_has_library(".")) {
        snprintf(g_sdk_root, sizeof(g_sdk_root), ".");
        return;
    }

    // 3. Auto-detect relative to the running compiler executable (<sdk_root>/bin/son.exe or <sdk_root>/son.exe)
    char exe_path[1024] = {0};
#ifdef _WIN32
    DWORD len = GetModuleFileNameA(NULL, exe_path, (DWORD)(sizeof(exe_path) - 1));
    if (len == 0 && argv0) {
        snprintf(exe_path, sizeof(exe_path), "%s", argv0);
    }
#else
    ssize_t rlen = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (rlen > 0) {
        exe_path[rlen] = '\0';
    } else if (argv0) {
        snprintf(exe_path, sizeof(exe_path), "%s", argv0);
    }
#endif

    // Normalize slashes to '/'
    for (char *p = exe_path; *p; p++) {
        if (*p == '\\') *p = '/';
    }

    // Walk up parent directories from exe_path to find 'library/core/types.sonl'
    char candidate[1024];
    snprintf(candidate, sizeof(candidate), "%s", exe_path);
    for (int depth = 0; depth < 4; depth++) {
        char *slash = strrchr(candidate, '/');
        if (!slash) break;
        *slash = '\0';
        if (candidate[0] != '\0' && dir_has_library(candidate)) {
            snprintf(g_sdk_root, sizeof(g_sdk_root), "%s", candidate);
            return;
        }
    }
}

const char* son_get_sdk_root(void) {
    return g_sdk_root;
}

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
