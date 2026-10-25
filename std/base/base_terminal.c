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

internal void term_style_start(Fs_File file, Str8 style)
{
    _term_state.file = file;
    if (term_is_color_allowed())
    {
        fmt_fprint(file, style);
    }
}

internal void term_style_end(void)
{
    fmt_fprint(_term_state.file, str8(TERM_RESET));
    _term_state.file = 0;
}

internal Str8 term_style_get(Str8 style)
{
    Str8 result = STRUCT_ZERO;
    if (term_is_color_allowed())
    {
        result = style;
    }
    return result;
}
