#include "std/base/base_include.h"
#include "std/os/os_include.h"
#include "std/audio/audio.h"

#include "std/base/base_include.c"
#include "std/os/os_include.c"
#include "std/audio/audio.c"

internal void listen_eprintfln(const char *format, ...)
{
    fmt_eprintf("%s8 ", log_level_string_get(Log_Level_Error));
    va_list args;
    va_start(args, format);
        fmt_veprintfln(format, args);
    va_end(args);
}

internal void listen_flags_parse(Str8 *path)
{
    bool help = false;
    bool version = false;
    Str8 description = str8("Super simple CLI music player.");
    Str8 usage = str8("[OPTIONS] <FILE>");
    
    flags_init();
    Flags_Option *option = NULL;
    
    option = flags_option_bool(str8("help"), &help, help, str8("Print help message"));
    flags_add_option_shortname(option, str8("h"));
    option = flags_option_bool(str8("version"), &version, help, str8("Print version"));
    flags_add_option_shortname(option, str8("v"));
    
    flags_arg_str(path, StructZeroType(Str8));
    
    Str8_Array *args = term_args_get();
    if (!flags_parse(args))
    {
        flags_print_help(description, usage);
        flags_print_error();
        os_exit(1);
    }
    else if (help)
    {
        flags_print_help(description, usage);
        os_exit(0);
    }
    else if (version)
    {
        flags_print_version();
        os_exit(0);
    }
    else if (path->length == 0)
    {
        flags_print_help(description, usage);
        listen_eprintfln("<FILE> path is empty.");
        os_exit(1);
    }
}

internal void listen_draw_ui(Str8 display_name, Audio_Music music, bool is_first_frame)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    float duration = audio_music_duration_get(music);
    float time_played = audio_music_time_played_get(music);
    Str8_List lines = STRUCT_ZERO;
    
    // ak: line 1: Track title
    uint32_t max_name = 62;
    if ((uint32_t)display_name.length > max_name)
    {
        display_name = str8f(scratch.arena, "%.*s...", max_name - 3, display_name.cstr);
    }
    str8_list_push(scratch.arena, &lines, display_name);
    
    // ak: line 2: Progress bar
    uint32_t bar_width = 48;
    float progress = (duration > 0.0f) ? (time_played / duration) : 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;
    uint32_t filled = (uint32_t)(progress * (float)bar_width);
    if (filled > bar_width) filled = bar_width;
    
    uint8_t *bar_chars = arena_push(scratch.arena, uint8_t, bar_width + 1);
    for (uint32_t i = 0; i < bar_width; i++)
    {
        if (i < filled)       bar_chars[i] = '=';
        else if (i == filled) bar_chars[i] = '>';
        else                  bar_chars[i] = '-';
    }
    bar_chars[bar_width] = 0;
    Str8 bar_str = str8_init(bar_chars, bar_width);
    
    int cur_total = (int)time_played;
    if (cur_total < 0) cur_total = 0;
    int cur_mins = cur_total / 60;
    int cur_secs = cur_total % 60;
    
    int dur_total = (int)duration;
    if (dur_total < 0) dur_total = 0;
    int dur_mins = dur_total / 60;
    int dur_secs = dur_total % 60;
    
    str8_list_pushf(scratch.arena, &lines, "%02d:%02d [%.*s] %02d:%02d",
        cur_mins, cur_secs, str8_varg(bar_str), dur_mins, dur_secs);
    
    // ak: build frame output in a single atomic Str8_List buffer
    Str8_List out = STRUCT_ZERO;
    if (!is_first_frame)
    {
        str8_list_pushf(scratch.arena, &out, TermCursorUp("%d")"\r", (int)(lines.length - 1));
    }
    
    for (Str8_Node *node = lines.first; node != NULL; node = node->next)
    {
        str8_list_push(scratch.arena, &out, str8(TERM_CLEAR_LINE));
        str8_list_push(scratch.arena, &out, node->str);
        if (node->next != NULL)
        {
            str8_list_push(scratch.arena, &out, str8("\n"));
        }
    }
    
    Str8 frame = str8_list_join(scratch.arena, &out, NULL);
    fmt_print(frame);
    
    arena_scratch_end(scratch);
}

internal uint64_t base_main(void)
{
    log_trace_level_set(log_trace_g, Log_Level_Warn);
    Str8 path = STRUCT_ZERO;
    
    listen_flags_parse(&path);
     
    if (!fs_file_path_exists(path))
    {
        listen_eprintfln("file does not exist at path '%s8'.", path);
        return 1;
    }
    
    Str8 ext = str8_skip_last_dot(path);
    if (audio_format_from_string(ext) == Audio_Format_Unknown)
    {
        listen_eprintfln("file format '.%s8' is not supported ('%s8').", ext, path);
        return 1;
    }
    
    audio_init();
    Audio_Music music = audio_music_load(path);
    if (!audio_music_is_valid(music))
    {
        listen_eprintfln("unable to play audio file '%s8'.", path);
        audio_uninit();
        return 1;
    }
    audio_music_play(music);
    Str8 file_name = str8_skip_last_slash(path);
    bool is_first_frame = true;
    
    fmt_printf(TermSetTitle("PLAYING: %s8"), file_name);
    fmt_print(str8(TERM_CURSOR_HIDE));
    while (audio_music_is_playing(music))
    {
        audio_music_update(music);
        listen_draw_ui(file_name, music, is_first_frame);
        is_first_frame = false;
        os_sleep_ms(50);
    }
    fmt_print(str8("\n"TERM_CURSOR_SHOW));
    
    flags_uninit();
    audio_music_stop(music);
    audio_music_unload(music);
    audio_uninit();
    
    return 0;
}
