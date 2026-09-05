//~ ak: Memory Allocation
//=============================================================================

internal void * os_mem_alloc(size_t size)
{
    void *result = os_mem_reserve(size);
    os_mem_commit(result, size);
    return result;
}

//~ ak: File System
//=============================================================================

//- ak: File Read

internal size_t os_file_read_full(Os_File file, void **data, Arena *arena)
{
    Str8 content = os_file_read_str_full(file, arena);
    *data = content.cstr;
    return content.size;
}

internal Str8 os_file_read_str(Os_File file, Rng1_U64 range, Arena *arena)
{
    size_t pre_pos = arena_pos(arena);
    Str8 result = STRUCT_ZERO;
    result.size = dim_rng1(range);
    result.length = result.size;
    result.cstr = arena_push(arena, uint8_t, result.size);
    size_t actual_read_size = os_file_read(file, range, result.cstr);
    if (actual_read_size < result.length)
    {
        arena_pop_to(arena, pre_pos + actual_read_size);
        result.length = actual_read_size;
        result.size = actual_read_size;
    }
    return result;
}

internal Str8 os_file_read_str_full(Os_File file, Arena *arena)
{
    Str8 result = STRUCT_ZERO;
    Os_File_Properties prop = os_file_properties(file);
    result = os_file_read_str(file, (Rng1_U64){0, prop.size}, arena);
    return result;
}

internal size_t os_path_read(Str8 path, Rng1_U64 range, void *data)
{
    size_t result = 0;
    Os_File file = os_file_open(path, Os_AccessFlag_Read|Os_AccessFlag_ShareRead);
    result = os_file_read(file, range, data);
    os_file_close(file);
    return result;
}

internal size_t os_path_read_full(Str8 path, void **data, Arena *arena)
{
    size_t result = 0;
    Os_File file = os_file_open(path, Os_AccessFlag_Read|Os_AccessFlag_ShareRead);
    result = os_file_read_full(file, data, arena);
    os_file_close(file);
    return result;
}

internal Str8 os_path_read_str(Str8 path, Rng1_U64 range, Arena *arena)
{
    Str8 result = STRUCT_ZERO;
    Os_File file = os_file_open(path, Os_AccessFlag_Read|Os_AccessFlag_ShareRead);
    result = os_file_read_str(file, range, arena);
    os_file_close(file);
    return result;
}

internal Str8 os_path_read_str_full(Str8 path, Arena *arena)
{
    Str8 result = STRUCT_ZERO;
    Os_File file = os_file_open(path, Os_AccessFlag_Read|Os_AccessFlag_ShareRead);
    Os_File_Properties prop = os_file_properties(file);
    result = os_file_read_str(file, (Rng1_U64){0, prop.size}, arena);
    os_file_close(file);
    return result;
}

//- ak: Directory Operations

internal bool os_dir_ensure(Str8 path)
{
    bool result = os_is_dir_exist(path);
    if (!result)
    {
        result = os_dir_make(path);
    }
    return result;
}

//~ ak: Command-Line Operations
//=============================================================================

internal Str8_Array *os_args_get(void)
{
    return &_os_core_state.args;
}

internal Str8 *os_program_path_get(void)
{
    return &_os_core_state.args.v[0];
}

//~ ak: OS Entry Points =======================================================

internal void os_main(void)
{
    _os_core_state.log_context = log_init();
#if BUILD_DEBUG
    _os_core_state.log_context.level = Log_Level_Info;
    fmt_println("# Program Output ============================================================ #");
#else
    _os_core_state.log_context.level = Log_Level_None;
#endif
    base_main();
}
