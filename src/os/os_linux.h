#ifndef OS_LINUX_H
#define OS_LINUX_H

//~ak: External Includes
//=============================================================================

#define _POSIX_C_SOURCE 200809L

#include <dirent.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <time.h>
extern char **environ;

//~ ak: Types
//=============================================================================

//~ ak: File System ===========================================================

//- ak: directory walking
typedef struct _Os_Linux_File_Walk _Os_Linux_File_Walk;
struct _Os_Linux_File_Walk
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

//~ ak: Functions
//=============================================================================

//~ ak: Helpers functions =====================================================

internal DateTime _os_linux_date_time_from_tm(struct tm in, uint32_t msec);
internal DenseTime _os_linux_dense_time_from_timespec(struct timespec in);
internal Os_File_Properties _os_linux_file_properties_from_stat(struct stat *s);

#endif // OS_LINUX_H
