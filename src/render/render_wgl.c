// ak: OpenGL helper functions
//=============================================================================

internal Void_Proc *_render_opengl_load_procedure(char *name)
{
    Void_Proc *p = (Void_Proc *)(void *)wglGetProcAddress(name);
    if (p == (Void_Proc*)1 || p == (Void_Proc*)2 || p == (Void_Proc*)3 || p == (Void_Proc*)-1)
    {
        p = 0;
    }
    return p;
}

// ak: Internal OpenGL functions
//=============================================================================

internal void _render_opengl_init(void)
{
    // Get device contex
    _render_wgl_state.hdc = GetDC(_wl_win32_state->handle);
    // Build pixel format descriptor
    int pf = 0;
    {
        PIXELFORMATDESCRIPTOR pfd = STRUCT_ZERO;
        pfd.nSize      = sizeof(pfd);
        pfd.nVersion   = 1;
        pfd.dwFlags    = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        pfd.cColorBits = 32;
        pfd.cAlphaBits = 8;
        pfd.iLayerType = PFD_MAIN_PLANE;
        pf = ChoosePixelFormat(_render_wgl_state.hdc, &pfd);
        DescribePixelFormat(_render_wgl_state.hdc, pf, sizeof(pfd), &pfd);
        SetPixelFormat(_render_wgl_state.hdc, pf, &pfd);
    }
    // ak: Make bootstrap contex
    HGLRC bootstrap_contex = wglCreateContext(_render_wgl_state.hdc);
    wglMakeCurrent(_render_wgl_state.hdc, bootstrap_contex);
    // ak: Load extensions
    FNWGLCHOOSEPIXELFORMATARBPROC    *wglChoosePixelFormatARB    = (FNWGLCHOOSEPIXELFORMATARBPROC*)(void*)_render_opengl_load_procedure("wglChoosePixelFormatARB");
    FNWGLCREATECONTEXTATTRIBSARBPROC *wglCreateContextAttribsARB = (FNWGLCREATECONTEXTATTRIBSARBPROC*)(void*)_render_opengl_load_procedure("wglCreateContextAttribsARB");
    FNWGLSWAPINTERVALEXTPROC         *wglSwapIntervalEXT         = (FNWGLSWAPINTERVALEXTPROC*)(void*)_render_opengl_load_procedure("wglSwapIntervalEXT");
    // ak: set up real pixel format
    {
        int pf_attribs_i[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, 1,
            WGL_SUPPORT_OPENGL_ARB, 1,
            WGL_DOUBLE_BUFFER_ARB,  1,
            WGL_PIXEL_TYPE_ARB,     WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB,     32,
            WGL_DEPTH_BITS_ARB,     24,
            WGL_STENCIL_BITS_ARB,   8,
            0
        };
        UINT num_formats = 0;
        wglChoosePixelFormatARB(_render_wgl_state.hdc, pf_attribs_i, 0, 1, &pf, &num_formats);
    }
    // ak: make real OpenGl contex
    if (pf)
    {
        int context_attribs[] =
        {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
            0
        };
        HGLRC real_contex = wglCreateContextAttribsARB(_render_wgl_state.hdc, bootstrap_contex, context_attribs);
        _render_wgl_state.contex = real_contex;
    }
    // ak: clean up bootstrap context
    wglMakeCurrent(_render_wgl_state.hdc, 0);
    wglDeleteContext(bootstrap_contex);
    wglMakeCurrent(_render_wgl_state.hdc, _render_wgl_state.contex);
    wglSwapIntervalEXT(1);
}

internal void _render_opengl_deinit(void)
{
    ReleaseDC(_wl_win32_state->handle, _render_wgl_state.hdc);
    wglDeleteContext(_render_wgl_state.contex);
}

internal void _render_opengl(void)
{
    SwapBuffers(_render_wgl_state.hdc);
}
