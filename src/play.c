#include "std/base/base_include.h"
#include "std/os/os_include.h"
#include "std/audio/audio.h"

#include "std/base/base_include.c"
#include "std/os/os_include.c"
#include "std/audio/audio.c"

internal void play_help_message()
{
    fmt_println(str8("USAGE:"));
    Str8 *program_name = os_program_path_get();
    fmt_printfln("   %s [OPTIONS]", program_name->cstr);
    fmt_println(str8("OPTIONS:"));
    flags_print_help();
}

internal int32_t base_main(void)
{
    Str8 path = STRUCT_ZERO;
    
    flags_init();
    Flags_Option *option = NULL;
    Flags_Arg *arg = NULL;
    
    bool help = false;
    option = flags_option_bool(str8("help"), &help, help, str8("Print help message"));
    flags_add_option_shortname(option, str8("h"));
    arg = flags_arg_str(&path, StructZeroType(Str8));
    flags_make_arg_required(arg);
    
    Str8_Array *args = term_args_get();
    if (!flags_parse(args))
    {
        play_help_message();
        flags_print_error();
        return 1;
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
    Str8 file_type = str8_skip_last_dot(path);
    if (audio_format_from_string(file_type) == Audio_Format_Unknown)
    {
        fmt_eprintf("%s8 file format '.%s8' is not supported.\n", log_level_string_get(Log_Level_Error), file_type);
        return 1;
    }
    
    audio_init();
    Audio_Music music = audio_music_load(path);
    if (!audio_music_is_valid(music))
    {
        fmt_eprintf("%s8 unable to play audio file '%s8'.\n", log_level_string_get(Log_Level_Error), path);
        return 1;
    }
    
    audio_music_play(music);
    while (true)
    {
        audio_music_update(music);
    }
    
    flags_uninit();
    audio_music_unload(music);
    audio_uninit();
    
    return 0;
}
