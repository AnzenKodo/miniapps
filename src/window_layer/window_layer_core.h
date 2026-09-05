// TODO(aman.v): Add better window error handling.

#ifndef WINDOW_LAYER_CORE_H
#define WINDOW_LAYER_CORE_H

// ak: Types
//=============================================================================

typedef struct Wl_Window Wl_Window;
struct Wl_Window
{
    uint64_t u64[1];
};

typedef enum Wl_Event_Kind
{
    Wl_Event_Kind_Null,
    Wl_Event_Kind_Press,
    Wl_Event_Kind_Release,
    Wl_Event_Kind_MouseMove,
    Wl_Event_Kind_Text,
    Wl_Event_Kind_Scroll,
    Wl_Event_Kind_WindowLoseFocus,
    Wl_Event_Kind_WindowClose,
    Wl_Event_Kind_WindowResize,
    Wl_Event_Kind_FileDrop,
    Wl_Event_Kind_Wakeup,
    Wl_Event_Kind_COUNT
}
Wl_Event_Kind;

typedef enum Wl_Modifiers
{
    Wl_Modifier_None   = 0,
    Wl_Modifier_Ctrl   = (1<<0),
    Wl_Modifier_Shift  = (1<<1),
    Wl_Modifier_Alt    = (1<<2),
    Wl_Modifier_Window = (1<<3),
}
Wl_Modifiers;

typedef enum Wl_Key
{
    Wl_Key_Null,
    Wl_Key_Esc,
    Wl_Key_F1,
    Wl_Key_F2,
    Wl_Key_F3,
    Wl_Key_F4,
    Wl_Key_F5,
    Wl_Key_F6,
    Wl_Key_F7,
    Wl_Key_F8,
    Wl_Key_F9,
    Wl_Key_F10,
    Wl_Key_F11,
    Wl_Key_F12,
    Wl_Key_F13,
    Wl_Key_F14,
    Wl_Key_F15,
    Wl_Key_F16,
    Wl_Key_F17,
    Wl_Key_F18,
    Wl_Key_F19,
    Wl_Key_F20,
    Wl_Key_F21,
    Wl_Key_F22,
    Wl_Key_F23,
    Wl_Key_F24,
    Wl_Key_Tick,
    Wl_Key_0,
    Wl_Key_1,
    Wl_Key_2,
    Wl_Key_3,
    Wl_Key_4,
    Wl_Key_5,
    Wl_Key_6,
    Wl_Key_7,
    Wl_Key_8,
    Wl_Key_9,
    Wl_Key_Minus,
    Wl_Key_Equal,
    Wl_Key_Backspace,
    Wl_Key_Tab,
    Wl_Key_Q,
    Wl_Key_W,
    Wl_Key_E,
    Wl_Key_R,
    Wl_Key_T,
    Wl_Key_Y,
    Wl_Key_U,
    Wl_Key_I,
    Wl_Key_O,
    Wl_Key_P,
    Wl_Key_LeftBracket,
    Wl_Key_RightBracket,
    Wl_Key_BackSlash,
    Wl_Key_CapsLock,
    Wl_Key_A,
    Wl_Key_S,
    Wl_Key_D,
    Wl_Key_F,
    Wl_Key_G,
    Wl_Key_H,
    Wl_Key_J,
    Wl_Key_K,
    Wl_Key_L,
    Wl_Key_Semicolon,
    Wl_Key_Quote,
    Wl_Key_Return,
    Wl_Key_Shift,
    Wl_Key_ShiftLeft,
    Wl_Key_ShiftRight,
    Wl_Key_Z,
    Wl_Key_X,
    Wl_Key_C,
    Wl_Key_V,
    Wl_Key_B,
    Wl_Key_N,
    Wl_Key_M,
    Wl_Key_Comma,
    Wl_Key_Period,
    Wl_Key_Slash,
    Wl_Key_Ctrl,
    Wl_Key_CtrlLeft,
    Wl_Key_CtrlRight,
    Wl_Key_Alt,
    Wl_Key_AltLeft,
    Wl_Key_AltRight,
    Wl_Key_Space,
    Wl_Key_Menu,
    Wl_Key_SuperLeft,
    Wl_Key_SuperRight,
    Wl_Key_Print,
    Wl_Key_ScrollLock,
    Wl_Key_Pause,
    Wl_Key_Insert,
    Wl_Key_Home,
    Wl_Key_PageUp,
    Wl_Key_Delete,
    Wl_Key_End,
    Wl_Key_PageDown,
    Wl_Key_Up,
    Wl_Key_Left,
    Wl_Key_Down,
    Wl_Key_Right,
    Wl_Key_Ex0,
    Wl_Key_Ex1,
    Wl_Key_Ex2,
    Wl_Key_Ex3,
    Wl_Key_Ex4,
    Wl_Key_Ex5,
    Wl_Key_Ex6,
    Wl_Key_Ex7,
    Wl_Key_Ex8,
    Wl_Key_Ex9,
    Wl_Key_Ex10,
    Wl_Key_Ex11,
    Wl_Key_Ex12,
    Wl_Key_Ex13,
    Wl_Key_Ex14,
    Wl_Key_Ex15,
    Wl_Key_Ex16,
    Wl_Key_Ex17,
    Wl_Key_Ex18,
    Wl_Key_Ex19,
    Wl_Key_Ex20,
    Wl_Key_Ex21,
    Wl_Key_Ex22,
    Wl_Key_Ex23,
    Wl_Key_Ex24,
    Wl_Key_Ex25,
    Wl_Key_Ex26,
    Wl_Key_Ex27,
    Wl_Key_Ex28,
    Wl_Key_Ex29,
    Wl_Key_NumLock,
    Wl_Key_NumSlash,
    Wl_Key_NumStar,
    Wl_Key_NumMinus,
    Wl_Key_NumPlus,
    Wl_Key_NumPeriod,
    Wl_Key_NumReturn,
    Wl_Key_Num0,
    Wl_Key_Num1,
    Wl_Key_Num2,
    Wl_Key_Num3,
    Wl_Key_Num4,
    Wl_Key_Num5,
    Wl_Key_Num6,
    Wl_Key_Num7,
    Wl_Key_Num8,
    Wl_Key_Num9,
    Wl_Key_LeftMouseButton,
    Wl_Key_MiddleMouseButton,
    Wl_Key_RightMouseButton,
    Wl_Key_COUNT,
}
Wl_Key;

typedef struct Wl_Event Wl_Event;
struct Wl_Event
{
    Wl_Event *next;
    Wl_Event *prev;
    uint64_t timestamp_us;
    Wl_Window window;
    Wl_Event_Kind kind;
    Wl_Modifiers modifiers;
    Wl_Key key;
    bool is_repeat;
    bool right_sided;
    uint32_t character;
    uint32_t repeat_count;
    Vec2_F32 pos;
    Vec2_F32 delta;
    Str8_List strings;
};

typedef struct Wl_Event_List Wl_Event_List;
struct Wl_Event_List
{
    size_t length;
    Wl_Event *first;
    Wl_Event *last;
};

typedef enum Wl_Cursor
{
    Wl_Cursor_Pointer,
    Wl_Cursor_IBar,
    Wl_Cursor_LeftRight,
    Wl_Cursor_UpDown,
    Wl_Cursor_DownRight,
    Wl_Cursor_UpRight,
    Wl_Cursor_UpDownLeftRight,
    Wl_Cursor_HandPoint,
    Wl_Cursor_Disabled,
    Wl_Cursor_COUNT,
} Wl_Cursor;

typedef struct _Wl_Core_State _Wl_Core_State;
struct _Wl_Core_State {
    bool   exit;
    Rng2_F32 display_rect;
};

// ak: Functions
//=============================================================================

// ak: Helper Functions =======================================================

internal Wl_Window wl_window_zero(void);
internal bool wl_window_match(Wl_Window a, Wl_Window b);

// ak: Basic window functions =================================================

internal void wl_init(void);
internal void wl_cleanup(void);
internal Wl_Window wl_window_open(Str8 title);
internal void wl_window_close(Wl_Window window);

// ak: Window close functions =================================================

internal void wl_exit(void);
internal bool wl_should_exit(void);

// ak: Event functions ========================================================

internal void wl_update_events(void);
internal Wl_Event_List wl_get_events(Arena *arena, bool wait);
internal void wl_set_fps(uint32_t fps);

// ak: Window property ========================================================

internal Rng2_F32 wl_rect_from_window(Wl_Window window);
internal Rng2_F32 wl_canvas_rect_from_window(Wl_Window window);
internal Rng2_F32 wl_display_rect(void);
internal void wl_window_pos_set(Wl_Window window, size_t x, size_t y);
internal void wl_window_icon_set_raw(Wl_Window window, void *icon_data, size_t width, size_t height);
internal void wl_window_border_set(Wl_Window window, bool enable);

// ak: Software render ========================================================

internal void wl_render_init(Wl_Window window, uint8_t *render_buffer);
internal void wl_render_deinit(void);
internal void wl_render_begin(Wl_Window window);
internal void wl_render_end(void);

// ak: Global variables
//=============================================================================

global _Wl_Core_State _wl_core_state = STRUCT_ZERO;

#endif // WINDOW_LAYER_CORE_H
