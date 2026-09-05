#include "os_core.c"

#if OS_LINUX
#   include "os_linux.c"
#elif OS_WINDOWS
#   include "os_windows.c"
#else
#   error OS layer not implemented for this operating system.
#endif
