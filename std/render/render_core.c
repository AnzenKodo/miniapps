// ak: Generated Code
//=============================================================================

#include "generated/render.meta.c"

// ak: Helpers
//=============================================================================

internal Render_Handle render_handle_zero(void)
{
    Render_Handle handle = STRUCT_ZERO;
    return handle;
}

internal bool render_handle_match(Render_Handle a, Render_Handle b)
{
    return MemMatchStruct(&a, &b);
}

internal Mat4x4_F32 render_sample_channel_map_from_tex2dformat(Render_Tex_2D_Format fmt)
{
    Mat4x4_F32 result =
    {
        {
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {0, 0, 0, 1},
        }
    };
    switch(fmt)
    {
        default:{}break;
        case Render_Tex_2D_Format_R8:
        {
            MemSetZeroArray(result.v[0]);
            result.v[0][0] = result.v[0][1] = result.v[0][2] = result.v[0][3] = 1.f;
        }break;
    }
    return result;
}


internal Render_Batch_List render_batch_list_make(size_t instance_size)
{
    Render_Batch_List list = STRUCT_ZERO;
    list.bytes_per_inst = instance_size;
    return list;
}

internal void *render_batch_list_push_inst(Arena *arena, Render_Batch_List *list, size_t batch_inst_cap)
{
    void *inst = NULL;
    {
        Render_Batch_Node *node = list->last;
        if(node == NULL || node->v.byte_count+list->bytes_per_inst > node->v.byte_cap)
        {
            node = arena_push(arena, Render_Batch_Node, 1);
            node->v.byte_cap = batch_inst_cap*list->bytes_per_inst;
            node->v.v = arena_push_nz(arena, uint8_t, node->v.byte_cap);
            SLLQueuePush(list->first, list->last, node);
            list->batch_count += 1;
        }
        inst = node->v.v + node->v.byte_count;
        node->v.byte_count += list->bytes_per_inst;
        list->byte_count += list->bytes_per_inst;
    }
    return inst;
}

internal Render_Pass *render_pass_from_kind(Arena *arena, Render_Pass_List *list, Render_Pass_Kind kind)
{
    Render_Pass_Node *node = list->last;
    if(!_render_pass_kind_batch_table[kind])
    {
        node = NULL;
    }
    if(node == NULL || node->v.kind != kind)
    {
        node = arena_push(arena, Render_Pass_Node, 1);
        SLLQueuePush(list->first, list->last, node);
        list->count += 1;
        node->v.kind = kind;
        node->v.params = arena_push(arena, uint8_t, _render_pass_kind_params_size_table[kind]);
    }
    return &node->v;
}
