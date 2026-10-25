//-----------------------------------------------------------------------------
// Reference:
// - By Epic Games Tools & Ryan Fleury: https://github.com/EpicGames/raddebugger/tree/e28a543573277315ffd32a3e983c8171570bb946/src/linux/base
// - By Allen Webster: https://github.com/4coder-archive/4coder/tree/master/platform_linux
//-----------------------------------------------------------------------------

#ifndef OS_LINUX_H
#define OS_LINUX_H

//~ak: External Includes
//=============================================================================

#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <time.h>
#include <spawn.h>
#include <signal.h>
#include <sys/wait.h>
extern char **environ;

//~ ak: Types
//=============================================================================

//~ ak: File System ===========================================================

//- ak: directory walking
typedef struct _Os_Linux_Walk _Os_Linux_Walk;
struct _Os_Linux_Walk
{
  DIR *dir;
  struct dirent *dp;
  Str8 path;
};

//~ ak: Defines
//=============================================================================

#define OS_STDIN  STDIN_FILENO
#define OS_STDOUT STDOUT_FILENO
#define OS_STDERR STDERR_FILENO

//~ ak: Macros
//=============================================================================

// ak: Linux Call Interruption Retry Helper
#define LNX_RETRY_ON_EINTR(expr)             \
(__extension__({                           \
__typeof__(expr) __ret;                    \
do {                                       \
__ret = (expr);                          \
} while ((__ret == -1) && errno == EINTR); \
__ret;                                     \
}))

//~ ak: Functions
//=============================================================================

//~ ak: Helpers functions =====================================================

internal DateTime _os_linux_date_time_from_tm(struct tm in, uint32_t msec);
internal DenseTime _os_linux_dense_time_from_timespec(struct timespec in);
internal Fs_Properties _os_linux_properties_from_stat(struct stat *s);

#endif // OS_LINUX_H
