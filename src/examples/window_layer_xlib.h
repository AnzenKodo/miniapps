#ifndef WINDOW_LAYER_LINUX2_H
#define WINDOW_LAYER_LINUX2_H

// X11 Includes
//=============================================================================
#include <X11/Xlib.h>
#include <X11/Xutil.h>

// Types
//=============================================================================
typedef struct Wl_Linux_State Wl_Linux_State;
struct Wl_Linux_State {
    Display *display;
    Window window;
    Atom wm_delete_window;
    Atom wm_sync_request;
};

// Global Variables
//=============================================================================
global Wl_Linux_State wl_linux_state = STRUCT_ZERO;

// Function Declarations
//=============================================================================
internal void wl_window_open(Str8 title, U32 win_width, U32 win_height);
internal void wl_window_close(void);
internal void wl_set_window_close(void);
internal bool wl_should_window_close(void);
internal void wl_update_events(void);
internal Wl_Event wl_get_event(void);
internal bool wl_is_key_pressed(Wl_Key key);
internal U32 wl_get_display_width(void);
internal U32 wl_get_display_height(void);
internal U32 wl_get_window_width(void);
internal U32 wl_get_window_height(void);

#endif // WINDOW_LAYER_LINUX2_H
