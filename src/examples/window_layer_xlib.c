#include "window_layer_linux2.h"

// Basic Window functions
//=============================================================================

internal void wl_window_open(Str8 title, U32 win_width, U32 win_height)
{
    Display *display = XOpenDisplay(NULL);

    // Visula Info ============================================================
    XVisualInfo visual_info = STRUCT_ZERO;
    render_set_wl_linux_visual_info(&visual_info, display);

    // Window Attributes ======================================================
    XSetWindowAttributes window_attributes;
    window_attributes.colormap = XCreateColormap(
        display,
        RootWindow(display, visual_info.screen),
        visual_info.visual, AllocNone
    );
	window_attributes.event_mask =
	    ExposureMask|
        ButtonPressMask|
        ButtonReleaseMask|
	    PointerMotionMask|
        KeyPressMask|
        KeyReleaseMask|
        FocusChangeMask|
        StructureNotifyMask;

    // Window =================================================================
    Window window = window = XCreateWindow(
        display,
        RootWindow(display, visual_info.screen),
        0, 0, win_width, win_height, 0,
        visual_info.depth, InputOutput, visual_info.visual,
        CWColormap|CWEventMask, &window_attributes
    );
    XMapWindow(display, window);

    // Window Properties ======================================================
    Atom wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", false);
    Atom wm_sync_request = XInternAtom(display, "_NET_WM_SYNC_REQUEST", false);
    Atom protocols[] = { wm_delete_window, wm_sync_request };
    XSetWMProtocols(display, window, protocols, ArrayCount(protocols));

    // Window Name ============================================================
    XStoreName(display, window, cast(const char *)title.str);

    // Window Layer State =====================================================
    wl_linux_state.display = display;
    wl_linux_state.window = window;
    wl_linux_state.wm_delete_window = wm_delete_window;
    wl_linux_state.wm_sync_request = wm_sync_request;
}

internal void wl_window_close(void)
{
    XDestroyWindow(wl_linux_state.display, wl_linux_state.window);
    XCloseDisplay(wl_linux_state.display);
}

// Window Close Functions
//=============================================================================

internal void wl_set_window_close(void)
{
    wl_state.win_should_close = true;
}

internal bool wl_should_window_close(void)
{
    return wl_state.win_should_close;
}

// Event Functions
//=============================================================================

internal void wl_update_events(void)
{
    wl_state.event = wl_get_event();
    if (wl_state.event.type == Wl_EventType_WindowClose) {
        wl_set_window_close();
    }
}

internal Wl_Event wl_get_event(void)
{
    Wl_Event event = STRUCT_ZERO;
    Display *display = wl_linux_state.display;

    while (XPending(display) > 0) {
        XEvent xevent = STRUCT_ZERO;
        XNextEvent(display, &xevent);

        switch(xevent.type)
        {
            default: {}break;
            case KeyPress:
            case KeyRelease:
            {
                // Determine mod_key
                Wl_ModKey mod_key = cast(Wl_ModKey)0;
                if(xevent.xkey.state & ShiftMask)   {
                    mod_key = cast(Wl_ModKey)(mod_key | Wl_ModKey_Shift);
                }
                if(xevent.xkey.state & ControlMask) {
                    mod_key = cast(Wl_ModKey)(mod_key | Wl_ModKey_Ctrl);
                }
                if(xevent.xkey.state & Mod1Mask) {
                    mod_key = cast(Wl_ModKey)(mod_key | Wl_ModKey_Alt);
                }

                // Map Keycode -> keysym
                U32 keysym = XLookupKeysym(&xevent.xkey, 0);

                // Map keysym -> Wl_Key
                Wl_Key key = Wl_Key_Null;
                switch(keysym)
                {
                    default:
                    {
                        if(XK_F1 <= keysym && keysym <= XK_F24) {
                            key = (Wl_Key)(Wl_Key_F1 + (keysym - XK_F1));
                        }
                        else if('0' <= keysym && keysym <= '9') {
                            key = (Wl_Key)(Wl_Key_0 + (keysym-'0'));
                        }
                        break;
                    }

                    case XK_Escape:{key = Wl_Key_Esc;};break;
                    case '-':{key = Wl_Key_Minus;}break;
                    case '=':{key = Wl_Key_Equal;}break;
                    case '[':{key = Wl_Key_LeftBracket;}break;
                    case ']':{key = Wl_Key_RightBracket;}break;
                    case ';':{key = Wl_Key_Semicolon;}break;
                    case '\'':{key = Wl_Key_Quote;}break;
                    case '.':{key = Wl_Key_Period;}break;
                    case ',':{key = Wl_Key_Comma;}break;
                    case '/':{key = Wl_Key_Slash;}break;
                    case '\\':{key = Wl_Key_BackSlash;}break;
                    case 'a':case 'A':{key = Wl_Key_A;}break;
                    case 'b':case 'B':{key = Wl_Key_B;}break;
                    case 'c':case 'C':{key = Wl_Key_C;}break;
                    case 'd':case 'D':{key = Wl_Key_D;}break;
                    case 'e':case 'E':{key = Wl_Key_E;}break;
                    case 'f':case 'F':{key = Wl_Key_F;}break;
                    case 'g':case 'G':{key = Wl_Key_G;}break;
                    case 'h':case 'H':{key = Wl_Key_H;}break;
                    case 'i':case 'I':{key = Wl_Key_I;}break;
                    case 'j':case 'J':{key = Wl_Key_J;}break;
                    case 'k':case 'K':{key = Wl_Key_K;}break;
                    case 'l':case 'L':{key = Wl_Key_L;}break;
                    case 'm':case 'M':{key = Wl_Key_M;}break;
                    case 'n':case 'N':{key = Wl_Key_N;}break;
                    case 'o':case 'O':{key = Wl_Key_O;}break;
                    case 'p':case 'P':{key = Wl_Key_P;}break;
                    case 'q':case 'Q':{key = Wl_Key_Q;}break;
                    case 'r':case 'R':{key = Wl_Key_R;}break;
                    case 's':case 'S':{key = Wl_Key_S;}break;
                    case 't':case 'T':{key = Wl_Key_T;}break;
                    case 'u':case 'U':{key = Wl_Key_U;}break;
                    case 'v':case 'V':{key = Wl_Key_V;}break;
                    case 'w':case 'W':{key = Wl_Key_W;}break;
                    case 'x':case 'X':{key = Wl_Key_X;}break;
                    case 'y':case 'Y':{key = Wl_Key_Y;}break;
                    case 'z':case 'Z':{key = Wl_Key_Z;}break;
                    case ' ':{key = Wl_Key_Space;}break;
                }

                if (xevent.type == KeyPress) {
                    event.type = Wl_EventType_Press;
                } else {
                    event.type = Wl_EventType_Release;
                }

                event.mod_key = mod_key;
                event.key = key;
            }break;

            // Mouse button presses/releases ==================================
            case ButtonPress:
            case ButtonRelease:
            {
                // Determine mod_key
                Wl_ModKey mod_key = cast(Wl_ModKey)0;
                if(xevent.xkey.state & ShiftMask)   {
                    mod_key = cast(Wl_ModKey)(mod_key | Wl_ModKey_Shift);
                }
                if(xevent.xkey.state & ControlMask) {
                    mod_key = cast(Wl_ModKey)(mod_key | Wl_ModKey_Ctrl);
                }
                if(xevent.xkey.state & Mod1Mask) {
                    mod_key = cast(Wl_ModKey)(mod_key | Wl_ModKey_Alt);
                }

                // ak: map button -> Wl_Key
                Wl_Key key = Wl_Key_Null;
                switch(xevent.xbutton.button)
                {
                    default:{}break;
                    case Button1:{key = Wl_Key_LeftMouseButton;}break;
                    case Button2:{key = Wl_Key_MiddleMouseButton;}break;
                    case Button3:{key = Wl_Key_RightMouseButton;}break;
                }

                if (xevent.type == ButtonPress) {
                    event.type = Wl_EventType_Press;
                } else {
                    event.type = Wl_EventType_Release;
                }
                event.mod_key = mod_key;
                event.key = key;
            }break;

            // Mouse Motion ===================================================
            case MotionNotify:
            {
                event.type = Wl_EventType_MouseMove;
                event.pos.x = (F32)xevent.xmotion.x;
                event.pos.y = (F32)xevent.xmotion.y;
            }break;

            // Window focus/unfocus ===========================================
            case FocusIn: break;
            case FocusOut:
            {
                event.type = Wl_EventType_WindowLoseFocus;
            }break;

            // Client messages ================================================
            case ClientMessage:
            {
                if((Atom)xevent.xclient.data.l[0] == wl_linux_state.wm_delete_window)
                {
                    event.type = Wl_EventType_WindowClose;
                }
                else if((Atom)xevent.xclient.data.l[0] == wl_linux_state.wm_sync_request)
                {
                }
            }break;

            // Window Resize ==================================================
            case ConfigureNotify:
            {
                wl_state.win_size.x = xevent.xconfigure.width;
                wl_state.win_size.y = xevent.xconfigure.height;
                event.type = Wl_EventType_WindowResize;
            }break;
        } // Switch
    } // While

    return event;
}

internal bool wl_is_key_pressed(Wl_Key key)
{
    return wl_state.event.key == key;
}

// Get Window Information
//=============================================================================

internal U32 wl_get_display_width(void)
{
    return DisplayWidth(wl_linux_state.display, XDefaultScreen(wl_linux_state.display));
}

internal U32 wl_get_display_height(void)
{
    return DisplayHeight(wl_linux_state.display, XDefaultScreen(wl_linux_state.display));
}

internal U32 wl_get_window_width(void)
{
   return (U32)wl_state.win_size.x;
}

internal U32 wl_get_window_height(void)
{
    return (U32)wl_state.win_size.y;
}
