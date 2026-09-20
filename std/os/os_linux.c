// ak: Helpers functions
//=============================================================================

internal DateTime _os_linux_date_time_from_tm(struct tm in, uint32_t msec)
{
    DateTime dt = STRUCT_ZERO;
    dt.sec  = in.tm_sec;
    dt.min  = in.tm_min;
    dt.hour = in.tm_hour;
    dt.day  = in.tm_mday-1;
    dt.mon  = in.tm_mon;
    dt.year = in.tm_year+1900;
    dt.msec = msec;
    return dt;
}

internal DenseTime _os_linux_dense_time_from_timespec(struct timespec in)
{
    DenseTime result = 0;
    {
        struct tm tm_time = STRUCT_ZERO;
        gmtime_r(&in.tv_sec, &tm_time);
        DateTime date_time = _os_linux_date_time_from_tm(
            tm_time, in.tv_nsec/Million(1)
        );
        result = dense_time_from_date_time(date_time);
    }
    return result;
}

internal Fs_Properties _os_linux_properties_from_stat(struct stat *s)
{
    Fs_Properties props = STRUCT_ZERO;
    props.size     = s->st_size;
    props.created  = _os_linux_dense_time_from_timespec(s->st_ctim);
    props.modified = _os_linux_dense_time_from_timespec(s->st_mtim);
    if (s->st_mode & S_IFDIR)
    {
        props.flags |= Fs_Property_Flag_IsFolder;
    }
    return props;
}

// ak: Os Core Functions
//=============================================================================

// ak: Memory =================================================================

internal size_t os_pagesize_get(void)
{
    size_t result = sysconf(_SC_PAGESIZE);
    return result;
}

// ak: Exit ===================================================================

internal void os_exit(int32_t exit_code)
{
    exit(exit_code);
}

// ak: Sleep ==================================================================

internal void os_sleep_us(uint64_t micosec)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(micosec / Million(1));
    ts.tv_nsec = (long)((micosec % Million(1)) * Thousand(1));
    nanosleep(&ts, NULL);
}

internal void os_sleep_ms(uint32_t millisec)
{
    usleep(millisec*Thousand(1));
}

// ak: Environment Variable
//=============================================================================

internal bool os_is_env_exists(Str8 name)
{
    bool result = false;
    for (char **e = environ; *e != NULL; e++)
    {
        Str8 env = str8_from_cstr(*e);
        uint64_t equal_pos = str8_find_substr(env, 0, str8("="), Str_Match_Flag_None);
        Str8 env_name = str8_prefix(env, equal_pos);
        if (str8_match(env_name, name, Str_Match_Flag_None))
        {
            result = true;
            break;
        }
    }
    return result;
}

internal Str8 os_env_get(Str8 name)
{
    Str8 result = STRUCT_ZERO;
    for (char **e = environ; *e != NULL; e++)
    {
        Str8 env = str8_from_cstr(*e);
        uint64_t equal_pos = str8_find_substr(env, 0, str8("="), Str_Match_Flag_None);
        Str8 env_name = str8_prefix(env, equal_pos);
        if (str8_match(env_name, name, Str_Match_Flag_None))
        {
            result = str8_skip(env, equal_pos+1);
            break;
        }
    }
    return result;
}

// ak: OS Entry Points
//=============================================================================

int main(int argc, char *argv[])
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    // ak: make array of args
    {
        _os_core_state.args = array_alloc(scratch.arena, Str8_Array, (size_t)argc);
        for (int i = 0; i < argc; i++)
        {
            Str8 str = str8_from_cstr(argv[i]);
            array_append(&_os_core_state.args, str);
        }
    }
    
    // ak: get os data home path
    {
        if (os_is_env_exists(str8("XDG_DATA_HOME")))
        {
            _os_core_state.data_home = os_env_get(str8("XDG_DATA_HOME"));
        }
        if (_os_core_state.data_home.length == 0)
        {
            if (os_is_env_exists(str8("HOME")))
            {
                Str8 home = os_env_get(str8("HOME"));
                if (home.length != 0)
                {
                    _os_core_state.data_home = str8f(scratch.arena, "%s8/.local/share", home);
                }
            }
        }
    }
    
    // ak: os entry point
    os_main();
    
    arena_scratch_end(scratch);
}

// ak: Os Functions for Base Layer
//=============================================================================

// ak: Memory =================================================================

internal void *mem_reserve(size_t size)
{
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
    { result = 0; }
    return result;
}
internal bool mem_commit(void *ptr, size_t size)
{
    int result = mprotect(ptr, size, PROT_READ|PROT_WRITE);
    return result == 0;
}

internal bool mem_decommit(void *ptr, size_t size)
{
    int result1 = madvise(ptr, size, MADV_DONTNEED);
    int result2 = mprotect(ptr, size, PROT_NONE);
    return result1 == 0 && result2 == 0;
}
internal bool mem_release(void *ptr, size_t size)
{
    int result = munmap(ptr, size);
    return result == 0;
}

// ak: File System ============================================================

// ak: Open and Close file

internal Fs_File fs_file_open(Str8 path, Fs_File_Access_Flags flags)
{
    int32_t access_flags = 0;
    if (flags & Fs_File_Access_Flag_Read && flags & Fs_File_Access_Flag_Write)
    {
        access_flags = O_RDWR;
    }
    else if (flags & Fs_File_Access_Flag_Write)
    {
        access_flags = O_WRONLY|O_TRUNC;
    }
    else if (flags & Fs_File_Access_Flag_Read)
    {
        access_flags = O_RDONLY;
    }
    if (flags & Fs_File_Access_Flag_Append)
    {
        access_flags |= O_APPEND;
    }
    if (flags & (Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Append))
    {
        access_flags |= O_CREAT;
    }
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str8 path_copy = str8_copy(scratch.arena, path);
    Fs_File file = open((char *)path_copy.cstr, access_flags, 0666);
    arena_scratch_end(scratch);
    if (!(flags & Fs_File_Access_Flag_Inherited))
    {
        fcntl(file, F_SETFD, FD_CLOEXEC);
    }
    // ak: Lock file based on given flags
    short share_mode = 0;
    if (!(flags & Fs_File_Access_Flag_ShareRead))
    {
        share_mode |= F_RDLCK;
    }
    if (!(flags & Fs_File_Access_Flag_ShareWrite))
    {
        share_mode |= F_WRLCK;
    }
    if (share_mode)
    {
        struct flock lock = STRUCT_ZERO;
        lock.l_type = share_mode;
        lock.l_start = 0;
        lock.l_whence = SEEK_SET;
        lock.l_len = 0;  // ak: Lock entire file
        fcntl(file, F_SETLK, &lock);
    }
    return file;
}

internal void fs_file_close(Fs_File file)
{
    close(file);
}

// ak: File Read

internal size_t fs_file_read_raw(Fs_File file, Rng1_U64 rng, void *out_data)
{
    size_t total_num_bytes_to_read = dim_rng1(rng);
    size_t total_num_bytes_read = 0;
    size_t total_num_bytes_left_to_read = total_num_bytes_to_read;
    while (total_num_bytes_left_to_read > 0)
    {
        int read_result = pread(
            file, (uint8_t *)out_data + total_num_bytes_read,
            total_num_bytes_left_to_read, rng.min + total_num_bytes_read
        );
        if (read_result >= 0)
        {
            total_num_bytes_read += read_result;
            total_num_bytes_left_to_read -= read_result;
        }
        else if (errno != EINTR)
        {
            break;
        }
    }
    return total_num_bytes_read;
}

// ak: File Write

internal size_t fs_file_write(Fs_File file, void *data, Rng1_U64 rng)
{
    size_t total_num_bytes_to_write = dim_rng1(rng);
    size_t total_num_bytes_written = 0;
    size_t total_num_bytes_left_to_write = total_num_bytes_to_write;
    while (total_num_bytes_left_to_write > 0)
    {
        int write_result = pwrite(
                file, (uint8_t *)data + total_num_bytes_written,
                total_num_bytes_left_to_write, rng.min + total_num_bytes_written
                );
        if (write_result >= 0)
        {
            total_num_bytes_written += write_result;
            total_num_bytes_left_to_write -= write_result;
        }
        else if (errno != EINTR)
        {
            break;
        }
    }
    return total_num_bytes_written;
}

internal size_t fs_file_write_append(Fs_File file, void *data, size_t size)
{
    size_t total_num_bytes_written = write(file, data, size);
    return total_num_bytes_written;
}

// ak: Properties

internal Fs_Properties fs_properties_from_file(Fs_File file)
{
    struct stat fd_stat = STRUCT_ZERO;
    int fstat_result = fstat(file, &fd_stat);
    Fs_Properties props = STRUCT_ZERO;
    if (fstat_result != -1)
    {
        props = _os_linux_properties_from_stat(&fd_stat);
    }
    return props;
}

internal Fs_Properties fs_properties_from_file_path(Str8 path)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str8 path_copy = str8_copy(scratch.arena, path);
    struct stat f_stat = STRUCT_ZERO;
    int stat_result = stat((char *)path_copy.cstr, &f_stat);
    Fs_Properties props = STRUCT_ZERO;
    if(stat_result != -1)
    {
        props = _os_linux_properties_from_stat(&f_stat);
    }
    arena_scratch_end(scratch);
    return props;
}

// ak: Walk

internal Fs_Walk *fs_walk_begin(Arena *arena, Str8 path, Fs_Walk_Flags flags)
{
    Fs_Walk *base_walk = arena_push(arena, Fs_Walk, 1);
    base_walk->flags = flags;
    _Os_Linux_Walk *walk = (_Os_Linux_Walk *)base_walk->memory;
    {
        Str8 path_copy = str8_copy(arena, path);
        walk->dir = opendir((char *)path_copy.cstr);
        walk->path = path_copy;
    }
    return base_walk;
}

internal bool fs_walk_next(Arena *arena, Fs_Walk *walk, Fs_Info *info_out)
{
    bool good = 0;
    _Os_Linux_Walk *linux_walk = (_Os_Linux_Walk *)walk->memory;
    while (true)
    {
        // ak: get next entry
        linux_walk->dp = readdir(linux_walk->dir);
        good = (linux_walk->dp != 0);
        // ak: unpack entry info
        struct stat st = STRUCT_ZERO;
        int stat_result = 0;
        if (good)
        {
            Arena_Temp scratch = arena_scratch_begin(&arena, 1);
            Str8 full_path = str8f(scratch.arena, "%.*s/%s", str8_varg(linux_walk->path), linux_walk->dp->d_name);
            stat_result = stat((char *)full_path.cstr, &st);
            arena_scratch_end(scratch);
        }
        // ak: determine if filtered
        bool filtered = 0;
        if (good)
        {
            filtered = ((st.st_mode == S_IFDIR && walk->flags & Fs_Walk_Flag_SkipFolders) ||
                    (st.st_mode == S_IFREG && walk->flags & Fs_Walk_Flag_SkipFiles) ||
                    (linux_walk->dp->d_name[0] == '.' && linux_walk->dp->d_name[1] == 0) ||
                    (linux_walk->dp->d_name[0] == '.' && linux_walk->dp->d_name[1] == '.' && linux_walk->dp->d_name[2] == 0));
        }
        // ak: output & exit, if good & unfiltered
        if (good && !filtered)
        {
            info_out->name = str8_copy(arena, str8_from_cstr(linux_walk->dp->d_name));
            if (stat_result != -1)
            {
                info_out->props = _os_linux_properties_from_stat(&st);
            }
            break;
        }
        // ak: exit if not good
        if (!good)
        {
            break;
        }
    }
    return good;
}

internal void fs_walk_end(Fs_Walk *walk)
{
    _Os_Linux_Walk *linux_walk = (_Os_Linux_Walk *)walk->memory;
    closedir(linux_walk->dir);
}

// ak: Directory Operations

internal bool fs_is_dir_exist(Str8 path)
{
    struct stat st;
    int result = stat((const char*)path.cstr, &st);
    return (result == 0) && S_ISDIR(st.st_mode);
}

internal bool fs_dir_make(Str8 path)
{
    int result = mkdir((const char *)path.cstr, 0700);
    return result == 0;
}

// ak: Exists

internal bool fs_file_path_exists(Str8 path)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str8 path_copy = str8_copy(scratch.arena, path);
    int access_result = access((char *)path_copy.cstr, F_OK);
    bool result = 0;
    if(access_result == 0)
    {
        result = 1;
    }
    arena_scratch_end(scratch);
    return result;
}

internal bool fs_dir_path_exists(Str8 path)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    bool exists = 0;
    Str8  path_copy = str8_copy(scratch.arena, path);
    DIR *handle = opendir((char *)path_copy.cstr);
    if(handle)
    {
        closedir(handle);
        exists = 1;
    }
    arena_scratch_end(scratch);
    return exists;
}

// ak: Time ===================================================================

internal uint32_t time_now_unix(void)
{
    time_t t = time(0);
    return (uint32_t)t;
}

internal uint64_t time_now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t result = t.tv_sec*Million(1) + (t.tv_nsec/Thousand(1));
    return result;
}

// ak: Terminal ===============================================================

internal bool term_is_terminal(Fs_File file)
{
    int result = isatty(file);
    return result;
}
