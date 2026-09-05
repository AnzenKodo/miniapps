// ak: Helper functions
//=============================================================================

internal Wl_Key _os_win32_os_key_from_vkey(WPARAM vkey)
{
    local_persist int32_t first = 1;
    local_persist Wl_Key key_table[256];
    if (first)
    {
        first = 0;
        MemSetZeroArray(key_table);
        key_table[(unsigned int)'A'] = Wl_Key_A;
        key_table[(unsigned int)'B'] = Wl_Key_B;
        key_table[(unsigned int)'C'] = Wl_Key_C;
        key_table[(unsigned int)'D'] = Wl_Key_D;
        key_table[(unsigned int)'E'] = Wl_Key_E;
        key_table[(unsigned int)'F'] = Wl_Key_F;
        key_table[(unsigned int)'G'] = Wl_Key_G;
        key_table[(unsigned int)'H'] = Wl_Key_H;
        key_table[(unsigned int)'I'] = Wl_Key_I;
        key_table[(unsigned int)'J'] = Wl_Key_J;
        key_table[(unsigned int)'K'] = Wl_Key_K;
        key_table[(unsigned int)'L'] = Wl_Key_L;
        key_table[(unsigned int)'M'] = Wl_Key_M;
        key_table[(unsigned int)'N'] = Wl_Key_N;
        key_table[(unsigned int)'O'] = Wl_Key_O;
        key_table[(unsigned int)'P'] = Wl_Key_P;
        key_table[(unsigned int)'Q'] = Wl_Key_Q;
        key_table[(unsigned int)'R'] = Wl_Key_R;
        key_table[(unsigned int)'S'] = Wl_Key_S;
        key_table[(unsigned int)'T'] = Wl_Key_T;
        key_table[(unsigned int)'U'] = Wl_Key_U;
        key_table[(unsigned int)'V'] = Wl_Key_V;
        key_table[(unsigned int)'W'] = Wl_Key_W;
        key_table[(unsigned int)'X'] = Wl_Key_X;
        key_table[(unsigned int)'Y'] = Wl_Key_Y;
        key_table[(unsigned int)'Z'] = Wl_Key_Z;
        for (uint64_t i = '0', j = Wl_Key_0; i <= '9'; i += 1, j += 1)
        {
            key_table[i] = (Wl_Key)j;
        }
        for (uint64_t i = VK_NUMPAD0, j = Wl_Key_0; i <= VK_NUMPAD9; i += 1, j += 1)
        {
            key_table[i] = (Wl_Key)j;
        }
        for (uint64_t i = VK_F1, j = Wl_Key_F1; i <= VK_F24; i += 1, j += 1)
        {
            key_table[i] = (Wl_Key)j;
        }
        key_table[VK_SPACE]     = Wl_Key_Space;
        key_table[VK_OEM_3]     = Wl_Key_Tick;
        key_table[VK_OEM_MINUS] = Wl_Key_Minus;
        key_table[VK_OEM_PLUS]  = Wl_Key_Equal;
        key_table[VK_OEM_4]     = Wl_Key_LeftBracket;
        key_table[VK_OEM_6]     = Wl_Key_RightBracket;
        key_table[VK_OEM_1]     = Wl_Key_Semicolon;
        key_table[VK_OEM_7]     = Wl_Key_Quote;
        key_table[VK_OEM_COMMA] = Wl_Key_Comma;
        key_table[VK_OEM_PERIOD]= Wl_Key_Period;
        key_table[VK_OEM_2]     = Wl_Key_Slash;
        key_table[VK_OEM_5]     = Wl_Key_BackSlash;
        key_table[VK_TAB]       = Wl_Key_Tab;
        key_table[VK_PAUSE]     = Wl_Key_Pause;
        key_table[VK_ESCAPE]    = Wl_Key_Esc;
        key_table[VK_UP]        = Wl_Key_Up;
        key_table[VK_LEFT]      = Wl_Key_Left;
        key_table[VK_DOWN]      = Wl_Key_Down;
        key_table[VK_RIGHT]     = Wl_Key_Right;
        key_table[VK_BACK]      = Wl_Key_Backspace;
        key_table[VK_RETURN]    = Wl_Key_Return;
        key_table[VK_DELETE]    = Wl_Key_Delete;
        key_table[VK_INSERT]    = Wl_Key_Insert;
        key_table[VK_PRIOR]     = Wl_Key_PageUp;
        key_table[VK_NEXT]      = Wl_Key_PageDown;
        key_table[VK_HOME]      = Wl_Key_Home;
        key_table[VK_END]       = Wl_Key_End;
        key_table[VK_CAPITAL]   = Wl_Key_CapsLock;
        key_table[VK_NUMLOCK]   = Wl_Key_NumLock;
        key_table[VK_SCROLL]    = Wl_Key_ScrollLock;
        key_table[VK_APPS]      = Wl_Key_Menu;
        key_table[VK_CONTROL]   = Wl_Key_Ctrl;
        key_table[VK_LCONTROL]  = Wl_Key_CtrlLeft;
        key_table[VK_RCONTROL]  = Wl_Key_CtrlRight;
        key_table[VK_SHIFT]     = Wl_Key_Shift;
        key_table[VK_LSHIFT]    = Wl_Key_ShiftLeft;
        key_table[VK_RSHIFT]    = Wl_Key_ShiftRight;
        key_table[VK_MENU]      = Wl_Key_Alt;
        key_table[VK_LMENU]     = Wl_Key_AltLeft;
        key_table[VK_RMENU]     = Wl_Key_AltRight;
        key_table[VK_DIVIDE]   = Wl_Key_NumSlash;
        key_table[VK_MULTIPLY] = Wl_Key_NumStar;
        key_table[VK_SUBTRACT] = Wl_Key_NumMinus;
        key_table[VK_ADD]      = Wl_Key_NumPlus;
        key_table[VK_DECIMAL]  = Wl_Key_NumPeriod;
        for (uint32_t i = 0; i < 10; i += 1)
        {
            key_table[VK_NUMPAD0 + i] = (Wl_Key)((uint64_t)Wl_Key_Num0 + i);
        }
        for (uint64_t i = 0xDF, j = 0; i < 0xFF; i += 1, j += 1)
        {
            key_table[i] = (Wl_Key)((uint64_t)Wl_Key_Ex0 + j);
        }
    }
    Wl_Key key = key_table[vkey&bitmask8];
    return key;
}

internal WPARAM _os_win32_vkey_from_os_key(Wl_Key key)
{
    WPARAM result = 0;
    {
        local_persist int32_t initialized = 0;
        local_persist WPARAM vkey_table[Wl_Key_COUNT] = STRUCT_ZERO;
        if (initialized == 0)
        {
            initialized = 1;
            vkey_table[Wl_Key_Esc] = VK_ESCAPE;
            for (Wl_Key ikey = Wl_Key_F1; ikey <= Wl_Key_F24; ikey = (Wl_Key)(ikey+1))
            {
                vkey_table[ikey] = VK_F1+(ikey-Wl_Key_F1);
            }
            vkey_table[Wl_Key_Tick] = VK_OEM_3;
            for (Wl_Key ikey = Wl_Key_0; ikey <= Wl_Key_9; ikey = (Wl_Key)(ikey+1))
            {
                vkey_table[ikey] = '0'+(ikey-Wl_Key_0);
            }
            vkey_table[Wl_Key_Minus]     = VK_OEM_MINUS;
            vkey_table[Wl_Key_Equal]     = VK_OEM_PLUS;
            vkey_table[Wl_Key_Backspace] = VK_BACK;
            vkey_table[Wl_Key_Tab] = VK_TAB;
            vkey_table[Wl_Key_Q] = 'Q';
            vkey_table[Wl_Key_W] = 'W';
            vkey_table[Wl_Key_E] = 'E';
            vkey_table[Wl_Key_R] = 'R';
            vkey_table[Wl_Key_T] = 'T';
            vkey_table[Wl_Key_Y] = 'Y';
            vkey_table[Wl_Key_U] = 'U';
            vkey_table[Wl_Key_I] = 'I';
            vkey_table[Wl_Key_O] = 'O';
            vkey_table[Wl_Key_P] = 'P';
            vkey_table[Wl_Key_LeftBracket]  = VK_OEM_4;
            vkey_table[Wl_Key_RightBracket] = VK_OEM_6;
            vkey_table[Wl_Key_BackSlash]    = VK_OEM_5;
            vkey_table[Wl_Key_CapsLock]     = VK_CAPITAL;
            vkey_table[Wl_Key_A] = 'A';
            vkey_table[Wl_Key_S] = 'S';
            vkey_table[Wl_Key_D] = 'D';
            vkey_table[Wl_Key_F] = 'F';
            vkey_table[Wl_Key_G] = 'G';
            vkey_table[Wl_Key_H] = 'H';
            vkey_table[Wl_Key_J] = 'J';
            vkey_table[Wl_Key_K] = 'K';
            vkey_table[Wl_Key_L] = 'L';
            vkey_table[Wl_Key_Semicolon] = VK_OEM_1;
            vkey_table[Wl_Key_Quote]     = VK_OEM_7;
            vkey_table[Wl_Key_Return]    = VK_RETURN;
            vkey_table[Wl_Key_Shift]     = VK_SHIFT;
            vkey_table[Wl_Key_Z] = 'Z';
            vkey_table[Wl_Key_X] = 'X';
            vkey_table[Wl_Key_C] = 'C';
            vkey_table[Wl_Key_V] = 'V';
            vkey_table[Wl_Key_B] = 'B';
            vkey_table[Wl_Key_N] = 'N';
            vkey_table[Wl_Key_M] = 'M';
            vkey_table[Wl_Key_Comma]      = VK_OEM_COMMA;
            vkey_table[Wl_Key_Period]     = VK_OEM_PERIOD;
            vkey_table[Wl_Key_Slash]      = VK_OEM_2;
            vkey_table[Wl_Key_Ctrl]       = VK_CONTROL;
            vkey_table[Wl_Key_Alt]        = VK_MENU;
            vkey_table[Wl_Key_Space]      = VK_SPACE;
            vkey_table[Wl_Key_Menu]       = VK_APPS;
            vkey_table[Wl_Key_ScrollLock] = VK_SCROLL;
            vkey_table[Wl_Key_Pause]      = VK_PAUSE;
            vkey_table[Wl_Key_Insert]     = VK_INSERT;
            vkey_table[Wl_Key_Home]       = VK_HOME;
            vkey_table[Wl_Key_PageUp]     = VK_PRIOR;
            vkey_table[Wl_Key_Delete]     = VK_DELETE;
            vkey_table[Wl_Key_End]        = VK_END;
            vkey_table[Wl_Key_PageDown]   = VK_NEXT;
            vkey_table[Wl_Key_Up]         = VK_UP;
            vkey_table[Wl_Key_Left]       = VK_LEFT;
            vkey_table[Wl_Key_Down]       = VK_DOWN;
            vkey_table[Wl_Key_Right]      = VK_RIGHT;
            for (Wl_Key ikey = Wl_Key_Ex0; ikey <= Wl_Key_Ex29; ikey = (Wl_Key)(ikey+1))
            {
                vkey_table[ikey] = 0xDF + (ikey-Wl_Key_Ex0);
            }
            vkey_table[Wl_Key_NumLock]   = VK_NUMLOCK;
            vkey_table[Wl_Key_NumSlash]  = VK_DIVIDE;
            vkey_table[Wl_Key_NumStar]   = VK_MULTIPLY;
            vkey_table[Wl_Key_NumMinus]  = VK_SUBTRACT;
            vkey_table[Wl_Key_NumPlus]   = VK_ADD;
            vkey_table[Wl_Key_NumPeriod] = VK_DECIMAL;
            vkey_table[Wl_Key_Num0] = VK_NUMPAD0;
            vkey_table[Wl_Key_Num1] = VK_NUMPAD1;
            vkey_table[Wl_Key_Num2] = VK_NUMPAD2;
            vkey_table[Wl_Key_Num3] = VK_NUMPAD3;
            vkey_table[Wl_Key_Num4] = VK_NUMPAD4;
            vkey_table[Wl_Key_Num5] = VK_NUMPAD5;
            vkey_table[Wl_Key_Num6] = VK_NUMPAD6;
            vkey_table[Wl_Key_Num7] = VK_NUMPAD7;
            vkey_table[Wl_Key_Num8] = VK_NUMPAD8;
            vkey_table[Wl_Key_Num9] = VK_NUMPAD9;
            vkey_table[Wl_Key_LeftMouseButton]   = VK_LBUTTON;
            vkey_table[Wl_Key_MiddleMouseButton] = VK_MBUTTON;
            vkey_table[Wl_Key_RightMouseButton]  = VK_RBUTTON;
        }
        result = vkey_table[key];
    }
    return result;
}

internal LRESULT CALLBACK _wl_win32_window_proc(HWND handle, UINT message, WPARAM w_param, LPARAM l_param)
{
    LRESULT result = 0;
    // bool release = 0;
    switch(message)
    {
        default:
        {
            result = DefWindowProcW(handle, message, w_param, l_param);
        } break;
        // ak: window size
        case WM_SIZE:
        {
            _wl_core_state.win_width  = LOWORD(l_param);
            _wl_core_state.win_height = HIWORD(l_param);
            _wl_win32_state->window_resize = true;
        } // fallthrough;
        case WM_PAINT:
        {
        } break;
        // ak: window close
        case WM_CLOSE:
        {
            _wl_win32_state->window_close = true;
        } break;
        // ak: window resize
        case WM_ENTERSIZEMOVE:
        {
            // event.type = Wl_EventType_WindowResize;
        } break;
        case WM_EXITSIZEMOVE:
        {
        } break;
        //
        // case WM_SYSCOMMAND:
        // {
        //     switch (w_param & 0xFFF0)
        //     {
        //         case SC_CLOSE: {
        //             event.type = Wl_EventType_WindowClose;
        //         } break;
        //     }
        // }
        // case WM_PAINT:
        // {
        // } break;
        // case WM_ENTERSIZEMOVE:
        // {
        //     event.type = Wl_EventType_WindowResize;
        // } break;
        // case WM_EXITSIZEMOVE:
        // {
        // } break;
        //
        // case WM_SYSCHAR:
        // {
        //     WORD vk_code = LOWORD(wParam);
        //     if (vk_code == VK_SPACE)
        //     {
        //       result = DefWindowProcW(hwnd, message, wParam, l_param);
        //     }
        //     else
        //     {
        //       result = 0;
        //     }
        // } break;
        //
        // case WM_CHAR:
        // {
        // } break;
        //
        // case WM_KILLFOCUS:
        // {
        // } break;
        //
        // case WM_SETCURSOR:
        // {
        // } break;
        //
        // case WM_DPICHANGED:
        // {
        // } break;
        //
        // ak: [custom border]
        // case WM_NCPAINT:
        // {
        // } break;
        // case WM_DWMCOMPOSITIONCHANGED:
        // {
        // } break;
        // case WM_WINDOWPOSCHANGED:
        // {
        // } break;
        // case WM_SETICON:
        // case WM_SETTEXT:
        // {
        // } break;
        //
        // case WM_NCACTIVATE:
        // {
        // } break;
        //
        // case WM_NCCALCSIZE:
        // {
        // } break;
        //
        // case WM_NCHITTEST:
        // {
        // } break;
    }
    return result;
}

// ak: Basic window functions
//=============================================================================

internal void wl_init(void)
{
    // ak: set up base shared state
    Arena *arena = arena_alloc();
    _wl_win32_state = arena_push(arena, _Wl_Win32_State, 1);
    _wl_win32_state->arena = arena;
    _wl_win32_state->instance = GetModuleHandle(0);
    
    // ak: set dpi awareness
    w32_SetProcessDpiAwarenessContext_Type *SetProcessDpiAwarenessContext_func = 0;
    HMODULE module = LoadLibraryA("user32.dll");
    if(module != 0)
    {
        SetProcessDpiAwarenessContext_func = (w32_SetProcessDpiAwarenessContext_Type*)GetProcAddress(module, "SetProcessDpiAwarenessContext");
        w32_GetDpiForWindow_func = (w32_GetDpiForWindow_Type*)GetProcAddress(module, "GetDpiForWindow");
        w32_GetDpiForMonitor_func = (w32_GetDpiForMonitor_Type *)GetProcAddress(module, "GetDpiForMonitor");
        w32_GetSystemMetricsForDpi_func = (w32_GetSystemMetricsForDpi_Type *)GetProcAddress(module, "GetSystemMetricsForDpi");
        FreeLibrary(module);
    }
    if(SetProcessDpiAwarenessContext_func != 0)
    {
        SetProcessDpiAwarenessContext_func(w32_DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    }
    else
    {
        HMODULE shcore = LoadLibraryA("shcore.dll");
        if(shcore)
        {
            typedef HRESULT (WINAPI* SetProcessDpiAwareness_t)(int);
            SetProcessDpiAwareness_t SetProcessDpiAwareness = (void*)GetProcAddress(shcore, "SetProcessDpiAwareness");
            if(SetProcessDpiAwareness)
            {
                SetProcessDpiAwareness(2);
            }
            FreeLibrary(shcore);
        }
        SetProcessDPIAware();
    }
    
    // ak: register graphical-window class
    {
        WNDCLASSEXW wndclass = {sizeof(wndclass)};
        wndclass.lpfnWndProc = _wl_win32_window_proc;
        wndclass.hInstance = _wl_win32_state->instance;
        wndclass.lpszClassName = L"graphical-window";
        wndclass.hCursor = LoadCursorA(0, IDC_ARROW);
        wndclass.hIcon = LoadIcon(_wl_win32_state->instance, MAKEINTRESOURCE(1));
        wndclass.style = CS_VREDRAW|CS_HREDRAW;
        ATOM wndatom = RegisterClassExW(&wndclass);
        (void)wndatom;
    }
}

internal Wl_Handle wl_window_open(Str8 title, unsigned int width, unsigned int height)
{
    // ak: make hwnd
    HWND hwnd = 0;
    {
        Arena_Temp scratch = arena_scratch_begin(0, 0);
        Str16 title16 = str16_from_8(scratch.arena, title);
        hwnd = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP,
                L"graphical-window",
                (WCHAR*)title16.cstr,
                WS_OVERLAPPEDWINDOW | WS_SIZEBOX,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0, 0,
                0, 0,
                _wl_win32_state->instance,
                0);
        DragAcceptFiles(hwnd, 1);
        arena_scratch_end(scratch);
    }

    // ak: make/fill window
    OS_W32_Window *window = os_w32_window_alloc();
    {
        window->hwnd = hwnd;
        window->hdc = GetDC(hwnd);
        if(w32_GetDpiForWindow_func != 0)
        {
            window->dpi = (F32)w32_GetDpiForWindow_func(hwnd);
        }
        else
        {
            window->dpi = 96.f;
        }
    }

    // ak: convert to handle + return
    OS_Handle result = os_w32_handle_from_window(window);
    return result;
}

internal void wl_window_close(void)
{
    DestroyWindow(_wl_win32_state->handle);
}

// ak: Event Functions
//=============================================================================

internal Wl_Event wl_get_event(void)
{
    Wl_Event event = STRUCT_ZERO;
    bool release = 0;
    MSG msg = STRUCT_ZERO;
    if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        switch(msg.message)
        {
            default:
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
                if (_wl_win32_state.window_close)
                {
                    event.type = Wl_EventType_WindowClose;
                    _wl_win32_state->window_close = false;
                }
                if (_wl_win32_state.window_resize)
                {
                    event.type = Wl_EventType_WindowResize;
                    _wl_win32_state->window_resize = false;
                }
            } break;
            // ak: Keyboard key presses/releases
            case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            {
                if (msg.wParam != VK_MENU &&
                   (msg.wParam < VK_F1 || VK_F24 < msg.wParam || msg.wParam == VK_F4))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } /* fallthrough */
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                int32_t was_down = (msg.lParam & bit31);
                int32_t is_down  = !(msg.lParam & bit32);
                int32_t is_repeat = 0;
                if (!is_down)
                {
                    release = 1;
                } else if (was_down)
                {
                    is_repeat = 1;
                }
                int32_t right_sided = 0;
                if ((msg.lParam & bit25) &&
                        (msg.wParam == VK_CONTROL || msg.wParam == VK_RCONTROL ||
                         msg.wParam == VK_MENU || msg.wParam == VK_RMENU ||
                         msg.wParam == VK_SHIFT || msg.wParam == VK_RSHIFT))
                {
                    right_sided = 1;
                }
                if (release)
                {
                    event.type = Wl_EventType_Release;
                } else {
                    event.type = Wl_EventType_Press;
                }
                event.key = _os_win32_os_key_from_vkey(msg.wParam);
                // event.repeat_count = msg.lParam & bitmask16;
                // event.is_repeat = is_repeat;
                // event.right_sided = right_sided;
                if (event.key == Wl_Key_Alt   && event.mod_key & Wl_ModKey_Alt)
                {
                    event.mod_key &= ~Wl_ModKey_Alt;
                }
                if (event.key == Wl_Key_Ctrl  && event.mod_key & Wl_ModKey_Ctrl)
                {
                    event.mod_key &= ~Wl_ModKey_Ctrl;
                }
                if (event.key == Wl_Key_Shift && event.mod_key & Wl_ModKey_Shift)
                {
                    event.mod_key &= ~Wl_ModKey_Shift;
                }
                Unused(is_repeat);
                Unused(right_sided);
            } break;
            // ak: mouse button presses/releases
            case WM_LBUTTONUP:
            case WM_MBUTTONUP:
            case WM_RBUTTONUP:
            {
                release = 1;
            } /* fallthrough */
            case WM_LBUTTONDOWN:
            case WM_MBUTTONDOWN:
            case WM_RBUTTONDOWN:
            {
                if (release)
                {
                    event.type = Wl_EventType_Release;
                }
                else
                {
                    event.type = Wl_EventType_Press;
                }
                switch (msg.message)
                {
                    case WM_LBUTTONUP: case WM_LBUTTONDOWN:
                        {
                            event.key = Wl_Key_LeftMouseButton;
                        } break;
                    case WM_MBUTTONUP: case WM_MBUTTONDOWN:
                        {
                            event.key = Wl_Key_MiddleMouseButton;
                        } break;
                    case WM_RBUTTONUP: case WM_RBUTTONDOWN:
                        {
                            event.key = Wl_Key_RightMouseButton;
                        } break;
                }
                event.pos.x = (float)(int16_t)LOWORD(msg.lParam);
                event.pos.y = (float)(int16_t)HIWORD(msg.lParam);
                if (release)
                {
                    ReleaseCapture();
                }
                else
                {
                    SetCapture(_wl_win32_state->handle);
                }
            } break;
            // ak: mouse motion
            case WM_MOUSEMOVE:
            {
                event.pos.x = (float)(int16_t)LOWORD(msg.lParam);
                event.pos.y = (float)(int16_t)HIWORD(msg.lParam);
            } break;
            case WM_MOUSEWHEEL:
            {
                int16_t wheel_delta = HIWORD(msg.wParam);
                POINT p;
                p.x = (int32_t)(int16_t)LOWORD(msg.lParam);
                p.y = (int32_t)(int16_t)HIWORD(msg.lParam);
                ScreenToClient(_wl_win32_state->handle, &p);
                event.pos.x = (float)p.x;
                event.pos.y = (float)p.y;
                event.delta = (Vec2_F32){0.f, -(float)wheel_delta};
            } break;
            case WM_MOUSEHWHEEL:
            {
                int16_t wheel_delta = HIWORD(msg.wParam);
                POINT p;
                p.x = (int32_t)LOWORD(msg.lParam);
                p.y = (int32_t)HIWORD(msg.lParam);
                ScreenToClient(_wl_win32_state->handle, &p);
                event.pos.x = (float)p.x;
                event.pos.y = (float)p.y;
                event.delta = (Vec2_F32){(float)wheel_delta, 0.f};
            } break;
            case WM_QUIT:
            {
                event.type = Wl_EventType_WindowClose;
            } break;
            case WM_DROPFILES:
            {
            } break;
        }
    }
    return event;
}

// ak: window property
//=============================================================================

internal void wl_window_pos_set(int x, int y)
{
    SetWindowPos(
        _wl_win32_state->handle, NULL, x, y, 0, 0,
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE
    );
}

internal void wl_window_icon_set_raw(uint32_t *icon_data, uint32_t width, uint32_t height)
{
    static HICON current_icon = NULL;
    if (current_icon)
    {
        DestroyIcon(current_icon);
        current_icon = NULL;
    }

    HDC hdc = GetDC(NULL);
    if (!hdc) return;

    BITMAPINFO bmi = STRUCT_ZERO;
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = (LONG)width;
    bmi.bmiHeader.biHeight = -(LONG)height;  // Top-down DIB
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void *pBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    ReleaseDC(NULL, hdc);

    if (!hBitmap || !pBits) return;

    // ak: The icon_data is ARGB in uint32_t, which in little-endian memory is
    // BGRA byte order, matching Windows 32bpp DIB exactly. Direct copy.
    memcpy(pBits, icon_data, width * height * sizeof(uint32_t));

    HICON hIcon = CopyImage(hBitmap, IMAGE_ICON, (int)width, (int)height, LR_DEFAULTCOLOR);
    DeleteObject(hBitmap);

    if (hIcon)
    {
        SendMessage(_wl_win32_state.handle, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        SendMessage(_wl_win32_state.handle, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        current_icon = hIcon;
    }
}

internal void wl_window_border_set(bool enable)
{
    // ak: get current window style
    LONG_PTR style = GetWindowLongPtr(_wl_win32_state->handle, GWL_STYLE);
    if (!enable)
    {
        // ak: borderless window
        style = WS_POPUP | WS_VISIBLE;
    }
    // ak: apply the new style
    SetWindowLongPtr(_wl_win32_state->handle, GWL_STYLE, style);
    // NOTE(ak): Force Windows to recalculate the window frame and client area
    // Without this, the non-client area (border/title) may not update properly
    SetWindowPos(_wl_win32_state->handle,
         NULL,
         0, 0, 0, 0,
         SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);
}

// ak: Software Render
//=============================================================================

internal void wl_render_init(void *render_buffer)
{
    _wl_win32_state->render_buffer = render_buffer;
    _wl_win32_state->bitmap_info.bmiHeader.biSize = sizeof(_wl_win32_state->bitmap_info.bmiHeader);
    _wl_win32_state->bitmap_info.bmiHeader.biPlanes = 1;
    _wl_win32_state->bitmap_info.bmiHeader.biBitCount = 32;
    _wl_win32_state->bitmap_info.bmiHeader.biCompression = BI_RGB;
    _wl_win32_state->bitmap_info.bmiHeader.biWidth = wl_display_width_get();
    _wl_win32_state->bitmap_info.bmiHeader.biHeight = wl_display_height_get();
}

internal void wl_render_deinit(void)
{
}

internal void wl_render_begin(void)
{
    _wl_win32_state->hdc = BeginPaint(_wl_win32_state->handle, &_wl_win32_state->paint);
}

internal void wl_render_end(void)
{
    StretchDIBits(
        _wl_win32_state->hdc,
        0, 0, wl_display_width_get(), wl_display_height_get(),
        0, 0, wl_display_width_get(), wl_display_height_get(),
        _wl_win32_state->render_buffer, &_wl_win32_state->bitmap_info,
        DIB_RGB_COLORS, SRCCOPY
    );
    EndPaint(_wl_win32_state->handle, &_wl_win32_state->paint);
}
