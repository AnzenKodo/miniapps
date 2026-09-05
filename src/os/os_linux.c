//~ ak: Helpers functions
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

internal Os_File_Properties _os_linux_file_properties_from_stat(struct stat *s)
{
    Os_File_Properties props = STRUCT_ZERO;
    props.size     = s->st_size;
    props.created  = _os_linux_dense_time_from_timespec(s->st_ctim);
    props.modified = _os_linux_dense_time_from_timespec(s->st_mtim);
    if (s->st_mode & S_IFDIR)
    {
        props.flags |= Os_File_Property_Flag_IsFolder;
    }
    return props;
}

//~ ak: Memory Allocation
//=============================================================================

internal void *os_mem_reserve(size_t size)
{
    void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    if (result == MAP_FAILED)
    { result = 0; }
    return result;
}
internal bool os_mem_commit(void *ptr, size_t size)
{
    int result = mprotect(ptr, size, PROT_READ|PROT_WRITE);
    return result == 0;
}

internal bool os_mem_decommit(void *ptr, size_t size)
{
    int result1 = madvise(ptr, size, MADV_DONTNEED);
    int result2 = mprotect(ptr, size, PROT_NONE);
    return result1 == 0 && result2 == 0;
}
internal bool os_mem_release(void *ptr, size_t size)
{
    int result = munmap(ptr, size);
    return result == 0;
}

internal size_t os_pagesize_get(void)
{
    size_t result = sysconf(_SC_PAGESIZE);
    return result;
}

//~ ak: File System
//=============================================================================

internal Os_File os_file_open(Str8 path, Os_AccessFlags flags)
{
    int32_t access_flags = 0;
    if (flags & Os_AccessFlag_Read && flags & Os_AccessFlag_Write)
    {
        access_flags = O_RDWR;
    }
    else if (flags & Os_AccessFlag_Write)
    {
        access_flags = O_WRONLY|O_TRUNC;
    }
    else if (flags & Os_AccessFlag_Read)
    {
        access_flags = O_RDONLY;
    }
    if (flags & Os_AccessFlag_Append)
    {
        access_flags |= O_APPEND;
    }
    if (flags & (Os_AccessFlag_Write|Os_AccessFlag_Append))
    {
        access_flags |= O_CREAT;
    }
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str8 path_copy = str8_copy(scratch.arena, path);
    Os_File file = open((char *)path_copy.cstr, access_flags, 0666);
    arena_scratch_end(scratch);
    if (!(flags & Os_AccessFlag_Inherited))
    {
        fcntl(file, F_SETFD, FD_CLOEXEC);
    }
    //- ak: Lock file based on given flags
    short share_mode = 0;
    if (!(flags & Os_AccessFlag_ShareRead))
    {
        share_mode |= F_RDLCK;
    }
    if (!(flags & Os_AccessFlag_ShareWrite))
    {
        share_mode |= F_WRLCK;
    }
    if (share_mode)
    {
        struct flock lock = STRUCT_ZERO;
        lock.l_type = share_mode;
        lock.l_start = 0;
        lock.l_whence = SEEK_SET;
        lock.l_len = 0;  //- ak: Lock entire file
        fcntl(file, F_SETLK, &lock);
    }
    return file;
}

internal void os_file_close(Os_File file)
{
    close(file);
}

internal size_t os_file_read(Os_File file, Rng1_U64 rng, void *out_data)
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

internal size_t os_file_write(Os_File file, void *data, Rng1_U64 rng)
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

internal size_t os_file_write_append(Os_File file, void *data, size_t size)
{
    size_t total_num_bytes_written = write(file, data, size);
    return total_num_bytes_written;
}

internal Os_File_Properties os_file_properties(Os_File file)
{
    struct stat fd_stat = STRUCT_ZERO;
    int fstat_result = fstat(file, &fd_stat);
    Os_File_Properties props = STRUCT_ZERO;
    if (fstat_result != -1)
    {
        props = _os_linux_file_properties_from_stat(&fd_stat);
    }
    return props;
}

//- ak: Directory Operations

internal bool os_is_dir_exist(Str8 path)
{
    struct stat st;
    int result = stat((const char*)path.cstr, &st);
    return (result == 0) && S_ISDIR(st.st_mode);
}

internal bool os_dir_make(Str8 path)
{
    int result = mkdir((const char *)path.cstr, 0700);
    return result == 0;
}

internal Os_File_Walk *os_file_walk_begin(Arena *arena, Str8 path, Os_File_Walk_Flags flags)
{
    Os_File_Walk *base_walk = arena_push(arena, Os_File_Walk, 1);
    base_walk->flags = flags;
    _Os_Linux_File_Walk *walk = (_Os_Linux_File_Walk *)base_walk->memory;
    {
        Str8 path_copy = str8_copy(arena, path);
        walk->dir = opendir((char *)path_copy.cstr);
        walk->path = path_copy;
    }
    return base_walk;
}

internal bool os_file_walk_next(Arena *arena, Os_File_Walk *walk, Os_File_Info *info_out)
{
    bool good = 0;
    _Os_Linux_File_Walk *linux_walk = (_Os_Linux_File_Walk *)walk->memory;
    while (true)
    {
        //- ak: get next entry
        linux_walk->dp = readdir(linux_walk->dir);
        good = (linux_walk->dp != 0);
        //- ak: unpack entry info
        struct stat st = STRUCT_ZERO;
        int stat_result = 0;
        if (good)
        {
            Arena_Temp scratch = arena_scratch_begin(&arena, 1);
            Str8 full_path = str8f(scratch.arena, "%.*s/%s", str8_varg(linux_walk->path), linux_walk->dp->d_name);
            stat_result = stat((char *)full_path.cstr, &st);
            arena_scratch_end(scratch);
        }
        //- ak: determine if filtered
        bool filtered = 0;
        if (good)
        {
            filtered = ((st.st_mode == S_IFDIR && walk->flags & Os_File_Walk_Flag_SkipFolders) ||
                    (st.st_mode == S_IFREG && walk->flags & Os_File_Walk_Flag_SkipFiles) ||
                    (linux_walk->dp->d_name[0] == '.' && linux_walk->dp->d_name[1] == 0) ||
                    (linux_walk->dp->d_name[0] == '.' && linux_walk->dp->d_name[1] == '.' && linux_walk->dp->d_name[2] == 0));
        }
        //- ak: output & exit, if good & unfiltered
        if (good && !filtered)
        {
            info_out->name = str8_copy(arena, str8_from_cstr(linux_walk->dp->d_name));
            if (stat_result != -1)
            {
                info_out->props = _os_linux_file_properties_from_stat(&st);
            }
            break;
        }
        //- ak: exit if not good
        if (!good)
        {
            break;
        }
    }
    return good;
}

internal void os_file_walk_end(Os_File_Walk *walk)
{
    _Os_Linux_File_Walk *linux_walk = (_Os_Linux_File_Walk *)walk->memory;
    closedir(linux_walk->dir);
}

//~ ak: Exit
//=============================================================================

internal void os_exit(int32_t exit_code)
{
    exit(exit_code);
}

//~ ak: Time
//=============================================================================

internal uint32_t os_now_unix(void)
{
    time_t t = time(0);
    return (uint32_t)t;
}

internal uint64_t os_now_us(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t result = t.tv_sec*Million(1) + (t.tv_nsec/Thousand(1));
    return result;
}

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

//~ ak: Command-Line Operations
//=============================================================================

internal bool os_is_term_mode(Os_File file)
{
    int result = isatty(file);
    return result;
}

//~ ak: Environment Variable
//=============================================================================

internal bool os_env_is_set(Str8 name)
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
        if (os_env_is_set(name))
        {
            result = str8_skip(env, equal_pos+1);
        }
    }
    return result;
}

//~ ak: OS Entry Points
//=============================================================================

int main(int argc, char *argv[])
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    _os_core_state.args = array_alloc(scratch.arena, Str8_Array, (size_t)argc);
    for (int i = 0; i < argc; i++)
    {
        Str8 str = str8_from_cstr(argv[i]);
        array_append(&_os_core_state.args, str);
    }
    os_main();
    arena_scratch_end(scratch);
}
