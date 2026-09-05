//- GENERATED CODE

#ifndef UI_META_H
#define UI_META_H

typedef struct UI_Pref_Width_Node UI_Pref_Width_Node; struct UI_Pref_Width_Node{UI_Pref_Width_Node *next; UI_Size v;};
typedef struct UI_Pref_Height_Node UI_Pref_Height_Node; struct UI_Pref_Height_Node{UI_Pref_Height_Node *next; UI_Size v;};
typedef struct UI_Child_Axis_Node UI_Child_Axis_Node; struct UI_Child_Axis_Node{UI_Child_Axis_Node *next; Axis_2d v;};
typedef struct UI_Parent_Node UI_Parent_Node; struct UI_Parent_Node{UI_Parent_Node *next; UI_Box * v;};
typedef struct UI_Font_Size_Node UI_Font_Size_Node; struct UI_Font_Size_Node{UI_Font_Size_Node *next; float v;};
#define UI_DeclStackNils \
struct\
{\
UI_Pref_Width_Node pref_width_nil_stack_top;\
UI_Pref_Height_Node pref_height_nil_stack_top;\
UI_Child_Axis_Node child_axis_nil_stack_top;\
UI_Parent_Node parent_nil_stack_top;\
UI_Font_Size_Node font_size_nil_stack_top;\
}
#define UI_InitStackNils(state) \
state->pref_width_nil_stack_top.v = ui_px(200.f, 1.f);\
state->pref_height_nil_stack_top.v = ui_px(2.f, 1.f);\
state->child_axis_nil_stack_top.v = Axis_2d_Y;\
state->parent_nil_stack_top.v = NULL;\
state->font_size_nil_stack_top.v = 16.f;\

#define UI_DeclStacks \
struct\
{\
struct { UI_Pref_Width_Node *top; UI_Size bottom_val; UI_Pref_Width_Node *free; } pref_width_stack;\
struct { UI_Pref_Height_Node *top; UI_Size bottom_val; UI_Pref_Height_Node *free; } pref_height_stack;\
struct { UI_Child_Axis_Node *top; Axis_2d bottom_val; UI_Child_Axis_Node *free; } child_axis_stack;\
struct { UI_Parent_Node *top; UI_Box * bottom_val; UI_Parent_Node *free; } parent_stack;\
struct { UI_Font_Size_Node *top; float bottom_val; UI_Font_Size_Node *free; } font_size_stack;\
}
#define UI_InitStacks(state) \
state->pref_width_stack.top = &state->pref_width_nil_stack_top; state->pref_width_stack.bottom_val = ui_px(200.f, 1.f); state->pref_width_stack.free = 0;\
state->pref_height_stack.top = &state->pref_height_nil_stack_top; state->pref_height_stack.bottom_val = ui_px(2.f, 1.f); state->pref_height_stack.free = 0;\
state->child_axis_stack.top = &state->child_axis_nil_stack_top; state->child_axis_stack.bottom_val = Axis_2d_Y; state->child_axis_stack.free = 0;\
state->parent_stack.top = &state->parent_nil_stack_top; state->parent_stack.bottom_val = NULL; state->parent_stack.free = 0;\
state->font_size_stack.top = &state->font_size_nil_stack_top; state->font_size_stack.bottom_val = 16.f; state->font_size_stack.free = 0;\

internal UI_Size ui_pref_width_top(void);
internal UI_Size ui_pref_height_top(void);
internal Axis_2d ui_child_axis_top(void);
internal UI_Box * ui_parent_top(void);
internal float ui_font_size_top(void);
internal UI_Size ui_pref_width_bottom(void);
internal UI_Size ui_pref_height_bottom(void);
internal Axis_2d ui_child_axis_bottom(void);
internal UI_Box * ui_parent_bottom(void);
internal float ui_font_size_bottom(void);
internal UI_Size ui_pref_width_push(UI_Size v);
internal UI_Size ui_pref_height_push(UI_Size v);
internal Axis_2d ui_child_axis_push(Axis_2d v);
internal UI_Box * ui_parent_push(UI_Box * v);
internal float ui_font_size_push(float v);
internal UI_Size ui_pref_width_pop(void);
internal UI_Size ui_pref_height_pop(void);
internal Axis_2d ui_child_axis_pop(void);
internal UI_Box * ui_parent_pop(void);
internal float ui_font_size_pop(void);
internal UI_Size ui_pref_width_next_set(UI_Size v);
internal UI_Size ui_pref_height_next_set(UI_Size v);
internal Axis_2d ui_child_axis_next_set(Axis_2d v);
internal UI_Box * ui_parent_next_set(UI_Box * v);
internal float ui_font_size_next_set(float v);
#define UI_Pref_Width(v) DeferLoop(ui_pref_width_push(v), ui_pref_width_pop())
#define UI_Pref_Height(v) DeferLoop(ui_pref_height_push(v), ui_pref_height_pop())
#define UI_Child_Axis(v) DeferLoop(ui_child_axis_push(v), ui_child_axis_pop())
#define UI_Parent(v) DeferLoop(ui_parent_push(v), ui_parent_pop())
#define UI_Font_Size(v) DeferLoop(ui_font_size_push(v), ui_font_size_pop())
#endif // UI_META_H
