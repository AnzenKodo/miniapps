#ifndef WINDOW_LAYER_XCB_H
#define WINDOW_LAYER_XCB_H

// ak: External Includes
//=============================================================================

#pragma push_macro("read_only")
#undef read_only
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <xcb/xcb_image.h>
#include <xcb/xcb_cursor.h>
#include <xcb/sync.h>
#include <poll.h>
#include <sys/eventfd.h>
#pragma pop_macro("read_only")
// ak: Types
//=============================================================================

typedef struct _Wl_X11_Window _Wl_X11_Window;
struct _Wl_X11_Window
{
    _Wl_X11_Window *next;
    _Wl_X11_Window *prev;
    xcb_window_t xwindow;
    Rng2_F32 rect;
    uint64_t counter_value;
    xcb_sync_counter_t counter_xid;
    xcb_image_t *image;
    uint8_t *pixels_buffer;
};

typedef struct _Wl_X11_State _Wl_X11_State;
struct _Wl_X11_State
{
    _Wl_X11_Window *free_window;
    _Wl_X11_Window *first_window;
    _Wl_X11_Window *last_window;
    Arena *arena;
    
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_key_symbols_t *key_symbols;
    int wakeup_fd;
    xcb_atom_t wm_delete_window;
    xcb_atom_t wm_protocols;
    xcb_atom_t wm_sync_request_counter;
    Wl_Cursor last_set_cursor;
    xcb_cursor_t cursors[Wl_Cursor_COUNT];
    
    // ak: software render
    xcb_pixmap_t pixmap;
    xcb_format_t *pixmap_format;
    xcb_gcontext_t gc;
};

// ak: Macros
//=============================================================================

#define WlX11LoadAtom(state, name) \
    xcb_atom_t name = XCB_ATOM_NONE; \
    if (name == XCB_ATOM_NONE) { \
        Str8 atom_name = str8(#name); \
        xcb_intern_atom_cookie_t cookie = xcb_intern_atom(state->connection, 0, atom_name.length, (char *)atom_name.cstr); \
        xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(state->connection, cookie, NULL); \
        if (reply) { \
            name = reply->atom; \
            free(reply); \
        } else { \
            name = XCB_ATOM_NONE; \
            LogErrorLine(&_os_core_state.log_context, "failed to load atom '"#name"'"); \
        } \
    }

// ak: Defines
//=============================================================================

#define XK_F1                            0xffbe
#define XK_F24                           0xffd5
#define XK_Escape                        0xff1b  /* U+001B ESCAPE */
#define XK_BackSpace                     0xff08  /* U+0008 BACKSPACE */
#define XK_Delete                        0xffff  /* U+007F DELETE */
#define XK_Return                        0xff0d  /* U+000D CARRIAGE RETURN */
#define XK_Pause                         0xff13  /* Pause, hold */
#define XK_Tab                           0xff09  /* U+0009 CHARACTER TABULATION */
#define XK_Left                          0xff51  /* Move left, left arrow */
#define XK_Right                         0xff53  /* Move right, right arrow */
#define XK_Up                            0xff52  /* Move up, up arrow */
#define XK_Down                          0xff54  /* Move down, down arrow */
#define XK_Home                          0xff50
#define XK_End                           0xff57  /* EOL */
#define XK_Prior                         0xff55  /* Prior, previous */
#define XK_Next                          0xff56  /* Next */
#define XK_Alt_L                         0xffe9  /* Left alt */
#define XK_Alt_R                         0xffea  /* Right alt */
#define XK_Shift_L                       0xffe1  /* Left shift */
#define XK_Shift_R                       0xffe2  /* Right shift */
#define XK_Control_L                     0xffe3  /* Left control */
#define XK_Control_R                     0xffe4  /* Right control */

// ak: Helper functions
//=============================================================================

internal _Wl_X11_Window *_wl_x11_window_from_xwindow(xcb_window_t xwindow);

// ak: Global Variables
//=============================================================================

global _Wl_X11_State *_wl_x11_state = NULL;

#endif // WINDOW_LAYER_XCB_H
