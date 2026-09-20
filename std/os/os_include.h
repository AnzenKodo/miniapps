#ifndef OS_INCLUDE_H
#define OS_INCLUDE_H

#include "os_core.h"

#if OS_LINUX
#   include "os_linux.h"
#elif OS_WINDOWS
#   include "os_windows.h"
#else
#   error OS layer not implemented for this operating system.
#endif

#endif // OS_INCLUDE_H
