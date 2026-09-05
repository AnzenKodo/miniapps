//~ ak: Helper Functions
//=============================================================================

internal uint64_t mdg_hash_from_string(Str8 string)
{
    uint64_t result = 5381;
    for (size_t i = 0; i < string.length; i += 1)
    {
        result = ((result << 5) + result) + string.cstr[i];
    }
    return result;
}

//~ ak: Message Lists
//=============================================================================

internal void mdg_msg_list_push(MDG_Msg_List *msgs, MDG_Msg *msg, Arena *arena)
{
    MDG_Msg_Node *n = arena_push(arena, MDG_Msg_Node, 1);
    MemCopyStruct(&n->v, msg);
    SLLQueuePush(msgs->first, msgs->last, n);
    msgs->count += 1;
}

// ak: C-String-Izing
//=============================================================================

internal Str8 mdg_c_string_literal_from_multiline_string(Str8 string, Arena *arena)
{
    Str8_List strings = STRUCT_ZERO;
    {
        str8_list_push(arena, &strings, str8("\"\"\n"));
        size_t active_line_start_off = 0;
        for (size_t off = 0; off <= string.length; off += 1)
        {
            bool is_newline = (off < string.length && (string.cstr[off] == '\n' || string.cstr[off] == '\r'));
            bool is_ender = (off >= string.length || is_newline);
            if (is_ender)
            {
                Str8 line = str8_substr(string, (Rng1_U64){ active_line_start_off, off });
                str8_list_push(arena, &strings, str8("\""));
                str8_list_push(arena, &strings, line);
                if (is_newline)
                {
                    str8_list_push(arena, &strings, str8("\\n\"\n"));
                }
                else
                {
                    str8_list_push(arena, &strings, str8("\"\n"));
                }
                active_line_start_off = off+1;
            }
            if (is_newline && string.cstr[off] == '\r')
            {
                active_line_start_off += 1;
                off += 1;
            }
        }
    }
    Str8 result = str8_list_join(arena, &strings, 0);
    return result;
}


internal Str8 mdg_c_array_literal_contents_from_string(Str8 string, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str8_List strings = STRUCT_ZERO;
    {
        for (size_t off = 0; off < string.length;)
        {
            size_t chunk_size = Min(string.length-off, 64);
            uint8_t *chunk_bytes = string.cstr+off;
            Str8 chunk_text_string = STRUCT_ZERO;
            chunk_text_string.size = chunk_size*5;
            chunk_text_string.length = chunk_text_string.size;
            chunk_text_string.cstr = arena_push(arena, uint8_t, chunk_text_string.length);
            for (size_t byte_idx = 0; byte_idx < chunk_size; byte_idx += 1)
            {
                Str8 byte_str = str8f(scratch.arena, "0x%02x,", chunk_bytes[byte_idx]);
                mem_copy(chunk_text_string.cstr+byte_idx*5, byte_str.cstr, byte_str.length);
            }
            off += chunk_size;
            str8_list_push(arena, &strings, chunk_text_string);
            str8_list_push(arena, &strings, str8("\n"));
        }
    }
    Str8 result = str8_list_join(arena, &strings, 0);
    arena_scratch_end(scratch);
    return result;
}

//~ ak: Map Functions
//=============================================================================

internal MDG_Map mdg_map_push(size_t slot_count, Arena *arena)
{
    MDG_Map map = STRUCT_ZERO;
    map.slots_count = slot_count;
    map.slots = arena_push(arena, MDG_Map_Slot, map.slots_count);
    return map;
}

internal void *mdg_map_ptr_from_string(MDG_Map *map, Str8 string)
{
    void *result = NULL;
    {
        uint64_t hash = mdg_hash_from_string(string);
        size_t slot_idx = hash%map->slots_count;
        MDG_Map_Slot *slot = &map->slots[slot_idx];
        for (MDG_Map_Node *n = slot->first; n != NULL; n = n->next)
        {
            if (str8_match(n->key, string, 0))
            {
                result = n->val;
                break;
            }
        }
    }
    return result;
}

internal void mdg_map_insert_ptr(MDG_Map *map, Str8 string, void *val, Arena *arena)
{
    uint64_t hash = mdg_hash_from_string(string);
    size_t slot_idx = hash%map->slots_count;
    MDG_Map_Slot *slot = &map->slots[slot_idx];
    MDG_Map_Node *n = arena_push(arena, MDG_Map_Node, 1);
    n->key = str8_copy(arena, string);
    n->val = val;
    SLLQueuePush(slot->first, slot->last, n);
}

//~ ak: String Expression Parsing
//=============================================================================

internal MDG_Str_Expr *mdg_str_expr_push(Arena *arena, MDG_Str_Expr_Op op, MD_Node *node)
{
  MDG_Str_Expr *expr = arena_push(arena, MDG_Str_Expr, 1);
  MemCopyStruct(expr, &mdg_str_expr_nil);
  expr->op = op;
  expr->node = node;
  return expr;
}

internal MDG_Str_Expr_ParseResult mdg_str_expr_parse_from_first_opl_min_prec(Arena *arena, MD_Node *first, MD_Node *opl, char min_prec)
{
    MDG_Str_Expr_ParseResult parse = STRUCT_ZERO;
    parse.root = &mdg_str_expr_nil;
    {
        MD_Node *it = first;
        
        //- ak: consume prefix operators
        MDG_Str_Expr *leafmost_op = &mdg_str_expr_nil;
        while (it != opl && !md_node_is_nil(it))
        {
            MDG_Str_Expr_Op found_op = MDG_Str_Expr_Op_Null;
            for (MDG_Str_Expr_Op op = (MDG_Str_Expr_Op)(MDG_Str_Expr_Op_Null+1);
                    op < MDG_Str_Expr_Op_COUNT;
                    op = (MDG_Str_Expr_Op)(op+1))
            {
                if (mdg_str_expr_op_kind_table[op] == MDG_Str_Expr_Op_Kind_Prefix &&
                        str8_match(it->string, mdg_str_expr_op_symbol_string_table[op], 0) &&
                        mdg_str_expr_op_precedence_table[op] >= min_prec)
                {
                    found_op = op;
                    break;
                }
            }
            if (found_op != MDG_Str_Expr_Op_Null)
            {
                MDG_Str_Expr *op_expr = mdg_str_expr_push(arena, found_op, it);
                if (leafmost_op == &mdg_str_expr_nil)
                {
                    leafmost_op = op_expr;
                }
                op_expr->left = parse.root;
                parse.root = op_expr;
                it = it->next;
            }
            else
            {
                break;
            }
        }
        
        //- ak: parse atom
        {
            MDG_Str_Expr *atom = &mdg_str_expr_nil;
            if (it->flags & (MD_Node_Flag_Identifier|MD_Node_Flag_Numeric|MD_Node_Flag_StringLiteral) &&
                    md_node_is_nil(it->first))
            {
                atom = mdg_str_expr_push(arena, MDG_Str_Expr_Op_Null, it);
                it = it->next;
            }
            else if (!md_node_is_nil(it->first))
            {
                MDG_Str_Expr_ParseResult subparse = mdg_str_expr_parse_from_first_opl_min_prec(arena, it->first, &md_nil_node, 0);
                atom = subparse.root;
                md_msg_list_concat_in_place(&parse.msgs, &subparse.msgs);
                it = it->next;
            }
            if (leafmost_op != &mdg_str_expr_nil)
            {
                leafmost_op->left = atom;
            }
            else
            {
                parse.root = atom;
            }
        }
        
        //- ak: parse binary operator extensions at this precedence level
        while (it != opl && !md_node_is_nil(it))
        {
            // ak: find binary op kind of `it`
            MDG_Str_Expr_Op found_op = MDG_Str_Expr_Op_Null;
            for (MDG_Str_Expr_Op op = (MDG_Str_Expr_Op)(MDG_Str_Expr_Op_Null+1);
                    op < MDG_Str_Expr_Op_COUNT;
                    op = (MDG_Str_Expr_Op)(op+1))
            {
                if (mdg_str_expr_op_kind_table[op] == MDG_Str_Expr_Op_Kind_Binary &&
                        str8_match(it->string, mdg_str_expr_op_symbol_string_table[op], 0) &&
                        mdg_str_expr_op_precedence_table[op] >= min_prec)
                {
                    found_op = op;
                    break;
                }
            }
            
            // ak: good found_op -> build binary expr
            if (found_op != MDG_Str_Expr_Op_Null)
            {
                MDG_Str_Expr *op_expr = mdg_str_expr_push(arena, found_op, it);
                if (leafmost_op == &mdg_str_expr_nil)
                {
                    leafmost_op = op_expr;
                }
                op_expr->left = parse.root;
                parse.root = op_expr;
                it = it->next;
            }
            else
            {
                break;
            }
            
            // ak: parse right hand side of binary operator
            MDG_Str_Expr_ParseResult subparse = mdg_str_expr_parse_from_first_opl_min_prec(arena, it, opl, mdg_str_expr_op_precedence_table[found_op]+1);
            parse.root->right = subparse.root;
            md_msg_list_concat_in_place(&parse.msgs, &subparse.msgs);
            if (subparse.root == &mdg_str_expr_nil)
            {
                md_msg_list_pushf(arena, &parse.msgs, it, MD_Msg_Level_Error, "Missing right-hand-side of '%S'.", mdg_str_expr_op_symbol_string_table[found_op]);
            }
            it = subparse.next_node;
        }
        
        // ak: store next node for more caller-side parsing
        parse.next_node = it;
    }
    return parse;
}

internal MDG_Str_Expr_ParseResult mdg_str_expr_parse_from_root(MD_Node *root, Arena *arena)
{
    MDG_Str_Expr_ParseResult parse = mdg_str_expr_parse_from_first_opl_min_prec(arena, root->first, &md_nil_node, 0);
    return parse;
}

//~ ak: Table Generation Functions
//=============================================================================

internal MDG_Node_Array mdg_node_array_make(size_t count, Arena *arena)
{
    MDG_Node_Array result = STRUCT_ZERO;
    result.count = count;
    result.v = arena_push(arena, MD_Node *, result.count);
    for (size_t index = 0; index < result.count; index += 1)
    {
        result.v[index] = &md_nil_node;
    }
    return result;
}

internal MDG_Node_Grid mdg_node_grid_make_from_node(MD_Node *root, Arena *arena)
{
    MDG_Node_Grid grid = STRUCT_ZERO;
    // ak: determine dimensions
    size_t row_count = md_child_count_from_node(root);
    size_t column_count = 0;
    for (MD_Node *row = root->first; !md_node_is_nil(row); row = row->next)
    {
        size_t cell_count_this_row = md_child_count_from_node(row);
        column_count = Max(column_count, cell_count_this_row);
    }
    // ak: fill grid
    grid.x_stride = 1;
    grid.y_stride = column_count;
    grid.cells = mdg_node_array_make(row_count*column_count, arena);
    grid.row_parents = mdg_node_array_make(row_count, arena);
    // ak: fill nodes
    {
        size_t y = 0;
        for (MD_Node *row = root->first; !md_node_is_nil(row); row = row->next)
        {
            size_t x = 0;
            grid.row_parents.v[y] = row;
            for (MD_Node *cell = row->first; !md_node_is_nil(cell); cell = cell->next)
            {
                grid.cells.v[x*grid.x_stride + y*grid.y_stride] = cell;
                x += 1;
            }
            y += 1;
        }
    }
    return grid;
}

internal MDG_Column_Desc_Array mdg_column_desc_array_from_tag(MD_Node *tag, Arena *arena)
{
    MDG_Column_Desc_Array result = STRUCT_ZERO;
    result.count = md_child_count_from_node(tag);
    result.v = arena_push(arena, MDG_Column_Desc, result.count);
    size_t index = 0;
    for (MD_Node *hdr = tag->first; !md_node_is_nil(hdr); hdr = hdr->next)
    {
        result.v[index].name = str8_copy(arena, hdr->string);
        result.v[index].kind = MDG_Column_Kind_DirectCell;
        if (md_node_has_tag(hdr, str8("tag_check"), 0))
        {
            result.v[index].kind = MDG_Column_Kind_CheckForTag;
        }
        if (md_node_has_tag(hdr, str8("tag_child"), 0))
        {
            Str8 tag_name = md_tag_from_string(hdr, str8("tag_child"), 0)->first->string;
            result.v[index].kind = MDG_Column_Kind_TagChild;
            result.v[index].tag_name = tag_name;
        }
        index += 1;
    }
    return result;
}

internal size_t mdg_column_index_from_name(MDG_Column_Desc_Array descs, Str8 name)
{
    size_t result = 0;
    for (size_t index = 0; index < descs.count; index += 1)
    {
        if (str8_match(descs.v[index].name, name, 0))
        {
            result = index;
            break;
        }
    }
    return result;
}

internal Str8 mdg_string_from_row_desc_index(MD_Node *row_parent, MDG_Column_Desc_Array descs, size_t index)
{
    Str8 result = STRUCT_ZERO;
    // ak: grab relevant column description
    MDG_Column_Desc *desc = 0;
    if (index < descs.count)
    {
        desc = descs.v + index;
    }
    
    // ak: grab node
    if (desc != 0)
    {
        switch (desc->kind)
        {
            default: break;
            case MDG_Column_Kind_DirectCell:
            {
                 // ak: determine grid index (shifted by synthetic columns)
                 size_t cell_idx = index;
                 for (size_t col_idx = 0; col_idx < descs.count && col_idx < index; col_idx += 1)
                 {
                     if (descs.v[col_idx].kind != MDG_Column_Kind_DirectCell)
                     {
                         cell_idx -= 1;
                     }
                 }
                 MD_Node *node = md_child_from_index(row_parent, cell_idx);
                 result = node->string;
            }break;
            
            case MDG_Column_Kind_CheckForTag:
            {
                 Str8 tag_name = desc->name;
                 MD_Node *tag = md_tag_from_string(row_parent, tag_name, 0);
                 result = md_node_is_nil(tag) ? str8("0") : str8("1");
            }break;
            
            case MDG_Column_Kind_TagChild:
            {
                 Str8 tag_name = desc->tag_name;
                 MD_Node *tag = md_tag_from_string(row_parent, tag_name, 0);
                 result = tag->first->string;
            }break;
        }
    }
    return result;
}

internal int64_t mdg_eval_table_expand_expr_numeric(MDG_Str_Expr *expr, MDG_TableExpand_Info *info)
{
    int64_t result = 0;
    MDG_Str_Expr_Op op = expr->op;
    switch (op)
    {
        default:
        {
            if (MDG_Str_Expr_Op_FirstString <= op && op <= MDG_Str_Expr_Op_LastString)
            {
                Arena_Temp scratch = arena_scratch_begin(0, 0);
                Str8_List result_strs = STRUCT_ZERO;
                mdg_eval_table_expand_expr_string(expr, info, &result_strs, scratch.arena);
                Str8 result_str = str8_list_join(scratch.arena, &result_strs, 0);
                try_s64_from_str8_c_rules(result_str, &result);
                arena_scratch_end(scratch);
            }
        } break;
        
        case MDG_Str_Expr_Op_Null:
        {
            try_s64_from_str8_c_rules(expr->node->string, &result);
        } break;
        
        //- ak: numeric arithmetic binary ops
        case MDG_Str_Expr_Op_Add:
        case MDG_Str_Expr_Op_Subtract:
        case MDG_Str_Expr_Op_Multiply:
        case MDG_Str_Expr_Op_Divide:
        case MDG_Str_Expr_Op_Modulo:
        case MDG_Str_Expr_Op_LeftShift:
        case MDG_Str_Expr_Op_RightShift:
        case MDG_Str_Expr_Op_BitwiseAnd:
        case MDG_Str_Expr_Op_BitwiseOr:
        case MDG_Str_Expr_Op_BitwiseXor:
        case MDG_Str_Expr_Op_BooleanAnd:
        case MDG_Str_Expr_Op_BooleanOr:
        {
            int64_t left_val = mdg_eval_table_expand_expr_numeric(expr->left, info);
            int64_t right_val = mdg_eval_table_expand_expr_numeric(expr->right, info);
            switch (op)
            {
                default:break;
                case MDG_Str_Expr_Op_Add:        result = left_val+right_val;  break;
                case MDG_Str_Expr_Op_Subtract:   result = left_val-right_val;  break;
                case MDG_Str_Expr_Op_Multiply:   result = left_val*right_val;  break;
                case MDG_Str_Expr_Op_Divide:     result = left_val/right_val;  break;
                case MDG_Str_Expr_Op_Modulo:     result = left_val%right_val;  break;
                case MDG_Str_Expr_Op_LeftShift:  result = left_val<<right_val; break;
                case MDG_Str_Expr_Op_RightShift: result = left_val>>right_val; break;
                case MDG_Str_Expr_Op_BitwiseAnd: result = left_val&right_val;  break;
                case MDG_Str_Expr_Op_BitwiseOr:  result = left_val|right_val;  break;
                case MDG_Str_Expr_Op_BitwiseXor: result = left_val^right_val;  break;
                case MDG_Str_Expr_Op_BooleanAnd: result = left_val&&right_val; break;
                case MDG_Str_Expr_Op_BooleanOr:  result = left_val||right_val; break;
            }
        } break;
        
        //- ak: prefix unary ops
        case MDG_Str_Expr_Op_BitwiseNegate:
        case MDG_Str_Expr_Op_BooleanNot:
        {
            int64_t right_val = mdg_eval_table_expand_expr_numeric(expr->left, info);
            switch (op)
            {
                default:break;
                case MDG_Str_Expr_Op_BitwiseNegate: result = (int64_t)(~((uint64_t)right_val)); break;
                case MDG_Str_Expr_Op_BooleanNot:    result = !right_val;
            }
        } break;
        
        //- ak: comparisons
        case MDG_Str_Expr_Op_Equals:
        case MDG_Str_Expr_Op_DoesNotEqual:
        {
            Arena_Temp scratch = arena_scratch_begin(0, 0);
            Str8_List left_strs = STRUCT_ZERO;
            Str8_List right_strs = STRUCT_ZERO;
            mdg_eval_table_expand_expr_string(expr->left, info, &left_strs,  scratch.arena);
            mdg_eval_table_expand_expr_string(expr->right, info, &right_strs, scratch.arena);
            Str8 left_str = str8_list_join(scratch.arena, &left_strs, 0);
            Str8 right_str = str8_list_join(scratch.arena, &right_strs, 0);
            bool match = str8_match(left_str, right_str, 0);
            result = (op == MDG_Str_Expr_Op_Equals ? match : !match);
            arena_scratch_end(scratch);
        } break;
    }

    return result;
}

internal void mdg_eval_table_expand_expr_string(MDG_Str_Expr *expr, MDG_TableExpand_Info *info, Str8_List *out, Arena *arena)
{
    MDG_Str_Expr_Op op = expr->op;
    switch (op)
    {
        default:
        {
            if (MDG_Str_Expr_Op_FirstNumeric <= op && op <= MDG_Str_Expr_Op_LastNumeric)
            {
                int64_t numeric_eval = mdg_eval_table_expand_expr_numeric(expr, info);
                Str8 numeric_eval_stringized = STRUCT_ZERO;
                if (md_node_has_tag(md_root_from_node(expr->node), str8("hex"), 0))
                {
                    numeric_eval_stringized = str8f(arena, "0x%lx", numeric_eval);
                }
                else
                {
                    numeric_eval_stringized = str8f(arena, "%ld", numeric_eval);
                }
                str8_list_push(arena, out, numeric_eval_stringized);
            }
        } break;
        
        case MDG_Str_Expr_Op_Null:
        {
            str8_list_push(arena, out, expr->node->string);
        } break;
        
        case MDG_Str_Expr_Op_Dot:
        {
            // ak: grab left/right
            MDG_Str_Expr *left_expr = expr->left;
            MD_Node *left_node = left_expr->node;
            MDG_Str_Expr *right_expr = expr->right;
            MD_Node *right_node = right_expr->node;
            // ak: grab table name (LHS of .) and column lookup string (RHS of .)
            Str8 expand_label = left_node->string;
            Str8 column_lookup = right_node->string;
            // ak: find which task corresponds to this table
            size_t row_idx = 0;
            MDG_Node_Grid *grid = NULL;
            MDG_Column_Desc_Array column_descs = STRUCT_ZERO;
            {
                for (MDG_TableExpand_Task *task = info->first_expand_task; task != NULL; task = task->next)
                {
                    if (str8_match(expand_label, task->expansion_label, 0))
                    {
                        row_idx = task->index;
                        grid = task->grid;
                        column_descs = task->column_descs;
                        break;
                    }
                }
            }
            // ak: grab row parent
            MD_Node *row_parent = &md_nil_node;
            if (grid && row_idx < grid->row_parents.count)
            {
                row_parent = grid->row_parents.v[row_idx];
            }
            // ak: get string for this table lookup
            Str8 lookup_string = STRUCT_ZERO;
            {
                size_t column_idx = 0;
                if (str8_match(column_lookup, str8("_it"), 0))
                {
                    lookup_string = str8f(arena, "%lu", row_idx);
                }
                else
                {
                    // NOTE(ak): numeric column lookup (column index)
                    if (right_node->flags & MD_Node_Flag_Numeric)
                    {
                        try_u64_from_str8_c_rules(column_lookup, &column_idx);
                    }
                    // NOTE(ak): string column lookup (column name)
                    if (right_node->flags & (MD_Node_Flag_Identifier|MD_Node_Flag_StringLiteral))
                    {
                        column_idx = mdg_column_index_from_name(column_descs, column_lookup);
                    }
                    lookup_string = mdg_string_from_row_desc_index(row_parent, column_descs, column_idx);
                    if (str8_match(lookup_string, str8("--"), 0))
                    {
                        lookup_string = info->missing_value_fallback;
                    }
                }
            }
            // ak: push lookup string
            {
                bool is_multiline = (str8_find_substr(lookup_string, 0, str8("\n"), 0) < lookup_string.length);
                if (is_multiline)
                {
                    lookup_string = indented_from_string(lookup_string, 4, arena);
                    lookup_string = raw_from_escaped_str8(lookup_string, arena);
                    lookup_string = raw_from_escaped_str8(lookup_string, arena);
                }
                str8_list_push(arena, out, lookup_string);
            }
        } break;
        
        case MDG_Str_Expr_Op_ExpandIfTrue:
        {
            int64_t bool_value = mdg_eval_table_expand_expr_numeric(expr->left, info);
            if (bool_value)
            {
                mdg_eval_table_expand_expr_string(expr->right, info, out, arena);
            }
        } break;
        
        case MDG_Str_Expr_Op_Concat:
        {
            mdg_eval_table_expand_expr_string(expr->left, info, out, arena);
            mdg_eval_table_expand_expr_string(expr->right, info, out, arena);
        } break;
        
        case MDG_Str_Expr_Op_BumpToColumn:
        {
            int64_t column = mdg_eval_table_expand_expr_numeric(expr->left, info);
            int64_t current_column = out->size;
            int64_t spaces_to_push = column - current_column;
            if (spaces_to_push > 0)
            {
                Str8 str = STRUCT_ZERO;
                str.size = spaces_to_push;
                str.length = spaces_to_push;
                str.cstr = arena_push(arena, uint8_t, spaces_to_push);
                for (int64_t index = 0; index < spaces_to_push; index += 1)
                {
                    str.cstr[index] = ' ';
                }
                str8_list_push(arena, out, str);
            }
        } break;
    }
}

internal void mdg_loop_table_column_expansion(Str8 strexpr, MDG_TableExpand_Info *info, MDG_TableExpand_Task *task, Str8_List *out, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    for (size_t it_idx = 0; it_idx < task->count; it_idx += 1)
    {
        task->index = it_idx;
        //- ak: iterate all further dimensions, if there's left in the chain
        if (task->next)
        {
            mdg_loop_table_column_expansion(strexpr, info, task->next, out, arena);
        }
        
        //- ak: if this is the last task in the chain, perform expansion
        else
        {
            Str8_List expansion_strs = STRUCT_ZERO;
            size_t start = 0;
            for (size_t char_idx = 0; char_idx <= strexpr.length;)
            {
                // ak: push plain text parts of strexpr
                if (char_idx == strexpr.length || strexpr.cstr[char_idx] == '$')
                {
                    Str8 plain_text_substr = str8_substr(strexpr, (Rng1_U64){start, char_idx});
                    start = char_idx;
                    if (plain_text_substr.length != 0)
                    {
                        str8_list_push(arena, &expansion_strs, plain_text_substr);
                    }
                }
                // ak: handle expansion expression
                if (strexpr.cstr[char_idx] == '$')
                {
                    Str8 string = str8_skip(strexpr, char_idx+1);
                    Rng1_U64 expr_range = STRUCT_ZERO;
                    uint64_t paren_nest = 0;
                    for (size_t index = 0; index < string.length; index += 1)
                    {
                        if (string.cstr[index] == '(')
                        {
                            paren_nest += 1;
                            if (paren_nest == 1)
                            {
                                expr_range.min = index;
                            }
                        }
                        if (string.cstr[index] == ')')
                        {
                            paren_nest -= 1;
                            if (paren_nest == 0)
                            {
                                expr_range.max = index+1;
                                break;
                            }
                        }
                    }
                    Str8 expr_string = str8_substr(string, expr_range);
                    MD_Tokenize expr_tokenize = md_tokenize_from_string(expr_string, scratch.arena);
                    MD_Parse expr_base_parse = md_parse_from_string_tokens(expr_string, expr_tokenize.tokens, str8(""), scratch.arena);
                    MDG_Str_Expr_ParseResult expr_parse = mdg_str_expr_parse_from_root(expr_base_parse.root->first, scratch.arena);
                    mdg_eval_table_expand_expr_string(expr_parse.root, info, &expansion_strs, arena);
                    char_idx = start = char_idx + 1 + expr_range.max;
                }
                else
                {
                    char_idx += 1;
                }
            }
            Str8 expansion_str = str8_list_join(arena, &expansion_strs, 0);
            if (expansion_str.length != 0)
            {
                expansion_str = raw_from_escaped_str8(expansion_str, arena);
                str8_list_push(arena, out, expansion_str);
            }
        }
    }
    arena_scratch_end(scratch);
}

internal Str8_List mdg_str_list_from_table_gen(MDG_Map grid_name_map, MDG_Map grid_column_desc_map, Str8 fallback, MD_Node *gen, Arena *arena)
{
    Str8_List result = STRUCT_ZERO;
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    if (md_node_is_nil(gen->first) && gen->string.length != 0)
    {
        str8_list_push(arena, &result, raw_from_escaped_str8(gen->string, arena));
        str8_list_push(arena, &result, str8("\n"));
    }
    else for (MD_Node *strexpr_node = gen->first; !md_node_is_nil(strexpr_node); strexpr_node = strexpr_node->next)
    {
        // ak: build task list
        MDG_TableExpand_Task *first_task = 0;
        MDG_TableExpand_Task *last_task = 0;
        for (MD_Node *tag = strexpr_node->first_tag; !md_node_is_nil(tag); tag = tag->next)
        {
            if (str8_match(tag->string, str8("expand"), 0))
            {
                // ak: grab args for this expansion
                MD_Node *table_name_node = md_child_from_index(tag, 0);
                MD_Node *expand_label_node = md_child_from_index(tag, 1);
                Str8 table_name = table_name_node->string;
                Str8 expand_label = expand_label_node->string;
                
                // ak: lookup table / column descriptions
                MDG_Node_Grid *grid = mdg_map_ptr_from_string(&grid_name_map, table_name);
                MDG_Column_Desc_Array *column_descs = mdg_map_ptr_from_string(&grid_column_desc_map, table_name);
                
                // ak: figure out row count
                size_t grid_row_count = 0;
                if (grid != 0)
                {
                    grid_row_count = grid->cells.count / grid->y_stride;
                }
                
                // ak: push task for this expansion
                if (grid != 0)
                {
                    MDG_TableExpand_Task *task = arena_push(scratch.arena, MDG_TableExpand_Task, 1);
                    task->expansion_label = expand_label;
                    task->grid = grid;
                    task->column_descs = *column_descs;
                    task->count = grid_row_count;
                    task->index = 0;
                    SLLQueuePush(first_task, last_task, task);
                }
            }
        }
        
        // ak: do expansion generation, OR just push this string if we have no
        // expansions
        {
            MDG_TableExpand_Info info = {first_task, fallback};
            if (first_task != 0)
            {
                mdg_loop_table_column_expansion(strexpr_node->string, &info, first_task, &result, arena);
            }
            else
            {
                str8_list_push(arena, &result, raw_from_escaped_str8(strexpr_node->string, arena));
            }
        }
    }
    arena_scratch_end(scratch);
    return result;
}

//~ ak: Layer Lookup Functions
//=============================================================================

internal Str8 mdg_layer_key_from_path(Str8 path, Str8 skip_str, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Str8 path_skip = str8_skip(path, skip_str.length);
    Str8 path_last_slash_chopped = str8_chop_last_slash(path_skip);
    Str8_List path_parts = str8_split_path(scratch.arena, path_last_slash_chopped);
    Str_Join join = STRUCT_ZERO;
    join.sep = str8("/");
    Str8 key = str8_list_join(arena, &path_parts, &join);
    arena_scratch_end(scratch);
    return key;
}

internal MDG_Layer *mdg_layer_from_key(MDG_State *state, Str8 key, Arena *arena)
{
    uint64_t hash = mdg_hash_from_string(key);
    size_t slot_idx = hash%state->slots_count;
    MDG_Layer_Slot *slot = &state->slots[slot_idx];
    MDG_Layer *layer = NULL;
    for (MDG_Layer_Node *n = slot->first; n != NULL; n = n->next)
    {
        if (str8_match(n->v.key, key, 0))
        {
            layer = &n->v;
            break;
        }
    }
    if (layer == NULL)
    {
        MDG_Layer_Node *n = arena_push(arena, MDG_Layer_Node, 1);
        SLLQueuePush(slot->first, slot->last, n);
        n->v.key = str8_copy(arena, key);
        layer = &n->v;
    }
    return layer;
}

//~ ak: Main Output Path Functions
//=============================================================================

internal MDG_State *mdg_state_init(size_t count, Arena *arena)
{
    MDG_State *state = arena_push(arena, MDG_State, 1);
    state->slots_count = count;
    state->slots = arena_push(arena, MDG_Layer_Slot, state->slots_count);
    return state;
}
