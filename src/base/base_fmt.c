// ak: ak: External Includes
//=============================================================================

#define STB_SPRINTF_IMPLEMENTATION
#include "./external/stb_sprintf.h"

// ak: FPrint
//=============================================================================

internal uint64_t fmt_fprint(Os_File file, const char *string)
{
    uint64_t length = cstr8_length((uint8_t *)string);
    os_file_write_append(file, (char *)string, length);
    return length;
}
internal uint64_t fmt_fprintln(Os_File file, const char *string)
{
    uint64_t length = cstr8_length((uint8_t *)string);
    fmt_fprint(file, string);
    fmt_fprint(file, "\n");
    return length;
}
internal uint64_t fmt_vfprintf(Os_File file, const char *format, va_list args)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    uint64_t written = 0;
    va_list args_copy;
    va_copy(args_copy, args);
        int32_t needed = fmt_vsnprintf(NULL, 0, format, args_copy) + 1;
    va_end(args_copy);
    if (needed > 0)
    {
        char *buffer = arena_push(scratch.arena, char, needed);
        va_copy(args_copy, args);
            written = fmt_vsprintf(buffer, format, args_copy);
        va_end(args_copy);
        fmt_fprint(file, buffer);
    }
    arena_scratch_end(scratch);
    return written;
}
internal uint64_t fmt_fprintf(Os_File file, const char *format, ...)
{
    uint64_t written = 0;
    va_list args;
    va_start(args, format);
        written = fmt_vfprintf(file, format, args);
    va_end(args);
    return written;
}
internal uint64_t fmt_vfprintfln(Os_File file, const char *format, va_list args)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    uint64_t written = 0;
    va_list args_copy;
    va_copy(args_copy, args);
        int32_t needed = fmt_vsnprintf(NULL, 0, format, args_copy) + 2;
    va_end(args_copy);
    if (needed > 0)
    {
        char *buffer = arena_push(scratch.arena, char, needed);
        va_copy(args_copy, args);
            written = fmt_vsprintf(buffer, format, args_copy);
        va_end(args_copy);
        buffer[written] = '\n';
        buffer[written+1] = '\0';
        fmt_fprint(file, buffer);
    }
    arena_scratch_end(scratch);
    return written;
}
internal uint64_t fmt_fprintfln(Os_File file, const char *format, ...)
{
    uint64_t written = 0;
    va_list args;
    va_start(args, format);
        written = fmt_vfprintfln(file, format, args);
    va_end(args);
    return written;
}

// ak: Print
//=============================================================================

internal void fmt_print(const char *string)
{
    fmt_fprint(OS_STDOUT, string);
}
internal void fmt_println(const char *string)
{
    fmt_fprintln(OS_STDOUT, string);
}
internal void fmt_vprintf(const char *format, va_list args)
{
    fmt_vfprintf(OS_STDOUT, format, args);
}
internal void fmt_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
        fmt_vprintf(format, args);
    va_end(args);
}
internal void fmt_vprintfln(const char *format, va_list args)
{
    fmt_vfprintfln(OS_STDOUT, format, args);
}
internal void fmt_printfln(const char *format, ...)
{
    va_list args;
    va_start(args, format);
        fmt_vprintfln(format, args);
    va_end(args);
}

// ak: EPrint
//=============================================================================

internal void fmt_eprint(const char *string)
{
    fmt_fprint(OS_STDERR, string);
}
internal void fmt_eprintln(const char *string)
{
    fmt_fprintln(OS_STDOUT, string);
}
internal void fmt_veprintf(const char *format, va_list args)
{
    fmt_vfprintf(OS_STDERR, format, args);
}
internal void fmt_eprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
        fmt_veprintf(format, args);
    va_end(args);
}
internal void fmt_veprintfln(const char *format, va_list args)
{
    fmt_vfprintfln(OS_STDERR, format, args);
}
internal void fmt_eprintfln(const char *format, ...)
{
    va_list args;
    va_start(args, format);
        fmt_veprintfln(format, args);
    va_end(args);
}
