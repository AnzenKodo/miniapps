// ak: Global variables
//=============================================================================

// ak: shader source
read_only global Str8 _render_opengl_shader_rect_vert_src = str8(
    ""
    "\n"
    "#version 330 core\n"
    "\n"
    "in vec4 c2v_dst_rect;\n"
    "in vec4 c2v_src_rect;\n"
    "in vec4 c2v_colors_0;\n"
    "in vec4 c2v_colors_1;\n"
    "in vec4 c2v_colors_2;\n"
    "in vec4 c2v_colors_3;\n"
    "in vec4 c2v_corner_radii;\n"
    "in vec4 c2v_style;  // x: border_thickness_px, y: softness_px, z: omit_texture, w: shear\n"
    "\n"
    "out vec2 v2p_sdf_sample_pos;\n"
    "out vec2 v2p_texcoord_pct;\n"
    "out vec2 v2p_rect_half_size_px;\n"
    "out vec4 v2p_tint;\n"
    "out float v2p_corner_radius;\n"
    "out float v2p_border_thickness;\n"
    "out float v2p_softness;\n"
    "out float v2p_omit_texture;\n"
    "\n"
    "uniform mat3 u_xform;\n"
    "uniform sampler2D u_tex_color;\n"
    "uniform vec2 u_viewport_size_px;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  // ak: constants\n"
    "  vec2 vertices[] = vec2[](vec2(-1, -1), vec2(-1, +1), vec2(+1, -1), vec2(+1, +1));\n"
    "  \n"
    "  // ak: unpack shears\n"
    "  float shears[] = float[](0, 0, c2v_style.w, c2v_style.w);\n"
    "  \n"
    "  // ak: find dst position\n"
    "  vec2 dst_half_size = (c2v_dst_rect.zw - c2v_dst_rect.xy) / 2;\n"
    "  vec2 dst_center    = (c2v_dst_rect.zw + c2v_dst_rect.xy) / 2;\n"
    "  vec2 dst_position  = vertices[gl_VertexID] * dst_half_size + dst_center;\n"
    "  dst_position.y += shears[gl_VertexID];\n"
    "  dst_position = (u_xform * vec3(dst_position.x, dst_position.y, 1)).xy;\n"
    "  \n"
    "  // ak: find src position\n"
    "  vec2 src_half_size = (c2v_src_rect.zw - c2v_src_rect.xy) / 2;\n"
    "  vec2 src_center    = (c2v_src_rect.zw + c2v_src_rect.xy) / 2;\n"
    "  vec2 src_position  = vertices[gl_VertexID] * src_half_size + src_center;\n"
    "  \n"
    "  // ak: find color\n"
    "  vec4 colors[] = vec4[](c2v_colors_0, c2v_colors_1, c2v_colors_2, c2v_colors_3);\n"
    "  vec4 color = colors[gl_VertexID];\n"
    "  \n"
    "  // ak: find corner radius\n"
    "  float corner_radii[] = float[](c2v_corner_radii.x, c2v_corner_radii.y, c2v_corner_radii.z, c2v_corner_radii.w);\n"
    "  float corner_radius = corner_radii[gl_VertexID];\n"
    "  \n"
    "  // ak: fill outputs\n"
    "  vec2 dst_verts_pct = vec2(((gl_VertexID >> 1) != 1) ? 1.f : 0.f,\n"
    "                            ((gl_VertexID & 1) != 0)  ? 0.f : 1.f);\n"
    "  ivec2 u_tex_color_size_i = textureSize(u_tex_color, 0);\n"
    "  vec2 u_tex_color_size = vec2(float(u_tex_color_size_i.x), float(u_tex_color_size_i.y));\n"
    "  {\n"
    "    gl_Position = vec4(2 * dst_position.x / u_viewport_size_px.x - 1,\n"
    "                       2 * (1 - dst_position.y / u_viewport_size_px.y) - 1,\n"
    "                       0.0, 1.0);\n"
    "    v2p_sdf_sample_pos    = (2.f * dst_verts_pct - 1.f) * dst_half_size;\n"
    "    v2p_texcoord_pct      = src_position / u_tex_color_size;\n"
    "    v2p_rect_half_size_px = dst_half_size;\n"
    "    v2p_tint              = color;\n"
    "    v2p_corner_radius     = corner_radius;\n"
    "    v2p_border_thickness  = c2v_style.x;\n"
    "    v2p_softness          = c2v_style.y;\n"
    "    v2p_omit_texture      = c2v_style.z;\n"
    "  }\n"
    "}\n"
    ""
);
read_only global Str8 _render_opengl_shader_rect_frag_src = str8(
    ""
    "\n"
    "#version 330 core\n"
    "\n"
    "in vec2 v2p_sdf_sample_pos;\n"
    "in vec2 v2p_texcoord_pct;\n"
    "in vec2 v2p_rect_half_size_px;\n"
    "in vec4 v2p_tint;\n"
    "in float v2p_corner_radius;\n"
    "in float v2p_border_thickness;\n"
    "in float v2p_softness;\n"
    "in float v2p_omit_texture;\n"
    "\n"
    "out vec4 final_color;\n"
    "\n"
    "uniform float u_opacity;\n"
    "uniform sampler2D u_tex_color;\n"
    "uniform mat4 u_texture_sample_channel_map;\n"
    "\n"
    "float rect_sdf(vec2 sample_pos, vec2 rect_half_size, float r)\n"
    "{\n"
    "  return length(max(abs(sample_pos) - rect_half_size + r, 0.0)) - r;\n"
    "}\n"
    "\n"
    "float linear_from_srgb_f32(float x)\n"
    "{\n"
    "  return x < 0.0404482362771082 ? x / 12.92 : pow((x + 0.055) / 1.055, 2.4);\n"
    "}\n"
    "\n"
    "vec4 linear_from_srgba(vec4 v)\n"
    "{\n"
    "  vec4 result = vec4(linear_from_srgb_f32(v.x),\n"
    "                     linear_from_srgb_f32(v.y),\n"
    "                     linear_from_srgb_f32(v.z),\n"
    "                     v.w);\n"
    "  return result;\n"
    "}\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  // ak: sample texture\n"
    "  vec4 albedo_sample = vec4(1, 1, 1, 1);\n"
    "  if(v2p_omit_texture < 1)\n"
    "  {\n"
    "    albedo_sample = u_texture_sample_channel_map * texture(u_tex_color, v2p_texcoord_pct);\n"
    "    albedo_sample = linear_from_srgba(albedo_sample);\n"
    "  }\n"
    "  \n"
    "  // ak: sample for borders\n"
    "  float border_sdf_t = 1;\n"
    "  if(v2p_border_thickness > 0)\n"
    "  {\n"
    "    float border_sdf_s = rect_sdf(v2p_sdf_sample_pos,\n"
    "                                  v2p_rect_half_size_px - vec2(v2p_softness*2.f, v2p_softness*2.f) - v2p_border_thickness,\n"
    "                                  max(v2p_corner_radius-v2p_border_thickness, 0));\n"
    "    border_sdf_t = smoothstep(0, 2*v2p_softness, border_sdf_s);\n"
    "  }\n"
    "  if(border_sdf_t < 0.001f)\n"
    "  {\n"
    "    discard;\n"
    "  }\n"
    "  \n"
    "  // ak: sample for corners\n"
    "  float corner_sdf_t = 1;\n"
    "  if(v2p_corner_radius > 0 || v2p_softness > 0.75f)\n"
    "  {\n"
    "    float corner_sdf_s = rect_sdf(v2p_sdf_sample_pos,\n"
    "                                  v2p_rect_half_size_px - vec2(v2p_softness*2.f, v2p_softness*2.f),\n"
    "                                  v2p_corner_radius);\n"
    "    corner_sdf_t = 1-smoothstep(0, 2*v2p_softness, corner_sdf_s);\n"
    "  }\n"
    "  \n"
    "  // ak: form+return final color\n"
    "  final_color = albedo_sample;\n"
    "  final_color *= v2p_tint;\n"
    "  final_color.a *= u_opacity;\n"
    "  final_color.a *= corner_sdf_t;\n"
    "  final_color.a *= border_sdf_t;\n"
    "}\n"
    ""
);

global read_only _Render_Opengl_Attribute _render_opengl_rect_input_attributes[] =
{
    {0, str8("c2v_dst_rect"),        GL_FLOAT,  4},
    {1, str8("c2v_src_rect"),        GL_FLOAT,  4},
    {2, str8("c2v_colors_0"),        GL_FLOAT,  4},
    {3, str8("c2v_colors_1"),        GL_FLOAT,  4},
    {4, str8("c2v_colors_2"),        GL_FLOAT,  4},
    {5, str8("c2v_colors_3"),        GL_FLOAT,  4},
    {6, str8("c2v_corner_radii"),    GL_FLOAT,  4},
    {7, str8("c2v_style"),           GL_FLOAT,  4},
};
global read_only _Render_Opengl_Attribute _render_opengl_single_color_output_attributes[] =
{
    {0, str8("final_color"), 0, 0},
};

// ak: shader source table
Str8 *_render_opengl_shader_kind_vert_src_table[_Render_Opengl_Shader_Kind_COUNT] = {
    &_render_opengl_shader_rect_vert_src,
};
Str8 *_render_opengl_shader_kind_frag_src_table[_Render_Opengl_Shader_Kind_COUNT] = {
    &_render_opengl_shader_rect_frag_src,
};

// ak: shader input and output table
_Render_Opengl_Attribute_Array _render_opengl_shader_kind_input_attributes_table[_Render_Opengl_Shader_Kind_COUNT] =
{
    { _render_opengl_rect_input_attributes, ArrayLength(_render_opengl_rect_input_attributes) },
};
_Render_Opengl_Attribute_Array _render_opengl_shader_kind_output_attributes_table[_Render_Opengl_Shader_Kind_COUNT] =
{
    { _render_opengl_single_color_output_attributes, ArrayLength(_render_opengl_single_color_output_attributes) },
};

// ak: Helpers
//=============================================================================

// ak: General helper functions

internal GLuint _render_opengl_instance_buffer_from_size(size_t size)
{
    GLuint buffer = _render_opengl_state->scratch_buffer_64kb;
    if (size > KB(64))
    {
        // ak: build buffer
        size_t flushed_buffer_size = size;
        flushed_buffer_size += MB(1)-1;
        flushed_buffer_size -= flushed_buffer_size%MB(1);
        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, flushed_buffer_size, 0, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // ak: push buffer to flush list
        _Render_Opengl_FlushBuffer *n = arena_push(_render_opengl_state->buffer_flush_arena, _Render_Opengl_FlushBuffer, 1);
        n->id = buffer;
        SLLQueuePush(_render_opengl_state->first_buffer_to_flush, _render_opengl_state->last_buffer_to_flush, n);
    }
    return buffer;
}

internal bool _render_opengl_scissor(Rng2_F32 clip, Vec2_F32 viewport_dim)
{
    int32_t x0 = Clamp(0, (int32_t)floor_f32(clip.x0), (int32_t)viewport_dim.x);
    int32_t y0 = Clamp(0, (int32_t)floor_f32(clip.y0), (int32_t)viewport_dim.y);
    int32_t x1 = Clamp(0, (int32_t)ceil_f32(clip.x1), (int32_t)viewport_dim.x);
    int32_t y1 = Clamp(0, (int32_t)ceil_f32(clip.y1), (int32_t)viewport_dim.y);
    int32_t width = x1 - x0;
    int32_t height = y1 - y0;
    if (width > 0 && height > 0)
    {
        glScissor(x0, (int32_t)viewport_dim.y - y1, width, height);
    }
    return width > 0 && height > 0;
}

internal void _render_opengl_debug_message_callback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
{
    Unused(source);
    Unused(type);
    Unused(id);
    Unused(severity);
    Unused(userParam);
    fmt_eprintf("[OpenGL] %.*s\n", (int)length, message);
}

// ak: texture helper functions

internal Render_Handle _render_opengl_handle_from_tex2d(_Render_Opengl_Tex_2D *tex2d)
{
    Render_Handle handle = {(uintptr_t)tex2d};
    return handle;
}

internal _Render_Opengl_Tex_2D *_render_opengl_tex2d_from_handle(Render_Handle handle)
{
  _Render_Opengl_Tex_2D *tex2d = (_Render_Opengl_Tex_2D *)handle.u64[0];
  return tex2d;
}

internal _Render_Opengl_Format_Info _render_opengl_format_info_from_tex2d_format(Render_Tex_2D_Format format)
{
    _Render_Opengl_Format_Info result = STRUCT_ZERO;
    switch (format)
    {
        case Render_Tex_2D_Format_R8:
        {
            result.internal_format = GL_R8;
            result.format = GL_RED;
            result.base_type = GL_UNSIGNED_BYTE;
        } break;
        case Render_Tex_2D_Format_RG8:
        {
            result.internal_format = GL_RG8;
            result.format = GL_RG;
            result.base_type = GL_UNSIGNED_BYTE;
        } break;
        case Render_Tex_2D_Format_RGBA8:
        {
            result.internal_format = GL_RGBA8;
            result.format = GL_RGBA;
            result.base_type = GL_UNSIGNED_BYTE;
        } break;
        case Render_Tex_2D_Format_BGRA8:
        {
            result.internal_format = GL_RGBA8;
            result.format = GL_BGRA;
            result.base_type = GL_UNSIGNED_BYTE;
        } break;
        case Render_Tex_2D_Format_R16:
        {
            result.internal_format = GL_R16;
            result.format = GL_RED;
            result.base_type = GL_UNSIGNED_SHORT;
        } break;
        case Render_Tex_2D_Format_RGBA16:
        {
            result.internal_format = GL_RGBA16;
            result.format = GL_RGBA;
            result.base_type = GL_UNSIGNED_SHORT;
        } break;
        case Render_Tex_2D_Format_R32:
        {
            result.internal_format = GL_R32F;
            result.format = GL_RED;
            result.base_type = GL_FLOAT;
        } break;
        case Render_Tex_2D_Format_RG32:
        {
            result.internal_format = GL_RG32F;
            result.format = GL_RG;
            result.base_type = GL_FLOAT;
        } break;
        case Render_Tex_2D_Format_RGBA32:
        {
            result.internal_format = GL_RGBA32F;
            result.format = GL_RGBA;
            result.base_type = GL_FLOAT;
        } break;
        case Render_Tex_2D_Format_COUNT:
        {
        } break;
    }
    return result;
}

// Core functions
//=============================================================================

internal void render_init(void)
{
    // ak: do os-specific portion of work
    _render_opengl_init();
    
    // ak: top-level initialization
    Arena *arena = arena_alloc();
    _render_opengl_state = arena_push(arena, _Render_Opengl_State, 1);
    _render_opengl_state->arena = arena;
    
    // ak: load opengl procedures
#define X(name, r, p) name = (name##_FunctionType *)(void*)_render_opengl_load_procedure(#name);
    RenderOpenglXMacro
#undef X
#ifdef BUILD_DEBUG
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(_render_opengl_debug_message_callback, 0);
#endif
    
    // ak: build all shaders
    for EachEnumVal(_Render_Opengl_Shader_Kind, k)
    {
        // ak: compile
        struct {
            GLenum type;
            Str8 *src;
            GLuint out;
            Str8 errors;
        } stages[] = {
            {
                GL_VERTEX_SHADER,
                _render_opengl_shader_kind_vert_src_table[k],
                0,
                str8_zero()
            },
            {
                GL_FRAGMENT_SHADER,
                _render_opengl_shader_kind_frag_src_table[k],
                0,
                str8_zero()
            },
        };
        for EachElement(idx, stages)
        {
            stages[idx].out = glCreateShader(stages[idx].type);
            GLint src_size = stages[idx].src->size;
#if OS_LINUX
            glShaderSource(stages[idx].out, 1, (const char * const*)&stages[idx].src->cstr, &src_size);
#else
            glShaderSource(stages[idx].out, 1, (char **)&stages[idx].src->cstr, &src_size);
#endif
            glCompileShader(stages[idx].out);
            GLint info_log_length = 0;
            GLint status = 0;
            glGetShaderiv(stages[idx].out, GL_COMPILE_STATUS, &status);
            glGetShaderiv(stages[idx].out, GL_INFO_LOG_LENGTH, &info_log_length);
            if(info_log_length != 0)
            {
                stages[idx].errors.cstr = arena_push(_render_opengl_state->arena, uint8_t, info_log_length+1);
                stages[idx].errors.size = info_log_length;
                glGetShaderInfoLog(stages[idx].out, info_log_length, 0, (char *)stages[idx].errors.cstr);
                LogErrorLine(&_os_core_state.log_context, "%.*s", info_log_length, stages[idx].errors.cstr);
            }
        }
        
        // ak: attach compilations to program
        GLuint program = glCreateProgram();
        for EachElement(idx, stages)
        {
            glAttachShader(program, stages[idx].out);
        }
        
        // ak: bind inputs
        _Render_Opengl_Attribute_Array inputs = _render_opengl_shader_kind_input_attributes_table[k];
        for EachIndex(idx, inputs.count)
        {
            glBindAttribLocation(program, inputs.v[idx].index, (char *)inputs.v[idx].name.cstr);
        }
        
        // ak: bind outputs
        _Render_Opengl_Attribute_Array outputs = _render_opengl_shader_kind_output_attributes_table[k];
        for EachIndex(idx, outputs.count)
        {
            glBindFragDataLocation(program, outputs.v[idx].index, (char *)outputs.v[idx].name.cstr);
        }
        
        // ak: link / validate / store
        {
            glLinkProgram(program);
            glValidateProgram(program);
            GLint status;
            glGetProgramiv(program, GL_LINK_STATUS, &status);
            if (!status)
            {
                char log[512];
                glGetProgramInfoLog(program, 512, NULL, log);
                LogErrorLine(&_os_core_state.log_context, "%s", log);
            }
            _render_opengl_state->shaders[k] = program;
        }
    }
    
    // ak: set up built-in resources
    {
        // ak: all-purpose VAO
        glGenVertexArrays(1, &_render_opengl_state->all_purpose_vao);
        
        // ak: scratch buffer
        {
            glGenBuffers(1, &_render_opengl_state->scratch_buffer_64kb);
            glBindBuffer(GL_ARRAY_BUFFER, _render_opengl_state->scratch_buffer_64kb);
            glBufferData(GL_ARRAY_BUFFER, KB(64), 0, GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
        }
        
        // ak: white texture
        {
            glGenTextures(1, &_render_opengl_state->white_texture);
            glBindTexture(GL_TEXTURE_2D, _render_opengl_state->white_texture);
            uint32_t white_pixel = 0xffffffff;
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, &white_pixel);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
    
    // ak: set up options
    glEnable(GL_FRAMEBUFFER_SRGB);
    
    // ak: set up buffer flush state
    _render_opengl_state->buffer_flush_arena = arena_alloc();
}

internal void render_cleanup(void)
{
    _render_opengl_cleanup();
}

internal void render_begin_frame(void)
{
}

internal void render_end_frame(void)
{
    for(_Render_Opengl_FlushBuffer *flush_buffer = _render_opengl_state->first_buffer_to_flush; flush_buffer != 0; flush_buffer = flush_buffer->next)
    {
        glDeleteBuffers(1, &flush_buffer->id);
    }
    arena_clear(_render_opengl_state->buffer_flush_arena);
    _render_opengl_state->first_buffer_to_flush = _render_opengl_state->last_buffer_to_flush = 0;
}

internal Render_Handle render_window_equip(Wl_Window window)
{
    _Render_Opengl_Window *window_opengl = _render_opengl_state->free_window;
    if(window_opengl != 0)
    {
        SLLStackPop(_render_opengl_state->free_window);
    }
    else
    {
        window_opengl = arena_push_nz(_render_opengl_state->arena, _Render_Opengl_Window, 1);
    }
    MemSetZeroStruct(window_opengl);
    window_opengl->window_os = _render_opengl_window_equip(window);
    Render_Handle result = {(uint64_t)window_opengl};
    return result;
}

internal void render_window_unequip(Wl_Window window, Render_Handle window_equip)
{
    _Render_Opengl_Window *window_opengl = (_Render_Opengl_Window *)window_equip.u64[0];
    _render_opengl_window_unequip(window, window_opengl->window_os);
    SLLStackPush(_render_opengl_state->free_window, window_opengl);
}

internal void render_window_begin_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Opengl_Window *window_opengl = (_Render_Opengl_Window *)handle.u64[0];
    _render_opengl_select_window(window, window_opengl->window_os);
    
    // ak: unpack window viewport info
    Rng2_F32 canvas_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 canvas_rect_dim = dim_rng2(canvas_rect);
    
    // ak: set up targets if needed
    if(canvas_rect_dim.x != window_opengl->last_canvas_rect_dim.x || canvas_rect_dim.y != window_opengl->last_canvas_rect_dim.y)
    {
        window_opengl->last_canvas_rect_dim = canvas_rect_dim;
        _Render_Opengl_RenderTarget *targets[] =
        {
            &window_opengl->stage_target,
            &window_opengl->stage_scratch_target,
        };
        for EachElement(idx, targets)
        {
            if(targets[idx]->fbo != 0)
            {
                glDeleteFramebuffers(1, &targets[idx]->fbo);
                glDeleteTextures(1, &targets[idx]->color_texture);
            }
            glGenFramebuffers(1, &targets[idx]->fbo);
            glGenTextures(1, &targets[idx]->color_texture);
            glBindFramebufferScope(GL_FRAMEBUFFER, targets[idx]->fbo)
            glBindTextureScope(GL_TEXTURE_2D, targets[idx]->color_texture)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLint)canvas_rect_dim.x, (GLint)canvas_rect_dim.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
                glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, targets[idx]->color_texture, 0);
            }
        }
    }
    
    // ak: clear and reset state
    {
        GLuint fbos[] =
        {
            window_opengl->stage_target.fbo,
            window_opengl->stage_scratch_target.fbo,
            0,
        };
        for EachElement(idx, fbos)
        {
            glBindFramebufferScope(GL_FRAMEBUFFER, fbos[idx])
            {
                GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
                glClearColor(1.f, 0.f, 1.f, 1.f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            }
        }
    }
    glViewport(0, 0, (GLsizei)canvas_rect_dim.x, (GLsizei)canvas_rect_dim.y);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

internal void render_window_end_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Opengl_Window *window_opengl = (_Render_Opengl_Window *)handle.u64[0];
    
    // ak: blit window's main staging frame buffer to window frame buffer
    glBindFramebufferScope(GL_READ_FRAMEBUFFER, window_opengl->stage_target.fbo)
    glBindFramebufferScope(GL_DRAW_FRAMEBUFFER, 0)
    {
        glBlitFramebuffer(0, 0, (GLint)window_opengl->last_canvas_rect_dim.x, (GLint)window_opengl->last_canvas_rect_dim.y,
                0, 0, (GLint)window_opengl->last_canvas_rect_dim.x, (GLint)window_opengl->last_canvas_rect_dim.y,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST);
    }
    
    // ak: swap buffers
    _render_opengl_window_swap(window, window_opengl->window_os);
}

internal void render_window_submit(Wl_Window window, Render_Handle window_equip, Render_Pass_List *passes)
{
    _Render_Opengl_Window *w = (_Render_Opengl_Window *)window_equip.u64[0];
    Rng2_F32 viewport_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 viewport_dim = dim_rng2(viewport_rect);
    for(Render_Pass_Node *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
        Render_Pass *pass = &pass_n->v;
        switch(pass->kind)
        {
            // ak: ui rendering pass
            case Render_Pass_Kind_UI:
            {
                // ak: unpack params
                Render_Pass_Params_UI *params = pass->params_ui;
                Render_Batch_Group_2D_List *rect_batch_groups = &params->rects;
                
                // ak: draw each batch group
                GLuint shader = _render_opengl_state->shaders[_Render_Opengl_Shader_Kind_Rect];
                glUseProgramScope(shader)
                glBindVertexArrayScope(_render_opengl_state->all_purpose_vao)
                glBindFramebufferScope(GL_FRAMEBUFFER, w->stage_target.fbo)
                {
                    for(Render_Batch_Group_2D_Node *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
                    {
                        Render_Batch_List *batches = &group_n->batches;
                        Render_Batch_Group_2D_Params *group_params = &group_n->params;
                        
                        // ak: unpack texture
                        Render_Tex_2D_Format texture_fmt = Render_Tex_2D_Format_RGBA8;
                        GLuint texture_id = _render_opengl_state->white_texture;
                        {
                            _Render_Opengl_Tex_2D *tex = _render_opengl_tex2d_from_handle(group_params->tex);
                            if(tex != 0)
                            {
                                texture_id = tex->id;
                                texture_fmt = tex->format;
                            }
                        }
                        
                        // ak: get & fill buffer
                        GLuint buffer = _render_opengl_instance_buffer_from_size(batches->byte_count);
                        {
                            glBindBuffer(GL_ARRAY_BUFFER, buffer);
                            size_t off = 0;
                            for(Render_Batch_Node *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
                            {
                                glBufferSubData(GL_ARRAY_BUFFER, off, batch_n->v.byte_count, batch_n->v.v);
                                off += batch_n->v.byte_count;
                            }
                        }
                        
                        // ak: bind input attributes
                        {
                            _Render_Opengl_Attribute_Array inputs = _render_opengl_shader_kind_input_attributes_table[_Render_Opengl_Shader_Kind_Rect];
                            size_t off = 0;
                            for EachIndex(idx, inputs.count)
                            {
                                glEnableVertexAttribArray(inputs.v[idx].index);
                                glVertexAttribDivisor(inputs.v[idx].index, 1);
                                glVertexAttribPointer(inputs.v[idx].index, inputs.v[idx].count, inputs.v[idx].type, GL_FALSE, sizeof(Render_Rect_2D_Inst), (void *)(off));
                                // TODO(ak): this is not correct if type != GL_FLOAT
                                off += inputs.v[idx].count*sizeof(GL_FLOAT);
                            }
                        }
                        
                        // ak: bind texture
                        {
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, texture_id);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                            switch(group_params->tex_sample_kind)
                            {
                                default:
                                case Render_Tex_2D_Sample_Kind_Nearest:
                                {
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                                }break;
                                case Render_Tex_2D_Sample_Kind_Linear:
                                {
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                                }break;
                            }
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
                            glUniform1i(glGetUniformLocation(shader, "u_tex_color"), 0);
                        }
                        
                        // ak: upload misc. uniforms
                        {
                            Mat4x4_F32 texture_sample_channel_map = render_sample_channel_map_from_tex2dformat(texture_fmt);
                            glUniformMatrix4fv(glGetUniformLocation(shader, "u_texture_sample_channel_map"), 1, 0, &texture_sample_channel_map.v[0][0]);
                            glUniform2f(glGetUniformLocation(shader, "u_viewport_size_px"), viewport_dim.x, viewport_dim.y);
                            glUniform1f(glGetUniformLocation(shader, "u_opacity"), 1.f - group_params->transparency);
                            glUniformMatrix3fv(glGetUniformLocation(shader, "u_xform"), 1, 0, &group_params->xform.v[0][0]);
                        }
                        
                        // ak: set up scissor
                        if(group_params->clip.x0 != 0 ||
                            group_params->clip.x1 != 0 ||
                            group_params->clip.y0 != 0 ||
                            group_params->clip.y1 != 0)
                        {
                            if(_render_opengl_scissor(group_params->clip, viewport_dim))
                            {
                                glEnable(GL_SCISSOR_TEST);
                            }
                        }
                        
                        // ak: draw
                        {
                            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, batches->byte_count / batches->bytes_per_inst);
                        }
                        
                        // ak: unset scissor
                        glDisable(GL_SCISSOR_TEST);
                    }
                }
            } break;
            case Render_Pass_Kind_COUNT:
            {
            } break;
        }
    }
}

// ak: Texture functions
//=============================================================================

internal Render_Handle render_tex2d_alloc(Render_Resource_Kind kind, Render_Tex_2D_Format format, Vec2_I32 size, void *data)
{
    // ak: allocate texture record
    _Render_Opengl_Tex_2D *tex2d = _render_opengl_state->free_tex2d;
    if(tex2d)
    {
        SLLStackPop(_render_opengl_state->free_tex2d);
    }
    else
    {
        tex2d = arena_push(_render_opengl_state->arena, _Render_Opengl_Tex_2D, 1);
    }
    
    // ak: map kind/format -> gl counterparts
    _Render_Opengl_Format_Info format_info = _render_opengl_format_info_from_tex2d_format(format);
    
    // ak: allocate GL texture
    glGenTextures(1, &tex2d->id);
    glBindTextureScope(GL_TEXTURE_2D, tex2d->id)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, format_info.internal_format, size.x, size.y, 0, format_info.format, format_info.base_type, data);
    }
    
    // ak: fill texture data
    tex2d->resource_kind = kind;
    tex2d->format = format;
    tex2d->size = size;
    
    // ak: bundle & return
    Render_Handle result = _render_opengl_handle_from_tex2d(tex2d);
    return result;
}

internal void render_tex2d_free(Render_Handle handle)
{
    _Render_Opengl_Tex_2D *tex2d = _render_opengl_tex2d_from_handle(handle);
    if (tex2d != 0)
    {
        glDeleteTextures(1, &tex2d->id);
        SLLStackPush(_render_opengl_state->free_tex2d, tex2d);
    }
}

internal void render_fill_tex2d_region(Render_Handle texture, Rng2_I32 subrect, void *data)
{
  _Render_Opengl_Tex_2D *tex = _render_opengl_tex2d_from_handle(texture);
  if(tex)
  {
    _Render_Opengl_Format_Info fmt_info = _render_opengl_format_info_from_tex2d_format(tex->format);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    Vec2_I32 rect_size = dim_rng2(subrect);
    glTexSubImage2D(GL_TEXTURE_2D, 0, subrect.x0, subrect.y0, rect_size.x, rect_size.y, fmt_info.format, fmt_info.base_type, data);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}
