#ifndef OS_WINDOWS_H
#define OS_WINDOWS_H

// ak: External Includes
//=============================================================================
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define NOGDICAPMASKS
// #define NOVIRTUALKEYCODES
// #define NOSYSMETRICS
#define NOICONS
#define NOKEYSTATES
#define NOSYSCOMMANDS
// #define NORASTEROPS
// #define NOSHOWWINDOW
#define NOATOM
#define NOMETAFILE
// #define NOMSG
#define NOSCROLL
#define NOTEXTMETRIC
// #define NOWINOFFSETS
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOCRYPT
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "shell32.lib")
#include <winsock2.h>
#include <winuser.h>
#include <mswsock.h>
#include <shellapi.h>
#include <Winuser.h>
#include <shellscalingapi.h>

// ak: Types
//=============================================================================

typedef struct _OS_Win32_State _OS_Win32_State;
struct _OS_Win32_State
{
    uint64_t microsecond_resolution;
};

typedef struct _Os_Win32_Walk_Iter _Os_Win32_Walk_Iter;
struct _Os_Win32_Walk_Iter
{
    HANDLE handle;
    WIN32_FIND_DATAW find_data;
    bool is_volume_iter;
    Str8_Array drive_strings;
    uint64_t drive_strings_iter_idx;
};

// ak: Function pointers ======================================================

typedef BOOL w32_SetProcessDpiAwarenessContext_Type(void* value);
typedef UINT w32_GetDpiForWindow_Type(HWND hwnd);
typedef HRESULT w32_GetDpiForMonitor_Type(HMONITOR hmonitor, MONITOR_DPI_TYPE dpiType, UINT *dpiX, UINT *dpiY);
typedef int w32_GetSystemMetricsForDpi_Type(int nIndex, UINT dpi);
#define w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((void*)-4)
global w32_GetDpiForWindow_Type *w32_GetDpiForWindow_func = 0;
global w32_GetDpiForMonitor_Type *w32_GetDpiForMonitor_func = 0;
global w32_GetSystemMetricsForDpi_Type *w32_GetSystemMetricsForDpi_func = 0;

// ak: Defines
//=============================================================================

#define OS_STDIN  (Os_File)GetStdHandle(STD_INPUT_HANDLE)
#define OS_STDOUT (Os_File)GetStdHandle(STD_OUTPUT_HANDLE)
#define OS_STDERR (Os_File)GetStdHandle(STD_ERROR_HANDLE)

// ak: Functions
//=============================================================================

// Helpers functions ==========================================================

internal uint32_t _os_win32_unix_time_from_file_time(FILETIME file_time);
internal Os_File_Property_Flags _os_win32_file_property_flags_from_dwFileAttributes(DWORD dwFileAttributes);
internal void _os_win32_dense_time_from_file_time(DenseTime *out, FILETIME *in);
internal void _os_win32_date_time_from_system_time(DateTime *out, SYSTEMTIME *in);

// ak: Global Variables
//=============================================================================

global _OS_Win32_State _os_win32_state = STRUCT_ZERO;

#endif // OS_WINDOWS_H
