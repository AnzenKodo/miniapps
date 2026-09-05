// render_sokol.c
//
// Full sokol_gfx render backend implementing the render_core.h interface.
//
// Backends:
//   Linux   -> SOKOL_GLCORE (GL 3.3 Core via existing EGL context,
//              identical to the OpenGL backend's context setup)
//   Windows -> SOKOL_D3D11  (D3D11 device/context created internally)
//
// Rendering is a faithful port of render_opengl.c:
//   • UI / Rect pass  — instanced draw of Render_Rect_2D_Inst structs
//   • Triangle pass   — single hardcoded triangle (test pass)

// ============================================================================
// ak: sokol_gfx single-file implementation
// ============================================================================

#include <stdio.h>   // fprintf, stderr

#include "render_sokol.h"


// ============================================================================
// ak: GLSL 3.30 shader sources
//
// Uniforms are sent as raw ranges (sg_apply_uniforms) using the NATIVE layout
// on GL (tight packing). We keep the shaders in separate uniform variables
// rather than UBOs to avoid std140 alignment headaches with mat3.
// ============================================================================

// ---------------------------------------------------------------------------
// Rect vertex shader
// ---------------------------------------------------------------------------
static const char *_render_sokol_rect_vs_src =
    "#version 330 core\n"
    "\n"
    "in vec4  c2v_dst_rect;\n"
    "in vec4  c2v_src_rect;\n"
    "in vec4  c2v_colors_0;\n"
    "in vec4  c2v_colors_1;\n"
    "in vec4  c2v_colors_2;\n"
    "in vec4  c2v_colors_3;\n"
    "in vec4  c2v_corner_radii;\n"
    "in vec4  c2v_style;\n" // x:border_thickness y:softness z:omit_texture w:shear
    "\n"
    "out vec2  v2p_sdf_sample_pos;\n"
    "out vec2  v2p_texcoord_pct;\n"
    "out vec2  v2p_rect_half_size_px;\n"
    "out vec4  v2p_tint;\n"
    "out float v2p_corner_radius;\n"
    "out float v2p_border_thickness;\n"
    "out float v2p_softness;\n"
    "out float v2p_omit_texture;\n"
    "\n"
    // VS uniforms — sent as 4×vec4:
    // u_col0 = mat3 column 0 (xform.v[0]), w=0
    // u_col1 = mat3 column 1 (xform.v[1]), w=0
    // u_col2 = mat3 column 2 (xform.v[2]), w=0
    // u_vp_px = vec2 viewport, zw=0
    "uniform vec4 u_col0;\n"
    "uniform vec4 u_col1;\n"
    "uniform vec4 u_col2;\n"
    "uniform vec4 u_vp_px;\n"
    "uniform sampler2D u_tex;\n"
    "\n"
    "void main(void) {\n"
    "  mat3 u_xform = mat3(u_col0.xyz, u_col1.xyz, u_col2.xyz);\n"
    "  vec2 u_viewport_size_px = u_vp_px.xy;\n"
    "  vec2 vertices[4];\n"
    "  vertices[0] = vec2(-1.0, -1.0);\n"
    "  vertices[1] = vec2(-1.0, +1.0);\n"
    "  vertices[2] = vec2(+1.0, -1.0);\n"
    "  vertices[3] = vec2(+1.0, +1.0);\n"
    "  float shears[4];\n"
    "  shears[0] = 0.0; shears[1] = 0.0;\n"
    "  shears[2] = c2v_style.w; shears[3] = c2v_style.w;\n"
    "  vec2 dst_half = (c2v_dst_rect.zw - c2v_dst_rect.xy) * 0.5;\n"
    "  vec2 dst_ctr  = (c2v_dst_rect.zw + c2v_dst_rect.xy) * 0.5;\n"
    "  vec2 dst_pos  = vertices[gl_VertexID] * dst_half + dst_ctr;\n"
    "  dst_pos.y += shears[gl_VertexID];\n"
    "  dst_pos = (u_xform * vec3(dst_pos, 1.0)).xy;\n"
    "  vec2 src_half = (c2v_src_rect.zw - c2v_src_rect.xy) * 0.5;\n"
    "  vec2 src_ctr  = (c2v_src_rect.zw + c2v_src_rect.xy) * 0.5;\n"
    "  vec2 src_pos  = vertices[gl_VertexID] * src_half + src_ctr;\n"
    "  vec4 colors[4];\n"
    "  colors[0]=c2v_colors_0; colors[1]=c2v_colors_1;\n"
    "  colors[2]=c2v_colors_2; colors[3]=c2v_colors_3;\n"
    "  float cr[4];\n"
    "  cr[0]=c2v_corner_radii.x; cr[1]=c2v_corner_radii.y;\n"
    "  cr[2]=c2v_corner_radii.z; cr[3]=c2v_corner_radii.w;\n"
    "  ivec2 tsz_i = textureSize(u_tex, 0);\n"
    "  vec2  tsz   = vec2(float(tsz_i.x), float(tsz_i.y));\n"
    "  vec2 dst_verts_pct = vec2(((gl_VertexID>>1)!=1)?1.0:0.0,\n"
    "                            ((gl_VertexID& 1)!=0)?0.0:1.0);\n"
    "  gl_Position = vec4(\n"
    "     2.0*dst_pos.x/u_viewport_size_px.x - 1.0,\n"
    "     2.0*(1.0 - dst_pos.y/u_viewport_size_px.y) - 1.0,\n"
    "     0.0, 1.0);\n"
    "  v2p_sdf_sample_pos    = (2.0*dst_verts_pct - 1.0) * dst_half;\n"
    "  v2p_texcoord_pct      = src_pos / tsz;\n"
    "  v2p_rect_half_size_px = dst_half;\n"
    "  v2p_tint              = colors[gl_VertexID];\n"
    "  v2p_corner_radius     = cr[gl_VertexID];\n"
    "  v2p_border_thickness  = c2v_style.x;\n"
    "  v2p_softness          = c2v_style.y;\n"
    "  v2p_omit_texture      = c2v_style.z;\n"
    "}\n";

// ---------------------------------------------------------------------------
// Rect fragment shader
// ---------------------------------------------------------------------------
static const char *_render_sokol_rect_fs_src =
    "#version 330 core\n"
    "\n"
    "in vec2  v2p_sdf_sample_pos;\n"
    "in vec2  v2p_texcoord_pct;\n"
    "in vec2  v2p_rect_half_size_px;\n"
    "in vec4  v2p_tint;\n"
    "in float v2p_corner_radius;\n"
    "in float v2p_border_thickness;\n"
    "in float v2p_softness;\n"
    "in float v2p_omit_texture;\n"
    "\n"
    "out vec4 final_color;\n"
    "\n"
    "uniform sampler2D u_tex;\n"
    "uniform mat4  u_channel_map;\n"
    // u_fs_params.x = opacity
    "uniform vec4  u_fs_params;\n"
    "\n"
    "float rect_sdf(vec2 p, vec2 hs, float r) {\n"
    "  return length(max(abs(p)-hs+r, 0.0))-r;\n"
    "}\n"
    "float srgb_to_linear(float x) {\n"
    "  return x<0.0404482362771082 ? x/12.92 : pow((x+0.055)/1.055, 2.4);\n"
    "}\n"
    "vec4 srgba_to_linear(vec4 v) {\n"
    "  return vec4(srgb_to_linear(v.r), srgb_to_linear(v.g),\n"
    "              srgb_to_linear(v.b), v.a);\n"
    "}\n"
    "void main(void) {\n"
    "  vec4 albedo = vec4(1.0);\n"
    "  if (v2p_omit_texture < 1.0) {\n"
    "    albedo = u_channel_map * texture(u_tex, v2p_texcoord_pct);\n"
    "    albedo = srgba_to_linear(albedo);\n"
    "  }\n"
    "  float border_t = 1.0;\n"
    "  if (v2p_border_thickness > 0.0) {\n"
    "    float s = rect_sdf(v2p_sdf_sample_pos,\n"
    "      v2p_rect_half_size_px - vec2(v2p_softness*2.0) - v2p_border_thickness,\n"
    "      max(v2p_corner_radius - v2p_border_thickness, 0.0));\n"
    "    border_t = smoothstep(0.0, 2.0*v2p_softness, s);\n"
    "  }\n"
    "  if (border_t < 0.001) discard;\n"
    "  float corner_t = 1.0;\n"
    "  if (v2p_corner_radius > 0.0 || v2p_softness > 0.75) {\n"
    "    float s = rect_sdf(v2p_sdf_sample_pos,\n"
    "      v2p_rect_half_size_px - vec2(v2p_softness*2.0),\n"
    "      v2p_corner_radius);\n"
    "    corner_t = 1.0 - smoothstep(0.0, 2.0*v2p_softness, s);\n"
    "  }\n"
    "  final_color   = albedo * v2p_tint;\n"
    "  final_color.a *= u_fs_params.x * corner_t * border_t;\n"
    "}\n";

// ---------------------------------------------------------------------------
// Triangle shaders (simple test pass)
// ---------------------------------------------------------------------------
static const char *_render_sokol_tri_vs_src =
    "#version 330 core\n"
    "void main() {\n"
    "  vec2 pos[3];\n"
    "  pos[0]=vec2(0.0,0.5); pos[1]=vec2(-0.5,-0.5); pos[2]=vec2(0.5,-0.5);\n"
    "  gl_Position = vec4(pos[gl_VertexID], 0.0, 1.0);\n"
    "}\n";

static const char *_render_sokol_tri_fs_src =
    "#version 330 core\n"
    "out vec4 color;\n"
    "void main() { color = vec4(1.0,1.0,0.0,1.0); }\n";

#if defined(SOKOL_VULKAN)

static const char *_render_sokol_rect_vk_vs_src =
    "#version 450\n"
    "layout(location = 0) in vec4 c2v_dst_rect;\n"
    "layout(location = 1) in vec4 c2v_src_rect;\n"
    "layout(location = 2) in vec4 c2v_colors_0;\n"
    "layout(location = 3) in vec4 c2v_colors_1;\n"
    "layout(location = 4) in vec4 c2v_colors_2;\n"
    "layout(location = 5) in vec4 c2v_colors_3;\n"
    "layout(location = 6) in vec4 c2v_corner_radii;\n"
    "layout(location = 7) in vec4 c2v_style;\n"
    "\n"
    "layout(location = 0) out vec2 v2p_sdf_sample_pos;\n"
    "layout(location = 1) out vec2 v2p_texcoord_pct;\n"
    "layout(location = 2) out vec2 v2p_rect_half_size_px;\n"
    "layout(location = 3) out vec4 v2p_tint;\n"
    "layout(location = 4) out float v2p_corner_radius;\n"
    "layout(location = 5) out float v2p_border_thickness;\n"
    "layout(location = 6) out float v2p_softness;\n"
    "layout(location = 7) out float v2p_omit_texture;\n"
    "\n"
    "layout(set = 0, binding = 0) uniform VS_Uniforms {\n"
    "    vec4 u_col0;\n"
    "    vec4 u_col1;\n"
    "    vec4 u_col2;\n"
    "    vec4 u_vp_px;\n"
    "};\n"
    "\n"
    "void main(void) {\n"
    "  mat3 u_xform = mat3(u_col0.xyz, u_col1.xyz, u_col2.xyz);\n"
    "  vec2 u_viewport_size_px = u_vp_px.xy;\n"
    "  vec2 vertices[4];\n"
    "  vertices[0] = vec2(-1.0, -1.0);\n"
    "  vertices[1] = vec2(-1.0, +1.0);\n"
    "  vertices[2] = vec2(+1.0, -1.0);\n"
    "  vertices[3] = vec2(+1.0, +1.0);\n"
    "  float shears[4];\n"
    "  shears[0] = 0.0; shears[1] = 0.0;\n"
    "  shears[2] = c2v_style.w; shears[3] = c2v_style.w;\n"
    "  vec2 dst_half = (c2v_dst_rect.zw - c2v_dst_rect.xy) * 0.5;\n"
    "  vec2 dst_ctr  = (c2v_dst_rect.zw + c2v_dst_rect.xy) * 0.5;\n"
    "  vec2 dst_pos  = vertices[gl_VertexIndex] * dst_half + dst_ctr;\n"
    "  dst_pos.y += shears[gl_VertexIndex];\n"
    "  dst_pos = (u_xform * vec3(dst_pos, 1.0)).xy;\n"
    "  vec2 src_half = (c2v_src_rect.zw - c2v_src_rect.xy) * 0.5;\n"
    "  vec2 src_ctr  = (c2v_src_rect.zw + c2v_src_rect.xy) * 0.5;\n"
    "  vec2 src_pos  = vertices[gl_VertexIndex] * src_half + src_ctr;\n"
    "  vec4 colors[4];\n"
    "  colors[0]=c2v_colors_0; colors[1]=c2v_colors_1;\n"
    "  colors[2]=c2v_colors_2; colors[3]=c2v_colors_3;\n"
    "  float cr[4];\n"
    "  cr[0]=c2v_corner_radii.x; cr[1]=c2v_corner_radii.y;\n"
    "  cr[2]=c2v_corner_radii.z; cr[3]=c2v_corner_radii.w;\n"
    "  vec2 dst_verts_pct = vec2(((gl_VertexIndex>>1)!=1)?1.0:0.0,\n"
    "                            ((gl_VertexIndex& 1)!=0)?0.0:1.0);\n"
    "  gl_Position = vec4(\n"
    "     2.0*dst_pos.x/u_viewport_size_px.x - 1.0,\n"
    "     2.0*dst_pos.y/u_viewport_size_px.y - 1.0,\n"
    "     0.0, 1.0);\n"
    "  v2p_sdf_sample_pos    = (2.0*dst_verts_pct - 1.0) * dst_half;\n"
    "  v2p_texcoord_pct      = src_pos;\n"
    "  v2p_rect_half_size_px = dst_half;\n"
    "  v2p_tint              = colors[gl_VertexIndex];\n"
    "  v2p_corner_radius     = cr[gl_VertexIndex];\n"
    "  v2p_border_thickness  = c2v_style.x;\n"
    "  v2p_softness          = c2v_style.y;\n"
    "  v2p_omit_texture      = c2v_style.z;\n"
    "}\n";

static const char *_render_sokol_rect_vk_fs_src =
    "#version 450\n"
    "layout(location = 0) in vec2 v2p_sdf_sample_pos;\n"
    "layout(location = 1) in vec2 v2p_texcoord_pct;\n"
    "layout(location = 2) in vec2 v2p_rect_half_size_px;\n"
    "layout(location = 3) in vec4 v2p_tint;\n"
    "layout(location = 4) in float v2p_corner_radius;\n"
    "layout(location = 5) in float v2p_border_thickness;\n"
    "layout(location = 6) in float v2p_softness;\n"
    "layout(location = 7) in float v2p_omit_texture;\n"
    "\n"
    "layout(location = 0) out vec4 final_color;\n"
    "\n"
    "layout(set = 0, binding = 1) uniform FS_Uniforms {\n"
    "    mat4 u_channel_map;\n"
    "    vec4 u_fs_params;\n"
    "};\n"
    "\n"
    "layout(set = 1, binding = 0) uniform texture2D u_tex;\n"
    "layout(set = 1, binding = 1) uniform sampler u_smp;\n"
    "\n"
    "float rect_sdf(vec2 p, vec2 hs, float r) {\n"
    "  return length(max(abs(p)-hs+r, 0.0))-r;\n"
    "}\n"
    "float srgb_to_linear(float x) {\n"
    "  return x<0.0404482362771082 ? x/12.92 : pow((x+0.055)/1.055, 2.4);\n"
    "}\n"
    "vec4 srgba_to_linear(vec4 v) {\n"
    "  return vec4(srgb_to_linear(v.r), srgb_to_linear(v.g),\n"
    "              srgb_to_linear(v.b), v.a);\n"
    "}\n"
    "void main(void) {\n"
    "  vec4 albedo = vec4(1.0);\n"
    "  if (v2p_omit_texture < 1.0) {\n"
    "    ivec2 tsz_i = textureSize(sampler2D(u_tex, u_smp), 0);\n"
    "    vec2 tsz = vec2(float(tsz_i.x), float(tsz_i.y));\n"
    "    vec2 uv = v2p_texcoord_pct / tsz;\n"
    "    albedo = u_channel_map * texture(sampler2D(u_tex, u_smp), uv);\n"
    "    albedo = srgba_to_linear(albedo);\n"
    "  }\n"
    "  float border_t = 1.0;\n"
    "  if (v2p_border_thickness > 0.0) {\n"
    "    float s = rect_sdf(v2p_sdf_sample_pos,\n"
    "      v2p_rect_half_size_px - vec2(v2p_softness*2.0) - v2p_border_thickness,\n"
    "      max(v2p_corner_radius - v2p_border_thickness, 0.0));\n"
    "    border_t = smoothstep(0.0, 2.0*v2p_softness, s);\n"
    "  }\n"
    "  if (border_t < 0.001) discard;\n"
    "  float corner_t = 1.0;\n"
    "  if (v2p_corner_radius > 0.0 || v2p_softness > 0.75) {\n"
    "    float s = rect_sdf(v2p_sdf_sample_pos,\n"
    "      v2p_rect_half_size_px - vec2(v2p_softness*2.0),\n"
    "      v2p_corner_radius);\n"
    "    corner_t = 1.0 - smoothstep(0.0, 2.0*v2p_softness, s);\n"
    "  }\n"
    "  final_color   = albedo * v2p_tint;\n"
    "  final_color.a *= u_fs_params.x * corner_t * border_t;\n"
    "}\n";

static const char *_render_sokol_tri_vk_vs_src =
    "#version 450\n"
    "void main() {\n"
    "  vec2 pos[3] = vec2[](\n"
    "    vec2(0.0, -0.5),\n"
    "    vec2(-0.5, 0.5),\n"
    "    vec2(0.5, 0.5)\n"
    "  );\n"
    "  gl_Position = vec4(pos[gl_VertexIndex], 0.0, 1.0);\n"
    "}\n";

static const char *_render_sokol_tri_vk_fs_src =
    "#version 450\n"
    "layout(location = 0) out vec4 color;\n"
    "void main() { color = vec4(1.0,1.0,0.0,1.0); }\n";

#endif

// ============================================================================
// ak: Internal globals
// ============================================================================

global _Render_Sokol_State *_render_sokol_state = STRUCT_ZERO;

// ============================================================================
// ak: Sokol logger
// ============================================================================

internal void
_render_sokol_log(const char *tag, uint32_t level, uint32_t id,
                  const char *msg, uint32_t line, const char *file,
                  void *user_data)
{
    (void)tag; (void)user_data;
    const char *lvl = (level==0)?"PANIC":(level==1)?"ERROR":
                      (level==2)?"WARN":"INFO";
    if(msg)
    {
        fprintf(stderr, "[sokol:%s] %s  (id=%u %s:%u)\n",
                lvl, msg, id, file?file:"?", line);
    }
}

// ============================================================================
// ak: Pixel format mapping
// ============================================================================

internal sg_pixel_format
_render_sokol_pixel_format(Render_Tex_2D_Format fmt)
{
    switch(fmt)
    {
        default:
        case Render_Tex_2D_Format_RGBA8:  return SG_PIXELFORMAT_RGBA8;
        case Render_Tex_2D_Format_R8:     return SG_PIXELFORMAT_R8;
        case Render_Tex_2D_Format_RG8:    return SG_PIXELFORMAT_RG8;
        case Render_Tex_2D_Format_BGRA8:  return SG_PIXELFORMAT_BGRA8;
        case Render_Tex_2D_Format_R16:    return SG_PIXELFORMAT_R16;
        case Render_Tex_2D_Format_RGBA16: return SG_PIXELFORMAT_RGBA16;
        case Render_Tex_2D_Format_R32:    return SG_PIXELFORMAT_R32F;
        case Render_Tex_2D_Format_RG32:   return SG_PIXELFORMAT_RG32F;
        case Render_Tex_2D_Format_RGBA32: return SG_PIXELFORMAT_RGBA32F;
    }
}

// ============================================================================
// ak: Handle helpers
// ============================================================================

internal Render_Handle
_render_handle_from_sokol_tex2d(_Render_Sokol_Tex_2D *t)
{
    Render_Handle h = {0};
    h.u64[0] = (uint64_t)(uintptr_t)t;
    return h;
}

internal _Render_Sokol_Tex_2D *
_render_sokol_tex2d_from_handle(Render_Handle h)
{
    return (_Render_Sokol_Tex_2D *)(uintptr_t)h.u64[0];
}

// ============================================================================
// ak: Build rect pipeline
// ============================================================================

internal sg_pipeline
_render_sokol_make_rect_pipeline(void)
{
    sg_shader shd;
#if defined(SOKOL_VULKAN)
    sg_range vs_code = _render_sokol_vulkan_compile_shader(_render_sokol_rect_vk_vs_src, "vert");
    sg_range fs_code = _render_sokol_vulkan_compile_shader(_render_sokol_rect_vk_fs_src, "frag");
    shd = sg_make_shader(&(sg_shader_desc){
        .label = "rect_shd",
        .vertex_func   = { .bytecode = vs_code, .entry = "main" },
        .fragment_func = { .bytecode = fs_code, .entry = "main" },
        .uniform_blocks[0] = {
            .stage  = SG_SHADERSTAGE_VERTEX,
            .size   = sizeof(float)*16,   // 4 × vec4
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .spirv_set0_binding_n = 0,
        },
        .uniform_blocks[1] = {
            .stage  = SG_SHADERSTAGE_FRAGMENT,
            .size   = sizeof(float)*20,  // 16f mat4 + 4f (opacity + 3 pad)
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .spirv_set0_binding_n = 1,
        },
        .views[0] = {
            .texture = {
                .stage = SG_SHADERSTAGE_FRAGMENT,
                .image_type = SG_IMAGETYPE_2D,
                .sample_type = SG_IMAGESAMPLETYPE_FLOAT,
                .spirv_set1_binding_n = 0,
            },
        },
        .samplers[0] = {
            .stage = SG_SHADERSTAGE_FRAGMENT,
            .spirv_set1_binding_n = 1,
        },
        .texture_sampler_pairs[0] = {
            .stage        = SG_SHADERSTAGE_FRAGMENT,
            .view_slot    = 0,
            .sampler_slot = 0,
            .glsl_name    = "u_tex",
        },
    });
    free((void*)vs_code.ptr);
    free((void*)fs_code.ptr);
#else
    // The shader uses separate (non-UBO) uniforms — gl_name is used by sokol
    // to glGetUniformLocation them on GL backends.
    shd = sg_make_shader(&(sg_shader_desc){
        .label = "rect_shd",
        .vertex_func   = { .source = _render_sokol_rect_vs_src, .entry = "main" },
        .fragment_func = { .source = _render_sokol_rect_fs_src, .entry = "main" },
        // VS uniform block:
        // 3 × vec4 (mat3 columns padded to vec4) + 1 × vec4 (viewport.xy + pad.xy)
        // = 4 × 16 = 64 bytes
        .uniform_blocks[0] = {
            .stage  = SG_SHADERSTAGE_VERTEX,
            .size   = sizeof(float)*16,   // 4 × vec4
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .glsl_uniforms[0] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "u_col0",  .array_count = 1 },
            .glsl_uniforms[1] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "u_col1",  .array_count = 1 },
            .glsl_uniforms[2] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "u_col2",  .array_count = 1 },
            .glsl_uniforms[3] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "u_vp_px", .array_count = 1 },
        },
        // FS uniform block:
        // mat4 (64 bytes) + float (4 bytes) + 3×float pad (12 bytes) = 80 bytes
        .uniform_blocks[1] = {
            .stage  = SG_SHADERSTAGE_FRAGMENT,
            .size   = sizeof(float)*20,  // 16f mat4 + 4f (opacity + 3 pad)
            .layout = SG_UNIFORMLAYOUT_NATIVE,
            .glsl_uniforms[0] = { .type = SG_UNIFORMTYPE_MAT4,   .glsl_name = "u_channel_map", .array_count = 1 },
            .glsl_uniforms[1] = { .type = SG_UNIFORMTYPE_FLOAT4, .glsl_name = "u_fs_params",   .array_count = 1 },
        },
        // Texture view for u_tex (sampler2D)
        .views[0] = {
            .texture.stage       = SG_SHADERSTAGE_FRAGMENT,
            .texture.image_type  = SG_IMAGETYPE_2D,
            .texture.sample_type = SG_IMAGESAMPLETYPE_FLOAT,
        },
        .samplers[0] = {
            .stage        = SG_SHADERSTAGE_FRAGMENT,
            .sampler_type = SG_SAMPLERTYPE_FILTERING,
        },
        .texture_sampler_pairs[0] = {
            .stage        = SG_SHADERSTAGE_FRAGMENT,
            .view_slot    = 0,
            .sampler_slot = 0,
            .glsl_name    = "u_tex",
        },
    });
#endif

    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .label  = "rect_pip",
        .shader = shd,
        .layout = {
            .buffers[0].step_func = SG_VERTEXSTEP_PER_INSTANCE,
            .attrs = {
                [0] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_dst_rect
                [1] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_src_rect
                [2] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_colors_0
                [3] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_colors_1
                [4] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_colors_2
                [5] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_colors_3
                [6] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_corner_radii
                [7] = { .buffer_index=0, .format=SG_VERTEXFORMAT_FLOAT4 }, // c2v_style
            },
        },
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLE_STRIP,
        .colors[0] = {
            .blend = {
                .enabled          = true,
                .src_factor_rgb   = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb   = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
                .src_factor_alpha = SG_BLENDFACTOR_ONE,
                .dst_factor_alpha = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .depth = { .pixel_format = SG_PIXELFORMAT_NONE },
    });
    _render_sokol_state->shd_rect = shd;
    return pip;
}

// ============================================================================
// ak: Build triangle pipeline
// ============================================================================

internal sg_pipeline
_render_sokol_make_triangle_pipeline(void)
{
    sg_shader shd;
#if defined(SOKOL_VULKAN)
    sg_range vs_code = _render_sokol_vulkan_compile_shader(_render_sokol_tri_vk_vs_src, "vert");
    sg_range fs_code = _render_sokol_vulkan_compile_shader(_render_sokol_tri_vk_fs_src, "frag");
    shd = sg_make_shader(&(sg_shader_desc){
        .label         = "tri_shd",
        .vertex_func   = { .bytecode = vs_code, .entry = "main" },
        .fragment_func = { .bytecode = fs_code, .entry = "main" },
    });
    free((void*)vs_code.ptr);
    free((void*)fs_code.ptr);
#else
    shd = sg_make_shader(&(sg_shader_desc){
        .label         = "tri_shd",
        .vertex_func   = { .source = _render_sokol_tri_vs_src, .entry = "main" },
        .fragment_func = { .source = _render_sokol_tri_fs_src, .entry = "main" },
    });
#endif
    sg_pipeline pip = sg_make_pipeline(&(sg_pipeline_desc){
        .label          = "tri_pip",
        .shader         = shd,
        .primitive_type = SG_PRIMITIVETYPE_TRIANGLES,
        .colors[0] = {
            .blend = {
                .enabled        = true,
                .src_factor_rgb = SG_BLENDFACTOR_SRC_ALPHA,
                .dst_factor_rgb = SG_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
            },
        },
        .depth = { .pixel_format = SG_PIXELFORMAT_NONE },
    });
    _render_sokol_state->shd_triangle = shd;
    return pip;
}

// ============================================================================
// ak: Texture view helper — wraps sg_image in an sg_view for bindings
// ============================================================================

internal sg_view
_render_sokol_texture_view(sg_image image)
{
    return sg_make_view(&(sg_view_desc){
        .texture = {
            .image = image,
        },
    });
}

// ============================================================================
// ak: render_init
// ============================================================================

internal void
render_init(void)
{
    _render_sokol_platform_init();

    Arena *arena = arena_alloc();
    _render_sokol_state = arena_push(arena, _Render_Sokol_State, 1);
    _render_sokol_state->arena = arena;

    sg_environment env = _render_sokol_build_environment();
    sg_setup(&(sg_desc){
        .environment = env,
        .logger      = { .func = _render_sokol_log },
    });

    // White 1×1 RGBA8 fallback texture
    {
        uint8_t w[4] = {255,255,255,255};
        _render_sokol_state->white_image = sg_make_image(&(sg_image_desc){
            .width  = 1, .height = 1,
            .pixel_format = SG_PIXELFORMAT_RGBA8,
            .usage = { .immutable = true },
            .data.mip_levels[0] = { .ptr=w, .size=4 },
            .label = "white_1x1",
        });
        _render_sokol_state->white_sampler = sg_make_sampler(&(sg_sampler_desc){
            .min_filter = SG_FILTER_NEAREST,
            .mag_filter = SG_FILTER_NEAREST,
            .wrap_u     = SG_WRAP_REPEAT,
            .wrap_v     = SG_WRAP_REPEAT,
            .label      = "white_smp",
        });
        _render_sokol_state->white_view = _render_sokol_texture_view(
            _render_sokol_state->white_image);
    }

    // Pipelines
    _render_sokol_state->pip_rect     = _render_sokol_make_rect_pipeline();
    _render_sokol_state->pip_triangle = _render_sokol_make_triangle_pipeline();

    // Streaming instance buffer (1 MB initial, grows if needed)
    {
        size_t sz = MB(1);
        _render_sokol_state->inst_buf_size = sz;
        _render_sokol_state->inst_buf = sg_make_buffer(&(sg_buffer_desc){
            .size  = sz,
            .usage = { .stream_update = true },
            .label = "rect_inst",
        });
    }
}

// ============================================================================
// ak: render_deinit
// ============================================================================

internal void
render_deinit(void)
{
    if(!_render_sokol_state) return;

    sg_destroy_view(_render_sokol_state->white_view);
    sg_destroy_image(_render_sokol_state->white_image);
    sg_destroy_sampler(_render_sokol_state->white_sampler);
    sg_destroy_pipeline(_render_sokol_state->pip_rect);
    sg_destroy_pipeline(_render_sokol_state->pip_triangle);
    sg_destroy_shader(_render_sokol_state->shd_rect);
    sg_destroy_shader(_render_sokol_state->shd_triangle);
    sg_destroy_buffer(_render_sokol_state->inst_buf);

    for(_Render_Sokol_Tex_2D *t = _render_sokol_state->free_tex2d; t; t = t->next)
    {
        if(t->view_nearest.id  != SG_INVALID_ID) sg_destroy_view(t->view_nearest);
        if(t->view_linear.id   != SG_INVALID_ID) sg_destroy_view(t->view_linear);
        if(t->image.id         != SG_INVALID_ID) sg_destroy_image(t->image);
        if(t->smp_nearest.id   != SG_INVALID_ID) sg_destroy_sampler(t->smp_nearest);
        if(t->smp_linear.id    != SG_INVALID_ID) sg_destroy_sampler(t->smp_linear);
    }

    sg_shutdown();
    _render_sokol_platform_deinit();
    arena_free(_render_sokol_state->arena);
    _render_sokol_state = NULL;
}

// ============================================================================
// ak: Frame / window lifecycle
// ============================================================================

internal void render_begin_frame(void) {}

internal void render_end_frame(void)   {}

internal Render_Handle
render_window_equip(Wl_Window window)
{
    _Render_Sokol_Window *sw = _render_sokol_state->free_window;
    if(sw) { SLLStackPop(_render_sokol_state->free_window); }
    else   { sw = arena_push(_render_sokol_state->arena, _Render_Sokol_Window, 1); }
    MemSetZeroStruct(sw);
    sw->platform_handle = _render_sokol_window_open(window);
    Render_Handle h = { (uint64_t)(uintptr_t)sw };
    return h;
}

internal void
render_window_unequip(Wl_Window window, Render_Handle handle)
{
    _Render_Sokol_Window *sw = (_Render_Sokol_Window *)(uintptr_t)handle.u64[0];
    _render_sokol_window_close(window, sw);
    SLLStackPush(_render_sokol_state->free_window, sw);
}

internal void
render_window_begin_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Sokol_Window *sw = (_Render_Sokol_Window *)(uintptr_t)handle.u64[0];
    _render_sokol_window_make_current(window, sw);
}

internal void
render_window_end_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Sokol_Window *sw = (_Render_Sokol_Window *)(uintptr_t)handle.u64[0];
    sg_commit();
    _render_sokol_present_window(window, sw);
}

// ============================================================================
// ak: render_window_submit — main draw
// ============================================================================

internal void
render_window_submit(Wl_Window window, Render_Handle handle, Render_Pass_List *passes)
{
    _Render_Sokol_Window *sw = (_Render_Sokol_Window *)(uintptr_t)handle.u64[0];

    Rng2_F32 canvas = wl_canvas_rect_from_window(window);
    Vec2_F32 vp_dim = dim2(canvas);
    int vp_w = (int)vp_dim.x;
    int vp_h = (int)vp_dim.y;
    if(vp_w <= 0 || vp_h <= 0) return;

    // Begin pass (clear to black)
    sg_swapchain sc = _render_sokol_swapchain_from_window(sw, vp_w, vp_h);
    sg_begin_pass(&(sg_pass){
        .action = {
            .colors[0] = {
                .load_action  = SG_LOADACTION_CLEAR,
                .store_action = SG_STOREACTION_STORE,
                .clear_value  = { 0.f, 0.f, 0.f, 1.f },
            },
        },
        .swapchain = sc,
    });
    sg_apply_viewport(0, 0, vp_w, vp_h, true);

    for(Render_Pass_Node *pn = passes->first; pn; pn = pn->next)
    {
        switch(pn->v.kind)
        {
            // ----------------------------------------------------------------
            case Render_Pass_Kind_UI:
            // ----------------------------------------------------------------
            {
                Render_Pass_Params_UI *params = pn->v.params_ui;
                sg_apply_pipeline(_render_sokol_state->pip_rect);

                for(_Render_Batch_Group_2D_Node *gn = params->rects.first; gn; gn = gn->next)
                {
                    Render_Batch_List          *batches = &gn->batches;
                    _Render_Batch_Group_2D_Params *gp   = &gn->params;
                    if(batches->byte_count == 0) continue;

                    // Grow instance buffer if needed
                    if(batches->byte_count > _render_sokol_state->inst_buf_size)
                    {
                        sg_destroy_buffer(_render_sokol_state->inst_buf);
                        size_t ns = (batches->byte_count + MB(1)-1) & ~(size_t)(MB(1)-1);
                        _render_sokol_state->inst_buf_size = ns;
                        _render_sokol_state->inst_buf = sg_make_buffer(&(sg_buffer_desc){
                            .size  = ns,
                            .usage = { .stream_update = true },
                            .label = "rect_inst",
                        });
                    }

                    // Flatten batch list into a temporary contiguous buffer and upload
                    {
                        Arena_Temp tmp = arena_scratch_begin(0, 0);
                        uint8_t *flat  = arena_push(tmp.arena, uint8_t, batches->byte_count);
                        size_t off = 0;
                        for(Render_Batch_Node *bn = batches->first; bn; bn = bn->next)
                        {
                            mem_copy(flat + off, bn->v.v, bn->v.byte_count);
                            off += bn->v.byte_count;
                        }
                        sg_update_buffer(_render_sokol_state->inst_buf,
                            &(sg_range){ .ptr=flat, .size=batches->byte_count });
                        arena_scratch_end(tmp);
                    }

                    // Resolve texture/sampler/view
                    sg_view    tex_view;
                    sg_sampler tex_smp;
                    Render_Tex_2D_Format tex_fmt = Render_Tex_2D_Format_RGBA8;
                    {
                        _Render_Sokol_Tex_2D *t = _render_sokol_tex2d_from_handle(gp->tex);
                        if(t && t->image.id != SG_INVALID_ID)
                        {
                            tex_fmt = t->format;
                            if(gp->tex_sample_kind == Render_Tex_2d_Sample_Kind_Linear)
                            {
                                tex_view = t->view_linear;
                                tex_smp  = t->smp_linear;
                            }
                            else
                            {
                                tex_view = t->view_nearest;
                                tex_smp  = t->smp_nearest;
                            }
                        }
                        else
                        {
                            tex_view = _render_sokol_state->white_view;
                            tex_smp  = _render_sokol_state->white_sampler;
                        }
                    }

                    // Bindings
                    sg_apply_bindings(&(sg_bindings){
                        .vertex_buffers[0] = _render_sokol_state->inst_buf,
                        .views[0]          = tex_view,
                        .samplers[0]       = tex_smp,
                    });

                    // Scissor
                    bool has_clip = (gp->clip.x0 != 0 || gp->clip.y0 != 0 ||
                                     gp->clip.x1 != 0 || gp->clip.y1 != 0);
                    if(has_clip)
                    {
                        int sx0 = (int)floor_f32(Clamp(0.f, gp->clip.x0, vp_dim.x));
                        int sy0 = (int)floor_f32(Clamp(0.f, gp->clip.y0, vp_dim.y));
                        int sx1 = (int) ceil_f32(Clamp(0.f, gp->clip.x1, vp_dim.x));
                        int sy1 = (int) ceil_f32(Clamp(0.f, gp->clip.y1, vp_dim.y));
                        sg_apply_scissor_rect(sx0, sy0, sx1-sx0, sy1-sy0, true);
                    }

                    // VS uniforms — 4×vec4 (64 bytes)
                    // u_col0..2 = mat3 cols (xyz) with w=0
                    // u_vp_px   = viewport size in xy, zw=0
                    {
                        struct { float d[16]; } vs;
                        // col0
                        vs.d[0]  = gp->xform.v[0][0];
                        vs.d[1]  = gp->xform.v[0][1];
                        vs.d[2]  = gp->xform.v[0][2];
                        vs.d[3]  = 0.f;
                        // col1
                        vs.d[4]  = gp->xform.v[1][0];
                        vs.d[5]  = gp->xform.v[1][1];
                        vs.d[6]  = gp->xform.v[1][2];
                        vs.d[7]  = 0.f;
                        // col2
                        vs.d[8]  = gp->xform.v[2][0];
                        vs.d[9]  = gp->xform.v[2][1];
                        vs.d[10] = gp->xform.v[2][2];
                        vs.d[11] = 0.f;
                        // viewport
                        vs.d[12] = vp_dim.x;
                        vs.d[13] = vp_dim.y;
                        vs.d[14] = 0.f;
                        vs.d[15] = 0.f;
                        sg_apply_uniforms(0, &(sg_range){ .ptr=&vs, .size=sizeof(vs) });
                    }

                    // FS uniforms — mat4 channel map (64 bytes) + vec4 fs_params (16 bytes)
                    // u_fs_params.x = opacity
                    {
                        Mat4x4_F32 chan = render_sample_channel_map_from_tex2dformat(tex_fmt);
                        struct { float map[16]; float fs_params[4]; } fs;
                        mem_copy(fs.map, chan.v, sizeof(fs.map));
                        fs.fs_params[0] = 1.f - gp->transparency;
                        fs.fs_params[1] = 0.f;
                        fs.fs_params[2] = 0.f;
                        fs.fs_params[3] = 0.f;
                        sg_apply_uniforms(1, &(sg_range){ .ptr=&fs, .size=sizeof(fs) });
                    }

                    int inst_count = (int)(batches->byte_count / batches->bytes_per_inst);
                    sg_draw(0, 4, inst_count);

                    if(has_clip)
                    {
                        sg_apply_scissor_rect(0, 0, vp_w, vp_h, true);
                    }
                }
            } break;

            // ----------------------------------------------------------------
            case Render_Pass_Kind_Triangle:
            // ----------------------------------------------------------------
            {
                sg_apply_pipeline(_render_sokol_state->pip_triangle);
                sg_draw(0, 3, 1);
            } break;

            default: break;
        }
    }

    sg_end_pass();
}

// ============================================================================
// ak: Texture API
// ============================================================================

internal Render_Handle
render_tex2d_alloc(Render_Resource_Kind kind, Render_Tex_2D_Format format,
                   Vec2_I32 size, void *data, Arena *arena)
{
    (void)arena;
    if(!_render_sokol_state) return render_handle_zero();

    sg_image_usage usage = {0};
    switch(kind)
    {
        default:
        case Render_Resource_Kind_Static:  usage.immutable     = true; break;
        case Render_Resource_Kind_Dynamic: usage.dynamic_update = true; break;
        case Render_Resource_Kind_Stream:  usage.stream_update  = true; break;
    }

    sg_image_desc img_desc = {0};
    img_desc.type         = SG_IMAGETYPE_2D;
    img_desc.width        = size.x;
    img_desc.height       = size.y;
    img_desc.pixel_format = _render_sokol_pixel_format(format);
    img_desc.usage        = usage;
    img_desc.label        = "user_tex2d";
    if(data && usage.immutable)
    {
        img_desc.data.mip_levels[0] = (sg_range){
            .ptr  = data,
            .size = (size_t)size.x * (size_t)size.y * 4,
        };
    }

    sg_image img = sg_make_image(&img_desc);
    if(img.id == SG_INVALID_ID) return render_handle_zero();

    // Recycle or allocate node
    _Render_Sokol_Tex_2D *t = _render_sokol_state->free_tex2d;
    if(t) { SLLStackPop(_render_sokol_state->free_tex2d); MemSetZeroStruct(t); }
    else  { t = arena_push(_render_sokol_state->arena, _Render_Sokol_Tex_2D, 1); }

    t->image = img;
    t->smp_nearest = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_NEAREST, .mag_filter = SG_FILTER_NEAREST,
        .wrap_u = SG_WRAP_REPEAT, .wrap_v = SG_WRAP_REPEAT,
    });
    t->smp_linear = sg_make_sampler(&(sg_sampler_desc){
        .min_filter = SG_FILTER_LINEAR, .mag_filter = SG_FILTER_LINEAR,
        .wrap_u = SG_WRAP_REPEAT, .wrap_v = SG_WRAP_REPEAT,
    });
    t->view_nearest = _render_sokol_texture_view(img);
    t->view_linear  = _render_sokol_texture_view(img);
    t->resource_kind = kind;
    t->format        = format;
    t->size          = size;

    return _render_handle_from_sokol_tex2d(t);
}

internal void
render_tex2d_free(Render_Handle handle)
{
    if(!_render_sokol_state) return;
    _Render_Sokol_Tex_2D *t = _render_sokol_tex2d_from_handle(handle);
    if(!t) return;
    if(t->view_nearest.id != SG_INVALID_ID) sg_destroy_view(t->view_nearest);
    if(t->view_linear.id  != SG_INVALID_ID) sg_destroy_view(t->view_linear);
    if(t->image.id        != SG_INVALID_ID) sg_destroy_image(t->image);
    if(t->smp_nearest.id  != SG_INVALID_ID) sg_destroy_sampler(t->smp_nearest);
    if(t->smp_linear.id   != SG_INVALID_ID) sg_destroy_sampler(t->smp_linear);
    MemSetZeroStruct(t);
    SLLStackPush(_render_sokol_state->free_tex2d, t);
}

// ============================================================================
// ak: Platform-specific section
// ============================================================================

#if OS_LINUX

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>
#include <xcb/xcb.h>
#include <xcb/xcb_image.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct _Render_Sokol_Vulkan_Win _Render_Sokol_Vulkan_Win;
struct _Render_Sokol_Vulkan_Win
{
    Wl_Window window;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
    
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    
    Vec2_F32 last_canvas_rect_dim;
    uint32_t current_image_idx;
};

global VkInstance _sg_vk_instance = VK_NULL_HANDLE;
global VkPhysicalDevice _sg_vk_phys_dev = VK_NULL_HANDLE;
global VkDevice _sg_vk_device = VK_NULL_HANDLE;
global VkQueue _sg_vk_queue = VK_NULL_HANDLE;
global uint32_t _sg_vk_queue_family_index = 0xFFFFFFFF;

internal sg_pixel_format _render_sokol_vk_pixel_format(VkFormat fmt)
{
    if (fmt == VK_FORMAT_B8G8R8A8_UNORM) return SG_PIXELFORMAT_BGRA8;
    return SG_PIXELFORMAT_RGBA8;
}

internal sg_range _render_sokol_vulkan_compile_shader(const char *src, const char *stage)
{
    pid_t pid = getpid();
    char glsl_path[256];
    char spv_path[256];
    sprintf(glsl_path, "/tmp/sokol_shader_%d.%s", pid, stage);
    sprintf(spv_path, "/tmp/sokol_shader_%d.%s.spv", pid, stage);

    FILE *f = fopen(glsl_path, "wb");
    if (!f) return (sg_range){0};
    fwrite(src, 1, strlen(src), f);
    fclose(f);

    char cmd[1024];
    sprintf(cmd, "glslc -fshader-stage=%s %s -o %s", stage, glsl_path, spv_path);
    int status = system(cmd);
    unlink(glsl_path);
    if (status != 0) {
        return (sg_range){0};
    }

    FILE *spvf = fopen(spv_path, "rb");
    if (!spvf) return (sg_range){0};
    fseek(spvf, 0, SEEK_END);
    long size = ftell(spvf);
    fseek(spvf, 0, SEEK_SET);

    uint8_t *code = malloc(size);
    fread(code, 1, size, spvf);
    fclose(spvf);
    unlink(spv_path);

    return (sg_range){ .ptr = code, .size = (size_t)size };
}

internal void _render_sokol_platform_init(void)
{
    // Create VkInstance
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME,
        "VK_EXT_debug_utils"
    };
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "cmono",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "cmono",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_3,
    };
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 3,
        .ppEnabledExtensionNames = extensions,
    };
    if (vkCreateInstance(&instance_info, NULL, &_sg_vk_instance) != VK_SUCCESS)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create Vulkan instance for SOKOL.");
        exit(1);
    }
    
    // Select physical device
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(_sg_vk_instance, &device_count, NULL);
    if (device_count == 0)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to find GPUs with Vulkan support.");
        exit(1);
    }
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(_sg_vk_instance, &device_count, devices);
    _sg_vk_phys_dev = devices[0];
    free(devices);
    
    // Find graphics queue family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_sg_vk_phys_dev, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_sg_vk_phys_dev, &queue_family_count, queue_families);
    
    _sg_vk_queue_family_index = 0xFFFFFFFF;
    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            _sg_vk_queue_family_index = i;
            break;
        }
    }
    free(queue_families);
    
    if (_sg_vk_queue_family_index == 0xFFFFFFFF)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to find a graphics queue family.");
        exit(1);
    }
    
    // Setup VkPhysicalDeviceSynchronization2Features, VkPhysicalDeviceDescriptorBufferFeaturesEXT, and VkPhysicalDeviceBufferDeviceAddressFeatures
    static VkPhysicalDeviceSynchronization2Features synchronization2_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES,
        .pNext = NULL,
        .synchronization2 = VK_TRUE,
    };
    static VkPhysicalDeviceDescriptorBufferFeaturesEXT descriptor_buffer_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT,
        .pNext = &synchronization2_features,
        .descriptorBuffer = VK_TRUE,
    };
    static VkPhysicalDeviceBufferDeviceAddressFeatures buffer_device_address_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = &descriptor_buffer_features,
        .bufferDeviceAddress = VK_TRUE,
    };

    // Create logical device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = _sg_vk_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    const char *device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        "VK_EXT_descriptor_buffer",
        "VK_KHR_buffer_device_address"
    };
    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &buffer_device_address_features,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 3,
        .ppEnabledExtensionNames = device_extensions,
    };
    if (vkCreateDevice(_sg_vk_phys_dev, &device_info, NULL, &_sg_vk_device) != VK_SUCCESS)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create Vulkan logical device.");
        exit(1);
    }
    
    vkGetDeviceQueue(_sg_vk_device, _sg_vk_queue_family_index, 0, &_sg_vk_queue);
}

internal void _render_sokol_platform_deinit(void)
{
    if (_sg_vk_device != VK_NULL_HANDLE)
    {
        vkDeviceWaitIdle(_sg_vk_device);
        vkDestroyDevice(_sg_vk_device, NULL);
        _sg_vk_device = VK_NULL_HANDLE;
    }
    if (_sg_vk_instance != VK_NULL_HANDLE)
    {
        vkDestroyInstance(_sg_vk_instance, NULL);
        _sg_vk_instance = VK_NULL_HANDLE;
    }
}

internal sg_environment _render_sokol_build_environment(void)
{
    sg_environment env = {0};
    env.defaults.color_format = SG_PIXELFORMAT_BGRA8;
    env.defaults.depth_format = SG_PIXELFORMAT_NONE;
    env.defaults.sample_count = 1;
    env.vulkan.instance = (const void*)_sg_vk_instance;
    env.vulkan.physical_device = (const void*)_sg_vk_phys_dev;
    env.vulkan.device = (const void*)_sg_vk_device;
    env.vulkan.queue = (const void*)_sg_vk_queue;
    env.vulkan.queue_family_index = _sg_vk_queue_family_index;
    return env;
}

internal void
_render_sokol_vulkan_win_resize(_Render_Sokol_Vulkan_Win *vw, int w, int h)
{
    vkDeviceWaitIdle(_sg_vk_device);
    
    for (uint32_t i = 0; i < vw->image_count; i++)
    {
        vkDestroyImageView(_sg_vk_device, vw->image_views[i], NULL);
    }
    VkSwapchainKHR old_swapchain = vw->swapchain;
    
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_sg_vk_phys_dev, vw->surface, &capabilities);
    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0xFFFFFFFF)
    {
        extent.width = (uint32_t)w;
        extent.height = (uint32_t)h;
    }
    vw->swapchain_extent = extent;
    vw->last_canvas_rect_dim = (Vec2_F32){ (float)w, (float)h };
    
    uint32_t min_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && min_image_count > capabilities.maxImageCount)
    {
        min_image_count = capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vw->surface,
        .minImageCount = min_image_count,
        .imageFormat = vw->swapchain_format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = old_swapchain,
    };
    vkCreateSwapchainKHR(_sg_vk_device, &swapchain_info, NULL, &vw->swapchain);
    vkDestroySwapchainKHR(_sg_vk_device, old_swapchain, NULL);
    
    vkGetSwapchainImagesKHR(_sg_vk_device, vw->swapchain, &vw->image_count, NULL);
    vw->images = realloc(vw->images, sizeof(VkImage) * vw->image_count);
    vkGetSwapchainImagesKHR(_sg_vk_device, vw->swapchain, &vw->image_count, vw->images);
    
    vw->image_views = realloc(vw->image_views, sizeof(VkImageView) * vw->image_count);
    for (uint32_t i = 0; i < vw->image_count; i++)
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vw->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = vw->swapchain_format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };
        vkCreateImageView(_sg_vk_device, &view_info, NULL, &vw->image_views[i]);
    }
}

internal sg_swapchain
_render_sokol_swapchain_from_window(_Render_Sokol_Window *sw, int w, int h)
{
    _Render_Sokol_Vulkan_Win *vw = (_Render_Sokol_Vulkan_Win *)(uintptr_t)sw->platform_handle.u64[0];
    if(vw->swapchain_extent.width != (uint32_t)w || vw->swapchain_extent.height != (uint32_t)h)
    {
        _render_sokol_vulkan_win_resize(vw, w, h);
    }
    
    sg_swapchain sc = {0};
    sc.width          = w;
    sc.height         = h;
    sc.sample_count   = 1;
    sc.color_format   = _render_sokol_vk_pixel_format(vw->swapchain_format);
    sc.depth_format   = SG_PIXELFORMAT_NONE;
    sc.vulkan.render_image = (const void*)vw->images[vw->current_image_idx];
    sc.vulkan.render_view = (const void*)vw->image_views[vw->current_image_idx];
    sc.vulkan.render_finished_semaphore = (const void*)vw->render_finished_semaphore;
    sc.vulkan.present_complete_semaphore = (const void*)vw->image_available_semaphore;
    return sc;
}

internal Render_Handle _render_sokol_window_open(Wl_Window window)
{
    _Wl_X11_Window *window_os = (_Wl_X11_Window *)window.u64[0];
    
    _Render_Sokol_Vulkan_Win *vw = arena_push_nz(_render_sokol_state->arena, _Render_Sokol_Vulkan_Win, 1);
    MemSetZeroStruct(vw);
    vw->window = window;
    
    // Create surface
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = _wl_x11_state->connection,
        .window = window_os->xwindow,
    };
    if (vkCreateXcbSurfaceKHR(_sg_vk_instance, &surface_create_info, NULL, &vw->surface) != VK_SUCCESS)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create Vulkan surface for SOKOL.");
        exit(1);
    }
    
    // Get capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_sg_vk_phys_dev, vw->surface, &capabilities);
    
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_sg_vk_phys_dev, vw->surface, &format_count, NULL);
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_sg_vk_phys_dev, vw->surface, &format_count, formats);
    
    VkFormat swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    for (uint32_t i = 0; i < format_count; i++)
    {
        if (formats[i].format == VK_FORMAT_B8G8R8A8_UNORM || formats[i].format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            swapchain_format = formats[i].format;
            break;
        }
    }
    free(formats);
    vw->swapchain_format = swapchain_format;
    
    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0xFFFFFFFF)
    {
        Rng2_F32 canvas_rect = wl_canvas_rect_from_window(window);
        Vec2_F32 canvas_rect_dim = dim2(canvas_rect);
        extent.width = (uint32_t)canvas_rect_dim.x;
        extent.height = (uint32_t)canvas_rect_dim.y;
    }
    vw->swapchain_extent = extent;
    vw->last_canvas_rect_dim = (Vec2_F32){ (float)extent.width, (float)extent.height };
    
    // Create swapchain
    uint32_t min_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && min_image_count > capabilities.maxImageCount)
    {
        min_image_count = capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vw->surface,
        .minImageCount = min_image_count,
        .imageFormat = swapchain_format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_FIFO_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };
    vkCreateSwapchainKHR(_sg_vk_device, &swapchain_info, NULL, &vw->swapchain);
    
    // Get swapchain images
    vkGetSwapchainImagesKHR(_sg_vk_device, vw->swapchain, &vw->image_count, NULL);
    vw->images = malloc(sizeof(VkImage) * vw->image_count);
    vkGetSwapchainImagesKHR(_sg_vk_device, vw->swapchain, &vw->image_count, vw->images);
    
    // Create image views
    vw->image_views = malloc(sizeof(VkImageView) * vw->image_count);
    for (uint32_t i = 0; i < vw->image_count; i++)
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = vw->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain_format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };
        vkCreateImageView(_sg_vk_device, &view_info, NULL, &vw->image_views[i]);
    }
    
    // Create sync primitives
    VkSemaphoreCreateInfo sem_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(_sg_vk_device, &sem_info, NULL, &vw->image_available_semaphore);
    vkCreateSemaphore(_sg_vk_device, &sem_info, NULL, &vw->render_finished_semaphore);
    
    Render_Handle h = { (uint64_t)(uintptr_t)vw };
    return h;
}

internal void _render_sokol_window_close(Wl_Window window, _Render_Sokol_Window *sw)
{
    (void)window;
    _Render_Sokol_Vulkan_Win *vw = (_Render_Sokol_Vulkan_Win *)(uintptr_t)sw->platform_handle.u64[0];
    if (vw != NULL)
    {
        vkDeviceWaitIdle(_sg_vk_device);
        
        vkDestroySemaphore(_sg_vk_device, vw->image_available_semaphore, NULL);
        vkDestroySemaphore(_sg_vk_device, vw->render_finished_semaphore, NULL);
        
        for (uint32_t i = 0; i < vw->image_count; i++)
        {
            vkDestroyImageView(_sg_vk_device, vw->image_views[i], NULL);
        }
        free(vw->images);
        free(vw->image_views);
        
        vkDestroySwapchainKHR(_sg_vk_device, vw->swapchain, NULL);
        vkDestroySurfaceKHR(_sg_vk_instance, vw->surface, NULL);
    }
}

internal void _render_sokol_window_make_current(Wl_Window window, _Render_Sokol_Window *sw)
{
    _Render_Sokol_Vulkan_Win *vw = (_Render_Sokol_Vulkan_Win *)(uintptr_t)sw->platform_handle.u64[0];
    VkResult res = vkAcquireNextImageKHR(_sg_vk_device, vw->swapchain, UINT64_MAX, vw->image_available_semaphore, VK_NULL_HANDLE, &vw->current_image_idx);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR)
    {
        Rng2_F32 canvas = wl_canvas_rect_from_window(window);
        Vec2_F32 vp_dim = dim2(canvas);
        int w = (int)vp_dim.x;
        int h = (int)vp_dim.y;
        if (w > 0 && h > 0)
        {
            _render_sokol_vulkan_win_resize(vw, w, h);
            vkAcquireNextImageKHR(_sg_vk_device, vw->swapchain, UINT64_MAX, vw->image_available_semaphore, VK_NULL_HANDLE, &vw->current_image_idx);
        }
    }
}

internal void _render_sokol_present_window(Wl_Window window, _Render_Sokol_Window *sw)
{
    (void)window;
    _Render_Sokol_Vulkan_Win *vw = (_Render_Sokol_Vulkan_Win *)(uintptr_t)sw->platform_handle.u64[0];
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &vw->render_finished_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &vw->swapchain,
        .pImageIndices = &vw->current_image_idx,
    };
    vkQueuePresentKHR(_sg_vk_queue, &present_info);
}

// ============================================================================
#elif OS_WINDOWS
// ============================================================================

#include <d3d11.h>
#include <dxgi.h>

typedef struct _Render_Sokol_D3D11_Win _Render_Sokol_D3D11_Win;
struct _Render_Sokol_D3D11_Win
{
    IDXGISwapChain         *swap_chain;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    ID3D11Texture2D        *ds_tex;
    int                     w, h;
};

global ID3D11Device        *_sg_d3d11_dev = NULL;
global ID3D11DeviceContext *_sg_d3d11_ctx = NULL;

internal void _render_sokol_platform_init(void)
{
    D3D_FEATURE_LEVEL fl[] = { D3D_FEATURE_LEVEL_11_0 };
    UINT flags = 0;
#if BUILD_DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, flags,
                      fl, 1, D3D11_SDK_VERSION,
                      &_sg_d3d11_dev, NULL, &_sg_d3d11_ctx);
}

internal void _render_sokol_platform_deinit(void)
{
    if(_sg_d3d11_ctx) _sg_d3d11_ctx->lpVtbl->Release(_sg_d3d11_ctx);
    if(_sg_d3d11_dev) _sg_d3d11_dev->lpVtbl->Release(_sg_d3d11_dev);
}

internal sg_environment _render_sokol_build_environment(void)
{
    sg_environment env = {0};
    env.defaults.color_format = SG_PIXELFORMAT_BGRA8;
    env.defaults.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    env.defaults.sample_count = 1;
    env.d3d11.device         = (const void *)_sg_d3d11_dev;
    env.d3d11.device_context = (const void *)_sg_d3d11_ctx;
    return env;
}

internal _Render_Sokol_D3D11_Win *
_render_sokol_d3d11_win_alloc(HWND hwnd, int w, int h)
{
    _Render_Sokol_D3D11_Win *dw = push_array(_render_sokol_state->arena,
                                              _Render_Sokol_D3D11_Win, 1);

    IDXGIDevice  *dxgi_dev = NULL;
    IDXGIAdapter *adapter  = NULL;
    IDXGIFactory *factory  = NULL;
    _sg_d3d11_dev->lpVtbl->QueryInterface(_sg_d3d11_dev,
        &IID_IDXGIDevice, (void**)&dxgi_dev);
    dxgi_dev->lpVtbl->GetAdapter(dxgi_dev, &adapter);
    adapter->lpVtbl->GetParent(adapter, &IID_IDXGIFactory, (void**)&factory);

    DXGI_SWAP_CHAIN_DESC scd = {0};
    scd.BufferCount       = 2;
    scd.BufferDesc.Width  = w; scd.BufferDesc.Height = h;
    scd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    scd.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow      = hwnd;
    scd.SampleDesc.Count  = 1;
    scd.Windowed          = TRUE;
    scd.SwapEffect        = DXGI_SWAP_EFFECT_DISCARD;
    factory->lpVtbl->CreateSwapChain(factory,
        (IUnknown*)_sg_d3d11_dev, &scd, &dw->swap_chain);
    factory->lpVtbl->Release(factory);
    adapter->lpVtbl->Release(adapter);
    dxgi_dev->lpVtbl->Release(dxgi_dev);

    ID3D11Texture2D *bb = NULL;
    dw->swap_chain->lpVtbl->GetBuffer(dw->swap_chain, 0,
        &IID_ID3D11Texture2D, (void**)&bb);
    _sg_d3d11_dev->lpVtbl->CreateRenderTargetView(_sg_d3d11_dev,
        (ID3D11Resource*)bb, NULL, &dw->rtv);
    bb->lpVtbl->Release(bb);

    D3D11_TEXTURE2D_DESC dsd = {0};
    dsd.Width=w; dsd.Height=h; dsd.MipLevels=1; dsd.ArraySize=1;
    dsd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsd.SampleDesc.Count=1; dsd.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    _sg_d3d11_dev->lpVtbl->CreateTexture2D(_sg_d3d11_dev, &dsd, NULL, &dw->ds_tex);
    _sg_d3d11_dev->lpVtbl->CreateDepthStencilView(_sg_d3d11_dev,
        (ID3D11Resource*)dw->ds_tex, NULL, &dw->dsv);
    dw->w=w; dw->h=h;
    return dw;
}

internal void
_render_sokol_d3d11_win_resize(_Render_Sokol_D3D11_Win *dw, int w, int h)
{
    if(dw->rtv)    dw->rtv->lpVtbl->Release(dw->rtv);
    if(dw->dsv)    dw->dsv->lpVtbl->Release(dw->dsv);
    if(dw->ds_tex) dw->ds_tex->lpVtbl->Release(dw->ds_tex);
    dw->rtv=NULL; dw->dsv=NULL; dw->ds_tex=NULL;
    dw->swap_chain->lpVtbl->ResizeBuffers(dw->swap_chain,0,w,h,DXGI_FORMAT_UNKNOWN,0);
    ID3D11Texture2D *bb=NULL;
    dw->swap_chain->lpVtbl->GetBuffer(dw->swap_chain,0,&IID_ID3D11Texture2D,(void**)&bb);
    _sg_d3d11_dev->lpVtbl->CreateRenderTargetView(_sg_d3d11_dev,(ID3D11Resource*)bb,NULL,&dw->rtv);
    bb->lpVtbl->Release(bb);
    D3D11_TEXTURE2D_DESC dsd={0};
    dsd.Width=w; dsd.Height=h; dsd.MipLevels=1; dsd.ArraySize=1;
    dsd.Format=DXGI_FORMAT_D24_UNORM_S8_UINT; dsd.SampleDesc.Count=1;
    dsd.BindFlags=D3D11_BIND_DEPTH_STENCIL;
    _sg_d3d11_dev->lpVtbl->CreateTexture2D(_sg_d3d11_dev,&dsd,NULL,&dw->ds_tex);
    _sg_d3d11_dev->lpVtbl->CreateDepthStencilView(_sg_d3d11_dev,(ID3D11Resource*)dw->ds_tex,NULL,&dw->dsv);
    dw->w=w; dw->h=h;
}

internal sg_swapchain
_render_sokol_swapchain_from_window(_Render_Sokol_Window *sw, int w, int h)
{
    _Render_Sokol_D3D11_Win *dw = (_Render_Sokol_D3D11_Win *)(uintptr_t)sw->platform_handle.u64[0];
    if(dw->w != w || dw->h != h) _render_sokol_d3d11_win_resize(dw, w, h);

    sg_swapchain sc = {0};
    sc.width  = w; sc.height = h;
    sc.sample_count = 1;
    sc.color_format = SG_PIXELFORMAT_BGRA8;
    sc.depth_format = SG_PIXELFORMAT_DEPTH_STENCIL;
    sc.d3d11.render_view        = (const void *)dw->rtv;
    sc.d3d11.depth_stencil_view = (const void *)dw->dsv;
    sc.d3d11.resolve_view       = NULL;
    return sc;
}

internal Render_Handle _render_sokol_window_open(Wl_Window window)
{
    _Wl_Win32_Window *w32 = (_Wl_Win32_Window *)(uintptr_t)window.u64[0];
    int wi = (int)(w32->rect.x1 - w32->rect.x0);
    int hi = (int)(w32->rect.y1 - w32->rect.y0);
    _Render_Sokol_D3D11_Win *dw = _render_sokol_d3d11_win_alloc(w32->hwnd, wi, hi);
    Render_Handle h = { (uint64_t)(uintptr_t)dw };
    return h;
}

internal void _render_sokol_window_close(Wl_Window window, _Render_Sokol_Window *sw)
{
    (void)window;
    _Render_Sokol_D3D11_Win *dw = (_Render_Sokol_D3D11_Win *)(uintptr_t)sw->platform_handle.u64[0];
    if(dw->rtv)       dw->rtv->lpVtbl->Release(dw->rtv);
    if(dw->dsv)       dw->dsv->lpVtbl->Release(dw->dsv);
    if(dw->ds_tex)    dw->ds_tex->lpVtbl->Release(dw->ds_tex);
    if(dw->swap_chain)dw->swap_chain->lpVtbl->Release(dw->swap_chain);
}

internal void _render_sokol_window_make_current(Wl_Window window, _Render_Sokol_Window *sw)
{
    (void)window; (void)sw; // D3D11 has no "current context"
}

internal void _render_sokol_present_window(Wl_Window window, _Render_Sokol_Window *sw)
{
    (void)window;
    _Render_Sokol_D3D11_Win *dw = (_Render_Sokol_D3D11_Win *)(uintptr_t)sw->platform_handle.u64[0];
    dw->swap_chain->lpVtbl->Present(dw->swap_chain, 1, 0);
}

#else
#   error "render_sokol.c: unsupported platform"
#endif
