//-----------------------------------------------------------------------------
// Reference:
// - By Epic Games Tools & Ryan Fleury: https://github.com/EpicGames/raddebugger/tree/e28a543573277315ffd32a3e983c8171570bb946/src/render/opengl/linux/egl
// - By Pedro Minicz: https://gist.github.com/pedrominicz/2d81559c5fb66d23d6bb627570956605
// - By Vitali Baumtrok: https://github.com/vbsw/opengl-xlib-example
//-----------------------------------------------------------------------------

#ifndef RENDER_EGL_H
#define RENDER_EGL_H

// ak: External Includes
//=============================================================================

#include <EGL/egl.h>
#include <GLES2/gl2.h>

// ak: Types
//=============================================================================

typedef struct _Render_Egl_Window _Render_Egl_Window;
struct _Render_Egl_Window
{
    _Render_Egl_Window *next;
    _Render_Egl_Window *prev;
    EGLSurface surface;
};

typedef struct _Render_Egl_State _Render_Egl_State;
struct _Render_Egl_State
{
    _Render_Egl_Window *free_window;
    Arena *arena;
    EGLConfig  config;
    EGLDisplay display;
    EGLContext context;
};

// ak: Global Variables
//=============================================================================

global _Render_Egl_State *_render_egl_state = NULL;

#endif // RENDER_EGL_H
