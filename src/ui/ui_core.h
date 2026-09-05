// NOTE(ak): This UI module code is reference from:
// - https://www.dgtlgrove.com/p/ui-part-2-build-it-every-frame-immediate
// - https://www.dgtlgrove.com/p/ui-part-3-the-widget-building-language
// - https://github.com/EpicGamesExt/raddebugger/blob/master/src/ui/ui_core.h
// - https://github.com/EpicGamesExt/raddebugger/blob/master/src/ui/ui_core.c

#ifndef UI_CORE_H
#define UI_CORE_H

//~ ak: Types
//=============================================================================

typedef enum UI_Box_Flags
{
    UI_Box_Flag_DrawBackground = (1<<0),
    UI_Box_Flag_FixedWidth = (1<<1),
    UI_Box_Flag_AllowOverflowX = (1<<2),
    UI_Box_Flag_FloatingX = (1<<3),
} UI_Box_Flags;

typedef enum UI_Size_Kind
{
    UI_Size_Kind_Pixels,
    UI_Size_Kind_TextContent,
    UI_Size_Kind_ParentPct,
    UI_Size_Kind_ChildrenSum,
} UI_Size_Kind;

typedef struct UI_Size UI_Size;
struct UI_Size
{
    UI_Size_Kind kind;
    float value;
    float strictness;
};

typedef struct UI_Key UI_Key;
struct UI_Key
{
  uint64_t u64[1];
};

typedef struct UI_Box UI_Box;
struct UI_Box
{
    UI_Box *hash_next;
    UI_Box *hash_prev;
    UI_Box *parent;
    UI_Box *next_sibling;
    UI_Box *prev_sibling;
    UI_Box *first_child;
    UI_Box *last_child;
    UI_Key key;
    UI_Box_Flags flags;
    size_t child_count;
    float text_padding;
    Axis_2d child_axis;
    UI_Size pref_size[Axis_2d_COUNT];
    Vec2_F32 fixed_size;
    Vec2_F32 fixed_position;
    Vec4_F32 rect;
    Vec2_F32 view_bounds;
    size_t first_touched_build_index;
    size_t last_touched_build_index;
};


typedef struct UI_Box_Rec UI_Box_Rec;
struct UI_Box_Rec
{
    UI_Box *next;
    size_t push_count;
    size_t pop_count;
};

typedef struct UI_BoxHashSlot UI_BoxHashSlot;
struct UI_BoxHashSlot
{
    UI_Box *hash_first;
    UI_Box *hash_last;
};

#include "./generated/ui.meta.h"

typedef struct UI_State UI_State;
struct UI_State
{
    UI_Box *root;
    Arena *arena;
    Arena *build_arenas[2];
    size_t build_box_count;
    size_t build_index;
    
    // ak: box cache
    UI_Box *first_free_box;
    size_t box_table_size;
    UI_BoxHashSlot *box_table;

    UI_DeclStackNils;
    UI_DeclStacks;
};

//~ ak: Macro
//=============================================================================

#define ui_px(value, strictness)         (UI_Size){ UI_Size_Kind_Pixels, value, strictness }
#define ui_em(value, strictness)         (UI_Size){ UI_Size_Kind_Pixels, (value) * ui_font_size_top(), strictness }
#define ui_text_dim(padding, strictness) (UI_Size){ UI_Size_Kind_TextContent, padding, strictness }
#define ui_pct(value, strictness)        (UI_Size){ UI_Size_Kind_ParentPct, value, strictness }
#define ui_children_sum(strictness)      (UI_Size){ UI_Size_Kind_ChildrenSum, 0.f, strictness }

#define UI_StackTop(state, name_upper, name_lower) \
    return state->name_lower##_stack.top->v

#define UI_StackBottom(state, name_upper, name_lower) \
    return state->name_lower##_stack.bottom_val

#define UI_StackPush(state, name_upper, name_lower, type, new_value) \
    UI_##name_upper##_Node *node = state->name_lower##_stack.free;\
    if (node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
    else {node = arena_push(ui_build_arena(), UI_##name_upper##_Node, 1);}\
    type old_value = state->name_lower##_stack.top->v;\
    node->v = new_value;\
    SLLStackPush(state->name_lower##_stack.top, node);\
    if (node->next == &state->name_lower##_nil_stack_top)\
    {\
        state->name_lower##_stack.bottom_val = (node->v);\
    }\
    return old_value

#define UI_StackPop(state, name_upper, name_lower) \
    UI_##name_upper##_Node *popped = state->name_lower##_stack.top;\
    if (popped != &state->name_lower##_nil_stack_top)\
    {\
        SLLStackPop(state->name_lower##_stack.top);\
        SLLStackPush(state->name_lower##_stack.free, popped);\
    }\
    return popped->v

#define UI_StackSetNext(state, name_upper, name_lower, type, new_value) \
    UI_##name_upper##_Node *node = state->name_lower##_stack.free;\
    if (node != 0) {SLLStackPop(state->name_lower##_stack.free);}\
    else {node = arena_push(ui_build_arena(), UI_##name_upper##_Node, 1);}\
    type old_value = state->name_lower##_stack.top->v;\
    node->v = new_value;\
    SLLStackPush(state->name_lower##_stack.top, node);\
    return old_value;

#define UI_Build(state) DeferLoop(ui_build_begin(state), ui_build_end())

//~ ak: Functions
//=============================================================================

internal UI_Box_Rec ui_box_rec_df(UI_Box *box, UI_Box *root, uint64_t sib_member_off, uint64_t child_member_off);
#define ui_box_rec_df_pre(box, root) ui_box_rec_df(box, root, OffsetOf(UI_Box, next_sibling), OffsetOf(UI_Box, first_child))
#define ui_box_rec_df_post(box, root) ui_box_rec_df(box, root, OffsetOf(UI_Box, prev_sibling), OffsetOf(UI_Box, last_child))

//~ ak: Globals
//=============================================================================


PragmaNoWarnMissingFieldInitPush()

read_only global UI_Box ui_box_nil =
{
    &ui_box_nil,
    &ui_box_nil,
    &ui_box_nil,
    &ui_box_nil,
    &ui_box_nil,
    &ui_box_nil,
    &ui_box_nil,
};

PragmaPop()

global UI_State *ui_state = NULL;

#endif // UI_CORE_H
