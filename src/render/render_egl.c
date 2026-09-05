// ak: OpenGL helper functions
//=============================================================================

internal Void_Proc *_render_opengl_load_procedure(const char *name)
{
    Void_Proc *p = (Void_Proc *)(void *)eglGetProcAddress(name);
    if (p == (Void_Proc*)1 || p == (Void_Proc*)2 || p == (Void_Proc*)3 || p == (Void_Proc*)-1)
    {
        p = NULL;
    }
    return p;
}

// ak: Internal OpenGL functions
//=============================================================================

// ak: layer initialization and cleanup

internal void _render_opengl_init(void)
{
    // ak: set up state
    {
        Arena *arena = arena_alloc();
        _render_egl_state = arena_push(arena, _Render_Egl_State, 1);
        _render_egl_state->arena = arena;
    }
    
    // ak: get EGL display
    {
        _render_egl_state->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (_render_egl_state->display == EGL_NO_DISPLAY)
        {
            LogErrorLine(&_os_core_state.log_context, "Failed to get EGL display.");
            os_exit(1);
        }
    }
    
    // ak: initialize GL version
    EGLint egl_version_major = 0;
    EGLint egl_version_minor = 0;
    if (!eglInitialize(_render_egl_state->display, &egl_version_major, &egl_version_minor))
    {
        LogErrorLine(&_os_core_state.log_context, "Couldn't initialize EGL display.");
        os_exit(1);
    }
    if (egl_version_major < 1 || (egl_version_major == 1 && egl_version_minor < 5))
    {
        LogErrorLine(&_os_core_state.log_context, "Unsupported EGL version (%i.%i, need at least 1.5)", egl_version_major, egl_version_minor);
        os_exit(1);
    }
    
    // ak: pick GL API
    if (!eglBindAPI(EGL_OPENGL_API))
    {
        LogErrorLine(&_os_core_state.log_context, "Couldn't initialize EGL API to OpenGL.");
        os_exit(1);
    }
    
    // ak: construct context
    {
        bool debug_mode = false;
#if BUILD_DEBUG
        debug_mode = true;
#endif
        EGLint options[] =
        {
            EGL_CONTEXT_MAJOR_VERSION, 3,
            EGL_CONTEXT_MINOR_VERSION, 3,
            EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
            debug_mode ? EGL_CONTEXT_OPENGL_DEBUG : EGL_NONE, EGL_TRUE,
            EGL_NONE,
        };
        _render_egl_state->context = eglCreateContext(_render_egl_state->display, 0, EGL_NO_CONTEXT, options);
        if (_render_egl_state->context == EGL_NO_CONTEXT)
        {
            LogErrorLine(&_os_core_state.log_context, "Couldn't create OpenGL context with EGL.");
            exit(1);
        }
    }
    
    eglMakeCurrent(_render_egl_state->display, 0, 0, _render_egl_state->context);
#ifndef SOKOL_GLCORE
    glDrawBuffer(GL_BACK);
#endif
}

internal void _render_opengl_cleanup(void)
{
    if (_render_egl_state != 0)
    {
        eglMakeCurrent(_render_egl_state->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (_render_egl_state->context != EGL_NO_CONTEXT)
        {
            eglDestroyContext(_render_egl_state->display, _render_egl_state->context);
            _render_egl_state->context = EGL_NO_CONTEXT;
        }
        for (_Render_Egl_Window *w = _render_egl_state->free_window; w != NULL; w = w->next)
        {
            if (w->surface != EGL_NO_SURFACE)
            {
                eglDestroySurface(_render_egl_state->display, w->surface);
                w->surface = EGL_NO_SURFACE;
            }
        }
        if (_render_egl_state->display != EGL_NO_DISPLAY)
        {
            eglTerminate(_render_egl_state->display);
            _render_egl_state->display = EGL_NO_DISPLAY;
        }
        eglReleaseThread();
        if (_render_egl_state->arena != NULL)
        {
            arena_free(_render_egl_state->arena);
        }
        _render_egl_state = NULL;
    }
}

// aK: window functions

internal Render_Handle _render_opengl_window_equip(Wl_Window window)
{
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    _Render_Egl_Window *window_egl = _render_egl_state->free_window;
    if(window_egl != NULL)
    {
        SLLStackPop(_render_egl_state->free_window);
    }
    else
    {
        window_egl = arena_push(_render_egl_state->arena, _Render_Egl_Window, 1);
    }
    {
        EGLint surface_options[] =
        {
            EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
            EGL_NONE,
        };
        if(_render_egl_state->config == NULL)
        {
            // ak: get all EGL configs
            EGLConfig configs[256] = {0};
            EGLint configs_count = 0;
            {
                EGLint options[] =
                {
                    EGL_SURFACE_TYPE,      EGL_WINDOW_BIT,
                    EGL_CONFORMANT,        EGL_OPENGL_BIT,
                    EGL_RENDERABLE_TYPE,   EGL_OPENGL_BIT,
                    EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
                    EGL_RED_SIZE,      8,
                    EGL_GREEN_SIZE,    8,
                    EGL_BLUE_SIZE,     8,
                    EGL_DEPTH_SIZE,   24,
                    EGL_STENCIL_SIZE,  8,
                    EGL_NONE,
                };
                if(!eglChooseConfig(_render_egl_state->display, options, configs, ArrayLength(configs), &configs_count) || configs_count == 0)
                {
                    LogErrorLine(&_os_core_state.log_context, "Couldn't choose EGL configuration.");
                    exit(1);
                }
            }
            
            // ak: actually choose the egl config
            {
                EGLint config_options[] =
                {
                    EGL_GL_COLORSPACE, EGL_GL_COLORSPACE_SRGB,
                    EGL_NONE,
                };
                for (EGLint idx = 0; idx < configs_count; idx += 1)
                {
                    window_egl->surface = eglCreateWindowSurface(_render_egl_state->display, configs[idx], window_os->xwindow, config_options);
                    if(window_egl->surface != EGL_NO_SURFACE)
                    {
                        _render_egl_state->config = configs[idx];
                        break;
                    }
                }
                if(_render_egl_state->config == 0)
                {
                    LogErrorLine(&_os_core_state.log_context, "Couldn't find a suitable EGL configuration.");
                    exit(1);
                }
            }
        }
        else
        {
            window_egl->surface = eglCreateWindowSurface(_render_egl_state->display, _render_egl_state->config, window_os->xwindow, surface_options);
        }
        if(window_egl->surface == EGL_NO_SURFACE)
        {
            LogErrorLine(&_os_core_state.log_context, "Couldn't create EGL surface.");
            exit(1);
        }
    }
    Render_Handle result = {(uint64_t)window_egl};
    return result;
}

internal void _render_opengl_window_unequip(Wl_Window window, Render_Handle handle)
{
    Unused(window);
    _Render_Egl_Window *window_egl = (_Render_Egl_Window *)handle.u64[0];
    if (window_egl != 0 && window_egl->surface != EGL_NO_SURFACE)
    {
        eglDestroySurface(_render_egl_state->display, window_egl->surface);
        window_egl->surface = EGL_NO_SURFACE;
    }
    SLLStackPush(_render_egl_state->free_window, window_egl);
}

internal void _render_opengl_select_window(Wl_Window window, Render_Handle r)
{
    _Wl_X11_Window *w = (_Wl_X11_Window *)window.u64[0];
    _Render_Egl_Window *window_egl = (_Render_Egl_Window *)r.u64[0];
    eglMakeCurrent(_render_egl_state->display, window_egl->surface, window_egl->surface, _render_egl_state->context);
}

internal void _render_opengl_window_swap(Wl_Window window, Render_Handle r)
{
    _Wl_X11_Window *w = (_Wl_X11_Window *)window.u64[0];
    _Render_Egl_Window *window_egl = (_Render_Egl_Window *)r.u64[0];
    eglSwapBuffers(_render_egl_state->display, window_egl->surface);
}

