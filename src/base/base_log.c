//~ ak: Helper Functions
//=============================================================================

internal const char *log_get_reset_color(Log_Context *log)
{
    const char *reset_color = "";
    if (log->enable_color_log && term_is_color_allowed())
    {
        reset_color = TERM_RESET;
    }
    return reset_color;
}

internal const char *log_get_level_color(Log_Level level)
{
    const char *level_color = "";
    switch (level)
    {
        case Log_Level_None:
        {
        } break;
        case Log_Level_Info:
        {
            level_color = TERM_FG_BLUE;
        } break;
        case Log_Level_Debug:
        {
            level_color = TERM_FG_MAGENTA;
        } break;
        case Log_Level_Warn:
        {
            level_color = TERM_FG_YELLOW;
        } break;
        case Log_Level_Error:
        {
            level_color = TERM_FG_RED;
        } break;
    }
    return level_color;
}
internal const char *log_get_level_string(Log_Level level)
{
    const char *level_string = "";
    switch (level)
    {
        case Log_Level_None:
        {
            level_string = "";
        } break;
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
    }
    return level_string;
}

internal const char *log_get_file_info_color(Log_Context *log)
{
    const char *line_info_color = "";
    if (log->enable_color_log && term_is_color_allowed())
    {
        line_info_color = TERM_FG_CYAN;
    }
    return line_info_color;
}

internal void log_print_color_level(Log_Context *log, Log_Level level)
{
    if (log->print_level_prefix)
    {
        const char *level_string = log_get_level_string(level);
        const char *level_color = "";
        if (log->enable_color_log && term_is_color_allowed())
        {
            level_color = log_get_level_color(level);
        }
        fmt_fprintf(log->file, "%s%s%s ", level_color, level_string, log_get_reset_color(log));
    }
}

//~ ak: Initialization Functions
//=============================================================================

internal Log_Context log_init(void)
{
    Log_Context log = STRUCT_ZERO;
    log.level = Log_Level_Info;
    log.file = OS_STDOUT;
    log.print_level_prefix = true;
    log.enable_color_log = true;
    return log;
}

//~ ak: Log Level Print Functions
//=============================================================================

//- ak: print normal

internal void log_print(Log_Context *log, Log_Level level, const char *string)
{
    if (level >= log->level)
    {
        log_print_color_level(log, level);
        fmt_fprint(log->file, string);
    }
}

internal void log_info(Log_Context *log, const char *string)
{
    log_print(log, Log_Level_Info, string);
}
internal void log_debug(Log_Context *log, const char *string)
{
    log_print(log, Log_Level_Debug, string);
}
internal void log_warn(Log_Context *log, const char *string)
{
    log_print(log, Log_Level_Warn, string);
}
internal void log_error(Log_Context *log, const char *string)
{
    log_print(log, Log_Level_Error, string);
}

//- ak: print with newline

internal void log_println(Log_Context *log, Log_Level level, const char *string)
{
    if (level >= log->level)
    {
        log_print_color_level(log, level);
        fmt_fprintln(log->file, string);
    }
}

internal void log_infoln(Log_Context *log, const char *string)
{
    log_println(log, Log_Level_Info, string);
}
internal void log_debugln(Log_Context *log, const char *string)
{
    log_println(log, Log_Level_Debug, string);
}
internal void log_warnln(Log_Context *log, const char *string)
{
    log_println(log, Log_Level_Warn, string);
}
internal void log_errorln(Log_Context *log, const char *string)
{
    log_println(log, Log_Level_Error, string);
}

//- ak: print with format

internal void log_vprintf(Log_Context *log, Log_Level level, const char *format, va_list args)
{
    if (level >= log->level)
    {
        log_print_color_level(log, level);
        va_list args_copy;
        va_copy(args_copy, args);
            fmt_vfprintf(log->file, format, args_copy);
        va_end(args_copy);
    }
}

internal void log_infof(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintf(log, Log_Level_Info, format, args);
    va_end(args);
}
internal void log_debugf(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintf(log, Log_Level_Debug, format, args);
    va_end(args);
}
internal void log_warnf(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintf(log, Log_Level_Warn, format, args);
    va_end(args);
}
internal void log_errorf(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintf(log, Log_Level_Error, format, args);
    va_end(args);
}

//- ak: print with format and newline

internal void log_vprintfln(Log_Context *log, Log_Level level, const char *format, va_list args)
{
    if (level >= log->level)
    {
        log_print_color_level(log, level);
        va_list args_copy;
        va_copy(args_copy, args);
            fmt_vfprintfln(log->file, format, args_copy);
        va_end(args_copy);
    }
}

internal void log_infofln(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintfln(log, Log_Level_Info, format, args);
    va_end(args);
}
internal void log_debugfln(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintfln(log, Log_Level_Debug, format, args);
    va_end(args);
}
internal void log_warnfln(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintfln(log, Log_Level_Warn, format, args);
    va_end(args);
}
internal void log_errorfln(Log_Context *log, const char *format, ...)
{
    va_list args;
    va_start(args, format);
        log_vprintfln(log, Log_Level_Error, format, args);
    va_end(args);
}
