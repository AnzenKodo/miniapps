//~ ak: State
//=============================================================================

typedef struct _Os_Core_State _Os_Core_State;
struct _Os_Core_State
{
    Str8_Array args;
    Str8 data_home;
    Process_Info process_info;
};

global _Os_Core_State _os_core_state = STRUCT_ZERO;

// ak: File
//=============================================================================

internal Str8 os_get_data_home_path(void)
{
    return _os_core_state.data_home;
}

// ak: Command-Line Operations
//=============================================================================

internal Str8 os_program_path_get(void)
{
    return _os_core_state.args.v[0];
}

// ak: OS Entry Points
//=============================================================================

internal int32_t os_main(void)
{
    log_trace_g = log_alloc();
    int exit_code = base_main();
    log_free(log_trace_g);
    return exit_code;
}

//~ ak: Os Functions for Base Layer
//=============================================================================

//- ak: Terminal ==============================================================

internal Str8_Array *term_args_get(void)
{
    return &_os_core_state.args;
}

//- ak: Processes =============================================================

internal Process_Info *process_info_get(void)
{
    return &_os_core_state.process_info;
}
