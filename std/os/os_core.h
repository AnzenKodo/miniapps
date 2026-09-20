//-----------------------------------------------------------------------------
// Reference:
// - By Epic Games Tools & Ryan Fleury: https://github.com/EpicGames/raddebugger/tree/aea4a4ff9d6ed823b432d94b84fe88054f69265b/src/os/core
//-----------------------------------------------------------------------------

#ifndef OS_CORE_H
#define OS_CORE_H

// ak: External Includes
//=============================================================================

#include <fcntl.h>
#include <errno.h>

// ak: Types
//=============================================================================

// ak: Private OS State ======================================================

typedef struct _Os_Core_State _Os_Core_State;
struct _Os_Core_State
{
    Str8_Array args;
    Log_Context log_context;
    Str8 data_home;
};

// ak: Functions
//=============================================================================

// ak: Memory =================================================================

internal size_t os_pagesize_get(void);

// ak: Exit ===================================================================

internal void os_exit(int32_t exit_code);

// ak: Sleep ==================================================================

internal void os_sleep_us(uint64_t microsec);
internal void os_sleep_ms(uint32_t millisec);

// ak: Command-Line Operations ================================================

internal Str8 *os_program_path_get(void);

// ak: Environment Variable ===================================================

internal bool os_is_env_exists(Str8 name);
internal Str8 os_env_get(Str8 name);

// ak: OS Entry Points ========================================================

internal void os_main(void);

// ak: Global Variables
//=============================================================================

global _Os_Core_State _os_core_state = STRUCT_ZERO;

#endif // OS_CORE_H
