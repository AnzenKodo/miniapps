#include "render_webgpu.h"
#include <string.h>

//~ Structs for callback tracking
struct RequestAdapterData {
    WGPUAdapter adapter;
    WGPURequestAdapterStatus status;
    bool completed;
};

internal void _wgpu_request_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void *userdata1, void *userdata2)
{
    struct RequestAdapterData *data = (struct RequestAdapterData *)userdata1;
    data->status = status;
    data->adapter = adapter;
    data->completed = true;
}

struct RequestDeviceData {
    WGPUDevice device;
    WGPURequestDeviceStatus status;
    bool completed;
};

internal void _wgpu_request_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void *userdata1, void *userdata2)
{
    struct RequestDeviceData *data = (struct RequestDeviceData *)userdata1;
    data->status = status;
    data->device = device;
    data->completed = true;
}

//~ WGSL Shader Sources
global const char *webgpu_shader_src =
    "struct PushConstants {\n"
    "    texture_sample_channel_map : mat4x4<f32>,\n"
    "    xform_col0 : vec4<f32>,\n"
    "    xform_col1 : vec4<f32>,\n"
    "    xform_col2 : vec4<f32>,\n"
    "    viewport_size_px : vec2<f32>,\n"
    "    opacity : f32,\n"
    "    pad : f32,\n"
    "};\n"
    "\n"
    "@group(0) @binding(1) var<uniform> push : PushConstants;\n"
    "\n"
    "struct VertexInput {\n"
    "    @location(0) c2v_dst_rect : vec4<f32>,\n"
    "    @location(1) c2v_src_rect : vec4<f32>,\n"
    "    @location(2) c2v_colors_0 : vec4<f32>,\n"
    "    @location(3) c2v_colors_1 : vec4<f32>,\n"
    "    @location(4) c2v_colors_2 : vec4<f32>,\n"
    "    @location(5) c2v_colors_3 : vec4<f32>,\n"
    "    @location(6) c2v_corner_radii : vec4<f32>,\n"
    "    @location(7) c2v_style : vec4<f32>,\n"
    "};\n"
    "\n"
    "struct VertexOutput {\n"
    "    @builtin(position) position : vec4<f32>,\n"
    "    @location(0) sdf_sample_pos : vec2<f32>,\n"
    "    @location(1) texcoord_pct : vec2<f32>,\n"
    "    @location(2) rect_half_size_px : vec2<f32>,\n"
    "    @location(3) tint : vec4<f32>,\n"
    "    @location(4) corner_radius : f32,\n"
    "    @location(5) border_thickness : f32,\n"
    "    @location(6) softness : f32,\n"
    "    @location(7) omit_texture : f32,\n"
    "};\n"
    "\n"
    "@vertex\n"
    "fn vs_main(@builtin(vertex_index) VertexIndex : u32, input : VertexInput) -> VertexOutput {\n"
    "    var vertices = array<vec2<f32>, 4>(\n"
    "        vec2<f32>(-1.0, -1.0),\n"
    "        vec2<f32>(-1.0, 1.0),\n"
    "        vec2<f32>(1.0, -1.0),\n"
    "        vec2<f32>(1.0, 1.0)\n"
    "    );\n"
    "    var shears = array<f32, 4>(0.0, 0.0, input.c2v_style.w, input.c2v_style.w);\n"
    "    \n"
    "    var dst_half_size = (input.c2v_dst_rect.zw - input.c2v_dst_rect.xy) / 2.0;\n"
    "    var dst_center    = (input.c2v_dst_rect.zw + input.c2v_dst_rect.xy) / 2.0;\n"
    "    var dst_position  = vertices[VertexIndex] * dst_half_size + dst_center;\n"
    "    dst_position.y += shears[VertexIndex];\n"
    "    \n"
    "    var u_xform = mat3x3<f32>(\n"
    "        push.xform_col0.xyz,\n"
    "        push.xform_col1.xyz,\n"
    "        push.xform_col2.xyz\n"
    "    );\n"
    "    var pos3 = u_xform * vec3<f32>(dst_position.x, dst_position.y, 1.0);\n"
    "    dst_position = pos3.xy;\n"
    "    \n"
    "    var src_half_size = (input.c2v_src_rect.zw - input.c2v_src_rect.xy) / 2.0;\n"
    "    var src_center    = (input.c2v_src_rect.zw + input.c2v_src_rect.xy) / 2.0;\n"
    "    var src_position  = vertices[VertexIndex] * src_half_size + src_center;\n"
    "    \n"
    "    var colors = array<vec4<f32>, 4>(\n"
    "        input.c2v_colors_0,\n"
    "        input.c2v_colors_1,\n"
    "        input.c2v_colors_2,\n"
    "        input.c2v_colors_3\n"
    "    );\n"
    "    var color = colors[VertexIndex];\n"
    "    \n"
    "    var corner_radii = array<f32, 4>(\n"
    "        input.c2v_corner_radii.x,\n"
    "        input.c2v_corner_radii.y,\n"
    "        input.c2v_corner_radii.z,\n"
    "        input.c2v_corner_radii.w\n"
    "    );\n"
    "    var corner_radius = corner_radii[VertexIndex];\n"
    "    \n"
    "    var dst_verts_pct = vec2<f32>(\n"
    "        select(1.0, 0.0, (VertexIndex >> 1u) != 1u),\n"
    "        select(0.0, 1.0, (VertexIndex & 1u) != 0u)\n"
    "    );\n"
    "    \n"
    "    var out : VertexOutput;\n"
    "    out.position = vec4<f32>(\n"
    "        2.0 * dst_position.x / push.viewport_size_px.x - 1.0,\n"
    "        1.0 - 2.0 * dst_position.y / push.viewport_size_px.y,\n"
    "        0.0,\n"
    "        1.0\n"
    "    );\n"
    "    \n"
    "    out.sdf_sample_pos    = (2.0 * dst_verts_pct - 1.0) * dst_half_size;\n"
    "    out.texcoord_pct      = src_position;\n"
    "    out.rect_half_size_px = dst_half_size;\n"
    "    out.tint              = color;\n"
    "    out.corner_radius     = corner_radius;\n"
    "    out.border_thickness  = input.c2v_style.x;\n"
    "    out.softness          = input.c2v_style.y;\n"
    "    out.omit_texture      = input.c2v_style.z;\n"
    "    return out;\n"
    "}\n"
    "\n"
    "@group(0) @binding(0) var u_tex_color : texture_2d<f32>;\n"
    "@group(1) @binding(0) var u_sampler : sampler;\n"
    "\n"
    "fn rect_sdf(sample_pos : vec2<f32>, rect_half_size : vec2<f32>, r : f32) -> f32 {\n"
    "    return length(max(abs(sample_pos) - rect_half_size + r, vec2<f32>(0.0, 0.0))) - r;\n"
    "}\n"
    "\n"
    "fn linear_from_srgb_f32(x : f32) -> f32 {\n"
    "    return select(pow((x + 0.055) / 1.055, 2.4), x / 12.92, x < 0.0404482362771082);\n"
    "}\n"
    "\n"
    "fn linear_from_srgba(v : vec4<f32>) -> vec4<f32> {\n"
    "    return vec4<f32>(\n"
    "        linear_from_srgb_f32(v.x),\n"
    "        linear_from_srgb_f32(v.y),\n"
    "        linear_from_srgb_f32(v.z),\n"
    "        v.w\n"
    "    );\n"
    "}\n"
    "\n"
    "@fragment\n"
    "fn fs_main(input : VertexOutput) -> @location(0) vec4<f32> {\n"
    "    var albedo_sample = vec4<f32>(1.0, 1.0, 1.0, 1.0);\n"
    "    if (input.omit_texture < 1.0) {\n"
    "        var tex_size = vec2<f32>(textureDimensions(u_tex_color, 0));\n"
    "        var uv = input.texcoord_pct / tex_size;\n"
    "        var tex_color = textureSample(u_tex_color, u_sampler, uv);\n"
    "        albedo_sample = push.texture_sample_channel_map * tex_color;\n"
    "        albedo_sample = linear_from_srgba(albedo_sample);\n"
    "    }\n"
    "    \n"
    "    var border_sdf_t = 1.0;\n"
    "    if (input.border_thickness > 0.0) {\n"
    "        var border_sdf_s = rect_sdf(\n"
    "            input.sdf_sample_pos,\n"
    "            input.rect_half_size_px - vec2<f32>(input.softness * 2.0, input.softness * 2.0) - input.border_thickness,\n"
    "            max(input.corner_radius - input.border_thickness, 0.0)\n"
    "        );\n"
    "        border_sdf_t = smoothstep(0.0, 2.0 * input.softness, border_sdf_s);\n"
    "    }\n"
    "    if (border_sdf_t < 0.001) {\n"
    "        discard;\n"
    "    }\n"
    "    \n"
    "    var corner_sdf_t = 1.0;\n"
    "    if (input.corner_radius > 0.0 || input.softness > 0.75) {\n"
    "        var corner_sdf_s = rect_sdf(\n"
    "            input.sdf_sample_pos,\n"
    "            input.rect_half_size_px - vec2<f32>(input.softness * 2.0, input.softness * 2.0),\n"
    "            input.corner_radius\n"
    "        );\n"
    "        corner_sdf_t = 1.0 - smoothstep(0.0, 2.0 * input.softness, corner_sdf_s);\n"
    "    }\n"
    "    \n"
    "    var final_color = albedo_sample;\n"
    "    final_color = final_color * input.tint;\n"
    "    final_color.a = final_color.a * push.opacity;\n"
    "    final_color.a = final_color.a * corner_sdf_t;\n"
    "    final_color.a = final_color.a * border_sdf_t;\n"
    "    return final_color;\n"
    "}\n";

global const char *webgpu_tri_shader_src =
    "@vertex\n"
    "fn vs_tri(@builtin(vertex_index) VertexIndex : u32) -> @builtin(position) vec4<f32> {\n"
    "    var vertices = array<vec2<f32>, 3>(\n"
    "        vec2<f32>(0.0, 0.5),\n"
    "        vec2<f32>(-0.5, -0.5),\n"
    "        vec2<f32>(0.5, -0.5)\n"
    "    );\n"
    "    return vec4<f32>(vertices[VertexIndex], 0.0, 1.0);\n"
    "}\n"
    "\n"
    "@fragment\n"
    "fn fs_tri() -> @location(0) vec4<f32> {\n"
    "    return vec4<f32>(1.0, 1.0, 0.0, 1.0);\n"
    "}\n";

//~ Helper Functions
internal WGPUStringView wgpu_str(const char *s)
{
    WGPUStringView r = {
        .data = s,
        .length = s ? strlen(s) : 0,
    };
    return r;
}

internal WGPUTextureFormat _webgpu_format_from_tex2d_format(Render_Tex_2D_Format format)
{
    switch(format)
    {
        case Render_Tex_2D_Format_R8:     return WGPUTextureFormat_R8Unorm;
        case Render_Tex_2D_Format_RG8:    return WGPUTextureFormat_RG8Unorm;
        case Render_Tex_2D_Format_RGBA8:  return WGPUTextureFormat_RGBA8Unorm;
        case Render_Tex_2D_Format_BGRA8:  return WGPUTextureFormat_BGRA8Unorm;
        case Render_Tex_2D_Format_R16:    return WGPUTextureFormat_R16Unorm;
        case Render_Tex_2D_Format_RGBA16: return WGPUTextureFormat_RGBA16Unorm;
        case Render_Tex_2D_Format_R32:    return WGPUTextureFormat_R32Float;
        case Render_Tex_2D_Format_RG32:   return WGPUTextureFormat_RG32Float;
        case Render_Tex_2D_Format_RGBA32: return WGPUTextureFormat_RGBA32Float;
        default:                         return WGPUTextureFormat_RGBA8Unorm;
    }
}

//~ Core Functions
internal void _webgpu_init_pipelines(WGPUTextureFormat format)
{
    // Compile rect shaders
    WGPUShaderSourceWGSL rect_wgsl = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_ShaderSourceWGSL,
        },
        .code = wgpu_str(webgpu_shader_src),
    };
    WGPUShaderModuleDescriptor rect_shader_desc = {
        .nextInChain = &rect_wgsl.chain,
        .label = wgpu_str("Rect Shader Source"),
    };
    WGPUShaderModule rect_shader = wgpuDeviceCreateShaderModule(_render_webgpu_state->device, &rect_shader_desc);
    
    // Compile tri shaders
    WGPUShaderSourceWGSL tri_wgsl = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_ShaderSourceWGSL,
        },
        .code = wgpu_str(webgpu_tri_shader_src),
    };
    WGPUShaderModuleDescriptor tri_shader_desc = {
        .nextInChain = &tri_wgsl.chain,
        .label = wgpu_str("Tri Shader Source"),
    };
    WGPUShaderModule tri_shader = wgpuDeviceCreateShaderModule(_render_webgpu_state->device, &tri_shader_desc);
    
    // Create rect pipeline
    WGPUVertexAttribute rect_attribs[8] = {
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, dst), .shaderLocation = 0 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, src), .shaderLocation = 1 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, colors[0]), .shaderLocation = 2 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, colors[1]), .shaderLocation = 3 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, colors[2]), .shaderLocation = 4 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, colors[3]), .shaderLocation = 5 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, corner_radii[0]), .shaderLocation = 6 },
        { .format = WGPUVertexFormat_Float32x4, .offset = offsetof(Render_Rect_2D_Inst, border_thickness), .shaderLocation = 7 },
    };

    WGPUVertexBufferLayout rect_buffer_layout = {
        .arrayStride = sizeof(Render_Rect_2D_Inst),
        .stepMode = WGPUVertexStepMode_Instance,
        .attributeCount = 8,
        .attributes = rect_attribs,
    };
    
    WGPUBlendState blend_state = {
        .color = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        },
        .alpha = {
            .operation = WGPUBlendOperation_Add,
            .srcFactor = WGPUBlendFactor_SrcAlpha,
            .dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
        },
    };
    
    WGPUColorTargetState color_target = {
        .format = format,
        .blend = &blend_state,
        .writeMask = WGPUColorWriteMask_All,
    };
    
    WGPUFragmentState fragment_state = {
        .module = rect_shader,
        .entryPoint = wgpu_str("fs_main"),
        .targetCount = 1,
        .targets = &color_target,
    };
    
    WGPURenderPipelineDescriptor rect_pipeline_desc = {
        .label = wgpu_str("Rect Pipeline"),
        .layout = _render_webgpu_state->pipeline_layout,
        .vertex = {
            .module = rect_shader,
            .entryPoint = wgpu_str("vs_main"),
            .bufferCount = 1,
            .buffers = &rect_buffer_layout,
        },
        .primitive = {
            .topology = WGPUPrimitiveTopology_TriangleStrip,
            .stripIndexFormat = WGPUIndexFormat_Undefined,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        .multisample = {
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = WGPU_FALSE,
        },
        .fragment = &fragment_state,
    };
    
    _render_webgpu_state->pipeline_rect = wgpuDeviceCreateRenderPipeline(_render_webgpu_state->device, &rect_pipeline_desc);
    
    // Create tri pipeline
    WGPUPipelineLayoutDescriptor pipeline_layout_desc_tri = {
        .label = wgpu_str("Tri Pipeline Layout"),
        .bindGroupLayoutCount = 0,
        .bindGroupLayouts = NULL,
    };
    WGPUPipelineLayout pipeline_layout_tri = wgpuDeviceCreatePipelineLayout(_render_webgpu_state->device, &pipeline_layout_desc_tri);
    
    WGPUColorTargetState color_target_tri = {
        .format = format,
        .blend = &blend_state,
        .writeMask = WGPUColorWriteMask_All,
    };
    
    WGPUFragmentState fragment_state_tri = {
        .module = tri_shader,
        .entryPoint = wgpu_str("fs_tri"),
        .targetCount = 1,
        .targets = &color_target_tri,
    };
    
    WGPURenderPipelineDescriptor tri_pipeline_desc = {
        .label = wgpu_str("Tri Pipeline"),
        .layout = pipeline_layout_tri,
        .vertex = {
            .module = tri_shader,
            .entryPoint = wgpu_str("vs_tri"),
            .bufferCount = 0,
            .buffers = NULL,
        },
        .primitive = {
            .topology = WGPUPrimitiveTopology_TriangleList,
            .stripIndexFormat = WGPUIndexFormat_Undefined,
            .frontFace = WGPUFrontFace_CCW,
            .cullMode = WGPUCullMode_None,
        },
        .multisample = {
            .count = 1,
            .mask = 0xFFFFFFFF,
            .alphaToCoverageEnabled = WGPU_FALSE,
        },
        .fragment = &fragment_state_tri,
    };
    
    _render_webgpu_state->pipeline_tri = wgpuDeviceCreateRenderPipeline(_render_webgpu_state->device, &tri_pipeline_desc);

    // Release shaders and transient layouts
    wgpuShaderModuleRelease(rect_shader);
    wgpuShaderModuleRelease(tri_shader);
    wgpuPipelineLayoutRelease(pipeline_layout_tri);
}

internal void render_init(void)
{
    Arena *arena = arena_alloc();
    _render_webgpu_state = arena_push(arena, _Render_Webgpu_State, 1);
    _render_webgpu_state->arena = arena;
    
    // Create Instance
    WGPUInstanceDescriptor instance_desc = {0};
    _render_webgpu_state->instance = wgpuCreateInstance(&instance_desc);
    if (!_render_webgpu_state->instance)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create WebGPU instance.");
        exit(1);
    }
    
    // Request Adapter
    WGPURequestAdapterOptions options = {
        .powerPreference = WGPUPowerPreference_HighPerformance,
        .backendType = WGPUBackendType_Vulkan, // Vulkan under the hood!
    };
    
    struct RequestAdapterData adapter_data = {0};
    WGPURequestAdapterCallbackInfo callback_info = {
        .mode = WGPUCallbackMode_WaitAnyOnly,
        .callback = _wgpu_request_adapter_callback,
        .userdata1 = &adapter_data,
    };
    
    WGPUFuture adapter_future = wgpuInstanceRequestAdapter(_render_webgpu_state->instance, &options, callback_info);
    WGPUFutureWaitInfo wait_info = {
        .future = adapter_future,
        .completed = false,
    };
    while (!adapter_data.completed) {
        wgpuInstanceWaitAny(_render_webgpu_state->instance, 1, &wait_info, 100000000); // 100ms
    }
    
    if (adapter_data.status != WGPURequestAdapterStatus_Success || !adapter_data.adapter)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to request WebGPU adapter.");
        exit(1);
    }
    _render_webgpu_state->adapter = adapter_data.adapter;
    
    // Request Device
    struct RequestDeviceData device_data = {0};
    WGPURequestDeviceCallbackInfo device_callback_info = {
        .mode = WGPUCallbackMode_WaitAnyOnly,
        .callback = _wgpu_request_device_callback,
        .userdata1 = &device_data,
    };
    WGPUDeviceDescriptor device_desc = {
        .label = wgpu_str("WGPU Device"),
    };
    WGPUFuture device_future = wgpuAdapterRequestDevice(_render_webgpu_state->adapter, &device_desc, device_callback_info);
    WGPUFutureWaitInfo device_wait_info = {
        .future = device_future,
        .completed = false,
    };
    while (!device_data.completed) {
        wgpuInstanceWaitAny(_render_webgpu_state->instance, 1, &device_wait_info, 100000000);
    }
    
    if (device_data.status != WGPURequestDeviceStatus_Success || !device_data.device)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to request WebGPU device.");
        exit(1);
    }
    _render_webgpu_state->device = device_data.device;
    _render_webgpu_state->queue = wgpuDeviceGetQueue(_render_webgpu_state->device);
    
    // Create Samplers
    WGPUSamplerDescriptor sampler_desc_nearest = {
        .label = wgpu_str("Nearest Sampler"),
        .addressModeU = WGPUAddressMode_ClampToEdge,
        .addressModeV = WGPUAddressMode_ClampToEdge,
        .addressModeW = WGPUAddressMode_ClampToEdge,
        .magFilter = WGPUFilterMode_Nearest,
        .minFilter = WGPUFilterMode_Nearest,
        .mipmapFilter = WGPUMipmapFilterMode_Nearest,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 32.0f,
        .compare = WGPUCompareFunction_Undefined,
        .maxAnisotropy = 1,
    };
    _render_webgpu_state->sampler_nearest = wgpuDeviceCreateSampler(_render_webgpu_state->device, &sampler_desc_nearest);

    WGPUSamplerDescriptor sampler_desc_linear = {
        .label = wgpu_str("Linear Sampler"),
        .addressModeU = WGPUAddressMode_ClampToEdge,
        .addressModeV = WGPUAddressMode_ClampToEdge,
        .addressModeW = WGPUAddressMode_ClampToEdge,
        .magFilter = WGPUFilterMode_Linear,
        .minFilter = WGPUFilterMode_Linear,
        .mipmapFilter = WGPUMipmapFilterMode_Nearest,
        .lodMinClamp = 0.0f,
        .lodMaxClamp = 32.0f,
        .compare = WGPUCompareFunction_Undefined,
        .maxAnisotropy = 1,
    };
    _render_webgpu_state->sampler_linear = wgpuDeviceCreateSampler(_render_webgpu_state->device, &sampler_desc_linear);
    
    // Create Bind Group Layouts
    WGPUBindGroupLayoutEntry bind_layout_entries_tex[2] = {
        {
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .texture = {
                .sampleType = WGPUTextureSampleType_Float,
                .viewDimension = WGPUTextureViewDimension_2D,
                .multisampled = WGPU_FALSE,
            },
        },
        {
            .binding = 1,
            .visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment,
            .buffer = {
                .type = WGPUBufferBindingType_Uniform,
                .hasDynamicOffset = WGPU_TRUE,
                .minBindingSize = sizeof(Webgpu_Push_Constants),
            },
        }
    };
    
    WGPUBindGroupLayoutDescriptor bind_layout_desc_tex = {
        .label = wgpu_str("Texture Bind Group Layout"),
        .entryCount = 2,
        .entries = bind_layout_entries_tex,
    };
    _render_webgpu_state->bind_group_layout_tex = wgpuDeviceCreateBindGroupLayout(_render_webgpu_state->device, &bind_layout_desc_tex);

    WGPUBindGroupLayoutEntry bind_layout_entries_sampler[1] = {
        {
            .binding = 0,
            .visibility = WGPUShaderStage_Fragment,
            .sampler = {
                .type = WGPUSamplerBindingType_Filtering,
            },
        }
    };
    
    WGPUBindGroupLayoutDescriptor bind_layout_desc_sampler = {
        .label = wgpu_str("Sampler Bind Group Layout"),
        .entryCount = 1,
        .entries = bind_layout_entries_sampler,
    };
    _render_webgpu_state->bind_group_layout_sampler = wgpuDeviceCreateBindGroupLayout(_render_webgpu_state->device, &bind_layout_desc_sampler);
    
    // Create Sampler Bind Groups
    WGPUBindGroupEntry sampler_entry_nearest = {
        .binding = 0,
        .sampler = _render_webgpu_state->sampler_nearest,
    };
    WGPUBindGroupDescriptor sampler_bg_desc_nearest = {
        .label = wgpu_str("Sampler Bind Group Nearest"),
        .layout = _render_webgpu_state->bind_group_layout_sampler,
        .entryCount = 1,
        .entries = &sampler_entry_nearest,
    };
    _render_webgpu_state->bind_group_sampler_nearest = wgpuDeviceCreateBindGroup(_render_webgpu_state->device, &sampler_bg_desc_nearest);

    WGPUBindGroupEntry sampler_entry_linear = {
        .binding = 0,
        .sampler = _render_webgpu_state->sampler_linear,
    };
    WGPUBindGroupDescriptor sampler_bg_desc_linear = {
        .label = wgpu_str("Sampler Bind Group Linear"),
        .layout = _render_webgpu_state->bind_group_layout_sampler,
        .entryCount = 1,
        .entries = &sampler_entry_linear,
    };
    _render_webgpu_state->bind_group_sampler_linear = wgpuDeviceCreateBindGroup(_render_webgpu_state->device, &sampler_bg_desc_linear);
    
    // Create Pipeline Layout
    WGPUBindGroupLayout layouts[2] = {
        _render_webgpu_state->bind_group_layout_tex,
        _render_webgpu_state->bind_group_layout_sampler,
    };
    
    WGPUPipelineLayoutDescriptor pipeline_layout_desc = {
        .label = wgpu_str("Rect Pipeline Layout"),
        .bindGroupLayoutCount = 2,
        .bindGroupLayouts = layouts,
    };
    _render_webgpu_state->pipeline_layout = wgpuDeviceCreatePipelineLayout(_render_webgpu_state->device, &pipeline_layout_desc);
    
    // Create Instance Buffer
    _render_webgpu_state->instance_buffer_size = MB(2);
    WGPUBufferDescriptor inst_buf_desc = {
        .label = wgpu_str("Instance Buffer"),
        .usage = WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst,
        .size = _render_webgpu_state->instance_buffer_size,
    };
    _render_webgpu_state->instance_buffer = wgpuDeviceCreateBuffer(_render_webgpu_state->device, &inst_buf_desc);
    
    // Create Dynamic Uniform Buffer for Push Constants (64KB)
    _render_webgpu_state->push_buffer_size = 64 * 1024;
    WGPUBufferDescriptor push_buf_desc = {
        .label = wgpu_str("Push Constants Buffer"),
        .usage = WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst,
        .size = _render_webgpu_state->push_buffer_size,
    };
    _render_webgpu_state->push_buffer = wgpuDeviceCreateBuffer(_render_webgpu_state->device, &push_buf_desc);
    
    // Allocate single white 1x1 texture
    uint32_t white_pixel = 0xFFFFFFFF;
    _render_webgpu_state->white_texture.resource_kind = Render_Resource_Kind_Static;
    _render_webgpu_state->white_texture.format = Render_Tex_2D_Format_RGBA8;
    _render_webgpu_state->white_texture.size = (Vec2_I32){1, 1};
    
    WGPUTextureDescriptor white_tex_desc = {
        .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        .dimension = WGPUTextureDimension_2D,
        .size = { 1, 1, 1 },
        .format = WGPUTextureFormat_RGBA8Unorm,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };
    _render_webgpu_state->white_texture.texture = wgpuDeviceCreateTexture(_render_webgpu_state->device, &white_tex_desc);
    
    WGPUTextureViewDescriptor white_view_desc = {
        .format = WGPUTextureFormat_RGBA8Unorm,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,
    };
    _render_webgpu_state->white_texture.texture_view = wgpuTextureCreateView(_render_webgpu_state->white_texture.texture, &white_view_desc);
    
    // Bind group for white texture
    WGPUBindGroupEntry white_bg_entries[2] = {
        {
            .binding = 0,
            .textureView = _render_webgpu_state->white_texture.texture_view,
        },
        {
            .binding = 1,
            .buffer = _render_webgpu_state->push_buffer,
            .offset = 0,
            .size = sizeof(Webgpu_Push_Constants),
        }
    };
    WGPUBindGroupDescriptor white_bg_desc = {
        .label = wgpu_str("White Texture Bind Group"),
        .layout = _render_webgpu_state->bind_group_layout_tex,
        .entryCount = 2,
        .entries = white_bg_entries,
    };
    _render_webgpu_state->white_texture.bind_group = wgpuDeviceCreateBindGroup(_render_webgpu_state->device, &white_bg_desc);
    
    // Upload white pixel
    WGPUTexelCopyTextureInfo destination = {
        .texture = _render_webgpu_state->white_texture.texture,
        .mipLevel = 0,
        .origin = { .x = 0, .y = 0, .z = 0 },
        .aspect = WGPUTextureAspect_All,
    };
    WGPUTexelCopyBufferLayout data_layout = {
        .offset = 0,
        .bytesPerRow = 4,
        .rowsPerImage = 1,
    };
    WGPUExtent3D write_size = { 1, 1, 1 };
    wgpuQueueWriteTexture(_render_webgpu_state->queue, &destination, &white_pixel, 4, &data_layout, &write_size);
}

internal void render_deinit(void)
{
    if (_render_webgpu_state)
    {
        if (_render_webgpu_state->sampler_nearest) wgpuSamplerRelease(_render_webgpu_state->sampler_nearest);
        if (_render_webgpu_state->sampler_linear) wgpuSamplerRelease(_render_webgpu_state->sampler_linear);
        if (_render_webgpu_state->bind_group_sampler_nearest) wgpuBindGroupRelease(_render_webgpu_state->bind_group_sampler_nearest);
        if (_render_webgpu_state->bind_group_sampler_linear) wgpuBindGroupRelease(_render_webgpu_state->bind_group_sampler_linear);
        if (_render_webgpu_state->bind_group_layout_tex) wgpuBindGroupLayoutRelease(_render_webgpu_state->bind_group_layout_tex);
        if (_render_webgpu_state->bind_group_layout_sampler) wgpuBindGroupLayoutRelease(_render_webgpu_state->bind_group_layout_sampler);
        if (_render_webgpu_state->pipeline_layout) wgpuPipelineLayoutRelease(_render_webgpu_state->pipeline_layout);
        if (_render_webgpu_state->pipeline_rect) wgpuRenderPipelineRelease(_render_webgpu_state->pipeline_rect);
        if (_render_webgpu_state->pipeline_tri) wgpuRenderPipelineRelease(_render_webgpu_state->pipeline_tri);
        if (_render_webgpu_state->instance_buffer) wgpuBufferRelease(_render_webgpu_state->instance_buffer);
        if (_render_webgpu_state->push_buffer) wgpuBufferRelease(_render_webgpu_state->push_buffer);
        
        if (_render_webgpu_state->white_texture.texture) wgpuTextureRelease(_render_webgpu_state->white_texture.texture);
        if (_render_webgpu_state->white_texture.texture_view) wgpuTextureViewRelease(_render_webgpu_state->white_texture.texture_view);
        if (_render_webgpu_state->white_texture.bind_group) wgpuBindGroupRelease(_render_webgpu_state->white_texture.bind_group);
        
        if (_render_webgpu_state->queue) wgpuQueueRelease(_render_webgpu_state->queue);
        if (_render_webgpu_state->device) wgpuDeviceRelease(_render_webgpu_state->device);
        if (_render_webgpu_state->adapter) wgpuAdapterRelease(_render_webgpu_state->adapter);
        if (_render_webgpu_state->instance) wgpuInstanceRelease(_render_webgpu_state->instance);
        
        arena_free(_render_webgpu_state->arena);
        _render_webgpu_state = NULL;
    }
}

internal void render_begin_frame(void)
{
}

internal void render_end_frame(void)
{
}

internal Render_Handle render_window_equip(Wl_Window window)
{
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    _Render_Webgpu_Window *w = _render_webgpu_state->free_window;
    if (w)
    {
        SLLStackPop(_render_webgpu_state->free_window);
    }
    else
    {
        w = arena_push(_render_webgpu_state->arena, _Render_Webgpu_Window, 1);
    }
    w->window = window;
    
    // Create Surface from XCB window
    WGPUSurfaceSourceXCBWindow xcb_source = {
        .chain = {
            .next = NULL,
            .sType = WGPUSType_SurfaceSourceXCBWindow,
        },
        .connection = _wl_x11_state->connection,
        .window = window_os->xwindow,
    };
    
    WGPUSurfaceDescriptor desc = {
        .nextInChain = &xcb_source.chain,
        .label = wgpu_str("WGPU XCB Surface"),
    };
    w->surface = wgpuInstanceCreateSurface(_render_webgpu_state->instance, &desc);
    if (!w->surface)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create WebGPU surface.");
        exit(1);
    }
    
    // Get capabilities
    WGPUSurfaceCapabilities capabilities = {0};
    wgpuSurfaceGetCapabilities(w->surface, _render_webgpu_state->adapter, &capabilities);
    w->format = capabilities.formats[0];
    wgpuSurfaceCapabilitiesFreeMembers(capabilities);
    
    // Configure surface
    Rng2_F32 canvas_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 canvas_rect_dim = dim2(canvas_rect);
    w->last_canvas_rect_dim = canvas_rect_dim;
    
    WGPUSurfaceConfiguration config = {
        .device = _render_webgpu_state->device,
        .format = w->format,
        .usage = WGPUTextureUsage_RenderAttachment,
        .width = (uint32_t)canvas_rect_dim.x,
        .height = (uint32_t)canvas_rect_dim.y,
        .alphaMode = WGPUCompositeAlphaMode_Opaque,
        .presentMode = WGPUPresentMode_Fifo,
    };
    wgpuSurfaceConfigure(w->surface, &config);
    
    // Initialize pipelines if needed
    if (_render_webgpu_state->pipeline_rect == NULL)
    {
        _webgpu_init_pipelines(w->format);
    }
    
    Render_Handle result = {(uint64_t)w};
    return result;
}

internal void render_window_unequip(Wl_Window window, Render_Handle window_equip)
{
    Unused(window);
    _Render_Webgpu_Window *w = (_Render_Webgpu_Window *)window_equip.u64[0];
    if (w->surface)
    {
        wgpuSurfaceUnconfigure(w->surface);
        wgpuSurfaceRelease(w->surface);
        w->surface = NULL;
    }
    SLLStackPush(_render_webgpu_state->free_window, w);
}

internal void render_window_begin_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Webgpu_Window *w = (_Render_Webgpu_Window *)handle.u64[0];
    
    Rng2_F32 canvas_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 canvas_rect_dim = dim2(canvas_rect);
    
    if (canvas_rect_dim.x != w->last_canvas_rect_dim.x || canvas_rect_dim.y != w->last_canvas_rect_dim.y)
    {
        w->last_canvas_rect_dim = canvas_rect_dim;
        WGPUSurfaceConfiguration config = {
            .device = _render_webgpu_state->device,
            .format = w->format,
            .usage = WGPUTextureUsage_RenderAttachment,
            .width = (uint32_t)canvas_rect_dim.x,
            .height = (uint32_t)canvas_rect_dim.y,
            .alphaMode = WGPUCompositeAlphaMode_Opaque,
            .presentMode = WGPUPresentMode_Fifo,
        };
        wgpuSurfaceConfigure(w->surface, &config);
    }
    
    wgpuSurfaceGetCurrentTexture(w->surface, &w->current_surface_texture);
    if ((w->current_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal &&
         w->current_surface_texture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) ||
        !w->current_surface_texture.texture)
    {
        w->current_view = NULL;
        return;
    }
    
    WGPUTextureViewDescriptor view_desc = {
        .format = w->format,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,
    };
    w->current_view = wgpuTextureCreateView(w->current_surface_texture.texture, &view_desc);
}

internal void render_window_end_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Webgpu_Window *w = (_Render_Webgpu_Window *)handle.u64[0];
    if (w->current_view)
    {
        wgpuTextureViewRelease(w->current_view);
        w->current_view = NULL;
    }
    if (w->current_surface_texture.texture)
    {
        wgpuSurfacePresent(w->surface);
        wgpuTextureRelease(w->current_surface_texture.texture);
        w->current_surface_texture.texture = NULL;
    }
}

internal void render_window_submit(Wl_Window window, Render_Handle window_equip, Render_Pass_List *passes)
{
    _Render_Webgpu_Window *w = (_Render_Webgpu_Window *)window_equip.u64[0];
    Rng2_F32 viewport_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 viewport_dim = dim2(viewport_rect);
    
    if (!w->current_view) return;
    
    // Copy all batch instances to mapped instance buffer
    size_t total_bytes = 0;
    for (Render_Pass_Node *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
        Render_Pass *pass = &pass_n->v;
        if (pass->kind == Render_Pass_Kind_UI)
        {
            Render_Pass_Params_UI *params = pass->params_ui;
            _Render_Batch_Group_2D_List *rect_batch_groups = &params->rects;
            for (_Render_Batch_Group_2D_Node *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
            {
                total_bytes += group_n->batches.byte_count;
            }
        }
    }
    
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    uint8_t *flat_instances = arena_push_nz(scratch.arena, uint8_t, total_bytes);
    
    size_t current_flat_offset = 0;
    for (Render_Pass_Node *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
        Render_Pass *pass = &pass_n->v;
        if (pass->kind == Render_Pass_Kind_UI)
        {
            Render_Pass_Params_UI *params = pass->params_ui;
            _Render_Batch_Group_2D_List *rect_batch_groups = &params->rects;
            for (_Render_Batch_Group_2D_Node *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
            {
                Render_Batch_List *batches = &group_n->batches;
                if (batches->byte_count == 0) continue;
                
                size_t batch_offset = 0;
                for (Render_Batch_Node *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
                {
                    memcpy(flat_instances + current_flat_offset + batch_offset, batch_n->v.v, batch_n->v.byte_count);
                    batch_offset += batch_n->v.byte_count;
                }
                current_flat_offset += batches->byte_count;
            }
        }
    }
    
    if (total_bytes > 0)
    {
        wgpuQueueWriteBuffer(_render_webgpu_state->queue, _render_webgpu_state->instance_buffer, 0, flat_instances, total_bytes);
    }
    arena_scratch_end(scratch);
    
    // Command Encoding & Render Pass
    WGPUCommandEncoderDescriptor encoder_desc = {
        .label = wgpu_str("WGPU Render Encoder"),
    };
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(_render_webgpu_state->device, &encoder_desc);
    
    WGPURenderPassColorAttachment color_attachment = {
        .view = w->current_view,
        .depthSlice = WGPU_DEPTH_SLICE_UNDEFINED,
        .resolveTarget = NULL,
        .loadOp = WGPULoadOp_Clear,
        .storeOp = WGPUStoreOp_Store,
        .clearValue = { 0.0, 0.0, 0.0, 0.0 },
    };
    
    WGPURenderPassDescriptor render_pass_desc = {
        .label = wgpu_str("WGPU Render Pass"),
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment,
        .depthStencilAttachment = NULL,
    };
    
    WGPURenderPassEncoder pass_encoder = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    wgpuRenderPassEncoderSetViewport(pass_encoder, 0.0f, 0.0f, (float)w->last_canvas_rect_dim.x, (float)w->last_canvas_rect_dim.y, 0.0f, 1.0f);
    wgpuRenderPassEncoderSetScissorRect(pass_encoder, 0, 0, (uint32_t)w->last_canvas_rect_dim.x, (uint32_t)w->last_canvas_rect_dim.y);
    
    size_t current_gpu_offset = 0;
    size_t batch_index = 0;
    
    for (Render_Pass_Node *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
        Render_Pass *pass = &pass_n->v;
        switch (pass->kind)
        {
            case Render_Pass_Kind_UI:
            {
                Render_Pass_Params_UI *params = pass->params_ui;
                _Render_Batch_Group_2D_List *rect_batch_groups = &params->rects;
                
                wgpuRenderPassEncoderSetPipeline(pass_encoder, _render_webgpu_state->pipeline_rect);
                
                for (_Render_Batch_Group_2D_Node *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
                {
                    Render_Batch_List *batches = &group_n->batches;
                    _Render_Batch_Group_2D_Params *group_params = &group_n->params;
                    
                    if (batches->byte_count == 0) continue;
                    
                    // Bind vertex buffer for instance data
                    wgpuRenderPassEncoderSetVertexBuffer(pass_encoder, 0, _render_webgpu_state->instance_buffer, current_gpu_offset, batches->byte_count);
                    current_gpu_offset += batches->byte_count;
                    
                    // Dynamic texture binding
                    Render_Tex_2D_Format texture_fmt = Render_Tex_2D_Format_RGBA8;
                    _Render_Webgpu_Tex_2D *tex = (_Render_Webgpu_Tex_2D *)group_params->tex.u64[0];
                    if (!tex)
                    {
                        tex = &_render_webgpu_state->white_texture;
                    }
                    texture_fmt = tex->format;
                    
                    // Prepare and write Push Constants
                    Webgpu_Push_Constants push = {0};
                    push.texture_sample_channel_map = render_sample_channel_map_from_tex2dformat(texture_fmt);
                    push.xform_col0 = (Vec4_F32){ group_params->xform.v[0][0], group_params->xform.v[0][1], group_params->xform.v[0][2], 0.0f };
                    push.xform_col1 = (Vec4_F32){ group_params->xform.v[1][0], group_params->xform.v[1][1], group_params->xform.v[1][2], 0.0f };
                    push.xform_col2 = (Vec4_F32){ group_params->xform.v[2][0], group_params->xform.v[2][1], group_params->xform.v[2][2], 0.0f };
                    push.viewport_size_px = viewport_dim;
                    push.opacity = 1.0f - group_params->transparency;
                    
                    size_t push_offset = batch_index * 256;
                    wgpuQueueWriteBuffer(_render_webgpu_state->queue, _render_webgpu_state->push_buffer, push_offset, &push, sizeof(push));
                    
                    // Bind texture group with dynamic uniform offset
                    uint32_t dynamic_offset = (uint32_t)push_offset;
                    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 0, tex->bind_group, 1, &dynamic_offset);
                    
                    // Bind sampler group
                    WGPUBindGroup sampler_bg = (group_params->tex_sample_kind == Render_Tex_2d_Sample_Kind_Linear) ? _render_webgpu_state->bind_group_sampler_linear : _render_webgpu_state->bind_group_sampler_nearest;
                    wgpuRenderPassEncoderSetBindGroup(pass_encoder, 1, sampler_bg, 0, NULL);
                    
                    // Scissor Test
                    if (group_params->clip.x0 != 0 || group_params->clip.x1 != 0 || group_params->clip.y0 != 0 || group_params->clip.y1 != 0)
                    {
                        int32_t clip_x = (int32_t)group_params->clip.x0;
                        int32_t clip_y = (int32_t)group_params->clip.y0;
                        int32_t clip_w = (int32_t)(group_params->clip.x1 - group_params->clip.x0);
                        int32_t clip_h = (int32_t)(group_params->clip.y1 - group_params->clip.y0);
                        if (clip_x < 0) clip_x = 0;
                        if (clip_y < 0) clip_y = 0;
                        if (clip_x + clip_w > (int32_t)w->last_canvas_rect_dim.x) {
                            clip_w = (int32_t)w->last_canvas_rect_dim.x - clip_x;
                        }
                        if (clip_y + clip_h > (int32_t)w->last_canvas_rect_dim.y) {
                            clip_h = (int32_t)w->last_canvas_rect_dim.y - clip_y;
                        }
                        
                        if (clip_w <= 0 || clip_h <= 0) continue; // nothing to draw
                        
                        wgpuRenderPassEncoderSetScissorRect(pass_encoder, (uint32_t)clip_x, (uint32_t)clip_y, (uint32_t)clip_w, (uint32_t)clip_h);
                    }
                    else
                    {
                        wgpuRenderPassEncoderSetScissorRect(pass_encoder, 0, 0, (uint32_t)w->last_canvas_rect_dim.x, (uint32_t)w->last_canvas_rect_dim.y);
                    }
                    
                    // Draw
                    uint32_t instance_count = (uint32_t)(batches->byte_count / batches->bytes_per_inst);
                    wgpuRenderPassEncoderDraw(pass_encoder, 4, instance_count, 0, 0);
                    
                    batch_index += 1;
                }
            }
            break;
            
            case Render_Pass_Kind_Triangle:
            {
                wgpuRenderPassEncoderSetPipeline(pass_encoder, _render_webgpu_state->pipeline_tri);
                wgpuRenderPassEncoderSetScissorRect(pass_encoder, 0, 0, (uint32_t)w->last_canvas_rect_dim.x, (uint32_t)w->last_canvas_rect_dim.y);
                wgpuRenderPassEncoderDraw(pass_encoder, 3, 1, 0, 0);
            }
            break;
            
            default:
                break;
        }
    }
    
    wgpuRenderPassEncoderEnd(pass_encoder);
    wgpuRenderPassEncoderRelease(pass_encoder);
    
    WGPUCommandBufferDescriptor cmd_buf_desc = {
        .label = wgpu_str("Render Command Buffer"),
    };
    WGPUCommandBuffer command_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buf_desc);
    wgpuCommandEncoderRelease(encoder);
    
    wgpuQueueSubmit(_render_webgpu_state->queue, 1, &command_buffer);
    wgpuCommandBufferRelease(command_buffer);
}

internal Render_Handle render_tex2d_alloc(Render_Resource_Kind kind, Render_Tex_2D_Format format, Vec2_I32 size, void *data, Arena *arena)
{
    _Render_Webgpu_Tex_2D *tex = _render_webgpu_state->free_tex2d;
    if (tex)
    {
        SLLStackPop(_render_webgpu_state->free_tex2d);
    }
    else
    {
        tex = arena_push(arena, _Render_Webgpu_Tex_2D, 1);
    }
    
    tex->resource_kind = kind;
    tex->format = format;
    tex->size = size;
    
    WGPUTextureFormat wgpu_format = _webgpu_format_from_tex2d_format(format);
    
    WGPUTextureDescriptor tex_desc = {
        .usage = WGPUTextureUsage_TextureBinding | WGPUTextureUsage_CopyDst,
        .dimension = WGPUTextureDimension_2D,
        .size = { (uint32_t)size.x, (uint32_t)size.y, 1 },
        .format = wgpu_format,
        .mipLevelCount = 1,
        .sampleCount = 1,
    };
    tex->texture = wgpuDeviceCreateTexture(_render_webgpu_state->device, &tex_desc);
    
    WGPUTextureViewDescriptor view_desc = {
        .format = wgpu_format,
        .dimension = WGPUTextureViewDimension_2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = WGPUTextureAspect_All,
    };
    tex->texture_view = wgpuTextureCreateView(tex->texture, &view_desc);
    
    // Create Bind Group for this texture
    WGPUBindGroupEntry bg_entries[2] = {
        {
            .binding = 0,
            .textureView = tex->texture_view,
        },
        {
            .binding = 1,
            .buffer = _render_webgpu_state->push_buffer,
            .offset = 0,
            .size = sizeof(Webgpu_Push_Constants),
        }
    };
    WGPUBindGroupDescriptor bg_desc = {
        .label = wgpu_str("Texture Bind Group"),
        .layout = _render_webgpu_state->bind_group_layout_tex,
        .entryCount = 2,
        .entries = bg_entries,
    };
    tex->bind_group = wgpuDeviceCreateBindGroup(_render_webgpu_state->device, &bg_desc);
    
    if (data)
    {
        size_t bpp = 4;
        switch (format)
        {
            case Render_Tex_2D_Format_R8:     bpp = 1; break;
            case Render_Tex_2D_Format_RG8:    bpp = 2; break;
            case Render_Tex_2D_Format_RGBA8:  bpp = 4; break;
            case Render_Tex_2D_Format_BGRA8:  bpp = 4; break;
            case Render_Tex_2D_Format_R16:    bpp = 2; break;
            case Render_Tex_2D_Format_RGBA16: bpp = 8; break;
            case Render_Tex_2D_Format_R32:    bpp = 4; break;
            case Render_Tex_2D_Format_RG32:   bpp = 8; break;
            case Render_Tex_2D_Format_RGBA32: bpp = 16; break;
            default: break;
        }
        
        WGPUTexelCopyTextureInfo destination = {
            .texture = tex->texture,
            .mipLevel = 0,
            .origin = { .x = 0, .y = 0, .z = 0 },
            .aspect = WGPUTextureAspect_All,
        };
        WGPUTexelCopyBufferLayout data_layout = {
            .offset = 0,
            .bytesPerRow = size.x * bpp,
            .rowsPerImage = size.y,
        };
        WGPUExtent3D write_size = { (uint32_t)size.x, (uint32_t)size.y, 1 };
        wgpuQueueWriteTexture(_render_webgpu_state->queue, &destination, data, size.x * size.y * bpp, &data_layout, &write_size);
    }
    
    Render_Handle result = {(uint64_t)tex};
    return result;
}

internal void render_tex2d_free(Render_Handle handle)
{
    _Render_Webgpu_Tex_2D *tex = (_Render_Webgpu_Tex_2D *)handle.u64[0];
    if (tex)
    {
        if (tex->bind_group)
        {
            wgpuBindGroupRelease(tex->bind_group);
            tex->bind_group = NULL;
        }
        if (tex->texture_view)
        {
            wgpuTextureViewRelease(tex->texture_view);
            tex->texture_view = NULL;
        }
        if (tex->texture)
        {
            wgpuTextureDestroy(tex->texture);
            wgpuTextureRelease(tex->texture);
            tex->texture = NULL;
        }
        SLLStackPush(_render_webgpu_state->free_tex2d, tex);
    }
}
