// ak: File Read
//=============================================================================

internal U8Array fs_file_read(Fs_File file, Rng1_U64 range, Arena *arena)
{
    size_t pre_pos = arena_pos(arena);
    U8Array result = STRUCT_ZERO;
    result.size = dim_rng1(range);
    result.length = result.size;
    result.v = arena_push(arena, uint8_t, result.size);
    size_t actual_read_size = fs_file_read_raw(file, range, result.v);
    if (actual_read_size < result.length)
    {
        arena_pop_to(arena, pre_pos + actual_read_size);
        result.length = actual_read_size;
    }
    return result;
}

internal U8Array fs_file_read_full(Fs_File file, Arena *arena)
{
    U8Array result = STRUCT_ZERO;
    Fs_Properties prop = fs_properties_from_file(file);
    result = fs_file_read(file, (Rng1_U64){0, prop.size}, arena);
    return result;
}

internal U8Array fs_file_path_read(Str8 path, Rng1_U64 range, Arena *arena)
{
    U8Array result = STRUCT_ZERO;
    Fs_File file = fs_file_open(path, Fs_File_Access_Flag_Read|Fs_File_Access_Flag_ShareRead);
    result = fs_file_read(file, range, arena);
    fs_file_close(file);
    return result;
}

internal U8Array fs_file_path_read_full(Str8 path, Arena *arena)
{
    U8Array result = STRUCT_ZERO;
    Fs_File file = fs_file_open(path, Fs_File_Access_Flag_Read|Fs_File_Access_Flag_ShareRead);
    result = fs_file_read_full(file, arena);
    fs_file_close(file);
    return result;
}

// ak: File Write
//=============================================================================

internal size_t fs_file_write(Fs_File file, U8Array data)
{
    return fs_file_write_rng(file, data, rng1_u64(0, data.length));
}
internal size_t fs_file_write_rng(Fs_File file, U8Array data, Rng1_U64 rng)
{
    return fs_file_write_rng_raw(file, (void *)data.v, rng);
}
internal size_t fs_file_write_append(Fs_File file, U8Array data)
{
    return fs_file_write_append_raw(file, (void *)data.v, data.length);
}

internal size_t fs_file_path_write(Str8 file_path, U8Array data)
{
    return fs_file_path_write_rng(file_path, data, rng1_u64(0, data.length));
}
internal size_t fs_file_path_write_rng(Str8 file_path, U8Array data, Rng1_U64 rng)
{
    size_t result = 0;
    Fs_File file = fs_file_open(file_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_ShareWrite);
    result = fs_file_write_rng(file, data, rng);
    fs_file_close(file);
    return result;
}
internal size_t fs_file_path_write_append(Str8 file_path, U8Array data)
{
    size_t result = 0;
    Fs_File file = fs_file_open(file_path, Fs_File_Access_Flag_Append|Fs_File_Access_Flag_ShareWrite);
    result = fs_file_write_append(file, data);
    fs_file_close(file);
    return result;
}

// ak: Directory Operations
//=============================================================================

internal bool fs_dir_ensure(Str8 path)
{
    bool result = fs_is_dir_exist(path);
    if (!result)
    {
        result = fs_dir_make(path);
    }
    return result;
}
