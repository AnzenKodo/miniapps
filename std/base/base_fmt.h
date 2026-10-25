//-----------------------------------------------------------------------------
// Reference:
// - By Go Authors: https://pkg.go.dev/fmt@go1.27.2
//-----------------------------------------------------------------------------

#ifndef BASE_FMT_H
#define BASE_FMT_H

// ak: Vendor Includes
//=============================================================================

#define STB_SPRINTF_DECORATE(name) fmt_##name
#define STB_SPRINTF_STATIC
#include "./vendor/stb_sprintf.h"

// ak: Functions
//=============================================================================

// ak: SPrint =================================================================

internal int fmt_sprintf(char *buf, char const *format, ...);
internal int fmt_snprintf(char *buf, int count, char const *format, ...);
internal int fmt_vsprintf(char * buf, char const * format, va_list va);
internal int fmt_vsnprintf(char *buf, int count, char const *format, va_list va);

// ak: FPrint =================================================================

internal size_t fmt_fprint(Fs_File file, Str8 string);
internal size_t fmt_fprintln(Fs_File file, Str8 string);
internal size_t fmt_vfprintf(Fs_File file, const char *format, va_list args);
internal size_t fmt_fprintf(Fs_File file, const char *format, ...);
internal size_t fmt_vfprintfln(Fs_File file, const char *format, va_list args);
internal size_t fmt_fprintfln(Fs_File file, const char *format, ...);

// ak: Print ==================================================================

internal void fmt_print(Str8 string);
internal void fmt_println(Str8 string);
internal void fmt_vprintf(const char *format, va_list args);
internal void fmt_printf(const char *format, ...);
internal void fmt_vprintfln(const char *format, va_list args);
internal void fmt_printfln(const char *format, ...);

// EPrint =====================================================================

internal void fmt_eprint(Str8 string);
internal void fmt_eprintln(Str8 string);
internal void fmt_veprintf(const char *format, va_list args);
internal void fmt_eprintf(const char *format, ...);
internal void fmt_veprintfln(const char *format, va_list args);
internal void fmt_eprintfln(const char *format, ...);

#endif // BASE_FMT_H
