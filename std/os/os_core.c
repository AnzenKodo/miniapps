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

internal void os_main(void)
{
    g_log = log_alloc();
    base_main();
}
