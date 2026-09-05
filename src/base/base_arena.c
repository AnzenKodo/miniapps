// ak: Arena
#include <stdio.h>
//=============================================================================

internal Arena *_arena_alloc(ArenaParams *params)
{
    Arena *result = NULL;
    size_t pagesize = os_pagesize_get();
    size_t reserve_size = AlignPow2(params->reserve_size, pagesize);
    size_t commit_size = AlignPow2(params->commit_size, pagesize);
    Arena* arena = (Arena *)os_mem_reserve(reserve_size);
    if (os_mem_commit(arena, commit_size))
    {
        arena->reserve_size = reserve_size;
        arena->commit_size = commit_size;
        arena->pos = ARENA_HEADER_SIZE;
        arena->commit_pos = commit_size;
        arena->allocation_site_file = params->allocation_site_file;
        arena->allocation_site_line = params->allocation_site_line;
        result = arena;
        AsanPoisonMemoryRegion(arena, commit_size);
        AsanUnpoisonMemoryRegion(arena, ARENA_HEADER_SIZE);
    }
    return result;
}
internal void arena_free(Arena* arena)
{
    AsanUnpoisonMemoryRegion(arena, arena->reserve_size);
    os_mem_release(arena, arena->reserve_size);
}

internal size_t arena_pos(Arena *arena)
{
    return arena->pos;
}

internal void *_arena_push(Arena *arena, size_t size, size_t align, bool fill_zero)
{
    char *result = NULL;
    size_t pos_align = AlignPow2(arena->pos, align);
    size_t pos_new = pos_align + size;
    if (pos_new < arena->reserve_size)
    {
        if (pos_new > arena->commit_pos)
        {
            size_t commit_pos_new = pos_new;
            commit_pos_new += arena->commit_size - 1;
            commit_pos_new -= commit_pos_new % arena->commit_size;
            commit_pos_new =  Min(commit_pos_new, arena->reserve_size);
            char* mem = (char*)arena + arena->commit_pos;
            size_t commit_size = commit_pos_new - arena->commit_pos;
            if (os_mem_commit(mem, commit_size))
            {
                arena->commit_pos = commit_pos_new;
                AsanPoisonMemoryRegion(mem, commit_size);
            }
        }
        arena->pos = pos_new;
        result = (char*)arena + pos_align;
        AsanUnpoisonMemoryRegion(result, size);
        if (fill_zero)
        {
            mem_set(result, 0, size);
        }
    }
    return result;
}
internal void arena_pop(Arena *arena, size_t size)
{
    size = Min(size, arena->pos - ARENA_HEADER_SIZE);
    arena->pos -= size;
    AsanPoisonMemoryRegion((char*)arena + arena->pos, size);
}
internal void arena_pop_to(Arena *arena, size_t pos)
{
    size_t size = pos < arena->pos ? arena->pos - pos : 0;
    arena_pop(arena, size);
}
internal void arena_clear(Arena *arena)
{
    arena_pop_to(arena, ARENA_HEADER_SIZE);
}

// ak: Temp Arena
//=============================================================================

internal Arena_Temp arena_temp_begin(Arena *arena)
{
    return (Arena_Temp)
    {
        .arena = arena,
        .pos = arena->pos,
    };
}
internal void arena_temp_end(Arena_Temp temp)
{
    arena_pop_to(temp.arena, temp.pos);
}

// ak: Scratch Arena
//=============================================================================

internal Arena *arena_scratch(Arena **conflicts, size_t length)
{
    Arena *result = 0;
    Arena **arena_ptr = _arena_scratch_global;
    for (size_t i = 0; i < ArrayLength(_arena_scratch_global); i += 1, arena_ptr += 1)
    {
        Arena **conflict_ptr = conflicts;
        bool has_conflict = 0;
        for (size_t j = 0; j < length; j += 1, conflict_ptr += 1)
        {
            if (*arena_ptr == *conflict_ptr)
            {
                has_conflict = 1;
                break;
            }
        }
        if (!has_conflict)
        {
            result = *arena_ptr;
            if (result == NULL)
            {
                result = arena_alloc();
                *arena_ptr = result;
            }
            break;
        }
    }
    return result;
}
