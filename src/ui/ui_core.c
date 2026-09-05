internal UI_State *ui_state_alloc(void)
{
    Arena *arena = arena_alloc();
    UI_State *state = arena_push(arena, UI_State, 1);
    state->arena = arena;
    state->build_arenas[0] = arena_alloc();
    state->build_arenas[1] = arena_alloc();
    state->box_table_size = 4096;
    state->box_table = arena_push(state->arena, UI_BoxHashSlot, state->box_table_size);
    UI_InitStackNils(state);
    return state;
}

internal void ui_state_release(UI_State *state)
{
    for (size_t i = 0; i < ArrayLength(state->build_arenas); i += 1)
    {
        arena_free(state->build_arenas[i]);
    }
    arena_free(state->arena);
}

internal Arena *ui_build_arena(void)
{
    Arena *result = ui_state->build_arenas[ui_state->build_index%ArrayLength(ui_state->build_arenas)];
    return result;
}

internal UI_Box *ui_root_from_state(UI_State *state)
{
    return state->root;
}

internal bool ui_box_is_nil(UI_Box *box)
{
    return box == 0 || box == &ui_box_nil;
}


internal bool ui_key_match(UI_Key a, UI_Key b)
{
    return a.u64[0] == b.u64[0];
}

internal UI_Key ui_key_zero(void)
{
    UI_Key result = STRUCT_ZERO;
    return result;
}

internal UI_Box *ui_box_from_key(UI_Key key)
{
    UI_Box *result = &ui_box_nil;
    if (!ui_key_match(key, ui_key_zero()))
    {
        size_t slot = key.u64[0] % ui_state->box_table_size;
        for (UI_Box *box = ui_state->box_table[slot].hash_first; !ui_box_is_nil(box); box = box->hash_next)
        {
            if (ui_key_match(box->key, key))
            {
                result = box;
                break;
            }
        }
    }
    return result;
}

internal UI_Key ui_active_seed_key(void)
{
    UI_Box *keyed_ancestor = &ui_box_nil;
    {
        for (UI_Box *p = ui_parent_top(); !ui_box_is_nil(p); p = p->parent)
        {
            if (!ui_key_match(ui_key_zero(), p->key))
            {
                keyed_ancestor = p;
                break;
            }
        }
    }
    return keyed_ancestor->key;
}

internal UI_Key ui_key_from_string(UI_Key seed_key, Str8 string)
{
    UI_Key result = STRUCT_ZERO;
    if (string.size != 0)
    {
        uint64_t hash_replace_signifier_pos = str8_find_substr(string, 0, str8("###"), 0);
        Str8 hash_part = string;
        if (hash_replace_signifier_pos < string.size)
        {
            hash_part = str8_skip(string, hash_replace_signifier_pos);
        }
        result.u64[0] = u64_hash_from_seed_str8(seed_key.u64[0], hash_part);
    }
    return result;
}

internal UI_Box *ui_box_build_from_key(UI_Box_Flags flags, UI_Key key)
{
    ui_state->build_box_count += 1;
    UI_Box *parent = ui_parent_top();
    
    // ak: try to get box
    UI_Box *box = ui_box_from_key(key);
    bool box_first_frame = ui_box_is_nil(box);
    
    // ak: zero key on duplicate
    if (!box_first_frame && box->last_touched_build_index == ui_state->build_index)
    {
        box = &ui_box_nil;
        key = ui_key_zero();
        box_first_frame = true;
    }
    
    // ak: gather info from box
    bool box_is_transient = ui_key_match(key, ui_key_zero());
    
    // ak: allocate box if it doesn't yet exist
    if (box_first_frame)
    {
        box = !box_is_transient ? ui_state->first_free_box : 0;
        if (!ui_box_is_nil(box))
        {
            SLLStackPop_N(ui_state->first_free_box, next_sibling);
        }
        else
        {
            box = arena_push_nz(box_is_transient ? ui_build_arena() : ui_state->arena, UI_Box, 1);
        }
        MemSetZeroStruct(box);
    }
    
    // ak: zero out per-frame state
    {
        box->first_child = box->last_child = box->next_sibling = box->prev_sibling = box->parent = &ui_box_nil;
        box->child_count = 0;
        box->flags = 0;
        MemSetZeroArray(box->pref_size);
    }
    
    //- ak: hook into persistent state table
    if (box_first_frame && !box_is_transient)
    {
        uint64_t slot = key.u64[0] % ui_state->box_table_size;
        DLLInsert_NPZ(&ui_box_nil, ui_state->box_table[slot].hash_first, ui_state->box_table[slot].hash_last, ui_state->box_table[slot].hash_last, box, hash_next, hash_prev);
    }
    
    //- ak: hook into per-frame tree structure
    if (!ui_box_is_nil(parent))
    {
        DLLPushBack_NPZ(&ui_box_nil, parent->first_child, parent->last_child, box, next_sibling, prev_sibling);
        parent->child_count += 1;
        box->parent = parent;
    }
    
    box->flags = flags;
    box->pref_size[Axis_2d_X] = ui_state->pref_width_stack.top->v;
    box->pref_size[Axis_2d_Y] = ui_state->pref_height_stack.top->v;
    box->child_axis = ui_state->child_axis_stack.top->v;
    box->last_touched_build_index = ui_state->build_index;
    return box;
}

internal UI_Box *ui_box_build_from_string(UI_Box_Flags flags, Str8 string)
{
    UI_Key key = ui_key_from_string(ui_active_seed_key(), string);
    UI_Box *box = ui_box_build_from_key(flags, key);
    return box;
}

internal UI_Box *ui_box_build_from_stringf(UI_Box_Flags flags, char *fmt, ...)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    va_list args;
    va_start(args, fmt);
    Str8 string = str8fv(scratch.arena, fmt, args);
    UI_Box *result = ui_box_build_from_string(flags, string);
    va_end(args);
    arena_scratch_end(scratch);
    return result;
}

internal UI_Box_Rec ui_box_rec_df(UI_Box *box, UI_Box *root, uint64_t sib_member_off, uint64_t child_member_off)
{
    UI_Box_Rec result = STRUCT_ZERO;
    if (!ui_box_is_nil(*MemFromOffset(UI_Box **, box, child_member_off)))
    {
        result.next = *MemFromOffset(UI_Box **, box, child_member_off);
        result.push_count = 1;
    }
    else for (UI_Box *p = box; !ui_box_is_nil(p) && p != root; p = p->parent)
    {
        if (!ui_box_is_nil(*MemFromOffset(UI_Box **, p, sib_member_off)))
        {
            result.next = *MemFromOffset(UI_Box **, p, sib_member_off);
            break;
        }
        result.pop_count += 1;
    }
    return result;
}

internal void _ui_calc_sizes_standalone(UI_Box *root, Axis_2d axis)
{
    for (UI_Box *box = root; !ui_box_is_nil(box); box = ui_box_rec_df_pre(box, root).next)
    {
        switch(box->pref_size[axis].kind)
        {
            default:{}break;
            case UI_Size_Kind_Pixels:
            {
                box->fixed_size.v[axis] = box->pref_size[axis].value;
            } break;
        }
    }
}

internal void _ui_layout_position(UI_Box *root, Axis_2d axis)
{
    for (UI_Box *box = root; !ui_box_is_nil(box); box = ui_box_rec_df_pre(box, root).next)
    {
        float layout_position = 0;

        //- ak: lay out children
        float bounds = 0;
        for (UI_Box *child = box->first_child; !ui_box_is_nil(child); child = child->next_sibling)
        {
            // ak: calculate fixed position & size
            if (!(child->flags & (UI_Box_Flag_FloatingX<<axis)))
            {
                child->fixed_position.v[axis] = layout_position;
                if (box->child_axis == axis)
                {
                    layout_position += child->fixed_size.v[axis];
                    bounds += child->fixed_size.v[axis];
                }
                else
                {
                    bounds = Max(bounds, child->fixed_size.v[axis]);
                }
            }

            // ak: determine final rect for child, given fixed_position & size
            child->rect.xy.v[axis] = box->rect.xy.v[axis] + child->fixed_position.v[axis];
            child->rect.zw.v[axis] = child->rect.xy.v[axis] + child->fixed_size.v[axis];
            child->rect.xy.x = floor_f32(child->rect.xy.x);
            child->rect.xy.y = floor_f32(child->rect.xy.y);
            child->rect.zw.x = floor_f32(child->rect.zw.x);
            child->rect.zw.y = floor_f32(child->rect.zw.y);
        }

        //- ak: store view bounds
        {
            box->view_bounds.v[axis] = bounds;
        }
    }
}

internal void ui_build_begin(UI_State *state, Wl_Window window)
{
    ui_state = state;
    ui_state->root = &ui_box_nil;
    UI_InitStacks(ui_state);
    
    // ak: build root
    uint32_t width = wl_window_width_get(window);
    uint32_t height = wl_window_height_get(window);
    ui_pref_width_next_set(ui_px((float)width, 1.f));
    ui_pref_height_next_set(ui_px((float)height, 1.f));
    ui_child_axis_next_set(Axis_2d_Y);
    UI_Box *root = ui_box_build_from_stringf(UI_Box_Flag_DrawBackground, "window_root_###%I64x", 324);
    ui_parent_push(root);
    ui_state->root = root;
}

internal void ui_build_end(void)
{
    //- ak: prune untouched or transient boxes in the cache
    for (uint64_t slot_idx = 0; slot_idx < ui_state->box_table_size; slot_idx += 1)
    {
        for (UI_Box *box = ui_state->box_table[slot_idx].hash_first, *next = 0;
            !ui_box_is_nil(box);
            box = next)
        {
            next = box->hash_next;
            if (box->last_touched_build_index < ui_state->build_index ||
                ui_key_match(box->key, ui_key_zero()))
            {
                DLLRemove_NPZ(&ui_box_nil, ui_state->box_table[slot_idx].hash_first, ui_state->box_table[slot_idx].hash_last, box, hash_next, hash_prev);
                SLLStackPush_N(ui_state->first_free_box, box, next_sibling);
            }
        }
    }
    
    UI_Box *root = ui_root_from_state(ui_state);
    // ak: claculate size and position
    for (Axis_2d axis = (Axis_2d)0; axis < Axis_2d_COUNT; axis = (Axis_2d)(axis + 1))
    {
        _ui_calc_sizes_standalone(root, axis);
        _ui_layout_position(root, axis);
    }
    
    ui_state->build_index += 1;
    arena_clear(ui_build_arena());
}

internal void ui_button(Str8 string)
{
    TempUnused(string);
    UI_Box *box = ui_box_build_from_key(UI_Box_Flag_DrawBackground, ui_key_zero());
    TempUnused(box);
}
