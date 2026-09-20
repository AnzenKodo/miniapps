//- GENERATED CODE

internal UI_Size ui_pref_width_top(void) { UI_StackTop(ui_state, Pref_Width, pref_width); }
internal UI_Size ui_pref_height_top(void) { UI_StackTop(ui_state, Pref_Height, pref_height); }
internal Axis_2d ui_child_axis_top(void) { UI_StackTop(ui_state, Child_Axis, child_axis); }
internal UI_Box * ui_parent_top(void) { UI_StackTop(ui_state, Parent, parent); }
internal float ui_font_size_top(void) { UI_StackTop(ui_state, Font_Size, font_size); }
internal UI_Size ui_pref_width_bottom(void) { UI_StackBottom(ui_state, Pref_Width, pref_width); }
internal UI_Size ui_pref_height_bottom(void) { UI_StackBottom(ui_state, Pref_Height, pref_height); }
internal Axis_2d ui_child_axis_bottom(void) { UI_StackBottom(ui_state, Child_Axis, child_axis); }
internal UI_Box * ui_parent_bottom(void) { UI_StackBottom(ui_state, Parent, parent); }
internal float ui_font_size_bottom(void) { UI_StackBottom(ui_state, Font_Size, font_size); }
internal UI_Size ui_pref_width_push(UI_Size v) { UI_StackPush(ui_state, Pref_Width, pref_width, UI_Size, v); }
internal UI_Size ui_pref_height_push(UI_Size v) { UI_StackPush(ui_state, Pref_Height, pref_height, UI_Size, v); }
internal Axis_2d ui_child_axis_push(Axis_2d v) { UI_StackPush(ui_state, Child_Axis, child_axis, Axis_2d, v); }
internal UI_Box * ui_parent_push(UI_Box * v) { UI_StackPush(ui_state, Parent, parent, UI_Box *, v); }
internal float ui_font_size_push(float v) { UI_StackPush(ui_state, Font_Size, font_size, float, v); }
internal UI_Size ui_pref_width_pop(void) { UI_StackPop(ui_state, Pref_Width, pref_width); }
internal UI_Size ui_pref_height_pop(void) { UI_StackPop(ui_state, Pref_Height, pref_height); }
internal Axis_2d ui_child_axis_pop(void) { UI_StackPop(ui_state, Child_Axis, child_axis); }
internal UI_Box * ui_parent_pop(void) { UI_StackPop(ui_state, Parent, parent); }
internal float ui_font_size_pop(void) { UI_StackPop(ui_state, Font_Size, font_size); }
internal UI_Size ui_pref_width_next_set(UI_Size v) { UI_StackSetNext(ui_state, Pref_Width, pref_width, UI_Size, v); }
internal UI_Size ui_pref_height_next_set(UI_Size v) { UI_StackSetNext(ui_state, Pref_Height, pref_height, UI_Size, v); }
internal Axis_2d ui_child_axis_next_set(Axis_2d v) { UI_StackSetNext(ui_state, Child_Axis, child_axis, Axis_2d, v); }
internal UI_Box * ui_parent_next_set(UI_Box * v) { UI_StackSetNext(ui_state, Parent, parent, UI_Box *, v); }
internal float ui_font_size_next_set(float v) { UI_StackSetNext(ui_state, Font_Size, font_size, float, v); }
