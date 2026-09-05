// ak: Helper functions
//=============================================================================

internal _Wl_X11_Window *_wl_x11_window_from_xwindow(xcb_window_t xwindow)
{
    _Wl_X11_Window *result = NULL;
    for (_Wl_X11_Window *window = _wl_x11_state->first_window;
        window != NULL;
        window = window->next)
    {
        if (window->xwindow == xwindow)
        {
            result = window;
            break;
        }
    }
    return result;
}

// ak: Basic window functions
//=============================================================================

internal void wl_init(void)
{
    // ak: initialize basics
    Arena *arena = arena_alloc();
    _wl_x11_state = arena_push(arena, _Wl_X11_State, 1);
    _wl_x11_state->arena = arena;
    
    // ak: setup connection
    _wl_x11_state->connection = xcb_connect(NULL, NULL);
    _wl_x11_state->screen = xcb_setup_roots_iterator(xcb_get_setup(_wl_x11_state->connection)).data;
    
    // ak: calculate atoms
    WlX11LoadAtom(_wl_x11_state, WM_PROTOCOLS);
    WlX11LoadAtom(_wl_x11_state, WM_DELETE_WINDOW);
    WlX11LoadAtom(_wl_x11_state, WM_SYNC_REQUEST_COUNTER);
    _wl_x11_state->wm_protocols = WM_PROTOCOLS;
    _wl_x11_state->wm_delete_window = WM_DELETE_WINDOW;
    _wl_x11_state->wm_sync_request_counter = WM_SYNC_REQUEST_COUNTER;
    
    // ak: allocate the symbols table
    _wl_x11_state->key_symbols = xcb_key_symbols_alloc(_wl_x11_state->connection);
    
    // ak: loading cursors
    {
        xcb_cursor_context_t *contex;
        xcb_cursor_context_new(_wl_x11_state->connection, _wl_x11_state->screen, &contex);
        struct
        {
            Wl_Cursor cursor;
            const char *name;
        }
        map[] =
        {
            { Wl_Cursor_Pointer,         "left_ptr" },
            { Wl_Cursor_IBar,            "xterm" },
            { Wl_Cursor_LeftRight,       "sb_h_double_arrow" },
            { Wl_Cursor_UpDown,          "sb_v_double_arrow" },
            { Wl_Cursor_DownRight,       "bottom_right_corner" },
            { Wl_Cursor_UpRight,         "top_right_corner" },
            { Wl_Cursor_UpDownLeftRight, "fleur" },
            { Wl_Cursor_HandPoint,       "hand2" }, // or "hand1" / "pointer"
            { Wl_Cursor_Disabled,        "X_cursor" },
        };
        for (size_t i = 0; i < ArrayLength(map); i++)
        {
            _wl_x11_state->cursors[map[i].cursor] = xcb_cursor_load_cursor(contex, map[i].name);
        }
        xcb_cursor_context_free(contex);
    }
    
    // ak: create wakeup event for polling
    _wl_x11_state->wakeup_fd = eventfd(0, EFD_CLOEXEC);
    Assert(_wl_x11_state->wakeup_fd > 0);
}

internal void wl_window_cleanup(void)
{
    xcb_disconnect(_wl_x11_state->connection);
    xcb_key_symbols_free(_wl_x11_state->key_symbols);
    for (int i = 0; i < Wl_Cursor_COUNT; i++)
    {
        if (_wl_x11_state->cursors[i] != 0)
        {
            xcb_free_cursor(_wl_x11_state->connection, _wl_x11_state->cursors[i]);
            _wl_x11_state->cursors[i] = 0;
        }
    }
}

internal Wl_Window wl_window_open(Str8 title)
{
    _Wl_X11_Window *window_os = _wl_x11_state->free_window;
    if (window_os)
    {
        SLLStackPop(_wl_x11_state->free_window);
    }
    else
    {
        window_os = arena_push_nz(_wl_x11_state->arena, _Wl_X11_Window, 1);
    }
    MemSetZeroStruct(window_os);
    DLLPushBack(_wl_x11_state->first_window, _wl_x11_state->last_window, window_os);
    
    window_os->xwindow = xcb_generate_id(_wl_x11_state->connection);
    
    // ak: create window
    int mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[] = {
        _wl_x11_state->screen->black_pixel,
		XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS |
		XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
		XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |
		XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY | XCB_EVENT_MASK_FOCUS_CHANGE |
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
    };
    uint16_t width = _wl_x11_state->screen->width_in_pixels  / 2;
    uint16_t height = _wl_x11_state->screen->height_in_pixels / 2;
    xcb_create_window(
        _wl_x11_state->connection, XCB_COPY_FROM_PARENT, window_os->xwindow, _wl_x11_state->screen->root,
        0, 0, width, height, 0,
        XCB_WINDOW_CLASS_INPUT_OUTPUT, _wl_x11_state->screen->root_visual, mask, value_list
    );
    
    // ak: set window title
    xcb_change_property(
        _wl_x11_state->connection,
        XCB_PROP_MODE_REPLACE, window_os->xwindow, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
        8, title.length, title.cstr
    );
    xcb_change_property(
        _wl_x11_state->connection,
        XCB_PROP_MODE_REPLACE, window_os->xwindow, XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING,
        8, title.length, title.cstr
    );
    // xcb_change_property(
    //     _wl_x11_state->connection, XCB_PROP_MODE_REPLACE, window->xwindow, XCB_ATOM_WM_CLASS,
    //     XCB_ATOM_STRING, 8, sizeof("title""\0""Title"), "title\0Title"
    // );
    
    // ak: handle close event
    xcb_change_property(
        _wl_x11_state->connection, XCB_PROP_MODE_REPLACE, window_os->xwindow, _wl_x11_state->wm_protocols,
        XCB_ATOM_ATOM, 32, 1, &_wl_x11_state->wm_delete_window
    );
    
    // ak: map window
    xcb_map_window(_wl_x11_state->connection, window_os->xwindow);
    xcb_flush(_wl_x11_state->connection);
    
    // ak: get display size
    _wl_core_state.display_rect = rng2p(
        0.f, 0.f,
        (float)_wl_x11_state->screen->width_in_pixels,
        (float)_wl_x11_state->screen->height_in_pixels);
    window_os->rect = rng2p(0.0f, 0.0f, (float)width, (float)height);
     
    // ak: convert to handle & return
    Wl_Window window = {(uint64_t)window_os};
    return window;
}

internal void wl_window_close(Wl_Window window)
{
    if(wl_window_match(window, wl_window_zero())){return;}
    _Wl_X11_Window *w = (_Wl_X11_Window *)window.u64[0];
    xcb_destroy_window(_wl_x11_state->connection, w->xwindow);
    xcb_flush(_wl_x11_state->connection);
}

// ak: Event Functions
//=============================================================================

internal Wl_Event_List wl_get_events(Arena *arena, bool wait)
{
    Wl_Event_List events = STRUCT_ZERO;
    for(;;)
    {
        // ak: 1. check if we have events waiting in the queue.
        xcb_generic_event_t *event = xcb_poll_for_event(_wl_x11_state->connection);
        
        // ak: 2. if no events are pending and we are waiting, sleep/poll
        if(event == NULL)
        {
            struct pollfd poll_fds[2] =
            {
                {
                    .fd = xcb_get_file_descriptor(_wl_x11_state->connection),
                    .events = POLLIN,
                    .revents = 0
                },
                {
                    .fd = _wl_x11_state->wakeup_fd,
                    .events = POLLIN,
                    .revents = 0
                },
            };

            int timeout = (wait && events.length == 0) ? -1 : 0;
            int poll_status = poll(poll_fds, ArrayLength(poll_fds), timeout);
            Assert(poll_status >= 0);
            // ak: if we got woke up via eventfd
            if(poll_fds[1].revents & POLLIN)
            {
                uint64_t dummy = 0;
                read(_wl_x11_state->wakeup_fd, &dummy, sizeof(dummy));
                wait = 0;
            }
            // ak: attempt to pull event again
            event = xcb_poll_for_event(_wl_x11_state->connection);
        }
        
        // ak: 3. process all queued events
        // (without overwriting previous events in the loop)
        while(event != NULL)
        {
            bool set_mouse_cursor = false;
            uint8_t response_type = event->response_type & ~0x80;
            switch(response_type)
            {
                // ak: key presses/releases
                case XCB_KEY_PRESS:
                case XCB_KEY_RELEASE:
                {
                    xcb_key_press_event_t *key_event = (xcb_key_press_event_t *)event;
                    // ak: map modifiers
                    Wl_Modifiers modifiers = Wl_Modifier_None;
                    if(key_event->state & XCB_MOD_MASK_SHIFT)
                    {
                        modifiers = (Wl_Modifiers)(modifiers | Wl_Modifier_Shift);
                    }
                    if(key_event->state & XCB_MOD_MASK_CONTROL)
                    {
                        modifiers = (Wl_Modifiers)(modifiers | Wl_Modifier_Ctrl);
                    }
                    if(key_event->state & XCB_MOD_MASK_1) {
                        modifiers = (Wl_Modifiers)(modifiers | Wl_Modifier_Alt);
                    }
                    // ak: map raw keycode to standard keysym using xcb-keysyms
                    xcb_keysym_t keysym = xcb_key_symbols_get_keysym(_wl_x11_state->key_symbols, key_event->detail, 0);
                    bool is_right_sided = false;
                    Wl_Key key = Wl_Key_Null;
                    switch(keysym)
                    {
                        default:
                        {
                            if(0){}
                            else if(XK_F1 <= keysym && keysym <= XK_F24)
                            {
                                key = (Wl_Key)(Wl_Key_F1 + (keysym - XK_F1));
                            }
                            else if('0' <= keysym && keysym <= '9')
                            {
                                key = (Wl_Key)(Wl_Key_0 + (keysym - '0'));
                            }
                        } break;
                        case XK_Escape:    { key = Wl_Key_Esc;          }; break;
                        case XK_BackSpace: { key = Wl_Key_Backspace;    } break;
                        case XK_Delete:    { key = Wl_Key_Delete;       } break;
                        case XK_Return:    { key = Wl_Key_Return;       } break;
                        case XK_Pause:     { key = Wl_Key_Pause;        } break;
                        case XK_Tab:       { key = Wl_Key_Tab;          } break;
                        case XK_Left:      { key = Wl_Key_Left;         } break;
                        case XK_Right:     { key = Wl_Key_Right;        } break;
                        case XK_Up:        { key = Wl_Key_Up;           } break;
                        case XK_Down:      { key = Wl_Key_Down;         } break;
                        case XK_Home:      { key = Wl_Key_Home;         } break;
                        case XK_End:       { key = Wl_Key_End;          } break;
                        case XK_Prior:     { key = Wl_Key_PageUp;       } break;
                        case XK_Next:      { key = Wl_Key_PageDown;     } break;
                        case XK_Alt_L:     { key = Wl_Key_Alt;          } break;
                        case XK_Alt_R:     { key = Wl_Key_Alt; is_right_sided = true; } break;
                        case XK_Shift_L:   { key = Wl_Key_Shift;        } break;
                        case XK_Shift_R:   { key = Wl_Key_Shift; is_right_sided = true; } break;
                        case XK_Control_L: { key = Wl_Key_Ctrl;         } break;
                        case XK_Control_R: { key = Wl_Key_Ctrl; is_right_sided = true; } break;
                        case '-':          { key = Wl_Key_Minus;        } break;
                        case '=':          { key = Wl_Key_Equal;        } break;
                        case '[':          { key = Wl_Key_LeftBracket;  } break;
                        case ']':          { key = Wl_Key_RightBracket; } break;
                        case ';':          { key = Wl_Key_Semicolon;    } break;
                        case '\'':         { key = Wl_Key_Quote;        } break;
                        case '.':          { key = Wl_Key_Period;       } break;
                        case ',':          { key = Wl_Key_Comma;        } break;
                        case '/':          { key = Wl_Key_Slash;        } break;
                        case '\\':         { key = Wl_Key_BackSlash;    } break;
                        case 'a':case 'A': { key = Wl_Key_A;            } break;
                        case 'b':case 'B': { key = Wl_Key_B;            } break;
                        case 'c':case 'C': { key = Wl_Key_C;            } break;
                        case 'd':case 'D': { key = Wl_Key_D;            } break;
                        case 'e':case 'E': { key = Wl_Key_E;            } break;
                        case 'f':case 'F': { key = Wl_Key_F;            } break;
                        case 'g':case 'G': { key = Wl_Key_G;            } break;
                        case 'h':case 'H': { key = Wl_Key_H;            } break;
                        case 'i':case 'I': { key = Wl_Key_I;            } break;
                        case 'j':case 'J': { key = Wl_Key_J;            } break;
                        case 'k':case 'K': { key = Wl_Key_K;            } break;
                        case 'l':case 'L': { key = Wl_Key_L;            } break;
                        case 'm':case 'M': { key = Wl_Key_M;            } break;
                        case 'n':case 'N': { key = Wl_Key_N;            } break;
                        case 'o':case 'O': { key = Wl_Key_O;            } break;
                        case 'p':case 'P': { key = Wl_Key_P;            } break;
                        case 'q':case 'Q': { key = Wl_Key_Q;            } break;
                        case 'r':case 'R': { key = Wl_Key_R;            } break;
                        case 's':case 'S': { key = Wl_Key_S;            } break;
                        case 't':case 'T': { key = Wl_Key_T;            } break;
                        case 'u':case 'U': { key = Wl_Key_U;            } break;
                        case 'v':case 'V': { key = Wl_Key_V;            } break;
                        case 'w':case 'W': { key = Wl_Key_W;            } break;
                        case 'x':case 'X': { key = Wl_Key_X;            } break;
                        case 'y':case 'Y': { key = Wl_Key_Y;            } break;
                        case 'z':case 'Z': { key = Wl_Key_Z;            } break;
                        case ' ':          { key = Wl_Key_Space;        } break;
                    }
                    _Wl_X11_Window *window = _wl_x11_window_from_xwindow(key_event->event);
                    // NOTE(ak): Native text layout translation (such as XIM)
                    // is omitted here as XCB does not natively support XIM.
                    // For Unicode input, Xlib-XCB or library integration
                    // (such as xkbcommon) is recommended.

                    // Push keypress event
                    Wl_Event *e = wl_event_list_push_new(arena, &events, (response_type == XCB_KEY_PRESS) ? Wl_Event_Kind_Press : Wl_Event_Kind_Release);
                    e->window.u64[0] = (uint64_t)window;
                    e->modifiers = modifiers;
                    e->key = key;
                    e->right_sided = is_right_sided;
                } break;
                
                // ak: Mouse button presses/releases
                case XCB_BUTTON_PRESS:
                case XCB_BUTTON_RELEASE:
                {
                    xcb_button_press_event_t *button_event = (xcb_button_press_event_t *)event;
                    // ak: Map modifiers
                    Wl_Modifiers modifiers = Wl_Modifier_None;
                    if(button_event->state & XCB_MOD_MASK_SHIFT)
                    {
                        modifiers = (Wl_Modifiers)(modifiers | Wl_Modifier_Shift);
                    }
                    if(button_event->state & XCB_MOD_MASK_CONTROL)
                    {
                        modifiers = (Wl_Modifiers)(modifiers | Wl_Modifier_Ctrl);
                    }
                    if(button_event->state & XCB_MOD_MASK_1)
                    {
                        modifiers = (Wl_Modifiers)(modifiers | Wl_Modifier_Alt);
                    }
                    Wl_Key key = Wl_Key_Null;
                    switch(button_event->detail)
                    {
                        default:{} break;
                        case 1: { key = Wl_Key_LeftMouseButton;} break;
                        case 2: { key = Wl_Key_MiddleMouseButton;} break;
                        case 3: { key = Wl_Key_RightMouseButton;} break;
                    }
                    _Wl_X11_Window *window = _wl_x11_window_from_xwindow(button_event->event);
                    if(key != Wl_Key_Null)
                    {
                        Wl_Event *e = wl_event_list_push_new(arena, &events, (response_type ==
                                    XCB_BUTTON_PRESS) ? Wl_Event_Kind_Press : Wl_Event_Kind_Release);
                        e->window.u64[0] = (uint64_t)window;
                        e->modifiers = modifiers;
                        e->key = key;
                        e->pos = (Vec2_F32){ (float)button_event->event_x, (float)button_event->event_y };
                    }
                    // ak: Scroll wheels (Up / Down)
                    else if(button_event->detail == 4 || button_event->detail == 5)
                    {
                        Wl_Event *e = wl_event_list_push_new(arena, &events, Wl_Event_Kind_Scroll);
                        e->window.u64[0] = (uint64_t)window;
                        e->modifiers = modifiers;
                        e->delta = (Vec2_F32){ 0.f, button_event->detail == 4 ? -1.f : +1.f };
                        e->pos = (Vec2_F32){ (float)button_event->event_x, (float)button_event->event_y };
                    }
                } break;
                
                // ak: mouse motion
                case XCB_MOTION_NOTIFY:
                {
                    xcb_motion_notify_event_t *motion_event = (xcb_motion_notify_event_t *)event;
                    _Wl_X11_Window *window = _wl_x11_window_from_xwindow(motion_event->event);

                    Wl_Event *e = wl_event_list_push_new(arena, &events, Wl_Event_Kind_MouseMove);
                    e->window.u64[0] = (uint64_t)window;
                    e->pos.x = (float)motion_event->event_x;
                    e->pos.y = (float)motion_event->event_y;
                    set_mouse_cursor = true;
                } break;
                
                // ak: focus Events
                case XCB_FOCUS_IN: break;
                case XCB_FOCUS_OUT:
                {
                    xcb_focus_out_event_t *focus_event = (xcb_focus_out_event_t *)event;
                    _Wl_X11_Window *window = _wl_x11_window_from_xwindow(focus_event->event);
                    Wl_Event *e = wl_event_list_push_new(arena, &events,
                            Wl_Event_Kind_WindowLoseFocus);
                    e->window.u64[0] = (uint64_t)window;
                } break;
                
                // ak: window resize and positioning
                case XCB_CONFIGURE_NOTIFY:
                {
                    xcb_configure_notify_event_t *conf_event = (xcb_configure_notify_event_t *)event;
                    _Wl_X11_Window *window = _wl_x11_window_from_xwindow(conf_event->event);
                    if (window != NULL)
                    {
                        // ak: update cached window coordinates
                        window->rect = rng2p(
                            (float)conf_event->x + (float)conf_event->border_width,
                            (float)conf_event->y + (float)conf_event->border_width,
                            (float)conf_event->x + (float)conf_event->width,
                            (float)conf_event->y + (float)conf_event->height
                        );
                        Wl_Event *e = wl_event_list_push_new(arena, &events, Wl_Event_Kind_WindowResize);
                        e->window.u64[0] = (uint64_t)window;
                    }
                } break;
                
                // ak: client Messages (window closes & sync counters)
                case XCB_CLIENT_MESSAGE:
                {
                    xcb_client_message_event_t *msg_event = (xcb_client_message_event_t *)event;
                    _Wl_X11_Window *window = _wl_x11_window_from_xwindow(msg_event->window);

                    if(msg_event->data.data32[0] == _wl_x11_state->wm_delete_window)
                    {
                        Wl_Event *e = wl_event_list_push_new(arena, &events, Wl_Event_Kind_WindowClose);
                        e->window.u64[0] = (uint64_t)window;
                    }
                    else if(msg_event->data.data32[0] == _wl_x11_state->wm_sync_request_counter)
                    {
                        if(window != NULL)
                        {
                            // ak: update and acknowledge the window resize sync
                            // counter to avoid tearing
                            window->counter_value = 0;
                            window->counter_value |= (uint64_t)msg_event->data.data32[2];
                            window->counter_value |= ((uint64_t)msg_event->data.data32[3] << 32);
                            
                            xcb_sync_int64_t value;
                            value.lo = (uint32_t)(window->counter_value & 0xFFFFFFFF);
                            value.hi = (int32_t)(window->counter_value >> 32);

                            xcb_sync_set_counter(_wl_x11_state->connection, window->counter_xid, value);
                            xcb_flush(_wl_x11_state->connection);
                        }
                    }
                } break;
            }
            
            // ak: cursor logic
            if(set_mouse_cursor)
            {
                xcb_query_pointer_cookie_t cookie = xcb_query_pointer(_wl_x11_state->connection, _wl_x11_state->screen->root);
                xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(_wl_x11_state->connection, cookie, NULL);
                if(reply)
                {
                    xcb_change_window_attributes(_wl_x11_state->connection,
                        _wl_x11_state->screen->root,
                        XCB_CW_CURSOR,
                        &_wl_x11_state->cursors[_wl_x11_state->last_set_cursor]);
                    xcb_flush(_wl_x11_state->connection);
                    free(reply);
                }
            }
            
            free(event);
            event = xcb_poll_for_event(_wl_x11_state->connection);
        }

        // ak: Stop querying if we have events, or if we were running in non-blocking mode.
        if(events.length > 0 || (wait == 0 && events.length == 0))
        {
            break;
        }
    }

    return events;
}

// ak: Window property
//=============================================================================

internal Rng2_F32 wl_rect_from_window(Wl_Window window)
{
    if (wl_window_match(window, wl_window_zero()))
    {
        return (Rng2_F32){0, 0, 0, 0};
    }
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    return window_os->rect;
}

internal Rng2_F32 wl_canvas_rect_from_window(Wl_Window window)
{
    Rng2_F32 rect = wl_rect_from_window(window);
    Rng2_F32 canvas_rect = rng2p(0.f, 0.f, rect.x1-rect.x0, rect.y1-rect.y0);
    return canvas_rect;
}

internal void wl_window_pos_set(Wl_Window window, size_t x, size_t y)
{
    if (wl_window_match(window, wl_window_zero())){return;}
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    
    xcb_configure_window(_wl_x11_state->connection, window_os->xwindow,
        XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y,
        (int[2]){ (int)x, (int)y });
    xcb_flush(_wl_x11_state->connection);
}

internal void wl_window_icon_set_raw(Wl_Window window, void *icon_data, size_t width, size_t height)
{
    if (wl_window_match(window, wl_window_zero())){return;}
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    
    WlX11LoadAtom(_wl_x11_state, _NET_WM_ICON);
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    size_t data_size = 2 + width * height;
    size_t *data = arena_push(scratch.arena, size_t, data_size);
    data[0] = width;
    data[1] = height;
    mem_copy(data + 2, icon_data, width * height * sizeof(size_t));
    xcb_change_property(
        _wl_x11_state->connection, XCB_PROP_MODE_REPLACE, window_os->xwindow,
        _NET_WM_ICON, XCB_ATOM_CARDINAL, 32,
        data_size, data
    );
    arena_scratch_end(scratch);
}

internal void wl_window_border_set(Wl_Window window, bool enable)
{
    if (wl_window_match(window, wl_window_zero())){return;}
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    
    WlX11LoadAtom(_wl_x11_state, _MOTIF_WM_HINTS);
	struct {
        uint32_t flags;
        uint32_t functions;
        uint32_t decorations;
        int32_t  input_mode;
        uint32_t status;
    } hints = STRUCT_ZERO;
	hints.flags = 2;
	hints.decorations = enable;
    xcb_change_property(_wl_x11_state->connection, XCB_PROP_MODE_REPLACE, window_os->xwindow, _MOTIF_WM_HINTS, _MOTIF_WM_HINTS, 32, 5, &hints);
    xcb_flush(_wl_x11_state->connection);
}

// ak: software render
//=============================================================================

internal void wl_render_init(Wl_Window window, uint8_t *buffer)
{
    if (wl_window_match(window, wl_window_zero())){return;}
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    window_os->pixels_buffer = buffer;
    Rng2_F32 display_rect = wl_display_rect();
    
    // ak: create pixmap format
    if (!_wl_x11_state->pixmap)
    {
        _wl_x11_state->pixmap = xcb_generate_id(_wl_x11_state->connection);
        _wl_x11_state->pixmap_format = xcb_setup_pixmap_formats(
            xcb_get_setup(_wl_x11_state->connection)
        );
        int32_t pixmap_format_length = xcb_setup_pixmap_formats_length(
            xcb_get_setup(_wl_x11_state->connection)
        );
        for (int i = 0; i < pixmap_format_length; i++)
        {
            if (
                    _wl_x11_state->pixmap_format[i].depth==24 && _wl_x11_state->pixmap_format[i].bits_per_pixel==32
               )
            {
                _wl_x11_state->pixmap_format += i;
            }
        }
    }
    
    // ak: connect pixmap to window
    xcb_create_pixmap(
        _wl_x11_state->connection, _wl_x11_state->screen->root_depth,
        _wl_x11_state->pixmap, window_os->xwindow,
        display_rect.x1, display_rect.y1
    );
    
    // ak: create graphics context
    if (!_wl_x11_state->gc)
    {
        _wl_x11_state->gc = xcb_generate_id(_wl_x11_state->connection);
        uint32_t gc_mask = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND;
        uint32_t gc_values[] = {
            _wl_x11_state->screen->white_pixel, _wl_x11_state->screen->black_pixel
        };
        xcb_create_gc(
            _wl_x11_state->connection, _wl_x11_state->gc, _wl_x11_state->pixmap, gc_mask, gc_values
        );
    }
    
    // ak: create image
    window_os->image = xcb_image_create(
        display_rect.x1, display_rect.y1,
        XCB_IMAGE_FORMAT_Z_PIXMAP,
        _wl_x11_state->pixmap_format->scanline_pad,
        _wl_x11_state->pixmap_format->depth,
        _wl_x11_state->pixmap_format->bits_per_pixel, 0,
        (xcb_image_order_t)xcb_get_setup(_wl_x11_state->connection)->image_byte_order,
        XCB_IMAGE_ORDER_LSB_FIRST,
        window_os->pixels_buffer,
        // NOTE(ak):
        // Here was `sizeof(*window_os->pixels_buffer)` but changed to 1,
        // because in C, GCC has a non-standard extension where `sizeof(void) == 1`.
        1,
        window_os->pixels_buffer
    );
    xcb_flush(_wl_x11_state->connection);
}

internal void wl_render_deinit(void)
{
    xcb_free_pixmap(_wl_x11_state->connection, _wl_x11_state->pixmap);
    xcb_free_gc(_wl_x11_state->connection, _wl_x11_state->gc);
}

internal void wl_render_begin(Wl_Window window)
{
    if (wl_window_match(window, wl_window_zero())){return;}
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    
    xcb_image_put(
        _wl_x11_state->connection, _wl_x11_state->pixmap,
        _wl_x11_state->gc, window_os->image, 0, 0, 0
    );
    Rng2_F32 display_rect = wl_display_rect();
    xcb_copy_area(
        _wl_x11_state->connection, _wl_x11_state->pixmap, window_os->xwindow,
        _wl_x11_state->gc, 0, 0, 0, 0,
        display_rect.x1, display_rect.y1
    );
}

internal void wl_render_end(void)
{
}
