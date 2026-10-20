//-----------------------------------------------------------------------------
// Reference:
// - By Go Authors:
//      - https://pkg.go.dev/log@go1.27.2
//      - https://pkg.go.dev/slog@go1.27.2
// - By Epic Games Tools & Ryan Fleury:
//      - https://github.com/EpicGames/raddebugger/blob/e28a543573277315ffd32a3e983c8171570bb946/src/base/base_log.h
//      - https://github.com/EpicGames/raddebugger/blob/e28a543573277315ffd32a3e983c8171570bb946/src/base/base_log.c
// - By Ramon Santamaria:
//      - https://github.com/raysan5/raylib/blob/5dc3674c009278c9b6bf86e39b1705d55d0fe5f4/src/rcore.c
//      - https://github.com/raysan5/raylib/blob/28789122cec21e4c7a1689ab60f7a273f44dff31/src/raylib.h
//-----------------------------------------------------------------------------

#ifndef BASE_LOG_H
#define BASE_LOG_H

// ak: Types
//=============================================================================

typedef enum Log_Level
{
    Log_Level_Trace,
    Log_Level_Info,
    Log_Level_Debug,
    Log_Level_Warn,
    Log_Level_Error,
    Log_Level_COUNT
} Log_Level;

typedef struct Log_Node Log_Node;
struct Log_Node
{
    Log_Node *next;
    Str8 str;
    Str8 file_name;
    size_t line_number;
};

typedef struct Log_List Log_List;
struct Log_List
{
    Log_Node *first;
    Log_Node *last;
    size_t length;
};

typedef struct Log Log;
struct Log
{
    Arena *arena;
    Log_List list[Log_Level_COUNT];
    Log_Node *last_printed[Log_Level_COUNT];
    Log_Level trace_level;
};

// ak: Functions
//=============================================================================

// ak: Helper =================================================================

internal Str8 log_level_string_get(Log_Level level);
internal Fs_File std_type_from_log_level(Log_Level level);

// ak: Log Creation ===========================================================

internal Log *log_alloc(void);
internal void log_free(Log *log);

// ak: Log Builder ============================================================

internal void log_push(Log *log, Log_Level level, const char *file_name, size_t line_number, Str8 string);
internal void log_pushf(Log *log, Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

// ak: Log Trace ==============================================================

internal void log_trace_level_set(Log *log, Log_Level level);
internal void log_trace(Log *log, Log_Level level, const char *file_name, size_t line_number, Str8 string);
internal void log_tracef(Log *log, Log_Level level, const char *file_name, size_t line_number, const char *format, ...);

// ak: Macros
//=============================================================================

#define LogTrace(level, s)    log_trace(log_trace_g,  Log_Level_##level, FILE_PATH, LINE_NUMBER, (s))
#define LogTraceF(level, ...)   log_tracef(log_trace_g, Log_Level_##level, FILE_PATH, LINE_NUMBER, __VA_ARGS__)

// ak: Global
//=============================================================================

global Log *log_trace_g = NULL;

#endif // BASE_LOG_H
