#include "std/base/base_include.h"
#include "std/os/os_include.h"
#include "std/audio/audio.h"

#include "std/base/base_include.c"
#include "std/os/os_include.c"
#include "std/audio/audio.c"

#include <termios.h>
#include <poll.h>

// ak: Types
//=============================================================================

enum
{
    PLAY_KEY_NONE = 0,
    PLAY_KEY_UP = 1000,
    PLAY_KEY_DOWN,
    PLAY_KEY_RIGHT,
    PLAY_KEY_LEFT,
};

typedef struct Play_State Play_State;
struct Play_State
{
    Audio_Music music;
    Str8_Array playlist;
    int32_t current_index;
    Str8 file_name;
    float duration;
    float volume;
    bool paused;
    bool quit;
};

// ak: Globals
//=============================================================================

global struct termios _play_orig_termios;
global volatile bool  _play_should_quit = false;

// ak: Terminal Raw Mode
//=============================================================================

internal void play_term_raw_enable(void)
{
    tcgetattr(STDIN_FILENO, &_play_orig_termios);
    struct termios raw = _play_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

internal void play_term_raw_disable(void)
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &_play_orig_termios);
}

internal void play_term_restore(void)
{
    fmt_printf("\n\x1B[?25h");
    play_term_raw_disable();
}

internal int play_term_read_key(void)
{
    struct pollfd pfd = { .fd = STDIN_FILENO, .events = POLLIN };
    if (poll(&pfd, 1, 0) <= 0)
    {
        return PLAY_KEY_NONE;
    }
    
    char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1)
    {
        return PLAY_KEY_NONE;
    }
    
    if (c == '\x1B')
    {
        struct pollfd seq_pfd = { .fd = STDIN_FILENO, .events = POLLIN };
        if (poll(&seq_pfd, 1, 10) > 0)
        {
            char seq[2] = {0};
            if (read(STDIN_FILENO, &seq[0], 1) == 1 && seq[0] == '[')
            {
                if (poll(&seq_pfd, 1, 10) > 0)
                {
                    if (read(STDIN_FILENO, &seq[1], 1) == 1)
                    {
                        switch (seq[1])
                        {
                            case 'A': return PLAY_KEY_UP;
                            case 'B': return PLAY_KEY_DOWN;
                            case 'C': return PLAY_KEY_RIGHT;
                            case 'D': return PLAY_KEY_LEFT;
                        }
                    }
                }
            }
        }
        return 'q';
    }
    
    return (unsigned char)c;
}

// ak: Track Management
//=============================================================================

internal bool play_track_load(Play_State *state, int32_t index)
{
    if (index < 0 || index >= (int32_t)state->playlist.length)
    {
        return false;
    }
    
    if (audio_music_is_valid(state->music))
    {
        audio_music_stop(state->music);
        audio_music_unload(state->music);
        state->music = (Audio_Music){0};
    }
    
    Str8 path = state->playlist.v[index];
    Audio_Music music = audio_music_load(path);
    if (!audio_music_is_valid(music))
    {
        return false;
    }
    
    state->music = music;
    state->current_index = index;
    state->file_name = str8_skip_last_slash(path);
    state->duration = audio_music_duration_get(music);
    state->paused = false;
    audio_music_volume_set(music, state->volume);
    audio_music_play(music);
    return true;
}

internal void play_track_next(Play_State *state)
{
    if (state->playlist.length == 0) return;
    int32_t start = state->current_index;
    int32_t next = (start + 1) % (int32_t)state->playlist.length;
    while (next != start)
    {
        if (play_track_load(state, next)) return;
        next = (next + 1) % (int32_t)state->playlist.length;
    }
    play_track_load(state, start);
}

internal void play_track_prev(Play_State *state)
{
    if (state->playlist.length == 0) return;
    int32_t start = state->current_index;
    int32_t prev = start - 1;
    if (prev < 0) prev = (int32_t)state->playlist.length - 1;
    while (prev != start)
    {
        if (play_track_load(state, prev)) return;
        prev--;
        if (prev < 0) prev = (int32_t)state->playlist.length - 1;
    }
    play_track_load(state, start);
}

// ak: UI Drawing
//=============================================================================

internal void play_draw_ui(Play_State *state, bool is_first_frame)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    float time_played = audio_music_time_played_get(state->music);
    float duration    = state->duration;
    bool  playing     = audio_music_is_playing(state->music);
    
    Str8_List lines = STRUCT_ZERO;
    
    // Line 1: Track title
    int max_name = (state->playlist.length > 1) ? 55 : 62;
    Str8 display_name = state->file_name;
    if ((int)display_name.length > max_name)
    {
        display_name = str8f(scratch.arena, "%.*s...", max_name - 3, display_name.cstr);
    }
    
    if (state->playlist.length > 1)
    {
        str8_list_pushf(scratch.arena, &lines, "[%d/%d] %.*s",
            state->current_index + 1, (int)state->playlist.length, str8_varg(display_name));
    }
    else
    {
        str8_list_push(scratch.arena, &lines, display_name);
    }
    
    // Line 2: Progress bar
    int bar_width = 40;
    float progress = (duration > 0.0f) ? (time_played / duration) : 0.0f;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;
    int filled = (int)(progress * (float)bar_width);
    if (filled > bar_width) filled = bar_width;
    
    uint8_t *bar_chars = arena_push(scratch.arena, uint8_t, bar_width + 1);
    for (int i = 0; i < bar_width; i++)
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
    
    // Line 3: Status & Volume
    Str8 status_str = state->paused ? str8("[Paused] ") : (playing ? str8("[Playing]") : str8("[Stopped]"));
    int vol_percent = (int)(state->volume * 100.0f + 0.5f);
    if (vol_percent > 100) vol_percent = 100;
    if (vol_percent < 0)   vol_percent = 0;
    
    str8_list_pushf(scratch.arena, &lines, "%.*s  Volume: %3d%%",
        str8_varg(status_str), vol_percent);
    
    // Controls (with blank line before)
    str8_list_push(scratch.arena, &lines, str8(""));
    str8_list_push(scratch.arena, &lines, str8("[Space] Play/Pause [<-/->] Seek  [Up/Dn] Volume  [Ctrl-C] Quit"));
    if (state->playlist.length > 1)
    {
        str8_list_push(scratch.arena, &lines, str8("[N] Next  [P] Previous"));
    }
    
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

// ak: Input Handling
//=============================================================================

internal void play_handle_input(Play_State *state)
{
    int key = play_term_read_key();
    if (key == PLAY_KEY_NONE) return;
    
    switch (key)
    {
        case 3: // Ctrl-C
        case 'q':
        case 'Q':
        {
            state->quit = true;
        }
        break;
        
        case ' ':
        {
            if (state->paused)
            {
                audio_music_resume(state->music);
                state->paused = false;
            }
            else
            {
                audio_music_pause(state->music);
                state->paused = true;
            }
        }
        break;
        
        case 'n':
        case 'N':
        case '>':
        {
            play_track_next(state);
        }
        break;
        
        case 'p':
        case 'P':
        case '<':
        {
            play_track_prev(state);
        }
        break;
        
        case PLAY_KEY_UP:
        case '+':
        case '=':
        {
            state->volume += 0.05f;
            if (state->volume > 1.0f) state->volume = 1.0f;
            audio_music_volume_set(state->music, state->volume);
        }
        break;
        
        case PLAY_KEY_DOWN:
        case '-':
        case '_':
        {
            state->volume -= 0.05f;
            if (state->volume < 0.0f) state->volume = 0.0f;
            audio_music_volume_set(state->music, state->volume);
        }
        break;
        
        case PLAY_KEY_RIGHT:
        {
            float pos = audio_music_time_played_get(state->music);
            float target = pos + 5.0f;
            if (target >= state->duration) target = state->duration - 0.1f;
            if (target < 0.0f) target = 0.0f;
            audio_music_seek(state->music, target);
        }
        break;
        
        case PLAY_KEY_LEFT:
        {
            float pos = audio_music_time_played_get(state->music);
            float target = pos - 5.0f;
            if (target < 0.0f) target = 0.0f;
            audio_music_seek(state->music, target);
        }
        break;
    }
}

// ak: Help Message
//=============================================================================

internal void play_help_message(void)
{
    fmt_println(str8("USAGE:"));
    Str8 *program_name = os_program_path_get();
    fmt_printfln("   %s [OPTIONS] <..FILES>", program_name->cstr);
    fmt_println(str8("OPTIONS:"));
    flags_print_help();
}

// ak: Entry Point
//=============================================================================

internal int32_t base_main(void)
{
    log_trace_level_set(log_trace_g, Log_Level_Warn);
    Arena *arena = arena_alloc();
    Str8_Array paths = STRUCT_ZERO;
    
    flags_init();
    Flags_Option *option = NULL;
    Flags_Arg *arg = NULL;
    
    bool help = false;
    option = flags_option_bool(str8("help"), &help, help, str8("Print help message"));
    flags_add_option_shortname(option, str8("h"));
    
    arg = flags_arg_str_arr(&paths, NULL);
    flags_make_arg_required(arg);
    
    Str8_Array *args = term_args_get();
    bool parsed = flags_parse(args);
    
    if (help)
    {
        play_help_message();
        return 0;
    }
    
    if (!parsed || paths.length == 0)
    {
        play_help_message();
        flags_print_error();
        return 1;
    }
    
    // Filter valid paths into playlist
    Str8_List valid_list = STRUCT_ZERO;
    for (size_t i = 0; i < paths.length; i++)
    {
        Str8 path = paths.v[i];
        if (!fs_file_path_exists(path))
        {
            fmt_eprintf("%s8 file does not exist at path '%s8'.\n", log_level_string_get(Log_Level_Error), path);
            continue;
        }
        
        Str8 ext = str8_skip_last_dot(path);
        if (audio_format_from_string(ext) == Audio_Format_Unknown)
        {
            fmt_eprintf("%s8 file format '.%s8' is not supported ('%s8').\n", log_level_string_get(Log_Level_Error), ext, path);
            continue;
        }
        
        str8_list_push(arena, &valid_list, path);
    }
    
    if (valid_list.length == 0)
    {
        fmt_eprintf("%s8 no valid audio files to play.\n", log_level_string_get(Log_Level_Error));
        return 1;
    }
    
    Str8_Array playlist = str8_array_from_list(arena, &valid_list);
    
    audio_init();
    
    // Setup state
    Play_State state = STRUCT_ZERO;
    state.playlist      = playlist;
    state.current_index = 0;
    state.volume        = 1.0f;
    state.paused        = false;
    
    if (!play_track_load(&state, 0))
    {
        fmt_eprintf("%s8 unable to play audio file '%s8'.\n", log_level_string_get(Log_Level_Error), playlist.v[0]);
        audio_uninit();
        return 1;
    }
    
    // Setup terminal
    play_term_raw_enable();
    fmt_printf("\x1B[?25l"); // Hide cursor
    
    bool is_first_frame = true;
    while (!state.quit)
    {
        audio_music_update(state.music);
        play_handle_input(&state);
        
        // Auto-advance if track ended
        if (!audio_music_is_playing(state.music) && !state.paused)
        {
            if (state.current_index + 1 < (int32_t)state.playlist.length)
            {
                if (!play_track_load(&state, state.current_index + 1))
                {
                    play_track_next(&state);
                }
            }
            else
            {
                state.quit = true;
                break;
            }
        }
        
        play_draw_ui(&state, is_first_frame);
        is_first_frame = false;
        
        os_sleep_ms(50);
    }
    
    // Cleanup
    play_term_restore();
    flags_uninit();
    if (audio_music_is_valid(state.music))
    {
        audio_music_stop(state.music);
        audio_music_unload(state.music);
    }
    audio_uninit();
    arena_free(arena);
    
    return 0;
}
