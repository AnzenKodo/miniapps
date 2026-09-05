// ak: Helper Functions
//=============================================================================

internal Wl_Window wl_window_zero(void)
{
    Wl_Window window = STRUCT_ZERO;
    return window;
}

internal bool wl_window_match(Wl_Window a, Wl_Window b)
{
    return a.u64[0] == b.u64[0];
}

internal Wl_Event *wl_event_list_push_new(Arena *arena, Wl_Event_List *evts, Wl_Event_Kind kind)
{
    Wl_Event *evt = arena_push(arena, Wl_Event, 1);
    DLLPushBack(evts->first, evts->last, evt);
    evts->length += 1;
    evt->timestamp_us = os_now_us();
    evt->kind = kind;
    return evt;
}

// ak: Window Close Functions
//=============================================================================

internal void wl_exit(void)
{
    _wl_core_state.exit = true;
}

internal bool wl_should_exit(void)
{
    return _wl_core_state.exit;
}

// ak: Get Window Property
//=============================================================================

internal Rng2_F32 wl_display_rect(void)
{
    return _wl_core_state.display_rect;
}

// ak: FPS
//=============================================================================

internal void wl_set_fps(uint32_t fps)
{
    os_sleep_us(Million(1)/fps);
}
