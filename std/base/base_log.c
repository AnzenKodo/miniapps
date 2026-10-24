// ak: Helper =================================================================

internal Str8 log_level_string_get(Log_Level level)
{
    Str8 level_string = STRUCT_ZERO;
    switch (level)
    {
        case Log_Level_Trace:
        {
            level_string = str8("[TRACE]");
        } break;
        case Log_Level_Info:
        {
            level_string = str8("[INFO]");
        } break;
        case Log_Level_Debug:
        {
            level_string = str8("[DEBUG]");
        } break;
        case Log_Level_Warn:
        {
            level_string = str8("[WARN]");
        } break;
        case Log_Level_Error:
        {
            level_string = str8("[ERROR]");
        } break;
        case Log_Level_COUNT: {} break;
    }
    level_string.length = 7;
    return level_string;
}

internal Fs_File std_type_from_log_level(Log_Level level)
{
    Fs_File file = OS_STDOUT;
    if (level >= Log_Level_Warn) { file = OS_STDERR; }
    return file;
}

// ak: Log Creation ===========================================================

internal Log *log_alloc(void)
{
    Arena *arena = arena_alloc();
    Log *log = arena_push(arena, Log, 1);
    log->arena = arena;
    return log;
}

internal void log_free(Log *log)
{
    arena_free(log->arena);
}

// ak: Log Builder ============================================================

internal void log_push(Log *log, Log_Level level, const char *file_name, size_t line_number, Str8 string)
{
    if(log != NULL)
    {
        Log_Node *node = arena_push(log->arena, Log_Node, 1);
        node->str = str8_copy(log->arena, string);
        node->file_name = str8_from_cstr((char *)file_name);
        node->line_number = line_number;
        Log_List *list = &log->list[level];
        SLLQueuePush(list->first, list->last, node);
    }
}

internal void log_pushf(Log *log, Log_Level level, const char *file_name, size_t line_number, const char *format, ...)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    va_list args;
    va_start(args, format);
    Str8 string = str8fv(scratch.arena, format, args);
    log_push(log, level, file_name, line_number, string);
    va_end(args);
    arena_scratch_end(scratch);
}

// ak: Log Trace ==============================================================

internal void log_trace_level_set(Log *log, Log_Level level)
{
    log->trace_level = level;
}

internal void log_trace(Log *log, Log_Level level, const char *file_name, size_t line_number, Str8 string)
{
    log_push(log, level, file_name, line_number, string);
    if (level >= log->trace_level)
    {
        Fs_File file = std_type_from_log_level(level);
        Str8 level_string = log_level_string_get(level);
        fmt_fprintf(file, "%-*s ", str8_varg(level_string));
        if (level >= Log_Level_Warn)
        {
            fmt_fprintf(file, "%s:%zu ", file_name, line_number);
        }
        fmt_fprintln(file, string);
    }
}

internal void log_tracef(Log *log, Log_Level level, const char *file_name, size_t line_number, const char *format, ...)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    va_list args;
    va_start(args, format);
    Str8 string = str8fv(scratch.arena, format, args);
    log_trace(log, level, file_name, line_number, string);
    va_end(args);
    arena_scratch_end(scratch);
}

