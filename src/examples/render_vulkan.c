#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//~ ak: Shader Sources
//=============================================================================

global const char *rect_vert_shader_src = 
    "#version 450\n"
    "\n"
    "layout(location = 0) in vec4 c2v_dst_rect;\n"
    "layout(location = 1) in vec4 c2v_src_rect;\n"
    "layout(location = 2) in vec4 c2v_colors_0;\n"
    "layout(location = 3) in vec4 c2v_colors_1;\n"
    "layout(location = 4) in vec4 c2v_colors_2;\n"
    "layout(location = 5) in vec4 c2v_colors_3;\n"
    "layout(location = 6) in vec4 c2v_corner_radii;\n"
    "layout(location = 7) in vec4 c2v_style; // x: border_thickness_px, y: softness_px, z: omit_texture, w: shear\n"
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
    "layout(push_constant) uniform Push {\n"
    "    mat4 u_texture_sample_channel_map;\n"
    "    vec4 u_xform_col0;\n"
    "    vec4 u_xform_col1;\n"
    "    vec4 u_xform_col2;\n"
    "    vec2 u_viewport_size_px;\n"
    "    float u_opacity;\n"
    "} u_push;\n"
    "\n"
    "void main(void)\n"
    "{\n"
    "  vec2 vertices[] = vec2[](vec2(-1, -1), vec2(-1, 1), vec2(1, -1), vec2(1, 1));\n"
    "  float shears[] = float[](0, 0, c2v_style.w, c2v_style.w);\n"
    "  \n"
    "  vec2 dst_half_size = (c2v_dst_rect.zw - c2v_dst_rect.xy) / 2;\n"
    "  vec2 dst_center    = (c2v_dst_rect.zw + c2v_dst_rect.xy) / 2;\n"
    "  vec2 dst_position  = vertices[gl_VertexIndex] * dst_half_size + dst_center;\n"
    "  dst_position.y += shears[gl_VertexIndex];\n"
    "  mat3 u_xform = mat3(u_push.u_xform_col0.xyz, u_push.u_xform_col1.xyz, u_push.u_xform_col2.xyz);\n"
    "  dst_position = (u_xform * vec3(dst_position.x, dst_position.y, 1)).xy;\n"
    "  \n"
    "  vec2 src_half_size = (c2v_src_rect.zw - c2v_src_rect.xy) / 2;\n"
    "  vec2 src_center    = (c2v_src_rect.zw + c2v_src_rect.xy) / 2;\n"
    "  vec2 src_position  = vertices[gl_VertexIndex] * src_half_size + src_center;\n"
    "  \n"
    "  vec4 colors[] = vec4[](c2v_colors_0, c2v_colors_1, c2v_colors_2, c2v_colors_3);\n"
    "  vec4 color = colors[gl_VertexIndex];\n"
    "  \n"
    "  float corner_radii[] = float[](c2v_corner_radii.x, c2v_corner_radii.y, c2v_corner_radii.z, c2v_corner_radii.w);\n"
    "  float corner_radius = corner_radii[gl_VertexIndex];\n"
    "  \n"
    "  vec2 dst_verts_pct = vec2(((gl_VertexIndex >> 1) != 1) ? 1.f : 0.f,\n"
    "                            ((gl_VertexIndex & 1) != 0)  ? 0.f : 1.f);\n"
    "  \n"
    "  gl_Position = vec4(2 * dst_position.x / u_push.u_viewport_size_px.x - 1,\n"
    "                     2 * dst_position.y / u_push.u_viewport_size_px.y - 1,\n"
    "                     0.0, 1.0);\n"
    "  \n"
    "  v2p_sdf_sample_pos    = (2.f * dst_verts_pct - 1.f) * dst_half_size;\n"
    "  v2p_texcoord_pct      = src_position;\n"
    "  v2p_rect_half_size_px = dst_half_size;\n"
    "  v2p_tint              = color;\n"
    "  v2p_corner_radius     = corner_radius;\n"
    "  v2p_border_thickness  = c2v_style.x;\n"
    "  v2p_softness          = c2v_style.y;\n"
    "  v2p_omit_texture      = c2v_style.z;\n"
    "}\n";

global const char *rect_frag_shader_src = 
    "#version 450\n"
    "\n"
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
    "layout(push_constant) uniform Push {\n"
    "    mat4 u_texture_sample_channel_map;\n"
    "    vec4 u_xform_col0;\n"
    "    vec4 u_xform_col1;\n"
    "    vec4 u_xform_col2;\n"
    "    vec2 u_viewport_size_px;\n"
    "    float u_opacity;\n"
    "} u_push;\n"
    "\n"
    "layout(binding = 0) uniform sampler2D u_tex_color;\n"
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
    "  vec4 albedo_sample = vec4(1, 1, 1, 1);\n"
    "  if(v2p_omit_texture < 1)\n"
    "  {\n"
    "    ivec2 u_tex_color_size_i = textureSize(u_tex_color, 0);\n"
    "    vec2 u_tex_color_size = vec2(float(u_tex_color_size_i.x), float(u_tex_color_size_i.y));\n"
    "    vec2 uv = v2p_texcoord_pct / u_tex_color_size;\n"
    "    \n"
    "    albedo_sample = u_push.u_texture_sample_channel_map * texture(u_tex_color, uv);\n"
    "    albedo_sample = linear_from_srgba(albedo_sample);\n"
    "  }\n"
    "  \n"
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
    "  float corner_sdf_t = 1;\n"
    "  if(v2p_corner_radius > 0 || v2p_softness > 0.75f)\n"
    "  {\n"
    "    float corner_sdf_s = rect_sdf(v2p_sdf_sample_pos,\n"
    "                                  v2p_rect_half_size_px - vec2(v2p_softness*2.f, v2p_softness*2.f),\n"
    "                                  v2p_corner_radius);\n"
    "    corner_sdf_t = 1-smoothstep(0, 2*v2p_softness, corner_sdf_s);\n"
    "  }\n"
    "  \n"
    "  final_color = albedo_sample;\n"
    "  final_color *= v2p_tint;\n"
    "  final_color.a *= u_push.u_opacity;\n"
    "  final_color.a *= corner_sdf_t;\n"
    "  final_color.a *= border_sdf_t;\n"
    "}\n";

global const char *tri_vert_shader_src = 
    "#version 450\n"
    "void main() {\n"
    "    vec2 vertices[3] = vec2[](\n"
    "        vec2(0.0, -0.5),\n"
    "        vec2(-0.5, 0.5),\n"
    "        vec2(0.5, 0.5)\n"
    "    );\n"
    "    gl_Position = vec4(vertices[gl_VertexIndex], 0.0, 1.0);\n"
    "}\n";

global const char *tri_frag_shader_src = 
    "#version 450\n"
    "layout(location = 0) out vec4 color;\n"
    "void main() {\n"
    "    color = vec4(1.0, 1.0, 0.0, 1.0);\n"
    "}\n";

//~ ak: Helper Functions
//=============================================================================

internal uint32_t _vulkan_find_memory_type(VkPhysicalDevice physical_device, uint32_t type_filter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }
    return 0;
}

internal VkShaderModule _vulkan_create_shader_module(VkDevice device, const char *src, const char *stage)
{
    pid_t pid = getpid();
    char glsl_path[256];
    char spv_path[256];
    sprintf(glsl_path, "/tmp/shader_%d.%s", pid, stage);
    sprintf(spv_path, "/tmp/shader_%d.%s.spv", pid, stage);

    FILE *f = fopen(glsl_path, "wb");
    if (!f) return VK_NULL_HANDLE;
    fwrite(src, 1, strlen(src), f);
    fclose(f);

    char cmd[1024];
    sprintf(cmd, "glslc -fshader-stage=%s %s -o %s", stage, glsl_path, spv_path);
    int status = system(cmd);
    unlink(glsl_path);
    if (status != 0) {
        return VK_NULL_HANDLE;
    }

    FILE *spvf = fopen(spv_path, "rb");
    if (!spvf) return VK_NULL_HANDLE;
    fseek(spvf, 0, SEEK_END);
    long size = ftell(spvf);
    fseek(spvf, 0, SEEK_SET);

    uint32_t *code = malloc(size);
    fread(code, 1, size, spvf);
    fclose(spvf);
    unlink(spv_path);

    VkShaderModuleCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = size,
        .pCode = code,
    };
    VkShaderModule module;
    VkResult res = vkCreateShaderModule(device, &create_info, NULL, &module);
    free(code);
    if (res != VK_SUCCESS) return VK_NULL_HANDLE;
    return module;
}

internal VkFormat _vulkan_format_from_tex2d_format(Render_Tex_2D_Format format)
{
    switch(format)
    {
        case Render_Tex_2D_Format_R8:     return VK_FORMAT_R8_UNORM;
        case Render_Tex_2D_Format_RG8:    return VK_FORMAT_R8G8_UNORM;
        case Render_Tex_2D_Format_RGBA8:  return VK_FORMAT_R8G8B8A8_UNORM;
        case Render_Tex_2D_Format_BGRA8:  return VK_FORMAT_B8G8R8A8_UNORM;
        case Render_Tex_2D_Format_R16:    return VK_FORMAT_R16_UNORM;
        case Render_Tex_2D_Format_RGBA16: return VK_FORMAT_R16G16B16A16_UNORM;
        case Render_Tex_2D_Format_R32:    return VK_FORMAT_R32_SFLOAT;
        case Render_Tex_2D_Format_RG32:   return VK_FORMAT_R32G32_SFLOAT;
        case Render_Tex_2D_Format_RGBA32: return VK_FORMAT_R32G32B32A32_SFLOAT;
        default:                         return VK_FORMAT_R8G8B8A8_UNORM;
    }
}

internal void _vulkan_init_render_pass_and_pipelines(VkFormat swapchain_format)
{
    VkDevice device = _render_vulkan_state->device;

    // 1. Create Render Pass
    VkAttachmentDescription color_attachment = {
        .format = swapchain_format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
    };
    VkAttachmentReference color_attachment_ref = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    };
    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_ref,
    };
    VkSubpassDependency dependency = {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    };
    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency,
    };
    vkCreateRenderPass(device, &render_pass_info, NULL, &_render_vulkan_state->render_pass);

    // 2. Load Shaders
    VkShaderModule rect_vert = _vulkan_create_shader_module(device, rect_vert_shader_src, "vert");
    VkShaderModule rect_frag = _vulkan_create_shader_module(device, rect_frag_shader_src, "frag");
    VkShaderModule tri_vert = _vulkan_create_shader_module(device, tri_vert_shader_src, "vert");
    VkShaderModule tri_frag = _vulkan_create_shader_module(device, tri_frag_shader_src, "frag");

    // 3. Create Rect Pipeline
    VkPipelineShaderStageCreateInfo rect_stages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = rect_vert,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = rect_frag,
            .pName = "main",
        }
    };

    VkVertexInputBindingDescription rect_binding = {
        .binding = 0,
        .stride = sizeof(Render_Rect_2D_Inst),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE,
    };

    VkVertexInputAttributeDescription rect_attrs[8] = {
        { .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, dst) },
        { .location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, src) },
        { .location = 2, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, colors[0]) },
        { .location = 3, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, colors[1]) },
        { .location = 4, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, colors[2]) },
        { .location = 5, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, colors[3]) },
        { .location = 6, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, corner_radii[0]) },
        { .location = 7, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = offsetof(Render_Rect_2D_Inst, border_thickness) },
    };

    VkPipelineVertexInputStateCreateInfo rect_vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &rect_binding,
        .vertexAttributeDescriptionCount = 8,
        .pVertexAttributeDescriptions = rect_attrs,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
    };

    VkPipelineViewportStateCreateInfo viewport_state = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_NONE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_TRUE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .alphaBlendOp = VK_BLEND_OP_ADD,
    };

    VkPipelineColorBlendStateCreateInfo color_blending = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment,
    };

    VkDynamicState dynamic_states[2] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamic_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = 2,
        .pDynamicStates = dynamic_states,
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = rect_stages,
        .pVertexInputState = &rect_vertex_input,
        .pInputAssemblyState = &input_assembly,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterizer,
        .pMultisampleState = &multisampling,
        .pColorBlendState = &color_blending,
        .pDynamicState = &dynamic_state_info,
        .layout = _render_vulkan_state->pipeline_layout,
        .renderPass = _render_vulkan_state->render_pass,
        .subpass = 0,
    };
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &_render_vulkan_state->pipeline_rect);

    // 4. Create Tri Pipeline
    VkPipelineShaderStageCreateInfo tri_stages[2] = {
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = tri_vert,
            .pName = "main",
        },
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = tri_frag,
            .pName = "main",
        }
    };

    VkPipelineVertexInputStateCreateInfo tri_vertex_input = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo tri_input_assembly = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    color_blend_attachment.blendEnable = VK_FALSE;

    pipeline_info.pStages = tri_stages;
    pipeline_info.pVertexInputState = &tri_vertex_input;
    pipeline_info.pInputAssemblyState = &tri_input_assembly;
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipeline_info, NULL, &_render_vulkan_state->pipeline_tri);

    // Clean up shader modules
    vkDestroyShaderModule(device, rect_vert, NULL);
    vkDestroyShaderModule(device, rect_frag, NULL);
    vkDestroyShaderModule(device, tri_vert, NULL);
    vkDestroyShaderModule(device, tri_frag, NULL);
}

//~ ak: Core Functions
//=============================================================================

internal void render_init(void)
{
    Arena *arena = arena_alloc();
    _render_vulkan_state = arena_push(arena, _Render_Vulkan_State, 1);
    _render_vulkan_state->arena = arena;
    
    // Create VkInstance
    const char *extensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_XCB_SURFACE_EXTENSION_NAME
    };
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "cmono",
        .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
        .pEngineName = "cmono",
        .engineVersion = VK_MAKE_VERSION(1, 0, 0),
        .apiVersion = VK_API_VERSION_1_0,
    };
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = 2,
        .ppEnabledExtensionNames = extensions,
    };
    if (vkCreateInstance(&instance_info, NULL, &_render_vulkan_state->instance) != VK_SUCCESS)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create Vulkan instance.");
        exit(1);
    }
    
    // Select physical device
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(_render_vulkan_state->instance, &device_count, NULL);
    if (device_count == 0)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to find GPUs with Vulkan support.");
        exit(1);
    }
    VkPhysicalDevice *devices = malloc(sizeof(VkPhysicalDevice) * device_count);
    vkEnumeratePhysicalDevices(_render_vulkan_state->instance, &device_count, devices);
    _render_vulkan_state->physical_device = devices[0];
    free(devices);
    
    // Find graphics queue family
    uint32_t queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(_render_vulkan_state->physical_device, &queue_family_count, NULL);
    VkQueueFamilyProperties *queue_families = malloc(sizeof(VkQueueFamilyProperties) * queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(_render_vulkan_state->physical_device, &queue_family_count, queue_families);
    
    _render_vulkan_state->graphics_queue_family_index = 0xFFFFFFFF;
    for (uint32_t i = 0; i < queue_family_count; i++)
    {
        if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            _render_vulkan_state->graphics_queue_family_index = i;
            break;
        }
    }
    free(queue_families);
    
    if (_render_vulkan_state->graphics_queue_family_index == 0xFFFFFFFF)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to find a graphics queue family.");
        exit(1);
    }
    
    // Create logical device
    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = _render_vulkan_state->graphics_queue_family_index,
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };
    const char *device_extensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = 1,
        .ppEnabledExtensionNames = device_extensions,
    };
    if (vkCreateDevice(_render_vulkan_state->physical_device, &device_info, NULL, &_render_vulkan_state->device) != VK_SUCCESS)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create Vulkan logical device.");
        exit(1);
    }
    
    vkGetDeviceQueue(_render_vulkan_state->device, _render_vulkan_state->graphics_queue_family_index, 0, &_render_vulkan_state->graphics_queue);
    _render_vulkan_state->present_queue = _render_vulkan_state->graphics_queue;
    
    VkDevice device = _render_vulkan_state->device;
    
    // Create Descriptor Set Layout
    VkDescriptorSetLayoutBinding sampler_layout_binding = {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };
    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &sampler_layout_binding,
    };
    vkCreateDescriptorSetLayout(device, &layout_info, NULL, &_render_vulkan_state->descriptor_set_layout);
    
    // Create Pipeline Layout
    VkPushConstantRange push_constant_range = {
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(Vulkan_Push_Constants),
    };
    VkPipelineLayoutCreateInfo pipeline_layout_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &_render_vulkan_state->descriptor_set_layout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &push_constant_range,
    };
    vkCreatePipelineLayout(device, &pipeline_layout_info, NULL, &_render_vulkan_state->pipeline_layout);
    
    // Create Descriptor Pool
    VkDescriptorPoolSize pool_size = {
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1000,
    };
    VkDescriptorPoolCreateInfo pool_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1000,
        .poolSizeCount = 1,
        .pPoolSizes = &pool_size,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
    };
    vkCreateDescriptorPool(device, &pool_info, NULL, &_render_vulkan_state->descriptor_pool);
    
    // Create Samplers
    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_NEAREST,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_FALSE,
        .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_ALWAYS,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST,
    };
    vkCreateSampler(device, &sampler_info, NULL, &_render_vulkan_state->sampler_nearest);
    
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    vkCreateSampler(device, &sampler_info, NULL, &_render_vulkan_state->sampler_linear);
    
    // Create Command Pool & Command Buffer
    VkCommandPoolCreateInfo pool_info_cmd = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = _render_vulkan_state->graphics_queue_family_index,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
    };
    vkCreateCommandPool(device, &pool_info_cmd, NULL, &_render_vulkan_state->command_pool);
    
    VkCommandBufferAllocateInfo alloc_info_cmd = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = _render_vulkan_state->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };
    vkAllocateCommandBuffers(device, &alloc_info_cmd, &_render_vulkan_state->command_buffer);
    
    // Allocate instance buffer (4MB)
    _render_vulkan_state->instance_buffer_size = 4 * 1024 * 1024;
    VkBufferCreateInfo buf_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = _render_vulkan_state->instance_buffer_size,
        .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    vkCreateBuffer(device, &buf_info, NULL, &_render_vulkan_state->instance_buffer);
    
    VkMemoryRequirements mem_reqs;
    vkGetBufferMemoryRequirements(device, _render_vulkan_state->instance_buffer, &mem_reqs);
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = _vulkan_find_memory_type(_render_vulkan_state->physical_device, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    };
    vkAllocateMemory(device, &alloc_info, NULL, &_render_vulkan_state->instance_buffer_memory);
    vkBindBufferMemory(device, _render_vulkan_state->instance_buffer, _render_vulkan_state->instance_buffer_memory, 0);
    
    // Create White Texture
    uint32_t white_pixel = 0xFFFFFFFF;
    Render_Handle white_handle = render_tex2d_alloc(
        Render_Resource_Kind_Static,
        Render_Tex_2D_Format_RGBA8,
        (Vec2_I32){1, 1},
        &white_pixel,
        _render_vulkan_state->arena
    );
    _render_vulkan_state->white_texture = *(_Render_Vulkan_Tex_2D *)white_handle.u64[0];
}

internal void render_deinit(void)
{
    if (_render_vulkan_state != NULL)
    {
        VkDevice device = _render_vulkan_state->device;
        vkDeviceWaitIdle(device);
        
        // Free white texture resources
        vkDestroyImageView(device, _render_vulkan_state->white_texture.image_view, NULL);
        vkDestroyImage(device, _render_vulkan_state->white_texture.image, NULL);
        vkFreeMemory(device, _render_vulkan_state->white_texture.memory, NULL);
        
        // Destroy instance buffer
        vkDestroyBuffer(device, _render_vulkan_state->instance_buffer, NULL);
        vkFreeMemory(device, _render_vulkan_state->instance_buffer_memory, NULL);
        
        // Destroy samplers
        vkDestroySampler(device, _render_vulkan_state->sampler_nearest, NULL);
        vkDestroySampler(device, _render_vulkan_state->sampler_linear, NULL);
        
        // Destroy pools and layouts
        vkDestroyCommandPool(device, _render_vulkan_state->command_pool, NULL);
        vkDestroyDescriptorPool(device, _render_vulkan_state->descriptor_pool, NULL);
        
        if (_render_vulkan_state->pipeline_rect != VK_NULL_HANDLE) vkDestroyPipeline(device, _render_vulkan_state->pipeline_rect, NULL);
        if (_render_vulkan_state->pipeline_tri != VK_NULL_HANDLE) vkDestroyPipeline(device, _render_vulkan_state->pipeline_tri, NULL);
        if (_render_vulkan_state->pipeline_layout != VK_NULL_HANDLE) vkDestroyPipelineLayout(device, _render_vulkan_state->pipeline_layout, NULL);
        if (_render_vulkan_state->descriptor_set_layout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(device, _render_vulkan_state->descriptor_set_layout, NULL);
        if (_render_vulkan_state->render_pass != VK_NULL_HANDLE) vkDestroyRenderPass(device, _render_vulkan_state->render_pass, NULL);
        
        vkDestroyDevice(device, NULL);
        vkDestroyInstance(_render_vulkan_state->instance, NULL);
        
        arena_free(_render_vulkan_state->arena);
        _render_vulkan_state = NULL;
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
    
    _Render_Vulkan_Window *w = _render_vulkan_state->free_window;
    if (w)
    {
        SLLStackPop(_render_vulkan_state->free_window);
    }
    else
    {
        w = arena_push_nz(_render_vulkan_state->arena, _Render_Vulkan_Window, 1);
    }
    MemSetZeroStruct(w);
    w->window = window;
    
    // Create surface
    VkXcbSurfaceCreateInfoKHR surface_create_info = {
        .sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
        .connection = _wl_x11_state->connection,
        .window = window_os->xwindow,
    };
    if (vkCreateXcbSurfaceKHR(_render_vulkan_state->instance, &surface_create_info, NULL, &w->surface) != VK_SUCCESS)
    {
        LogErrorLine(&_os_core_state.log_context, "Failed to create Vulkan surface.");
        exit(1);
    }
    
    // Get capabilities
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_render_vulkan_state->physical_device, w->surface, &capabilities);
    
    uint32_t format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(_render_vulkan_state->physical_device, w->surface, &format_count, NULL);
    VkSurfaceFormatKHR *formats = malloc(sizeof(VkSurfaceFormatKHR) * format_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(_render_vulkan_state->physical_device, w->surface, &format_count, formats);
    
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
    w->swapchain_format = swapchain_format;
    
    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == 0xFFFFFFFF)
    {
        Rng2_F32 canvas_rect = wl_canvas_rect_from_window(window);
        Vec2_F32 canvas_rect_dim = dim2(canvas_rect);
        extent.width = (uint32_t)canvas_rect_dim.x;
        extent.height = (uint32_t)canvas_rect_dim.y;
    }
    w->swapchain_extent = extent;
    w->last_canvas_rect_dim = (Vec2_F32){ (float)extent.width, (float)extent.height };
    
    // Create render pass and pipelines if needed
    if (_render_vulkan_state->render_pass == VK_NULL_HANDLE)
    {
        _vulkan_init_render_pass_and_pipelines(swapchain_format);
    }
    
    // Create swapchain
    uint32_t min_image_count = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && min_image_count > capabilities.maxImageCount)
    {
        min_image_count = capabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchain_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = w->surface,
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
    vkCreateSwapchainKHR(_render_vulkan_state->device, &swapchain_info, NULL, &w->swapchain);
    
    // Get swapchain images
    vkGetSwapchainImagesKHR(_render_vulkan_state->device, w->swapchain, &w->image_count, NULL);
    w->images = malloc(sizeof(VkImage) * w->image_count);
    vkGetSwapchainImagesKHR(_render_vulkan_state->device, w->swapchain, &w->image_count, w->images);
    
    // Create image views
    w->image_views = malloc(sizeof(VkImageView) * w->image_count);
    for (uint32_t i = 0; i < w->image_count; i++)
    {
        VkImageViewCreateInfo view_info = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = w->images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = swapchain_format,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
        };
        vkCreateImageView(_render_vulkan_state->device, &view_info, NULL, &w->image_views[i]);
    }
    
    // Create framebuffers
    w->framebuffers = malloc(sizeof(VkFramebuffer) * w->image_count);
    for (uint32_t i = 0; i < w->image_count; i++)
    {
        VkFramebufferCreateInfo fb_info = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = _render_vulkan_state->render_pass,
            .attachmentCount = 1,
            .pAttachments = &w->image_views[i],
            .width = extent.width,
            .height = extent.height,
            .layers = 1,
        };
        vkCreateFramebuffer(_render_vulkan_state->device, &fb_info, NULL, &w->framebuffers[i]);
    }
    
    // Create sync primitives
    VkSemaphoreCreateInfo sem_info = { .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    vkCreateSemaphore(_render_vulkan_state->device, &sem_info, NULL, &w->image_available_semaphore);
    vkCreateSemaphore(_render_vulkan_state->device, &sem_info, NULL, &w->render_finished_semaphore);
    
    VkFenceCreateInfo fence_info = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT,
    };
    vkCreateFence(_render_vulkan_state->device, &fence_info, NULL, &w->in_flight_fence);
    
    Render_Handle result = {(uint64_t)w};
    return result;
}

internal void render_window_unequip(Wl_Window window, Render_Handle window_equip)
{
    Unused(window);
    _Render_Vulkan_Window *w = (_Render_Vulkan_Window *)window_equip.u64[0];
    if (w != NULL)
    {
        VkDevice device = _render_vulkan_state->device;
        vkDeviceWaitIdle(device);
        
        vkDestroySemaphore(device, w->image_available_semaphore, NULL);
        vkDestroySemaphore(device, w->render_finished_semaphore, NULL);
        vkDestroyFence(device, w->in_flight_fence, NULL);
        
        for (uint32_t i = 0; i < w->image_count; i++)
        {
            vkDestroyFramebuffer(device, w->framebuffers[i], NULL);
            vkDestroyImageView(device, w->image_views[i], NULL);
        }
        free(w->images);
        free(w->image_views);
        free(w->framebuffers);
        
        vkDestroySwapchainKHR(device, w->swapchain, NULL);
        vkDestroySurfaceKHR(_render_vulkan_state->instance, w->surface, NULL);
        
        SLLStackPush(_render_vulkan_state->free_window, w);
    }
}

internal void render_window_begin_frame(Wl_Window window, Render_Handle handle)
{
    _Render_Vulkan_Window *w = (_Render_Vulkan_Window *)handle.u64[0];
    
    Rng2_F32 canvas_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 canvas_rect_dim = dim2(canvas_rect);
    
    // Check if swapchain size changed
    if (canvas_rect_dim.x != w->last_canvas_rect_dim.x || canvas_rect_dim.y != w->last_canvas_rect_dim.y)
    {
        vkDeviceWaitIdle(_render_vulkan_state->device);
        w->last_canvas_rect_dim = canvas_rect_dim;
        
        for (uint32_t i = 0; i < w->image_count; i++)
        {
            vkDestroyFramebuffer(_render_vulkan_state->device, w->framebuffers[i], NULL);
            vkDestroyImageView(_render_vulkan_state->device, w->image_views[i], NULL);
        }
        VkSwapchainKHR old_swapchain = w->swapchain;
        
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_render_vulkan_state->physical_device, w->surface, &capabilities);
        VkExtent2D extent = capabilities.currentExtent;
        if (extent.width == 0xFFFFFFFF)
        {
            extent.width = (uint32_t)canvas_rect_dim.x;
            extent.height = (uint32_t)canvas_rect_dim.y;
        }
        w->swapchain_extent = extent;
        
        uint32_t min_image_count = capabilities.minImageCount + 1;
        if (capabilities.maxImageCount > 0 && min_image_count > capabilities.maxImageCount)
        {
            min_image_count = capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR swapchain_info = {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = w->surface,
            .minImageCount = min_image_count,
            .imageFormat = w->swapchain_format,
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
        vkCreateSwapchainKHR(_render_vulkan_state->device, &swapchain_info, NULL, &w->swapchain);
        vkDestroySwapchainKHR(_render_vulkan_state->device, old_swapchain, NULL);
        
        vkGetSwapchainImagesKHR(_render_vulkan_state->device, w->swapchain, &w->image_count, NULL);
        w->images = realloc(w->images, sizeof(VkImage) * w->image_count);
        vkGetSwapchainImagesKHR(_render_vulkan_state->device, w->swapchain, &w->image_count, w->images);
        
        w->image_views = realloc(w->image_views, sizeof(VkImageView) * w->image_count);
        for (uint32_t i = 0; i < w->image_count; i++)
        {
            VkImageViewCreateInfo view_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = w->images[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = w->swapchain_format,
                .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel = 0,
                .subresourceRange.levelCount = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount = 1,
            };
            vkCreateImageView(_render_vulkan_state->device, &view_info, NULL, &w->image_views[i]);
        }
        
        w->framebuffers = realloc(w->framebuffers, sizeof(VkFramebuffer) * w->image_count);
        for (uint32_t i = 0; i < w->image_count; i++)
        {
            VkFramebufferCreateInfo fb_info = {
                .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass = _render_vulkan_state->render_pass,
                .attachmentCount = 1,
                .pAttachments = &w->image_views[i],
                .width = extent.width,
                .height = extent.height,
                .layers = 1,
            };
            vkCreateFramebuffer(_render_vulkan_state->device, &fb_info, NULL, &w->framebuffers[i]);
        }
    }
    
    vkWaitForFences(_render_vulkan_state->device, 1, &w->in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(_render_vulkan_state->device, 1, &w->in_flight_fence);
    
    VkResult acquire_res = vkAcquireNextImageKHR(_render_vulkan_state->device, w->swapchain, UINT64_MAX, w->image_available_semaphore, VK_NULL_HANDLE, &w->current_image_idx);
    if (acquire_res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        // Recreate next frame
    }
}

internal void render_window_end_frame(Wl_Window window, Render_Handle handle)
{
    Unused(window);
    _Render_Vulkan_Window *w = (_Render_Vulkan_Window *)handle.u64[0];
    
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &w->render_finished_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &w->swapchain,
        .pImageIndices = &w->current_image_idx,
    };
    VkResult present_res = vkQueuePresentKHR(_render_vulkan_state->graphics_queue, &present_info);
    if (present_res == VK_ERROR_OUT_OF_DATE_KHR || present_res == VK_SUBOPTIMAL_KHR)
    {
        // Recreate next frame
    }
}

internal void render_window_submit(Wl_Window window, Render_Handle window_equip, Render_Pass_List *passes)
{
    _Render_Vulkan_Window *w = (_Render_Vulkan_Window *)window_equip.u64[0];
    Rng2_F32 viewport_rect = wl_canvas_rect_from_window(window);
    Vec2_F32 viewport_dim = dim2(viewport_rect);
    
    // Reset & Begin Command Buffer
    vkResetCommandBuffer(_render_vulkan_state->command_buffer, 0);
    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(_render_vulkan_state->command_buffer, &begin_info);
    
    // Begin Render Pass
    VkClearValue clear_value = {{{0.0f, 0.0f, 0.0f, 0.0f}}};
    VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = _render_vulkan_state->render_pass,
        .framebuffer = w->framebuffers[w->current_image_idx],
        .renderArea.offset = {0, 0},
        .renderArea.extent = w->swapchain_extent,
        .clearValueCount = 1,
        .pClearValues = &clear_value,
    };
    vkCmdBeginRenderPass(_render_vulkan_state->command_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    
    // Viewport & Scissor Setup
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)w->swapchain_extent.width,
        .height = (float)w->swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(_render_vulkan_state->command_buffer, 0, 1, &viewport);
    
    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = w->swapchain_extent,
    };
    vkCmdSetScissor(_render_vulkan_state->command_buffer, 0, 1, &scissor);
    
    size_t instance_buffer_offset = 0;
    
    for (Render_Pass_Node *pass_n = passes->first; pass_n != 0; pass_n = pass_n->next)
    {
        Render_Pass *pass = &pass_n->v;
        switch (pass->kind)
        {
            case Render_Pass_Kind_UI:
            {
                Render_Pass_Params_UI *params = pass->params_ui;
                _Render_Batch_Group_2D_List *rect_batch_groups = &params->rects;
                
                vkCmdBindPipeline(_render_vulkan_state->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_vulkan_state->pipeline_rect);
                
                for (_Render_Batch_Group_2D_Node *group_n = rect_batch_groups->first; group_n != 0; group_n = group_n->next)
                {
                    Render_Batch_List *batches = &group_n->batches;
                    _Render_Batch_Group_2D_Params *group_params = &group_n->params;
                    
                    if (batches->byte_count == 0) continue;
                    
                    // Copy batch instances to mapped instance buffer
                    if (instance_buffer_offset + batches->byte_count > _render_vulkan_state->instance_buffer_size)
                    {
                        // Buffer overflow safeguard, wrap around
                        instance_buffer_offset = 0;
                    }
                    
                    void *mapped;
                    vkMapMemory(_render_vulkan_state->device, _render_vulkan_state->instance_buffer_memory, instance_buffer_offset, batches->byte_count, 0, &mapped);
                    size_t batch_offset = 0;
                    for (Render_Batch_Node *batch_n = batches->first; batch_n != 0; batch_n = batch_n->next)
                    {
                        memcpy((uint8_t *)mapped + batch_offset, batch_n->v.v, batch_n->v.byte_count);
                        batch_offset += batch_n->v.byte_count;
                    }
                    vkUnmapMemory(_render_vulkan_state->device, _render_vulkan_state->instance_buffer_memory);
                    
                    // Bind vertex/instance buffer
                    VkDeviceSize offsets[] = {instance_buffer_offset};
                    vkCmdBindVertexBuffers(_render_vulkan_state->command_buffer, 0, 1, &_render_vulkan_state->instance_buffer, offsets);
                    instance_buffer_offset += batches->byte_count;
                    
                    // Dynamic texture binding
                    Render_Tex_2D_Format texture_fmt = Render_Tex_2D_Format_RGBA8;
                    _Render_Vulkan_Tex_2D *tex = (_Render_Vulkan_Tex_2D *)group_params->tex.u64[0];
                    if (!tex)
                    {
                        tex = &_render_vulkan_state->white_texture;
                    }
                    texture_fmt = tex->format;
                    
                    // Update sampler choice in descriptor set
                    VkDescriptorImageInfo image_info_desc = {
                        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                        .imageView = tex->image_view,
                        .sampler = (group_params->tex_sample_kind == Render_Tex_2d_Sample_Kind_Linear) ? _render_vulkan_state->sampler_linear : _render_vulkan_state->sampler_nearest,
                    };
                    VkWriteDescriptorSet write_desc = {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = tex->descriptor_set,
                        .dstBinding = 0,
                        .descriptorCount = 1,
                        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                        .pImageInfo = &image_info_desc,
                    };
                    vkUpdateDescriptorSets(_render_vulkan_state->device, 1, &write_desc, 0, NULL);
                    
                    vkCmdBindDescriptorSets(_render_vulkan_state->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_vulkan_state->pipeline_layout, 0, 1, &tex->descriptor_set, 0, NULL);
                    
                    // Prepare and submit Push Constants
                    Vulkan_Push_Constants push = {0};
                    push.texture_sample_channel_map = render_sample_channel_map_from_tex2dformat(texture_fmt);
                    push.xform_col0 = (Vec4_F32){ group_params->xform.v[0][0], group_params->xform.v[0][1], group_params->xform.v[0][2], 0.0f };
                    push.xform_col1 = (Vec4_F32){ group_params->xform.v[1][0], group_params->xform.v[1][1], group_params->xform.v[1][2], 0.0f };
                    push.xform_col2 = (Vec4_F32){ group_params->xform.v[2][0], group_params->xform.v[2][1], group_params->xform.v[2][2], 0.0f };
                    push.viewport_size_px = viewport_dim;
                    push.opacity = 1.0f - group_params->transparency;
                    
                    vkCmdPushConstants(_render_vulkan_state->command_buffer, _render_vulkan_state->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(push), &push);
                    
                    // Scissor Test
                    if (group_params->clip.x0 != 0 || group_params->clip.x1 != 0 || group_params->clip.y0 != 0 || group_params->clip.y1 != 0)
                    {
                        VkRect2D clip_scissor = {
                            .offset = {(int32_t)group_params->clip.x0, (int32_t)group_params->clip.y0},
                            .extent = {
                                (uint32_t)(group_params->clip.x1 - group_params->clip.x0),
                                (uint32_t)(group_params->clip.y1 - group_params->clip.y0)
                            }
                        };
                        if (clip_scissor.offset.x < 0) clip_scissor.offset.x = 0;
                        if (clip_scissor.offset.y < 0) clip_scissor.offset.y = 0;
                        if (clip_scissor.offset.x + (int32_t)clip_scissor.extent.width > (int32_t)w->swapchain_extent.width)
                        {
                            clip_scissor.extent.width = w->swapchain_extent.width - clip_scissor.offset.x;
                        }
                        if (clip_scissor.offset.y + (int32_t)clip_scissor.extent.height > (int32_t)w->swapchain_extent.height)
                        {
                            clip_scissor.extent.height = w->swapchain_extent.height - clip_scissor.offset.y;
                        }
                        vkCmdSetScissor(_render_vulkan_state->command_buffer, 0, 1, &clip_scissor);
                    }
                    else
                    {
                        vkCmdSetScissor(_render_vulkan_state->command_buffer, 0, 1, &scissor);
                    }
                    
                    // Draw
                    uint32_t instance_count = (uint32_t)(batches->byte_count / batches->bytes_per_inst);
                    vkCmdDraw(_render_vulkan_state->command_buffer, 4, instance_count, 0, 0);
                }
            }
            break;
            
            case Render_Pass_Kind_Triangle:
            {
                vkCmdBindPipeline(_render_vulkan_state->command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _render_vulkan_state->pipeline_tri);
                vkCmdSetScissor(_render_vulkan_state->command_buffer, 0, 1, &scissor);
                vkCmdDraw(_render_vulkan_state->command_buffer, 3, 1, 0, 0);
            }
            break;
            
            default:
                break;
        }
    }
    
    // End Render Pass & Command Buffer
    vkCmdEndRenderPass(_render_vulkan_state->command_buffer);
    vkEndCommandBuffer(_render_vulkan_state->command_buffer);
    
    // Submit
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &w->image_available_semaphore,
        .pWaitDstStageMask = wait_stages,
        .commandBufferCount = 1,
        .pCommandBuffers = &_render_vulkan_state->command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &w->render_finished_semaphore,
    };
    vkQueueSubmit(_render_vulkan_state->graphics_queue, 1, &submit_info, w->in_flight_fence);
}

//~ ak: Texture functions
//=============================================================================

internal Render_Handle render_tex2d_alloc(Render_Resource_Kind kind, Render_Tex_2D_Format format, Vec2_I32 size, void *data, Arena *arena)
{
    _Render_Vulkan_Tex_2D *tex = _render_vulkan_state->free_tex2d;
    if (tex)
    {
        SLLStackPop(_render_vulkan_state->free_tex2d);
    }
    else
    {
        tex = arena_push(arena, _Render_Vulkan_Tex_2D, 1);
    }
    
    tex->resource_kind = kind;
    tex->format = format;
    tex->size = size;
    
    VkFormat vk_format = _vulkan_format_from_tex2d_format(format);
    
    // Create image
    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .extent.width = size.x,
        .extent.height = size.y,
        .extent.depth = 1,
        .mipLevels = 1,
        .arrayLayers = 1,
        .format = vk_format,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    vkCreateImage(_render_vulkan_state->device, &image_info, NULL, &tex->image);
    
    VkMemoryRequirements mem_reqs;
    vkGetImageMemoryRequirements(_render_vulkan_state->device, tex->image, &mem_reqs);
    
    VkMemoryAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = mem_reqs.size,
        .memoryTypeIndex = _vulkan_find_memory_type(_render_vulkan_state->physical_device, mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    vkAllocateMemory(_render_vulkan_state->device, &alloc_info, NULL, &tex->memory);
    vkBindImageMemory(_render_vulkan_state->device, tex->image, tex->memory, 0);
    
    // Upload image data if present
    if (data)
    {
        VkBuffer staging_buffer;
        VkDeviceMemory staging_memory;
        VkBufferCreateInfo buffer_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size = mem_reqs.size,
            .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };
        vkCreateBuffer(_render_vulkan_state->device, &buffer_info, NULL, &staging_buffer);
        
        VkMemoryRequirements buf_mem_reqs;
        vkGetBufferMemoryRequirements(_render_vulkan_state->device, staging_buffer, &buf_mem_reqs);
        
        VkMemoryAllocateInfo buf_alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = buf_mem_reqs.size,
            .memoryTypeIndex = _vulkan_find_memory_type(_render_vulkan_state->physical_device, buf_mem_reqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        };
        vkAllocateMemory(_render_vulkan_state->device, &buf_alloc_info, NULL, &staging_memory);
        vkBindBufferMemory(_render_vulkan_state->device, staging_buffer, staging_memory, 0);
        
        void *mapped;
        vkMapMemory(_render_vulkan_state->device, staging_memory, 0, buf_mem_reqs.size, 0, &mapped);
        
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
        size_t raw_size = size.x * size.y * bpp;
        memcpy(mapped, data, raw_size);
        vkUnmapMemory(_render_vulkan_state->device, staging_memory);
        
        VkCommandBufferAllocateInfo cmd_alloc = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = _render_vulkan_state->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer temp_cmd;
        vkAllocateCommandBuffers(_render_vulkan_state->device, &cmd_alloc, &temp_cmd);
        
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(temp_cmd, &begin_info);
        
        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = tex->image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
        };
        vkCmdPipelineBarrier(temp_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        
        VkBufferImageCopy region = {
            .bufferOffset = 0,
            .bufferRowLength = 0,
            .bufferImageHeight = 0,
            .imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .imageSubresource.mipLevel = 0,
            .imageSubresource.baseArrayLayer = 0,
            .imageSubresource.layerCount = 1,
            .imageOffset = {0, 0, 0},
            .imageExtent = {size.x, size.y, 1},
        };
        vkCmdCopyBufferToImage(temp_cmd, staging_buffer, tex->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        vkCmdPipelineBarrier(temp_cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        
        vkEndCommandBuffer(temp_cmd);
        
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &temp_cmd,
        };
        vkQueueSubmit(_render_vulkan_state->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(_render_vulkan_state->graphics_queue);
        
        vkFreeCommandBuffers(_render_vulkan_state->device, _render_vulkan_state->command_pool, 1, &temp_cmd);
        vkDestroyBuffer(_render_vulkan_state->device, staging_buffer, NULL);
        vkFreeMemory(_render_vulkan_state->device, staging_memory, NULL);
    }
    else
    {
        VkCommandBufferAllocateInfo cmd_alloc = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = _render_vulkan_state->command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer temp_cmd;
        vkAllocateCommandBuffers(_render_vulkan_state->device, &cmd_alloc, &temp_cmd);
        
        VkCommandBufferBeginInfo begin_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };
        vkBeginCommandBuffer(temp_cmd, &begin_info);
        
        VkImageMemoryBarrier barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = tex->image,
            .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel = 0,
            .subresourceRange.levelCount = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount = 1,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        };
        vkCmdPipelineBarrier(temp_cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, NULL, 0, NULL, 1, &barrier);
        
        vkEndCommandBuffer(temp_cmd);
        
        VkSubmitInfo submit_info = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers = &temp_cmd,
        };
        vkQueueSubmit(_render_vulkan_state->graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
        vkQueueWaitIdle(_render_vulkan_state->graphics_queue);
        
        vkFreeCommandBuffers(_render_vulkan_state->device, _render_vulkan_state->command_pool, 1, &temp_cmd);
    }
    
    // Create image view
    VkImageViewCreateInfo view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = tex->image,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_format,
        .subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel = 0,
        .subresourceRange.levelCount = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount = 1,
    };
    vkCreateImageView(_render_vulkan_state->device, &view_info, NULL, &tex->image_view);
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo desc_alloc = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = _render_vulkan_state->descriptor_pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &_render_vulkan_state->descriptor_set_layout,
    };
    vkAllocateDescriptorSets(_render_vulkan_state->device, &desc_alloc, &tex->descriptor_set);
    
    // Default linear bind
    VkDescriptorImageInfo image_info_desc = {
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .imageView = tex->image_view,
        .sampler = _render_vulkan_state->sampler_nearest,
    };
    VkWriteDescriptorSet write_desc = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = tex->descriptor_set,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .pImageInfo = &image_info_desc,
    };
    vkUpdateDescriptorSets(_render_vulkan_state->device, 1, &write_desc, 0, NULL);
    
    Render_Handle handle = {(uint64_t)tex};
    return handle;
}

internal void render_tex2d_free(Render_Handle handle)
{
    _Render_Vulkan_Tex_2D *tex = (_Render_Vulkan_Tex_2D *)handle.u64[0];
    if (tex != 0)
    {
        VkDevice device = _render_vulkan_state->device;
        vkDeviceWaitIdle(device);
        
        vkDestroyImageView(device, tex->image_view, NULL);
        vkDestroyImage(device, tex->image, NULL);
        vkFreeMemory(device, tex->memory, NULL);
        
        SLLStackPush(_render_vulkan_state->free_tex2d, tex);
    }
}
