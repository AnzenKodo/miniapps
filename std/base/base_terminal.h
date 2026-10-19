#ifndef BASE_TERM_H
#define BASE_TERM_H

// ak: Defines
//=============================================================================

// ak: ASNI Codes ============================================================

//- ak: Style
#define TERM_RESET             str8("\x1B[0m")
#define TERM_BOLD              str8("\x1B[1m")
#define TERM_FAINT             str8("\x1B[2m")
#define TERM_ITALIC            str8("\x1B[3m")
#define TERM_UNDERLINE         str8("\x1B[4m")
#define TERM_BLINK_SLOW        str8("\x1B[5m")
#define TERM_BLINK_FAST        str8("\x1B[6m")
#define TERM_REVERSE           str8("\x1B[7m")
#define TERM_CONCEAL           str8("\x1B[8m")
#define TERM_STRIKETHROUGH     str8("\x1B[9m")

//- ak: Foreground
#define TERM_FG_BLACK           str8("\x1B[30m")
#define TERM_FG_RED             str8("\x1B[31m")
#define TERM_FG_GREEN           str8("\x1B[32m")
#define TERM_FG_YELLOW          str8("\x1B[33m")
#define TERM_FG_BLUE            str8("\x1B[34m")
#define TERM_FG_MAGENTA         str8("\x1B[35m")
#define TERM_FG_CYAN            str8("\x1B[36m")
#define TERM_FG_WHITE           str8("\x1B[37m")
#define TERM_FG_DEFAULT         str8("\x1B[39m")

//- ak: Bright Foreground
#define TERM_FG_BRIGHT_BLACK    str8("\x1B[90m")
#define TERM_FG_BRIGHT_RED      str8("\x1B[91m")
#define TERM_FG_BRIGHT_GREEN    str8("\x1B[92m")
#define TERM_FG_BRIGHT_YELLOW   str8("\x1B[93m")
#define TERM_FG_BRIGHT_BLUE     str8("\x1B[94m")
#define TERM_FG_BRIGHT_MAGENTA  str8("\x1B[95m")
#define TERM_FG_BRIGHT_CYAN     str8("\x1B[96m")
#define TERM_FG_BRIGHT_WHITE    str8("\x1B[97m")

//- ak: Background
#define TERM_BG_BLACK           str8("\x1B[40m")
#define TERM_BG_RED             str8("\x1B[41m")
#define TERM_BG_GREEN           str8("\x1B[42m")
#define TERM_BG_YELLOW          str8("\x1B[43m")
#define TERM_BG_BLUE            str8("\x1B[44m")
#define TERM_BG_MAGENTA         str8("\x1B[45m")
#define TERM_BG_CYAN            str8("\x1B[46m")
#define TERM_BG_WHITE           str8("\x1B[47m")
#define TERM_BG_DEFAULT         str8("\x1B[49m")

//- ak: Bright Background
#define TERM_BG_BRIGHT_BLACK    str8("\x1B[100m")
#define TERM_BG_BRIGHT_RED      str8("\x1B[101m")
#define TERM_BG_BRIGHT_GREEN    str8("\x1B[102m")
#define TERM_BG_BRIGHT_YELLOW   str8("\x1B[103m")
#define TERM_BG_BRIGHT_BLUE     str8("\x1B[104m")
#define TERM_BG_BRIGHT_MAGENTA  str8("\x1B[105m")
#define TERM_BG_BRIGHT_CYAN     str8("\x1B[106m")
#define TERM_BG_BRIGHT_WHITE    str8("\x1B[107m")

//- ak: Cursor
#define TERM_CURSOR_HOME        str8("\x1B[H")
#define TERM_CURSOR_SAVE        str8("\x1B[s")
#define TERM_CURSOR_RESTORE     str8("\x1B[u")
#define TERM_CURSOR_HIDE        str8("\x1B[?25l")
#define TERM_CURSOR_SHOW        str8("\x1B[?25h")

//- ak: Screen
#define TERM_CLEAR_SCREEN       str8("\x1B[2J")
#define TERM_CLEAR_LINE         str8("\x1B[2K")
#define TERM_CLEAR_TO_END       str8("\x1B[0J")
#define TERM_CLEAR_TO_START     str8("\x1B[1J")

// ak: Macros
//=============================================================================

// ak: Handle Cursor positions ===============================================
#define TermCursorUp(n)      "\x1B[" #n "A"
#define TermCursorDown(n)    "\x1B[" #n "B"
#define TermCursorRight(n)   "\x1B[" #n "C"
#define TermCursorLeft(n)    "\x1B[" #n "D"
#define TermCursorPos(r,c)   "\x1B[" #r ";" #c "H"

// ak: Set terminal title ====================================================
#define TermSetTitle(title)  "\x1B]0;" title "\x07"

// ak: Types
//=============================================================================

typedef struct _Term_State _Term_State;
struct _Term_State
{
    Fs_File file;
};

// ak: Functions
//=============================================================================

internal Str8_Array *term_args_get(void);
internal bool term_is_terminal(Fs_File file);

// ak: Terminal Style =========================================================

internal bool term_is_color_allowed(void);
internal void term_style_start(Fs_File file, Str8 style);
internal void term_style_end(void);
internal Str8 term_style_get(Str8 style);

// Global Variables
//=============================================================================

_Term_State _term_state = STRUCT_ZERO;

#endif // BASE_TERM_H
