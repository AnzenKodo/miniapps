// ak: Helper =================================================================

internal Str8 log_level_string_get(Log_Level level)
{
    const char *level_string = "";
    switch (level)
    {
        case Log_Level_Info:
        {
            level_string = "[INFO]";
        } break;
        case Log_Level_Debug:
        {
            level_string = "[DEBUG]";
        } break;
        case Log_Level_Warn:
        {
            level_string = "[WARN]";
        } break;
        case Log_Level_Error:
        {
            level_string = "[ERROR]";
        } break;
        case Log_Level_COUNT: {} break;
    }
    return str8(level_string);
}

// ak: Log Creation ===========================================================

internal Log *log_alloc(void)
{
    Arena *arena = arena_alloc();
    Log *log = arena_push(arena, Log, 1);
    log->arena = arena;
    return log;
}

internal void log_release(Log *log)
{
    arena_free(log->arena);
}

// ak: Log Building ===========================================================

internal void log_msg(Log *log, Log_Level level, const char *file_name, size_t line_number, Str8 string)
{
    if(log != NULL)
    {
        Str8 string_copy = str8_copy(log->arena, string);
        str8_list_push(log->arena, &log->strings[level], string_copy);
        log->file_name = str8(file_name);
        log->line_number = line_number;
    }
}

internal void log_msgf(Log *log, Log_Level level, const char *file_name, size_t line_number, const char *format, ...)
{
    if(log != NULL)
    {
        Arena_Temp scratch = arena_scratch_begin(NULL, 0);
        va_list args;
        va_start(args, format);
        Str8 string = str8fv(scratch.arena, format, args);
        log_msg(log, level, file_name, line_number, string);
        va_end(args);
        arena_scratch_end(scratch);
    }
}

// ak: Log Result =============================================================

internal Log_Result log_get_result(Log *log, size_t indent, Arena *arena)
{
    Log_Result result = STRUCT_ZERO;
    if(log != NULL)
    {
        if(arena != NULL)
        {
            for EachEnumVal(Log_Level, level)
            {
                Arena_Temp scratch = arena_scratch_begin(&arena, 1);
                Str8 result_unindented = str8_list_join(scratch.arena, &log->strings[level], 0);
                result.strings[level] = indented_from_string(result_unindented, indent, arena);
                arena_scratch_end(scratch);
            }
        }
    }
    return result;
}
