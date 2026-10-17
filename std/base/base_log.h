//-----------------------------------------------------------------------------
// Reference:
// - By Go Authors: https://pkg.go.dev/log
// - By Epic Games Tools & Ryan Fleury:
//      - https://github.com/EpicGames/raddebugger/blob/e28a543573277315ffd32a3e983c8171570bb946/src/base/base_log.h
//      - https://github.com/EpicGames/raddebugger/blob/e28a543573277315ffd32a3e983c8171570bb946/src/base/base_log.c
//-----------------------------------------------------------------------------

#ifndef BASE_LOG_H
#define BASE_LOG_H

// ak: Types
//=============================================================================

typedef enum Log_Level
{
  Log_Level_Info,
  Log_Level_Debug,
  Log_Level_Warn,
  Log_Level_Error,
  Log_Level_COUNT
} Log_Level;

typedef struct Log_Result Log_Result;
struct Log_Result
{
    Str8 strings[Log_Level_COUNT];
};

typedef struct Log Log;
struct Log
{
    Arena *arena;
    Str8_List strings[Log_Level_COUNT];
    Str8 file_name;
    size_t line_number;
};

// ak: Functions
//=============================================================================

// ak: Log Creation ===========================================================

internal Log *log_alloc(void);
internal void log_free(Log *log);

// ak: Log Building ===========================================================

internal void log_msg(Log *log, Log_Level level, const char *file_name, size_t line_number, Str8 string);
internal void log_msgf(Log *log, Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

// ak: Log Result =============================================================

internal Log_Result log_get_result(Log *log, size_t indent, Arena *arena);

// ak: Macros
//=============================================================================

#define LogInfo(log, s)     log_msg((log),  Log_Level_Info,  FILE_NAME, LINE_NUMBER, (s))
#define LogInfoF(log, ...)  log_msgf((log), Log_Level_Info,  FILE_NAME, LINE_NUMBER, __VA_ARGS__)
#define LogDebug(log, s)    log_msg((log),  Log_Level_Debug, FILE_NAME, LINE_NUMBER, (s))
#define LogDebugF(log, ...) log_msgf((log), Log_Level_Debug, FILE_NAME, LINE_NUMBER, __VA_ARGS__)
#define LogWarn(log, s)     log_msg((log),  Log_Level_Warn,  FILE_NAME, LINE_NUMBER, (s))
#define LogWarnF(log, ...)  log_msgf((log), Log_Level_Warn,  FILE_NAME, LINE_NUMBER, __VA_ARGS__)
#define LogError(log, s)    log_msg((log),  Log_Level_Error, FILE_NAME, LINE_NUMBER, (s))
#define LogErrorF(log, ...) log_msgf((log), Log_Level_Error, FILE_NAME, LINE_NUMBER, __VA_ARGS__)

#define LogInfoNamedBlock(s)     DeferLoop(log_infof("%S:\n{\n", (s)), log_infof("}\n"))
#define LogInfoNamedBlockF(...)  DeferLoop((log_infof(__VA_ARGS__), log_infof(":\n{\n")), log_infof("}\n"))
#define LogDebugNamedBlock(s)    DeferLoop(log_debugf("%S:\n{\n", (s)), log_debugf("}\n"))
#define LogDebugNamedBlockF(...) DeferLoop((log_debugf(__VA_ARGS__), log_debugf(":\n{\n")), log_debugf("}\n"))
#define LogWarnNamedBlock(s)     DeferLoop(log_warnf("%S:\n{\n", (s)), log_warnf("}\n"))
#define LogWarnNamedBlockF(...)  DeferLoop((log_warnf(__VA_ARGS__), log_warnf(":\n{\n")), log_warnf("}\n"))
#define LogErrorNamedBlock(s)    DeferLoop(log_errorf("%S:\n{\n", (s)), log_errorf("}\n"))
#define LogErrorNamedBlockF(...) DeferLoop((log_errorf(__VA_ARGS__), log_errorf(":\n{\n")), log_errorf("}\n"))

// ak: Global
//=============================================================================

global Log *g_log = NULL;

#endif // BASE_LOG_H
