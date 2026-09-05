#ifndef RENDER_CORE_H
#define RENDER_CORE_H

// ak: Generated Code
//=============================================================================

#include "./generated/render.meta.h"

// ak: Types
//=============================================================================

typedef struct Render_Handle Render_Handle;
struct Render_Handle
{
    uint64_t u64[1];
};

// ak: Instance Types =========================================================

typedef struct Render_Rect_2D_Inst Render_Rect_2D_Inst;
struct Render_Rect_2D_Inst
{
    Rng2_F32 dst;
    Rng2_F32 src;
    Vec4_F32 colors[Corner_COUNT];
    float corner_radii[Corner_COUNT];
    float border_thickness;
    float edge_softness;
    float white_texture_override;
    float shear;
};

// ak: Batch Types ============================================================

typedef struct Render_Batch Render_Batch;
struct Render_Batch
{
    uint8_t *v;
    size_t byte_count;
    size_t byte_cap;
};

typedef struct Render_Batch_Node Render_Batch_Node;
struct Render_Batch_Node
{
    Render_Batch_Node *next;
    Render_Batch v;
};

typedef struct Render_Batch_List Render_Batch_List;
struct Render_Batch_List
{
    Render_Batch_Node *first;
    Render_Batch_Node *last;
    size_t batch_count;
    size_t byte_count;
    size_t bytes_per_inst;
};

typedef struct Render_Batch_Group_2D_Params Render_Batch_Group_2D_Params;
struct Render_Batch_Group_2D_Params
{
    Render_Handle tex;
    Render_Tex_2D_Sample_Kind tex_sample_kind;
    Mat3x3_F32 xform;
    Rng2_F32 clip;
    float transparency;
};

typedef struct Render_Batch_Group_2D_Node Render_Batch_Group_2D_Node;
struct Render_Batch_Group_2D_Node
{
    Render_Batch_Group_2D_Node *next;
    Render_Batch_List batches;
    Render_Batch_Group_2D_Params params;
};

typedef struct Render_Batch_Group_2D_List Render_Batch_Group_2D_List;
struct Render_Batch_Group_2D_List
{
    Render_Batch_Group_2D_Node *first;
    Render_Batch_Group_2D_Node *last;
    size_t count;
};

// ak: Pass Types =============================================================

typedef struct Render_Pass_Params_UI Render_Pass_Params_UI;
struct Render_Pass_Params_UI
{
    Render_Batch_Group_2D_List rects;
};

typedef struct Render_Pass Render_Pass;
struct Render_Pass
{
    Render_Pass_Kind kind;
    union
    {
        void *params;
        Render_Pass_Params_UI *params_ui;
    };
};

typedef struct Render_Pass_Node Render_Pass_Node;
struct Render_Pass_Node
{
    Render_Pass_Node *next;
    Render_Pass v;
};

typedef struct Render_Pass_List Render_Pass_List;
struct Render_Pass_List
{
    Render_Pass_Node *first;
    Render_Pass_Node *last;
    size_t count;
};

// ak: Functions
//=============================================================================

// ak: Helpers ================================================================

internal bool render_handle_match(Render_Handle a, Render_Handle b);
internal Render_Handle render_handle_zero(void);
internal Mat4x4_F32 render_sample_channel_map_from_tex2dformat(Render_Tex_2D_Format fmt);

// ak: Backend Hooks ==========================================================

// ak: layer initialization and cleanup
internal void render_init(void);
internal void render_cleanup(void);

// ak: window setup/teardown
internal Render_Handle render_window_equip(Wl_Window window);
internal void render_window_unequip(Wl_Window window, Render_Handle window_equip);

// ak: frame markers
internal void render_begin_frame(void);
internal void render_end_frame(void);
internal void render_window_begin_frame(Wl_Window window, Render_Handle handle);
internal void render_window_end_frame(Wl_Window window, Render_Handle handle);

// ak: render pass submission
internal void render_window_submit(Wl_Window window, Render_Handle window_equip, Render_Pass_List *passes);

// ak: Texture functions
internal Render_Handle render_tex2d_alloc(Render_Resource_Kind kind, Render_Tex_2D_Format format, Vec2_I32 size, void *data);
internal void render_tex2d_free(Render_Handle handle);
internal void render_fill_tex2d_region(Render_Handle texture, Rng2_I32 subrect, void *data);

#endif // RENDER_CORE_H
