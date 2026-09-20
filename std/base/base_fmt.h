#ifndef BASE_FMT_H
#define BASE_FMT_H

// ak: External Includes
//=============================================================================

#define STB_SPRINTF_DECORATE(name) fmt_##name
#define STB_SPRINTF_STATIC
#include "./external/stb_sprintf.h"

// ak: Functions
//=============================================================================

// ak: SPrint =================================================================

internal int32_t fmt_sprintf(char *buf, char const *fmt, ...);
internal int32_t fmt_snprintf(char *buf, int count, char const *fmt, ...);
internal int32_t fmt_vsprintf(char * buf, char const * fmt, va_list va);
internal int32_t fmt_vsnprintf(char *buf, int count, char const *fmt, va_list va);

// ak: FPrint =================================================================

internal uint64_t fmt_fprint(Fs_File file, const char *string);
internal uint64_t fmt_fprintln(Fs_File file, const char *string);
internal uint64_t fmt_vfprintf(Fs_File file, const char *format, va_list args);
internal uint64_t fmt_fprintf(Fs_File file, const char *format, ...);
internal uint64_t fmt_vfprintfln(Fs_File file, const char *format, va_list args);
internal uint64_t fmt_fprintfln(Fs_File file, const char *format, ...);

// ak: Print ==================================================================

internal void fmt_print(const char *string);
internal void fmt_println(const char *string);
internal void fmt_vprintf(const char *format, va_list args);
internal void fmt_printf(const char *format, ...);
internal void fmt_vprintfln(const char *format, va_list args);
internal void fmt_printfln(const char *format, ...);

// EPrint =====================================================================

internal void fmt_eprint(const char *string);
internal void fmt_eprintln(const char *string);
internal void fmt_veprintf(const char *format, va_list args);
internal void fmt_eprintf(const char *format, ...);
internal void fmt_veprintfln(const char *format, va_list args);
internal void fmt_eprintfln(const char *format, ...);

#endif // BASE_FMT_H
