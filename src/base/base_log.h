#ifndef BASE_LOG_H
#define BASE_LOG_H

//~ ak: Types
//=============================================================================

typedef enum Log_Level {
    Log_Level_None,
    Log_Level_Info,
    Log_Level_Debug,
    Log_Level_Warn,
    Log_Level_Error,
} Log_Level;

typedef struct Log_Context Log_Context;
struct Log_Context {
    Log_Level level;
    Os_File file;
    bool print_level_prefix;
    bool enable_color_log;
};

//~ ak: Functions
//=============================================================================

//~ ak: Helper Functions ======================================================

internal const char *log_get_reset_color(Log_Context *log);
internal const char *log_get_level_color(Log_Level level);
internal const char *log_get_level_string(Log_Level level);
internal const char *log_get_file_info_color(Log_Context *log);
internal void log_print_color_level(Log_Context *log, Log_Level level);

//~ ak: Initialization Functions ==============================================

internal Log_Context log_init(void);

//~ ak: Log Level Print Functions =============================================

//- ak: print normal
internal void log_print(Log_Context *log, Log_Level level, const char *format);
internal void log_info(Log_Context *log, const char *format);
internal void log_debug(Log_Context *log, const char *format);
internal void log_warn(Log_Context *log, const char *format);
internal void log_error(Log_Context *log, const char *format);

//- ak: print with newline
internal void log_println(Log_Context *log, Log_Level level, const char *format);
internal void log_infoln(Log_Context *log, const char *format);
internal void log_debugln(Log_Context *log, const char *format);
internal void log_warnln(Log_Context *log, const char *format);
internal void log_errorln(Log_Context *log, const char *format);

//- ak: print with format
internal void log_vprintf(Log_Context *log, Log_Level level, const char *format, va_list args);
internal void log_infof(Log_Context *log, const char *format, ...);
internal void log_debugf(Log_Context *log, const char *format, ...);
internal void log_warnf(Log_Context *log, const char *format, ...);
internal void log_errorf(Log_Context *log, const char *format, ...);

//- ak: print with format and newline
internal void log_vprintfln(Log_Context *log, Log_Level level, const char *format, va_list args);
internal void log_infofln(Log_Context *log, const char *format, ...);
internal void log_debugfln(Log_Context *log, const char *format, ...);
internal void log_warnfln(Log_Context *log, const char *format, ...);
internal void log_errorfln(Log_Context *log, const char *format, ...);

//~ ak: Macros
//=============================================================================

#define LogPrintfLine(log, level, format, ...) do { \
    const char *line_info_color = log_get_file_info_color(log); \
    fmt_fprintf((log)->file, "%s%s:%d%s ", line_info_color, FILE_NAME, LINE_NUMBER, log_get_reset_color(log)); \
    log_print_color_level((log), level); \
    fmt_fprintfln((log)->file, format, ##__VA_ARGS__); \
} while(0)
#define LogInfoLine(log, format, ...) do { \
    LogPrintfLine(log, Log_Level_Info, format, ##__VA_ARGS__); \
} while(0)
#define LogDebugLine(log, format, ...) do { \
    LogPrintfLine(log, Log_Level_Debug, format, ##__VA_ARGS__); \
} while(0)
#define LogWarnLine(log, format, ...) do { \
    LogPrintfLine(log, Log_Level_Warn, format, ##__VA_ARGS__); \
} while(0)
#define LogErrorLine(log, format, ...) do { \
    LogPrintfLine(log, Log_Level_Error, format, ##__VA_ARGS__); \
} while(0)

#endif // BASE_LOG_H
