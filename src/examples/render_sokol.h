#ifndef RENDER_SOKOL_H
#define RENDER_SOKOL_H

// ak: Backend selector (must be defined before sokol_gfx.h)
//=============================================================================

#if OS_LINUX
#   ifndef SOKOL_VULKAN
#       define SOKOL_VULKAN          // Vulkan on Linux
#   endif
#   include <vulkan/vulkan.h>
#   include <vulkan/vulkan_xcb.h>
#   include <xcb/xcb.h>
#   include <xcb/xcb_image.h>
#   include <unistd.h>
#elif OS_WINDOWS
#   ifndef SOKOL_D3D11
#       define SOKOL_D3D11
#   endif
#endif

// sokol_gfx.h is NOT included here.
// It is included in render_sokol.c (with SOKOL_GFX_IMPL) so the
// implementation is compiled exactly once.
// We forward-declare only what the header itself needs:
//   sg_image, sg_view, sg_sampler, sg_buffer, sg_pipeline, sg_swapchain,
//   sg_environment  — all of these are uint32_t-based handle structs.
// Include the full header from C files that need it.
#define SOKOL_GFX_IMPL  // Trigger implementation — included only once due to the include guard above
#include "External/sokol_gfx.h"


// ak: Types
//=============================================================================

typedef struct _Render_Sokol_Tex_2D _Render_Sokol_Tex_2D;
struct _Render_Sokol_Tex_2D
{
    _Render_Sokol_Tex_2D  *next;
    _Render_Sokol_Tex_2D  *prev;
    sg_image              image;
    sg_view               view_nearest;   // texture view for nearest sampling
    sg_view               view_linear;    // texture view for linear sampling
    sg_sampler            smp_nearest;
    sg_sampler            smp_linear;
    Render_Resource_Kind  resource_kind;
    Render_Tex_2D_Format   format;
    Vec2_I32              size;
};

typedef struct _Render_Sokol_Window _Render_Sokol_Window;
struct _Render_Sokol_Window
{
    _Render_Sokol_Window *next;
    _Render_Sokol_Window *prev;
    Render_Handle         platform_handle;  // EGLSurface handle (Linux) / D3D11 win ptr (Windows)
};

typedef struct _Render_Sokol_State _Render_Sokol_State;
struct _Render_Sokol_State
{
    Arena                *arena;

    // White 1×1 fallback texture
    sg_image              white_image;
    sg_view               white_view;
    sg_sampler            white_sampler;

    // Shaders
    sg_shader             shd_rect;
    sg_shader             shd_triangle;

    // Pipelines
    sg_pipeline           pip_rect;
    sg_pipeline           pip_triangle;

    // Shared streaming instance buffer for rect draws
    sg_buffer             inst_buf;
    size_t                inst_buf_size;

    // Free-lists
    _Render_Sokol_Tex_2D  *free_tex2d;
    _Render_Sokol_Window *free_window;
};

// ak: Function prototypes (implemented in render_sokol.c)
//=============================================================================

// Pixel format conversion
internal sg_pixel_format  _render_sokol_pixel_format(Render_Tex_2D_Format fmt);

// Handle <-> pointer
internal Render_Handle         _render_handle_from_sokol_tex2d(_Render_Sokol_Tex_2D *t);
internal _Render_Sokol_Tex_2D  *_render_sokol_tex2d_from_handle(Render_Handle h);

// Texture view helper
internal sg_view _render_sokol_texture_view(sg_image image);

// Pipeline builders
internal sg_pipeline _render_sokol_make_rect_pipeline(void);
internal sg_pipeline _render_sokol_make_triangle_pipeline(void);

// Platform abstraction — implemented in the platform section of render_sokol.c
internal void          _render_sokol_platform_init(void);
internal void          _render_sokol_platform_deinit(void);
internal sg_environment _render_sokol_build_environment(void);
internal sg_swapchain  _render_sokol_swapchain_from_window(_Render_Sokol_Window *sw, int w, int h);
internal Render_Handle _render_sokol_window_open(Wl_Window window);
internal void          _render_sokol_window_close(Wl_Window window, _Render_Sokol_Window *sw);
internal void          _render_sokol_window_make_current(Wl_Window window, _Render_Sokol_Window *sw);
internal void          _render_sokol_present_window(Wl_Window window, _Render_Sokol_Window *sw);

#if defined(SOKOL_VULKAN)
internal sg_range _render_sokol_vulkan_compile_shader(const char *src, const char *stage);
#endif

// Sokol logger
internal void _render_sokol_log(const char *tag, uint32_t level, uint32_t id,
                                 const char *msg, uint32_t line, const char *file,
                                 void *user_data);

// ak: Global state
//=============================================================================

global _Render_Sokol_State *_render_sokol_state;

#endif // RENDER_SOKOL_H
