#include "std/base/base_include.h"
#include "std/os/os_include.h"
#include "std/audio/audio.h"

#include "std/base/base_include.c"
#include "std/os/os_include.c"
#include "std/audio/audio.c"

internal void play_draw_ui(Str8 display_name, Audio_Music music, bool is_first_frame)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    float duration = audio_music_duration_get(music);
    float time_played = audio_music_time_played_get(music);
    Str8_List lines = STRUCT_ZERO;
    
    // Line 1: Track title
    uint32_t max_name = 62;
    if ((uint32_t)display_name.length > max_name)
    {
        display_name = str8f(scratch.arena, "%.*s...", max_name - 3, display_name.cstr);
    }
    str8_list_push(scratch.arena, &lines, display_name);
    
    // Line 2: Progress bar
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
    
    // Build frame output in a single atomic Str8_List buffer
    Str8_List out = STRUCT_ZERO;
    if (!is_first_frame)
    {
        str8_list_pushf(scratch.arena, &out, "\x1B[%dA\r", (int)(lines.length - 1));
    }
    
    for (Str8_Node *node = lines.first; node != NULL; node = node->next)
    {
        str8_list_push(scratch.arena, &out, str8("\x1B[2K"));
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

internal void play_help_message(void)
{
    fmt_println(str8("USAGE:"));
    Str8 *program_name = os_program_path_get();
    fmt_printfln("   %s [OPTIONS] <FILE>", program_name->cstr);
    fmt_println(str8("OPTIONS:"));
    flags_print_help();
    fmt_println(str8("OPTIONS:"));
    fmt_printfln("   %s", BUILD_VERSION);
}

internal int32_t base_main(void)
{
    log_trace_level_set(log_trace_g, Log_Level_Warn);
    Str8 path = STRUCT_ZERO;
    bool help = false;
    
    // ak: parse flags
    {
        flags_init();
        Flags_Option *option = NULL;
        Flags_Arg *arg = NULL;
        
        option = flags_option_bool(str8("help"), &help, help, str8("Print help message"));
        flags_add_option_shortname(option, str8("h"));
        
        arg = flags_arg_str(&path, StructZeroType(Str8));
        flags_make_arg_required(arg);
        
        Str8_Array *args = term_args_get();
        bool parsed = flags_parse(args);
        
        if (!parsed)
        {
            play_help_message();
            flags_print_error();
            return 1;
        }
    }
    
    if (help)
    {
        play_help_message();
        return 0;
    }
    
    if (!fs_file_path_exists(path))
    {
        fmt_eprintf("%s8 file does not exist at path '%s8'.\n", log_level_string_get(Log_Level_Error), path);
        return 1;
    }
    
    Str8 ext = str8_skip_last_dot(path);
    if (audio_format_from_string(ext) == Audio_Format_Unknown)
    {
        fmt_eprintf("%s8 file format '.%s8' is not supported ('%s8').\n", log_level_string_get(Log_Level_Error), ext, path);
        return 1;
    }
    
    audio_init();
    Audio_Music music = audio_music_load(path);
    if (!audio_music_is_valid(music))
    {
        fmt_eprintf("%s8 unable to play audio file '%s8'.\n", log_level_string_get(Log_Level_Error), path);
        audio_uninit();
        return 1;
    }
    audio_music_play(music);
    Str8 file_name = str8_skip_last_slash(path);
    bool is_first_frame = true;
    
    fmt_print(str8("\x1B[?25l")); // hide cursor
    while (audio_music_is_playing(music))
    {
        audio_music_update(music);
        play_draw_ui(file_name, music, is_first_frame);
        is_first_frame = false;
        os_sleep_ms(50);
    }
    fmt_print(str8("\n\x1B[?25h")); // restore cursor and add newline
    
    flags_uninit();
    audio_music_stop(music);
    audio_music_unload(music);
    audio_uninit();
    
    return 0;
}
