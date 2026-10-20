// ak: ak: Vendor Includes
//=============================================================================

#define STB_SPRINTF_IMPLEMENTATION
#include "./vendor/stb_sprintf.h"

// ak: FPrint
//=============================================================================

internal size_t fmt_fprint(Fs_File file, Str8 string)
{
    fs_file_write_append_raw(file, string.cstr, string.length);
    return string.length;
}
internal size_t fmt_fprintln(Fs_File file, Str8 string)
{
    fmt_fprint(file, string);
    fmt_fprint(file, str8("\n"));
    return string.length;
}
internal size_t fmt_vfprintf(Fs_File file, const char *format, va_list args)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    Str8 string = str8fv(scratch.arena, format, args);
    fmt_fprint(file, string);
    arena_scratch_end(scratch);
    return string.size;
}
internal size_t fmt_fprintf(Fs_File file, const char *format, ...)
{
    size_t written = 0;
    va_list args;
    va_start(args, format);
        written = fmt_vfprintf(file, format, args);
    va_end(args);
    return written;
}
internal size_t fmt_vfprintfln(Fs_File file, const char *format, va_list args)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    size_t written = 0;
    va_list args_copy;
    va_copy(args_copy, args);
        size_t needed = fmt_vsnprintf(NULL, 0, format, args) + 1;
        char *cstr = arena_push_nz(scratch.arena, char, needed+1);
        written = fmt_vsnprintf(cstr, needed, format, args_copy);
        cstr[written] = '\n';
        cstr[written+1] = '\0';
        fmt_fprint(file, str8_from_cstr(cstr));
    va_end(args_copy);
    arena_scratch_end(scratch);
    return written;
}
internal size_t fmt_fprintfln(Fs_File file, const char *format, ...)
{
    size_t written = 0;
    va_list args;
    va_start(args, format);
        written = fmt_vfprintfln(file, format, args);
    va_end(args);
    return written;
}

// ak: Print
//=============================================================================

internal void fmt_print(Str8 string)
{
    fmt_fprint(OS_STDOUT, string);
}
internal void fmt_println(Str8 string)
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

internal void fmt_eprint(Str8 string)
{
    fmt_fprint(OS_STDERR, string);
}
internal void fmt_eprintln(Str8 string)
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
