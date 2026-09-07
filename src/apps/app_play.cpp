// ak: headers
#include "../base/base_include.h"
#include "../os/os_include.h"
#include "../audio/audio.h"

#include <termios.h>
#include <unistd.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

// ak: implementation
#include "../base/base_include.c"
#include "../os/os_include.c"
#include "../audio/audio.c"

// ak: Terminal Raw Mode
//=============================================================================

global struct termios g_orig_termios;
global bool           g_termios_saved = false;

internal void term_raw_disable(void)
{
    if (g_termios_saved)
    {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        printf("\x1B[?25h\n"); // Show cursor and move to next line
        fflush(stdout);
        g_termios_saved = false;
    }
}

internal void term_raw_enable(void)
{
    if (!isatty(STDIN_FILENO)) { return; }
    tcgetattr(STDIN_FILENO, &g_orig_termios);
    g_termios_saved = true;
    atexit(term_raw_disable);
    
    struct termios raw = g_orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    
    printf("\x1B[?25l"); // Hide cursor
    fflush(stdout);
}

internal void term_sig_handler(int sig)
{
    (void)sig;
    term_raw_disable();
    _exit(0);
}

// ak: Keys
//=============================================================================

typedef enum App_Key
{
    App_Key_None = 0,
    App_Key_Space,
    App_Key_Up,
    App_Key_Down,
    App_Key_Left,
    App_Key_Right,
    App_Key_Restart,
    App_Key_Help,
    App_Key_Quit,
} App_Key;

internal int term_read_byte_timeout(int timeout_ms)
{
    struct pollfd pfd;
    pfd.fd = STDIN_FILENO;
    pfd.events = POLLIN;
    int ret = poll(&pfd, 1, timeout_ms);
    if (ret > 0 && (pfd.revents & POLLIN))
    {
        uint8_t b = 0;
        if (read(STDIN_FILENO, &b, 1) == 1)
        {
            return (int)b;
        }
    }
    return -1;
}

internal App_Key term_read_key(void)
{
    uint8_t c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) { return App_Key_None; }
    
    if (c == ' ') { return App_Key_Space; }
    if (c == 'r' || c == 'R') { return App_Key_Restart; }
    if (c == '?') { return App_Key_Help; }
    if (c == 'q' || c == 'Q' || c == 3 /* Ctrl+C */) { return App_Key_Quit; }
    
    if (c == 27) // Escape sequence
    {
        int b1 = term_read_byte_timeout(40);
        if (b1 == -1) { return App_Key_Quit; } // Lone Esc quits
        int b2 = term_read_byte_timeout(40);
        if (b2 == -1) { return App_Key_None; }
        
        if (b1 == '[' || b1 == 'O')
        {
            switch (b2)
            {
                case 'A': return App_Key_Up;
                case 'B': return App_Key_Down;
                case 'C': return App_Key_Right;
                case 'D': return App_Key_Left;
            }
        }
    }
    return App_Key_None;
}

// ak: UI Helpers
//=============================================================================

#define PROGRESS_BAR_BLOCKS 20

// Strips ANSI escape sequences and counts visible UTF-8 characters
internal int visible_len(const char *s)
{
    int len = 0;
    while (*s)
    {
        if (*s == '\x1B')
        {
            s++;
            if (*s == '[')
            {
                s++;
                while (*s && (*s < 'A' || *s > 'Z') && (*s < 'a' || *s > 'z'))
                {
                    s++;
                }
                if (*s) s++;
            }
        }
        else
        {
            if ((*s & 0xC0) != 0x80)
            {
                len++;
            }
            s++;
        }
    }
    return len;
}

// Generates a smooth, beautiful progress bar with sub-block resolution, styled square brackets, and ANSI colors
internal void make_progress_bar(char *buf, size_t buf_size, float fraction, int total_blocks, const char *fill_color)
{
    if (fraction < 0.0f) fraction = 0.0f;
    if (fraction > 1.0f) fraction = 1.0f;
    
    bool use_color = term_is_color_allowed();
    
    // Sub-blocks for 1/8 increments: 0..8
    static const char *sub_blocks[9] = {
        " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"
    };
    
    float total_pos = fraction * (float)total_blocks;
    int filled = (int)total_pos;
    float remainder = total_pos - (float)filled;
    int sub_idx = (int)(remainder * 8.0f + 0.5f);
    
    if (sub_idx == 8)
    {
        filled++;
        sub_idx = 0;
    }
    if (filled > total_blocks)
    {
        filled = total_blocks;
        sub_idx = 0;
    }
    
    int empty = total_blocks - filled - (sub_idx > 0 ? 1 : 0);
    if (empty < 0) empty = 0;
    
    buf[0] = '\0';
    
    // Opening bracket
    if (use_color)
    {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), "\x1B[90m[\x1B[0m");
    }
    else
    {
        strcat(buf, "[");
    }
    
    // Filled blocks
    if (filled > 0)
    {
        if (use_color)
        {
            snprintf(buf + strlen(buf), buf_size - strlen(buf), "%s", fill_color);
        }
        for (int i = 0; i < filled && strlen(buf) + 4 < buf_size; i++)
        {
            strcat(buf, "█");
        }
    }
    
    // Fractional block for smooth progress
    if (sub_idx > 0 && strlen(buf) + 16 < buf_size)
    {
        if (use_color)
        {
            snprintf(buf + strlen(buf), buf_size - strlen(buf), "%s", fill_color);
        }
        strcat(buf, sub_blocks[sub_idx]);
    }
    
    if (use_color && (filled > 0 || sub_idx > 0))
    {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), "\x1B[0m");
    }
    
    // Empty whitespace
    for (int i = 0; i < empty && strlen(buf) + 2 < buf_size; i++)
    {
        strcat(buf, " ");
    }
    
    // Closing bracket
    if (use_color)
    {
        snprintf(buf + strlen(buf), buf_size - strlen(buf), "\x1B[90m]\x1B[0m");
    }
    else
    {
        strcat(buf, "]");
    }
}

internal void make_time_string(char *buf, size_t buf_size, double current_sec, double total_sec)
{
    if (current_sec < 0.0) current_sec = 0.0;
    if (total_sec < 0.0) total_sec = 0.0;
    if (current_sec > total_sec && total_sec > 0.0) current_sec = total_sec;
    
    bool use_color = term_is_color_allowed();
    
    int cur_total_s = (int)current_sec;
    int cur_m = cur_total_s / 60;
    int cur_s = cur_total_s % 60;
    
    int tot_total_s = (int)total_sec;
    int tot_m = tot_total_s / 60;
    int tot_s = tot_total_s % 60;
    
    if (tot_m >= 60)
    {
        int cur_h = cur_m / 60; cur_m %= 60;
        int tot_h = tot_m / 60; tot_m %= 60;
        if (use_color)
        {
            snprintf(buf, buf_size, "\x1B[1;97m%02d:%02d:%02d\x1B[0m \x1B[90m-\x1B[0m %d:%02d:%02d ",
                     cur_h, cur_m, cur_s, tot_h, tot_m, tot_s);
        }
        else
        {
            snprintf(buf, buf_size, "%02d:%02d:%02d - %d:%02d:%02d ",
                     cur_h, cur_m, cur_s, tot_h, tot_m, tot_s);
        }
    }
    else
    {
        if (use_color)
        {
            snprintf(buf, buf_size, "\x1B[1;97m%02d:%02d\x1B[0m \x1B[90m-\x1B[0m %d:%02d ",
                     cur_m, cur_s, tot_m, tot_s);
        }
        else
        {
            snprintf(buf, buf_size, "%02d:%02d - %d:%02d ",
                     cur_m, cur_s, tot_m, tot_s);
        }
    }
}

// ak: Main Entry
//=============================================================================

internal void base_main(void)
{
    Str8_Array *arg = os_args_get();
    if (arg->length < 2)
    {
        fmt_println("Usage: play <audio_file>");
        fmt_println("\nControls:");
        fmt_println("  Space       Play / Pause");
        fmt_println("  Left/Right  Seek -5s / +5s");
        fmt_println("  Up/Down     Volume -10% / +10%");
        fmt_println("  r           Restart song");
        fmt_println("  ?           Show / Hide shortcuts");
        fmt_println("  q           Quit");
        return;
    }
    
    Str8 path = array_get(arg, 1);
    if (str8_match(path, str8("-h"), Str_Match_Flag_None) || str8_match(path, str8("--help"), Str_Match_Flag_None))
    {
        fmt_println("Usage: play <audio_file>");
        fmt_println("\nControls:");
        fmt_println("  Space       Play / Pause");
        fmt_println("  Left/Right  Seek -5s / +5s");
        fmt_println("  Up/Down     Volume -10% / +10%");
        fmt_println("  r           Restart song");
        fmt_println("  ?           Show / Hide shortcuts");
        fmt_println("  q           Quit");
        return;
    }
    
    if (!audio_init(48000, 2))
    {
        fmt_eprintln("Error: Failed to initialize audio device.");
        return;
    }
    
    Audio_Handle audio = audio_load_from_path(path, 0);
    if (!audio_handle_is_valid(audio))
    {
        fmt_eprintfln("Error: Failed to load audio file '%.*s'", str8_varg(path));
        audio_cleanup();
        return;
    }
    
    float volume = 1.0f;
    Audio_Play_Params params = audio_play_params_default();
    params.volume = volume;
    
    Audio_Handle voice = audio_play(audio, params);
    if (!audio_handle_is_valid(voice))
    {
        fmt_eprintfln("Error: Failed to start playback for '%.*s'", str8_varg(path));
        audio_unload(audio);
        audio_cleanup();
        return;
    }
    
    double duration = audio_voice_get_duration_seconds(voice);
    
    // Set up terminal raw mode & signals
    signal(SIGINT, term_sig_handler);
    signal(SIGTERM, term_sig_handler);
    signal(SIGHUP, term_sig_handler);
    term_raw_enable();
    
    bool running = true;
    bool show_shortcuts = false;
    uint64_t show_volume_until_us = 0;
    int prev_rendered_lines = 0;
    
    while (running)
    {
        uint64_t now_us = os_now_us();
        
        // ak: Handle Input
        App_Key key = term_read_key();
        switch (key)
        {
            case App_Key_Space:
            {
                if (audio_voice_is_alive(voice))
                {
                    audio_voice_toggle_pause(voice);
                }
                else
                {
                    // If finished, replay from start
                    voice = audio_play(audio, params);
                    audio_voice_set_volume(voice, volume);
                }
            }
            break;
            
            case App_Key_Right:
            {
                double cur = audio_voice_is_alive(voice)
                    ? audio_voice_get_position_seconds(voice)
                    : duration;
                double target = cur + 5.0;
                if (duration > 0.0 && target > duration)
                {
                    target = duration;
                }
                if (!audio_voice_is_alive(voice))
                {
                    voice = audio_play(audio, params);
                    audio_voice_set_volume(voice, volume);
                }
                audio_voice_seek_seconds(voice, target);
            }
            break;
            
            case App_Key_Left:
            {
                double cur = audio_voice_is_alive(voice)
                    ? audio_voice_get_position_seconds(voice)
                    : duration;
                double target = cur - 5.0;
                if (target < 0.0)
                {
                    target = 0.0;
                }
                if (!audio_voice_is_alive(voice))
                {
                    voice = audio_play(audio, params);
                    audio_voice_set_volume(voice, volume);
                }
                audio_voice_seek_seconds(voice, target);
            }
            break;
            
            case App_Key_Restart:
            {
                if (audio_voice_is_alive(voice))
                {
                    audio_voice_seek_seconds(voice, 0.0);
                    audio_voice_resume(voice);
                }
                else
                {
                    voice = audio_play(audio, params);
                    audio_voice_set_volume(voice, volume);
                }
            }
            break;
            
            case App_Key_Up:
            {
                volume += 0.10f;
                if (volume > 1.0f) { volume = 1.0f; }
                params.volume = volume;
                if (audio_voice_is_alive(voice))
                {
                    audio_voice_set_volume(voice, volume);
                }
                show_volume_until_us = now_us + 2000000; // 2 seconds
            }
            break;
            
            case App_Key_Down:
            {
                volume -= 0.10f;
                if (volume < 0.0f) { volume = 0.0f; }
                params.volume = volume;
                if (audio_voice_is_alive(voice))
                {
                    audio_voice_set_volume(voice, volume);
                }
                show_volume_until_us = now_us + 2000000; // 2 seconds
            }
            break;
            
            case App_Key_Help:
            {
                show_shortcuts = !show_shortcuts;
            }
            break;
            
            case App_Key_Quit:
            {
                running = false;
            }
            break;
            
            case App_Key_None:
            default:
                break;
        }
        
        if (!running)
        {
            break;
        }
        
        // ak: Playback State
        bool is_alive = audio_voice_is_alive(voice);
        bool is_paused = is_alive && audio_voice_is_paused(voice);
        
        if (duration <= 0.0 && is_alive)
        {
            duration = audio_voice_get_duration_seconds(voice);
        }
        
        double current_pos = 0.0;
        if (is_alive)
        {
            current_pos = audio_voice_get_position_seconds(voice);
        }
        else
        {
            current_pos = duration;
        }
        
        // ak: Build Display Lines
        #define MAX_UI_LINES 16
        char lines[MAX_UI_LINES][256];
        int line_count = 0;
        bool use_color = term_is_color_allowed();
        
        // Line 1: Song Progress
        char time_buf[64];
        make_time_string(time_buf, sizeof(time_buf), current_pos, duration);
        
        char progress_bar[128];
        float song_frac = (duration > 0.0) ? (float)(current_pos / duration) : 0.0f;
        make_progress_bar(progress_bar, sizeof(progress_bar), song_frac, PROGRESS_BAR_BLOCKS, "\x1B[36m");
        
        const char *status_suffix = "";
        if (is_paused)
        {
            status_suffix = use_color ? " \x1B[33m[PAUSED]\x1B[0m" : " [PAUSED]";
        }
        
        snprintf(lines[line_count++], sizeof(lines[0]), "%s%s%s", time_buf, progress_bar, status_suffix);
        
        // Volume line (when increasing/decreasing)
        if (now_us < show_volume_until_us)
        {
            int vol_percent = (int)(volume * 100.0f + 0.5f);
            char vol_prefix[64];
            if (use_color)
            {
                snprintf(vol_prefix, sizeof(vol_prefix), "\x1B[1;33mVolume %d%%\x1B[0m", vol_percent);
            }
            else
            {
                snprintf(vol_prefix, sizeof(vol_prefix), "Volume %d%%", vol_percent);
            }
            
            int time_vis_len = visible_len(time_buf);
            int vol_vis_len  = visible_len(vol_prefix);
            int pad = time_vis_len - vol_vis_len;
            if (pad < 1) { pad = 1; }
            
            char vol_bar[128];
            make_progress_bar(vol_bar, sizeof(vol_bar), volume, PROGRESS_BAR_BLOCKS, "\x1B[33m");
            
            snprintf(lines[line_count++], sizeof(lines[0]), "%s%*s%s", vol_prefix, pad, "", vol_bar);
        }
        
        // Shortcuts lines (when ? is pressed)
        if (show_shortcuts)
        {
            lines[line_count++][0] = '\0'; // empty spacer line
            if (use_color)
            {
                snprintf(lines[line_count++], sizeof(lines[0]), "\x1B[1;96mKey Shortcuts:\x1B[0m");
                snprintf(lines[line_count++], sizeof(lines[0]), "  \x1B[1;97mSpace\x1B[0m       \x1B[90mPlay / Pause\x1B[0m");
                snprintf(lines[line_count++], sizeof(lines[0]), "  \x1B[1;97mLeft/Right\x1B[0m  \x1B[90mSeek -5s / +5s\x1B[0m");
                snprintf(lines[line_count++], sizeof(lines[0]), "  \x1B[1;97mUp/Down\x1B[0m     \x1B[90mVolume -10% / +10%\x1B[0m");
                snprintf(lines[line_count++], sizeof(lines[0]), "  \x1B[1;97mr\x1B[0m           \x1B[90mRestart song\x1B[0m");
                snprintf(lines[line_count++], sizeof(lines[0]), "  \x1B[1;97m?\x1B[0m           \x1B[90mToggle shortcuts\x1B[0m");
                snprintf(lines[line_count++], sizeof(lines[0]), "  \x1B[1;97mq\x1B[0m           \x1B[90mQuit\x1B[0m");
            }
            else
            {
                snprintf(lines[line_count++], sizeof(lines[0]), "Key Shortcuts:");
                snprintf(lines[line_count++], sizeof(lines[0]), "  Space       Play / Pause");
                snprintf(lines[line_count++], sizeof(lines[0]), "  Left/Right  Seek -5s / +5s");
                snprintf(lines[line_count++], sizeof(lines[0]), "  Up/Down     Volume -10% / +10%");
                snprintf(lines[line_count++], sizeof(lines[0]), "  r           Restart song");
                snprintf(lines[line_count++], sizeof(lines[0]), "  ?           Toggle shortcuts");
                snprintf(lines[line_count++], sizeof(lines[0]), "  q           Quit");
            }
        }
        
        // ak: Render UI (Flicker-Free)
        if (prev_rendered_lines > 1)
        {
            printf("\x1B[%dA\r", prev_rendered_lines - 1);
        }
        else if (prev_rendered_lines == 1)
        {
            printf("\r");
        }
        
        for (int i = 0; i < line_count; i++)
        {
            if (i > 0)
            {
                printf("\n");
            }
            printf("\x1B[2K%s", lines[i]);
        }
        
        if (line_count < prev_rendered_lines)
        {
            printf("\x1B[J");
        }
        
        prev_rendered_lines = line_count;
        fflush(stdout);
        
        // ak: Exit when playback is finished
        if (!is_alive)
        {
            os_sleep_ms(150);
            running = false;
            break;
        }
        
        os_sleep_ms(30);
    }
    
    // Clean up terminal
    term_raw_disable();
    
    // Clean up audio
    audio_unload(audio);
    audio_cleanup();
}
