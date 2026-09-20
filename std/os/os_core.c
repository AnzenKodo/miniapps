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
    _os_core_state.log_context = log_init();
#if BUILD_DEBUG
    _os_core_state.log_context.level = Log_Level_Info;
    fmt_println("# Program Output ============================================================ #");
#else
    _os_core_state.log_context.level = Log_Level_None;
#endif
    base_main();
}
