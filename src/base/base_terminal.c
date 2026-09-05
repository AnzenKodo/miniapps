internal bool term_is_color_allowed(void)
{
    bool result = true;
    // Str8 term_env = os_env_get(str8("TERM"));
    if (os_env_is_set(str8("NO_COLOR")) || !os_is_term_mode(OS_STDOUT))
    {
        result = false;
    }
    return result;
}

internal void term_style_start(Os_File file, const char *style)
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
