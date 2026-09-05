#ifndef BASE_TERM_H
#define BASE_TERM_H

//~ ak: Defines
//=============================================================================

//~ ak: ASNI Codes ============================================================

//- ak: Style
#define TERM_RESET             "\x1B[0m"
#define TERM_BOLD              "\x1B[1m"
#define TERM_FAINT             "\x1B[2m"
#define TERM_ITALIC            "\x1B[3m"
#define TERM_UNDERLINE         "\x1B[4m"
#define TERM_BLINK_SLOW        "\x1B[5m"
#define TERM_BLINK_FAST        "\x1B[6m"
#define TERM_REVERSE           "\x1B[7m"
#define TERM_CONCEAL           "\x1B[8m"
#define TERM_STRIKETHROUGH     "\x1B[9m"

//- ak: Foreground
#define TERM_FG_BLACK          "\x1B[30m"
#define TERM_FG_RED            "\x1B[31m"
#define TERM_FG_GREEN          "\x1B[32m"
#define TERM_FG_YELLOW         "\x1B[33m"
#define TERM_FG_BLUE           "\x1B[34m"
#define TERM_FG_MAGENTA        "\x1B[35m"
#define TERM_FG_CYAN           "\x1B[36m"
#define TERM_FG_WHITE          "\x1B[37m"
#define TERM_FG_DEFAULT        "\x1B[39m"

//- ak: Bright Foreground
#define TERM_FG_BRIGHT_BLACK   "\x1B[90m"
#define TERM_FG_BRIGHT_RED     "\x1B[91m"
#define TERM_FG_BRIGHT_GREEN   "\x1B[92m"
#define TERM_FG_BRIGHT_YELLOW  "\x1B[93m"
#define TERM_FG_BRIGHT_BLUE    "\x1B[94m"
#define TERM_FG_BRIGHT_MAGENTA "\x1B[95m"
#define TERM_FG_BRIGHT_CYAN    "\x1B[96m"
#define TERM_FG_BRIGHT_WHITE   "\x1B[97m"

//- ak: Background
#define TERM_BG_BLACK          "\x1B[40m"
#define TERM_BG_RED            "\x1B[41m"
#define TERM_BG_GREEN          "\x1B[42m"
#define TERM_BG_YELLOW         "\x1B[43m"
#define TERM_BG_BLUE           "\x1B[44m"
#define TERM_BG_MAGENTA        "\x1B[45m"
#define TERM_BG_CYAN           "\x1B[46m"
#define TERM_BG_WHITE          "\x1B[47m"
#define TERM_BG_DEFAULT        "\x1B[49m"

//- ak: Bright Background
#define TERM_BG_BRIGHT_BLACK   "\x1B[100m"
#define TERM_BG_BRIGHT_RED     "\x1B[101m"
#define TERM_BG_BRIGHT_GREEN   "\x1B[102m"
#define TERM_BG_BRIGHT_YELLOW  "\x1B[103m"
#define TERM_BG_BRIGHT_BLUE    "\x1B[104m"
#define TERM_BG_BRIGHT_MAGENTA "\x1B[105m"
#define TERM_BG_BRIGHT_CYAN    "\x1B[106m"
#define TERM_BG_BRIGHT_WHITE   "\x1B[107m"

//- ak: Cursor
#define TERM_CURSOR_HOME       "\x1B[H"
#define TERM_CURSOR_SAVE       "\x1B[s"
#define TERM_CURSOR_RESTORE    "\x1B[u"
#define TERM_CURSOR_HIDE       "\x1B[?25l"
#define TERM_CURSOR_SHOW       "\x1B[?25h"

//- ak: Screen
#define TERM_CLEAR_SCREEN      "\x1B[2J"
#define TERM_CLEAR_LINE        "\x1B[2K"
#define TERM_CLEAR_TO_END      "\x1B[0J"
#define TERM_CLEAR_TO_START    "\x1B[1J"

//~ ak: Macros
//=============================================================================

//~ ak: Handle Cursor positions ===============================================
#define TermCursorUp(n)      "\x1B[" #n "A"
#define TermCursorDown(n)    "\x1B[" #n "B"
#define TermCursorRight(n)   "\x1B[" #n "C"
#define TermCursorLeft(n)    "\x1B[" #n "D"
#define TermCursorPos(r,c)   "\x1B[" #r ";" #c "H"

//~ ak: Set terminal title ====================================================
#define TermSetTitle(title)  "\x1B]0;" title "\x07"

// Types
//=============================================================================

typedef struct _Term_State _Term_State;
struct _Term_State
{
    Os_File file;
};

// Functions
// ============================================================================

internal bool term_is_color_allowed(void);
internal void term_style_start(Os_File file, const char *style);
internal void term_style_end(void);
internal char *term_style_get(const char *style);

// Global Variables
//=============================================================================

_Term_State _term_state = STRUCT_ZERO;

#endif // BASE_TERM_H
