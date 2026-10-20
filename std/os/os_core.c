// ak: File
//=============================================================================

internal Str8 os_get_data_home_path(void)
{
    return _os_core_state.data_home;
}

// ak: Command-Line Operations
//=============================================================================

internal Str8 *os_program_path_get(void)
{
    return &_os_core_state.args.v[0];
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
