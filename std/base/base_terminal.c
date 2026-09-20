// ak: Arguments
//=============================================================================

internal Str8_Array *term_args_get(void)
{
    return &_os_core_state.args;
}

// ak: Terminal
//=============================================================================

internal bool term_is_color_allowed(void)
{
    bool result = true;
    // Str8 term_env = os_env_get(str8("TERM"));
    if (os_is_env_exists(str8("NO_COLOR")) || !term_is_terminal(OS_STDOUT))
    {
        result = false;
    }
    return result;
}

internal void term_style_start(Fs_File file, const char *style)
{
    _term_state.file = file;
    if (term_is_color_allowed())
    {
        fmt_fprint(file, style);
    }
}

internal void term_style_end(void)
{
    fmt_fprint(_term_state.file, TERM_RESET);
    _term_state.file = 0;
}

internal char *term_style_get(const char *style)
{
    char *result = NULL;
    if (term_is_color_allowed())
    {
        result = (char *)style;
    }
    return result;
}
