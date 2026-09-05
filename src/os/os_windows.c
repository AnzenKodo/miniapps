//~ ak: Helpers Functions
//=============================================================================

internal uint32_t _os_win32_unix_time_from_file_time(FILETIME file_time)
{
    uint64_t win32_time = ((uint64_t)file_time.dwHighDateTime << 32) | file_time.dwLowDateTime;
    uint64_t unix_time64 = ((win32_time - 0x19DB1DED53E8000ULL) / 10000000);

    Assert(unix_time64 <= max_u32);
    uint32_t unix_time32 = (uint32_t)unix_time64;

    return unix_time32;
}

internal Os_File_Property_Flags _os_win32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes)
{
    Os_File_Property_Flags flags = 0;
    if (dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        flags |= Os_File_Property_Flag_IsFolder;
    }
    return flags;
}

internal void _os_win32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in)
{
    out->year    = in->wYear;
    out->mon     = in->wMonth - 1;
    out->wday    = in->wDayOfWeek;
        out->day     = in->wDay;
        out->hour    = in->wHour;
        out->min     = in->wMinute;
        out->sec     = in->wSecond;
    out->msec    = in->wMilliseconds;
}

internal void _os_win32_dense_time_from_file_time(DenseTime *out, FILETIME *in)
{
    SYSTEMTIME systime = STRUCT_ZERO;
    FileTimeToSystemTime(in, &systime);
    DateTime date_time = STRUCT_ZERO;
    _os_win32_date_time_from_system_time(&date_time, &systime);
    *out = dense_time_from_date_time(date_time);
}

//~ ak: Memory Allocation
//=============================================================================

internal void *os_mem_reserve(size_t size)
{
    void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
    return result;
}

internal bool os_mem_commit(void *ptr, size_t size)
{
    LPVOID result = VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
    return result != NULL;
}

internal bool os_mem_decommit(void *ptr, size_t size)
{
    return VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal bool os_mem_release(void *ptr, size_t size)
{
    return VirtualFree(ptr, size, MEM_RELEASE);
}

internal size_t os_pagesize_get(void)
{
    SYSTEM_INFO sysinfo = { 0 };
    GetSystemInfo(&sysinfo);
    return sysinfo.dwPageSize;
}

//~ ak: File System
//=============================================================================

internal Os_File os_file_open(Str8 path, Os_AccessFlags flags)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str16 path16 = str16_from_8(scratch.arena, path);
    DWORD access_flags = 0;
    DWORD share_mode = 0;
    DWORD creation_disposition = OPEN_EXISTING;
    SECURITY_ATTRIBUTES security_attributes = {sizeof(security_attributes), 0, 0};
    if (flags & Os_AccessFlag_Read)
    {
        access_flags |= GENERIC_READ;
    }
    if (flags & Os_AccessFlag_Write)
    {
        access_flags |= GENERIC_WRITE;
    }
    if (flags & Os_AccessFlag_Execute)
    {
        access_flags |= GENERIC_EXECUTE;
    }
    if (flags & Os_AccessFlag_ShareRead)
    {
        share_mode |= FILE_SHARE_READ;
    }
    if (flags & Os_AccessFlag_ShareWrite)
    {
        share_mode |= FILE_SHARE_WRITE|FILE_SHARE_DELETE;
    }
    if (flags & Os_AccessFlag_Write)
    {
        creation_disposition = CREATE_ALWAYS;
    }
    if (flags & Os_AccessFlag_Append)
    {
        creation_disposition = OPEN_ALWAYS; access_flags |= FILE_APPEND_DATA;
    }
    if (flags & Os_AccessFlag_Inherited)
    {
        security_attributes.bInheritHandle = 1;
    }
    HANDLE handle = CreateFileW(
        (WCHAR *)path16.cstr, access_flags, share_mode, &security_attributes,
        creation_disposition, FILE_ATTRIBUTE_NORMAL, 0
    );
    arena_scratch_end(scratch);
    return (Os_File)handle;
}

internal void os_file_close(Os_File file)
{
    CloseHandle((HANDLE)file);
}

internal uint64_t os_file_read(Os_File file, Rng1_U64 rng, void *out_data)
{
    size_t size = 0;
    GetFileSizeEx((HANDLE)file, (LARGE_INTEGER *)&size);
    Rng1_U64 rng_clamped  = (Rng1_U64){Min(rng.min, size), Min(rng.max, size)};
    uint64_t total_read_size = 0;
    //- ak: read loop
    {
        uint64_t to_read = rng1_dim_u64(rng_clamped);
        for (uint64_t off = rng.min; total_read_size < to_read;)
        {
            uint64_t amt64 = to_read - total_read_size;
            uint32_t amt32 = u32_from_u64_saturate(amt64);
            DWORD read_size = 0;
            OVERLAPPED overlapped = STRUCT_ZERO;
            overlapped.Offset     = (off&0x00000000ffffffffull);
            overlapped.OffsetHigh = (off&0xffffffff00000000ull) >> 32;
            ReadFile((HANDLE)file, (uint8_t *)out_data + total_read_size, amt32, &read_size, &overlapped);
            off += read_size;
            total_read_size += read_size;
                if (read_size != amt32)
                {
                    break;
                }
        }
    }
    return total_read_size;
}


internal size_t os_file_write(Os_File file, void *data, Rng1_U64 rng)
{
    uint64_t src_off = 0;
    uint64_t dst_off = rng.min;
    uint64_t total_write_size = rng1_dim_u64(rng);
    for (;;)
    {
        void *bytes_src = (uint8_t *)data + src_off;
        uint64_t bytes_left = total_write_size - src_off;
        DWORD write_size = (DWORD)Min(MB(1), bytes_left);
        DWORD bytes_written = 0;
        OVERLAPPED overlapped = STRUCT_ZERO;
        overlapped.Offset = (dst_off&0x00000000ffffffffull);
        overlapped.OffsetHigh = (dst_off&0xffffffff00000000ull) >> 32;
        BOOL success = WriteFile((HANDLE)file, bytes_src, write_size, &bytes_written, &overlapped);
        if (success == 0)
        {
            break;
        }
        src_off += bytes_written;
        dst_off += bytes_written;
        if (bytes_left == 0)
        {
            break;
        }
    }
    return src_off;
}

internal size_t os_file_write_append(Os_File file, void *data, size_t size)
{
    DWORD total_num_bytes_written;
    WriteFile((HANDLE)file, data, (DWORD)size, &total_num_bytes_written, NULL);
    return total_num_bytes_written;
}

internal Os_File_Properties os_file_properties(Os_File file)
{
    Os_File_Properties props = STRUCT_ZERO;
    BY_HANDLE_FILE_INFORMATION info;
    BOOL info_good = GetFileInformationByHandle((HANDLE)file, &info);
    if (info_good)
    {
        uint32_t size_lo = info.nFileSizeLow;
        uint32_t size_hi = info.nFileSizeHigh;
        props.size  = (uint64_t)size_lo | (((uint64_t)size_hi)<<32);
        _os_win32_dense_time_from_file_time(&props.modified, &info.ftLastWriteTime);
        _os_win32_dense_time_from_file_time(&props.created, &info.ftCreationTime);
        props.flags = _os_win32_file_property_flags_from_dwFileAttributes(info.dwFileAttributes);
    }
    return props;
}

//- ak: Directory Operations

internal bool os_is_dir_exist(Str8 path)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str16 path16 = str16_from_8(scratch.arena, path);
    DWORD attr = GetFileAttributesW((WCHAR*)path16.cstr);
    arena_scratch_end(scratch);
    return (attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY);
}

internal bool os_dir_make(Str8 path)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str16 path16 = str16_from_8(scratch.arena, path);
    BOOL result = CreateDirectoryW((WCHAR*)path16.cstr, NULL);
    arena_scratch_end(scratch);
    return result != 0;
}

internal Os_File_Walk *os_file_walk_begin(Arena *arena, Str8 path, Os_File_Walk_Flags flags)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    Str8 path_with_wildcard = str8_cat(scratch.arena, path, str8("\\*"));
    Str16 path16 = str16_from_8(scratch.arena, path_with_wildcard);
    Os_File_Walk *walk = arena_push(arena, Os_File_Walk, 1);
    walk->flags = flags;
    _Os_Win32_Walk_Iter *win32_walk = (_Os_Win32_Walk_Iter*)walk->memory;
    if (path.size == 0)
    {
        win32_walk->is_volume_iter = 1;
        WCHAR buffer[512] = STRUCT_ZERO;
        DWORD length = GetLogicalDriveStringsW(sizeof(buffer), buffer);
        Str8_List drive_strings = STRUCT_ZERO;
        for (uint64_t off = 0; off < (uint64_t)length;)
        {
            Str16 next_drive_string_16 = str16_from_cstr((uint16_t *)buffer+off);
            off += next_drive_string_16.size+1;
            Str8 next_drive_string = str8_from_16(arena, next_drive_string_16);
            next_drive_string = str8_chop_last_slash(next_drive_string);
            str8_list_push(scratch.arena, &drive_strings, next_drive_string);
        }
        win32_walk->drive_strings = str8_array_from_list(arena, &drive_strings);
        win32_walk->drive_strings_iter_idx = 0;
    }
    else
    {
        win32_walk->handle = FindFirstFileExW((WCHAR*)path16.cstr, FindExInfoBasic, &win32_walk->find_data, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
    }
    arena_scratch_end(scratch);
    return walk;
}

internal bool os_file_walk_next(Arena *arena, Os_File_Walk *walk, Os_File_Info *info_out)
{
    bool result = 0;
    Os_File_Walk_Flags flags = walk->flags;
    _Os_Win32_Walk_Iter *win32_walk = (_Os_Win32_Walk_Iter*)walk->memory;
    if (win32_walk->is_volume_iter)
    {
        //- ak: volume iteration
        result = win32_walk->drive_strings_iter_idx < win32_walk->drive_strings.length;
        if (result != 0)
        {
            MemSetZeroStruct(info_out);
            info_out->name = win32_walk->drive_strings.v[win32_walk->drive_strings_iter_idx];
            info_out->props.flags |= Os_File_Property_Flag_IsFolder;
            win32_walk->drive_strings_iter_idx += 1;
        }
    }
    else
    {
        //- ak: file iteration
        if (!(flags & Os_File_Walk_Flag_Done) && win32_walk->handle != INVALID_HANDLE_VALUE)
        {
            do
            {
                //- ak: check is usable
                bool usable_file = 1;

                WCHAR *file_name = win32_walk->find_data.cFileName;
                DWORD attributes = win32_walk->find_data.dwFileAttributes;
                if (file_name[0] == '.')
                {
                    if (flags & Os_File_Walk_Flag_SkipHiddenFiles)
                    {
                        usable_file = 0;
                    }
                    else if (file_name[1] == 0)
                    {
                        usable_file = 0;
                    }
                    else if (file_name[1] == '.' && file_name[2] == 0)
                    {
                        usable_file = 0;
                    }
                }
                if (attributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    if (flags & Os_File_Walk_Flag_SkipFolders)
                    {
                        usable_file = 0;
                    }
                }
                else{
                    if (flags & Os_File_Walk_Flag_SkipFiles)
                    {
                        usable_file = 0;
                    }
                }
                //- ak: emit if usable
                if (usable_file)
                {
                    info_out->name = str8_from_16(arena, str16_from_cstr((uint16_t*)file_name));
                    info_out->props.size = (uint64_t)win32_walk->find_data.nFileSizeLow | (((uint64_t)win32_walk->find_data.nFileSizeHigh)<<32);
                    _os_win32_dense_time_from_file_time(&info_out->props.created,  &win32_walk->find_data.ftCreationTime);
                    _os_win32_dense_time_from_file_time(&info_out->props.modified, &win32_walk->find_data.ftLastWriteTime);
                    info_out->props.flags = _os_win32_file_property_flags_from_dwFileAttributes(attributes);
                    result = 1;
                    if (!FindNextFileW(win32_walk->handle, &win32_walk->find_data))
                    {
                        walk->flags |= Os_File_Walk_Flag_Done;
                    }
                    break;
                }
            }while(FindNextFileW(win32_walk->handle, &win32_walk->find_data));
        }
    }
    if (!result)
    {
        walk->flags |= Os_File_Walk_Flag_Done;
    }
    return result;
}

internal void os_file_walk_end(Os_File_Walk *walk)
{
    _Os_Win32_Walk_Iter *win32_walk = (_Os_Win32_Walk_Iter*)walk->memory;
    HANDLE zero_handle;
    MemSetZeroStruct(&zero_handle);
    if (!MemMatchStruct(&zero_handle, &win32_walk->handle))
    {
        FindClose(win32_walk->handle);
    }
}

//~ ak: Exit
//=============================================================================

internal void os_exit(int32_t exit_code)
{
    ExitProcess(exit_code);
}

//~ ak: Time
//=============================================================================

internal uint32_t os_now_unix(void)
{
    FILETIME file_time;
    GetSystemTimeAsFileTime(&file_time);
    uint32_t unix_time = _os_win32_unix_time_from_file_time(file_time);
    return unix_time;
}

internal uint64_t os_now_microsec(void)
{
    uint64_t result = 0;
    LARGE_INTEGER large_int_counter;
    if (QueryPerformanceCounter(&large_int_counter))
    {
        result = (large_int_counter.QuadPart*Million(1))/_os_win32_state.microsecond_resolution;
    }
    return result;
}

internal void os_sleep_microsec(uint64_t microsec)
{
    DWORD millisec = (DWORD)(microsec / 1000); //- ak: Integer division to get milliseconds
    if (microsec % 1000 >= 500)
    {
        millisec++; //- ak: Round up if the remaining microseconds are 500 or more
    }
    os_sleep_millisec(millisec);
}

internal void os_sleep_millisec(uint32_t millisec)
{
    Sleep(millisec);
}

//~ ak: Command-Line Operations
//=============================================================================

internal bool os_is_term_mode(Os_File file)
{
    bool result = false;
    DWORD dmode;
    result = GetConsoleMode((HANDLE)file, &dmode);
    return result;
}

//~ ak: Environment Variable
//=============================================================================

internal bool os_env_is_set(Str8 name)
{

    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str16 name16 = str16_from_8(scratch.arena, name);
    DWORD len = GetEnvironmentVariableW(name16.cstr, NULL, 0);
    arena_scratch_end(scratch);
    return len;
}

internal Str8 os_env_get(Str8 name)
{
    Str8 result = STRUCT_ZERO;
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str16 name16 = str16_from_8(scratch.arena, name);
    DWORD len = GetEnvironmentVariableW(name16.cstr, NULL, 0);
    if (len > 0)
    {
        uint16_t* value_buf = arena_push(scratch.arena, uint16_t, len);
        GetEnvironmentVariableW(name16.cstr, value_buf, len);
        Str16 value16 = str16_from_cstr(value_buf);
        arena_scratch_end(scratch);
        result = str8_from_16(scratch.arena, value16);
    }
    return result;
}

//~ ak: OS Entry Points
//=============================================================================

int main(void)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    //- ak: Setup argument array
    int args_count;
    LPWSTR *args = CommandLineToArgvW(GetCommandLineW(), &args_count);
    _os_core_state.args = array_alloc(scratch.arena, Str8_Array, args_count);
    for (int i = 0; i < args_count; i++)
    {
        Str16 str16 = str16_from_cstr(args[i]);
        Str8 str = str8_from_16(scratch.arena, str16);
        array_append(&_os_core_state.args, str);
    }
    // NOTE(ak): we need this to set now time.
    {
        _os_win32_state.microsecond_resolution  = 1;
        LARGE_INTEGER large_int_resolution;
        if (QueryPerformanceFrequency(&large_int_resolution))
        {
            _os_win32_state.microsecond_resolution = large_int_resolution.QuadPart;
        }
    }
    //- ak: Enable terminal color support
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode))
        {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    //- ak: Go to default OS entry point
    os_main();
    arena_scratch_end(scratch);
}
