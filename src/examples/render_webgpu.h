#ifndef RENDER_WEBGPU_H
#define RENDER_WEBGPU_H

#include <webgpu/webgpu.h>
#include <webgpu/wgpu.h>

// Struct for the uniform data / push constants
typedef struct Webgpu_Push_Constants Webgpu_Push_Constants;
struct Webgpu_Push_Constants {
    Mat4x4_F32 texture_sample_channel_map;
    Vec4_F32 xform_col0;
    Vec4_F32 xform_col1;
    Vec4_F32 xform_col2;
    Vec2_F32 viewport_size_px;
    float opacity;
    float pad;
};

typedef struct _Render_Webgpu_Window _Render_Webgpu_Window;
struct _Render_Webgpu_Window
{
    _Render_Webgpu_Window *next;
    _Render_Webgpu_Window *prev;
    
    Wl_Window window;
    WGPUSurface surface;
    WGPUTextureFormat format;
    Vec2_F32 last_canvas_rect_dim;
    WGPUTextureView current_view;
    WGPUSurfaceTexture current_surface_texture;
};

typedef struct _Render_Webgpu_Tex_2D _Render_Webgpu_Tex_2D;
struct _Render_Webgpu_Tex_2D
{
    _Render_Webgpu_Tex_2D *next;
    _Render_Webgpu_Tex_2D *prev;
    
    WGPUTexture texture;
    WGPUTextureView texture_view;
    WGPUBindGroup bind_group;
    
    Render_Resource_Kind resource_kind;
    Render_Tex_2D_Format format;
    Vec2_I32 size;
};

typedef struct _Render_Webgpu_State _Render_Webgpu_State;
struct _Render_Webgpu_State
{
    Arena *arena;
    
    WGPUInstance instance;
    WGPUAdapter adapter;
    WGPUDevice device;
    WGPUQueue queue;
    
    WGPUBindGroupLayout bind_group_layout_tex;
    WGPUBindGroupLayout bind_group_layout_sampler;
    WGPUPipelineLayout pipeline_layout;
    WGPURenderPipeline pipeline_rect;
    WGPURenderPipeline pipeline_tri;
    
    WGPUSampler sampler_nearest;
    WGPUSampler sampler_linear;
    
    WGPUBindGroup bind_group_sampler_nearest;
    WGPUBindGroup bind_group_sampler_linear;
    
    _Render_Webgpu_Window *free_window;
    _Render_Webgpu_Tex_2D *free_tex2d;
    
    _Render_Webgpu_Tex_2D white_texture;
    
    WGPUBuffer instance_buffer;
    size_t instance_buffer_size;
    
    // Dynamic uniform buffer for push constants
    WGPUBuffer push_buffer;
    size_t push_buffer_size;
};

global _Render_Webgpu_State *_render_webgpu_state = NULL;

#endif // RENDER_WEBGPU_H
