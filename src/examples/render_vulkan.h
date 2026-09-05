#ifndef RENDER_VULKAN_H
#define RENDER_VULKAN_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Struct for the uniform data / push constants
typedef struct Vulkan_Push_Constants Vulkan_Push_Constants;
struct Vulkan_Push_Constants {
    Mat4x4_F32 texture_sample_channel_map;
    Vec4_F32 xform_col0;
    Vec4_F32 xform_col1;
    Vec4_F32 xform_col2;
    Vec2_F32 viewport_size_px;
    float opacity;
    float pad;
};

typedef struct _Render_Vulkan_Window _Render_Vulkan_Window;
struct _Render_Vulkan_Window
{
    _Render_Vulkan_Window *next;
    _Render_Vulkan_Window *prev;
    
    Wl_Window window;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;
    VkFormat swapchain_format;
    VkExtent2D swapchain_extent;
    
    uint32_t image_count;
    VkImage *images;
    VkImageView *image_views;
    VkFramebuffer *framebuffers;
    
    VkSemaphore image_available_semaphore;
    VkSemaphore render_finished_semaphore;
    VkFence in_flight_fence;
    
    Vec2_F32 last_canvas_rect_dim;
    uint32_t current_image_idx;
};

typedef struct _Render_Vulkan_Tex_2D _Render_Vulkan_Tex_2D;
struct _Render_Vulkan_Tex_2D
{
    _Render_Vulkan_Tex_2D *next;
    _Render_Vulkan_Tex_2D *prev;
    
    VkImage image;
    VkDeviceMemory memory;
    VkImageView image_view;
    VkDescriptorSet descriptor_set;
    
    Render_Resource_Kind resource_kind;
    Render_Tex_2D_Format format;
    Vec2_I32 size;
};

typedef struct _Render_Vulkan_State _Render_Vulkan_State;
struct _Render_Vulkan_State
{
    Arena *arena;
    
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    uint32_t graphics_queue_family_index;
    VkQueue graphics_queue;
    VkQueue present_queue;
    
    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_set_layout;
    VkPipelineLayout pipeline_layout;
    VkPipeline pipeline_rect;
    VkPipeline pipeline_tri;
    
    VkDescriptorPool descriptor_pool;
    VkSampler sampler_nearest;
    VkSampler sampler_linear;
    
    VkCommandPool command_pool;
    VkCommandBuffer command_buffer;
    
    _Render_Vulkan_Window *free_window;
    _Render_Vulkan_Tex_2D *free_tex2d;
    
    _Render_Vulkan_Tex_2D white_texture;
    
    VkBuffer instance_buffer;
    VkDeviceMemory instance_buffer_memory;
    size_t instance_buffer_size;
};

global _Render_Vulkan_State *_render_vulkan_state = NULL;

#endif // RENDER_VULKAN_H
