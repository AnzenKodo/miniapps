#ifndef BASE_ALLOC_H
#define BASE_ALLOC_H

// ak: Arena
//=============================================================================

typedef struct Arena Arena;
struct Arena {
    size_t reserve_size;
    size_t commit_size;
    size_t commit_pos;
    size_t pos;
    char *allocation_site_file;
    int allocation_site_line;
};

typedef struct ArenaParams ArenaParams;
struct ArenaParams
{
    uint64_t reserve_size;
    uint64_t commit_size;
    void *optional_backing_buffer;
    char *allocation_site_file;
    int allocation_site_line;
};

#define ARENA_DEFAULT_RESERVE_SIZE MB(64)
#define ARENA_DEFAULT_COMMIT_SIZE  KB(64)

internal Arena *_arena_alloc(ArenaParams *params);
#if LANGUAGE_CPP
    inline Arena *arena_alloc_cpp(
        uint64_t reserve_size = ARENA_DEFAULT_RESERVE_SIZE,
        uint64_t commit_size = ARENA_DEFAULT_COMMIT_SIZE,
        void *optional_backing_buffer = NULL,
        const char *allocation_site_file = FILE_NAME,
        int allocation_site_line = LINE_NUMBER)
    {
        ArenaParams params = {
            reserve_size,
            commit_size,
            optional_backing_buffer,
            (char *)allocation_site_file, // Explicit cast to silence ISO C++ warning
            allocation_site_line
        };
        return _arena_alloc(&params);
    }
    #define arena_alloc(...) arena_alloc_cpp(__VA_ARGS__)
#else
    #define arena_alloc(...) _arena_alloc(&(ArenaParams){.reserve_size = ARENA_DEFAULT_RESERVE_SIZE, .commit_size = ARENA_DEFAULT_COMMIT_SIZE, .allocation_site_file = FILE_NAME, .allocation_site_line = LINE_NUMBER, __VA_ARGS__})
#endif
internal void arena_free(Arena *arena);

internal void *_arena_push(Arena *arena, size_t size, size_t align, bool fill_zero);
#define ARENA_HEADER_SIZE (sizeof(Arena))
#define arena_push(a, T, n) ((T *)(_arena_push(a, sizeof(T)*(n), AlignOf(T), true)))
#define arena_push_nz(a, T, n) ((T *)(_arena_push(a, sizeof(T)*(n), AlignOf(T), false)))
internal void arena_pop(Arena *arena, size_t size);
internal void arena_pop_to(Arena *arena, size_t pos);
internal void arena_clear(Arena *arena);

// ak: Temp Arena
//=============================================================================

typedef struct Arena_Temp Arena_Temp;
struct Arena_Temp
{
    Arena *arena;
    size_t pos;
};

internal Arena_Temp arena_temp_begin(Arena *arena);
internal void arena_temp_end(Arena_Temp temp);

// ak: Scratch Arena
//=============================================================================

thread_static Arena *_arena_scratch_global[2] = { NULL, NULL };

#define arena_scratch_begin(conflicts, count) arena_temp_begin(arena_scratch((conflicts), (count)))
#define arena_scratch_end(scratch) arena_temp_end(scratch)

internal Arena *arena_scratch(Arena **conflicts, size_t length);

#endif // BASE_ALLOC_H
