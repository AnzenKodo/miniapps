//~ ak: Message Functions
//=============================================================================

internal void md_msg_list_push(Arena *arena, MD_Msg_List *msgs, MD_Node *node, MD_Msg_Level level, Str8 string)
{
    MD_Msg *msg = arena_push(arena, MD_Msg, 1);
    msg->node = node;
    msg->level = level;
    msg->string = string;
    SLLQueuePush(msgs->first, msgs->last, msg);
    msgs->count += 1;
    msgs->worst_message_level = Max(level, msgs->worst_message_level);
}

internal void md_msg_list_pushf(Arena *arena, MD_Msg_List *msgs, MD_Node *node, MD_Msg_Level level, char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    Str8 string = str8fv(arena, fmt, args);
    md_msg_list_push(arena, msgs, node, level, string);
    va_end(args);
}

internal void md_msg_list_concat_in_place(MD_Msg_List *dst, MD_Msg_List *to_push)
{
  if (to_push->first != NULL)
  {
    if (dst->last)
    {
      dst->last->next = to_push->first;
      dst->last = to_push->last;
      dst->count += to_push->count;
      dst->worst_message_level = Max(dst->worst_message_level, to_push->worst_message_level);
    }
    else
    {
      MemCopyStruct(dst, to_push);
    }
  }
  MemSetZeroStruct(to_push);
}

//~ ak: Token Functions
//=============================================================================

internal void md_token_chunk_list_push(MD_Token_Chunk_List *list, size_t cap, MD_Token token, Arena *arena)
{
    MD_Token_Chunk_Node *node = list->last;
    if (node == NULL || node->count >= node->cap)
    {
        node = arena_push(arena, MD_Token_Chunk_Node, 1);
        node->cap = cap;
        node->v = arena_push_nz(arena, MD_Token, cap);
        SLLQueuePush(list->first, list->last, node);
        list->chunk_count += 1;
    }
    MemCopyStruct(&node->v[node->count], &token);
    node->count += 1;
    list->total_token_count += 1;
}

internal MD_Token_Array md_token_array_from_chunk_list(Arena *arena, MD_Token_Chunk_List *chunks)
{
    MD_Token_Array result = STRUCT_ZERO;
    result.count = chunks->total_token_count;
    result.v = arena_push_nz(arena, MD_Token, result.count);
    size_t write_index = 0;
    for (MD_Token_Chunk_Node *n = chunks->first; n != NULL; n = n->next)
    {
        mem_copy(result.v+write_index, n->v, sizeof(MD_Token)*n->count);
        write_index += n->count;
    }
    return result;
}

internal Str8 md_content_string_from_token_flags_string(MD_Token_Flags flags, Str8 string)
{
    size_t num_chop = 0;
    size_t num_skip = 0;
    {
        num_skip += 3*!!(flags & MD_Token_Flag_StringTriplet);
        num_chop += 3*!!(flags & MD_Token_Flag_StringTriplet);
        num_skip += 1*(!(flags & MD_Token_Flag_StringTriplet) && flags & MD_Token_Flag_StringLiteral);
        num_chop += 1*(!(flags & MD_Token_Flag_StringTriplet) && flags & MD_Token_Flag_StringLiteral);
    }
    Str8 result = string;
    result = str8_chop(result, num_chop);
    result = str8_skip(result, num_skip);
    return result;
}

//~ ak: Node Functions
//=============================================================================

//- ak: flag conversions

internal MD_Node_Flags md_node_flags_from_token_flags(MD_Token_Flags flags)
{
    MD_Node_Flags result = 0;
    result |= MD_Node_Flag_Identifier        *!! (flags & MD_Token_Flag_Identifier);
    result |= MD_Node_Flag_Numeric           *!! (flags & MD_Token_Flag_Numeric);
    result |= MD_Node_Flag_StringLiteral     *!! (flags & MD_Token_Flag_StringLiteral);
    result |= MD_Node_Flag_Symbol            *!! (flags & MD_Token_Flag_Symbol);
    result |= MD_Node_Flag_StringSingleQuote *!! (flags & MD_Token_Flag_StringSingleQuote);
    result |= MD_Node_Flag_StringDoubleQuote *!! (flags & MD_Token_Flag_StringDoubleQuote);
    result |= MD_Node_Flag_StringTick        *!! (flags & MD_Token_Flag_StringTick);
    result |= MD_Node_Flag_StringTriplet     *!! (flags & MD_Token_Flag_StringTriplet);
    return result;
}

//- ak: nil

internal bool md_node_is_nil(MD_Node *node)
{
    return (node == NULL || node == &md_nil_node || node->kind == MD_Node_Kind_Nil);
}

//- ak: tree building

internal MD_Node *md_node_push(MD_Node_Kind kind, MD_Node_Flags flags, Str8 string, Str8 raw_string, size_t src_offset, Arena *arena)
{
    MD_Node *node    = arena_push(arena, MD_Node, 1);
    node->first      = node->last = node->parent = node->next = node->prev = node->first_tag = node->last_tag = &md_nil_node;
    node->kind       = kind;
    node->flags      = flags;
    node->string     = string;
    node->raw_string = raw_string;
    node->src_offset = src_offset;
    return node;
}

internal void md_node_child_push(MD_Node *parent, MD_Node *node)
{
    node->parent = parent;
    DLLPushBack_NPZ(&md_nil_node, parent->first, parent->last, node, next, prev);
}

//- ak: tree introspection

internal MD_Node *md_node_from_chain_string(MD_Node *first, MD_Node *opl, Str8 string, Str_Match_Flags flags)
{
    MD_Node *result = &md_nil_node;
    for (MD_Node *n = first; !md_node_is_nil(n) && n != opl; n = n->next)
    {
        if (str8_match(n->string, string, flags))
        {
            result = n;
            break;
        }
    }
    return result;
}

internal MD_Node *md_node_from_chain_index(MD_Node *first, MD_Node *opl, size_t index)
{
    MD_Node *result = &md_nil_node;
    size_t idx = 0;
    for (MD_Node *n = first; !md_node_is_nil(n) && n != opl; n = n->next, idx += 1)
    {
        if (index == idx)
        {
            result = n;
            break;
        }
    }
    return result;
}


internal MD_Node *md_tag_from_string(MD_Node *node, Str8 tag_string, Str_Match_Flags flags)
{
    return md_node_from_chain_string(node->first_tag, &md_nil_node, tag_string, flags);
}

internal MD_Node *md_root_from_node(MD_Node *node)
{
    MD_Node *result = node;
    for (MD_Node *p = node->parent; (p->kind == MD_Node_Kind_Main || p->kind == MD_Node_Kind_Tag) && !md_node_is_nil(p); p = p->parent)
    {
        result = p;
    }
    return result;
}

internal MD_Node *md_child_from_index(MD_Node *node, size_t index)
{
    return md_node_from_chain_index(node->first, &md_nil_node, index);
}

internal size_t md_child_count_from_node(MD_Node *node)
{
    size_t result = 0;
    for (MD_Node *child = node->first; !md_node_is_nil(child); child = child->next)
    {
        result += 1;
    }
    return result;
}

internal bool md_node_has_tag(MD_Node *node, Str8 string, Str_Match_Flags flags)
{
    return !md_node_is_nil(md_tag_from_string(node, string, flags));
}

//~ ak: Tokenize Functions
//=============================================================================

internal MD_Tokenize md_tokenize_from_string(Str8 string, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    MD_Msg_List msgs = STRUCT_ZERO;
    MD_Token_Chunk_List tokens = STRUCT_ZERO;
    uint8_t *byte_first = string.cstr;
    uint8_t *byte_opl = byte_first + string.length;
    uint8_t *byte = byte_first;
    
    //- ak: scan string & produce tokens
    while (byte < byte_opl)
    {
        MD_Token_Flags token_flags = 0;
        uint8_t *token_start = NULL;
        (void)token_start;
        uint8_t *token_opl = NULL;
        
        //- ak: whitespace
        if (*byte == ' ' || *byte == '\t' || *byte == '\v' || *byte == '\r')
        {
            token_flags = MD_Token_Flag_Whitespace;
            token_start = byte;
            token_opl = byte;
            byte += 1;
            for (;byte <= byte_opl; byte += 1)
            {
                token_opl += 1;
                if (byte == byte_opl || (*byte != ' ' && *byte != '\t' && *byte != '\v' && *byte != '\r'))
                {
                    break;
                }
            }
        }
        
        //- ak: newlines
        else if (*byte == '\n')
        {
            token_flags = MD_Token_Flag_Newline;
            token_start = byte;
            token_opl = byte+1;
            byte += 1;
        }
        
        //- ak: single-line comments
        else if (byte+1 < byte_opl && *byte == '/' && byte[1] == '/')
        {
            token_flags = MD_Token_Flag_Comment;
            token_start = byte;
            token_opl = byte+2;
            byte += 2;
            bool escaped = false;
            for (;byte <= byte_opl; byte += 1)
            {
                token_opl += 1;
                if (byte == byte_opl)
                {
                    break;
                }
                if (escaped)
                {
                    escaped = false;
                }
                else
                {
                    if (*byte == '\n')
                    {
                        break;
                    }
                    else if (*byte == '\\')
                    {
                        escaped = true;
                    }
                }
            }
        }
        
        //- ak: multi-line comments
        else if (byte+1 < byte_opl && *byte == '/' && byte[1] == '*')
        {
            token_flags = MD_Token_Flag_Comment;
            token_start = byte;
            token_opl = byte+2;
            byte += 2;
            for (;byte <= byte_opl; byte += 1)
            {
                token_opl += 1;
                if (byte == byte_opl)
                {
                    token_flags |= MD_Token_Flag_BrokenComment;
                    break;
                }
                if (byte+1 < byte_opl && byte[0] == '*' && byte[1] == '/')
                {
                    token_opl += 2;
                    break;
                }
            }
        }
        
        //- ak: identifiers
        else if (('A' <= *byte && *byte <= 'Z') ||
            ('a' <= *byte && *byte <= 'z') ||
            *byte == '_' || utf8_class[*byte>>3] >= 2 )
        {
            token_flags = MD_Token_Flag_Identifier;
            token_start = byte;
            token_opl = byte;
            byte += 1;
            for (;byte <= byte_opl; byte += 1)
            {
                token_opl += 1;
                if (byte == byte_opl ||
                    (!('A' <= *byte && *byte <= 'Z') &&
                    !('a' <= *byte && *byte <= 'z')  &&
                    !('0' <= *byte && *byte <= '9')  &&
                    *byte != '_' && utf8_class[*byte>>3] < 2))
                {
                    break;
                }
            }
        }
        
        //- ak: numerics
        else if (('0' <= *byte && *byte <= '9') ||
            (*byte == '.' && byte+1 < byte_opl && '0' <= byte[1] && byte[1] <= '9') ||
            (*byte == '-' && byte+1 < byte_opl && '0' <= byte[1] && byte[1] <= '9') ||
            *byte == '_')
        {
            token_flags = MD_Token_Flag_Numeric;
            token_start = byte;
            token_opl = byte;
            byte += 1;
            for (;byte <= byte_opl; byte += 1)
            {
                token_opl += 1;
                if (byte == byte_opl ||
                    (!('A' <= *byte && *byte <= 'Z') &&
                     !('a' <= *byte && *byte <= 'z') &&
                     !('0' <= *byte && *byte <= '9') &&
                     *byte != '_' &&
                     *byte != '.'))
                {
                    break;
                }
            }
        }
        
        //- ak: triplet string literals
        else if (token_flags == 0 && byte+2 < byte_opl &&
            ((byte[0] == '"' && byte[1] == '"' && byte[2] == '"') ||
             (byte[0] == '\''&& byte[1] == '\''&& byte[2] == '\'') ||
             (byte[0] == '`' && byte[1] == '`' && byte[2] == '`')))
        {
            uint8_t literal_style = byte[0];
            token_flags = MD_Token_Flag_StringLiteral|MD_Token_Flag_StringTriplet;
            token_flags |= (literal_style == '\'')*MD_Token_Flag_StringSingleQuote;
            token_flags |= (literal_style ==  '"')*MD_Token_Flag_StringDoubleQuote;
            token_flags |= (literal_style ==  '`')*MD_Token_Flag_StringTick;
            token_start = byte;
            token_opl = byte+3;
            byte += 3;
            for (;byte <= byte_opl; byte += 1)
            {
                if (byte == byte_opl)
                {
                    token_flags |= MD_Token_Flag_BrokenStringLiteral;
                    token_opl = byte;
                    break;
                }
                if (byte+2 < byte_opl &&
                    (byte[0] == literal_style && byte[1] == literal_style &&
                     byte[2] == literal_style))
                {
                    byte += 3;
                    token_opl = byte;
                    break;
                }
            }
        }
        
        //- ak: singlet string literals
        else if (byte[0] == '"' || byte[0] == '\'' || byte[0] == '`')
        {
            uint8_t literal_style = byte[0];
            token_flags = MD_Token_Flag_StringLiteral;
            token_flags |= (literal_style == '\'') * MD_Token_Flag_StringSingleQuote;
            token_flags |= (literal_style ==  '"') * MD_Token_Flag_StringDoubleQuote;
            token_flags |= (literal_style ==  '`') * MD_Token_Flag_StringTick;
            token_start = byte;
            token_opl   = byte+1;
            byte += 1;
            bool escaped = false;
            for (;byte <= byte_opl; byte += 1)
            {
                if (byte == byte_opl || *byte == '\n')
                {
                    token_opl = byte;
                    token_flags |= MD_Token_Flag_BrokenStringLiteral;
                    break;
                }
                if (!escaped && byte[0] == '\\')
                {
                    escaped = true;
                }
                else if (!escaped && byte[0] == literal_style)
                {
                    token_opl = byte+1;
                    byte += 1;
                    break;
                }
                else if (escaped)
                {
                    escaped = false;
                }
            }
        }
        
        //- ak: non-reserved symbols
        else if (*byte == '~' || *byte == '!' || *byte == '$' || *byte == '%' ||
            *byte == '^'  || *byte == '&' || *byte == '*' || *byte == '-' ||
            *byte == '=' || *byte == '+' || *byte == '<' || *byte == '.' ||
            *byte == '>' || *byte == '/' || *byte == '?' || *byte == '|')
        {
            token_flags = MD_Token_Flag_Symbol;
            token_start = byte;
            token_opl = byte;
            byte += 1;
            for (;byte <= byte_opl; byte += 1)
            {
                token_opl += 1;
                if (byte == byte_opl ||
                        (*byte != '~' && *byte != '!' && *byte != '$' && *byte != '%' && *byte != '^' &&
                         *byte != '&' && *byte != '*' && *byte != '-' && *byte != '=' && *byte != '+' &&
                         *byte != '<' && *byte != '.' && *byte != '>' && *byte != '/' && *byte != '?' &&
                         *byte != '|'))
                {
                    break;
                }
            }
        }
        
        //- ak: reserved symbols
        else if (*byte == '{' || *byte == '}' || *byte == '(' || *byte == ')'  ||
            *byte == '[' || *byte == ']' || *byte == '#' || *byte == ',' ||
            *byte == '\\'|| *byte == ':' || *byte == ';' || *byte == '@')
        {
            token_flags = MD_Token_Flag_Reserved;
            token_start = byte;
            token_opl = byte+1;
            byte += 1;
        }
        
        //- ak: bad characters in all other cases
        else
        {
            token_flags = MD_Token_Flag_BadCharacter;
            token_start = byte;
            token_opl = byte+1;
            byte += 1;
        }
        
        //- ak; push token if formed
        if (token_flags != 0 && token_start != 0 && token_opl > token_start)
        {
            MD_Token token = (MD_Token){
                {
                    (size_t)(token_start - byte_first),
                    (size_t)(token_opl - byte_first)
                },
                token_flags
            };
            md_token_chunk_list_push(&tokens, 4096, token, scratch.arena);
        }
        
        //- ak: push errors on unterminated comments
        if (token_flags & MD_Token_Flag_BrokenComment)
        {
            MD_Node *error = md_node_push(MD_Node_Kind_ErrorMarker, 0, str8(""), str8(""), token_start - byte_first, arena);
            Str8 error_string = str8("Unterminated comment.");
            md_msg_list_push(arena, &msgs, error, MD_Msg_Level_Error, error_string);
        }
        
        //- ak: push errors on unterminated strings
        if (token_flags & MD_Token_Flag_BrokenStringLiteral)
        {
            MD_Node *error = md_node_push(MD_Node_Kind_ErrorMarker, 0, str8(""), str8(""), token_start - byte_first, arena);
            Str8 error_string = str8("Unterminated string literal.");
            md_msg_list_push(arena, &msgs, error, MD_Msg_Level_Error, error_string);
        }
    }
    
    //- ak: bake, fill & return
    MD_Tokenize result = STRUCT_ZERO;
    {
        result.tokens = md_token_array_from_chunk_list(arena, &tokens);
        result.msgs = msgs;
    }
    arena_scratch_end(scratch);
    return result;
}

//~ ak: Parse Functions
//=============================================================================

internal MD_Parse md_parse_from_string_tokens(Str8 string, MD_Token_Array tokens, Str8 filename, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    
    //- ak: set up outputs
    MD_Msg_List msgs = STRUCT_ZERO;
    MD_Node *root = md_node_push(MD_Node_Kind_File, 0, filename, string, 0, arena);
    
    //- ak: set up parse rule stack
    typedef enum MD_Parse_Work_Kind
    {
        MD_Parse_Work_Kind_Main,
        MD_Parse_Work_Kind_MainImplicit,
        MD_Parse_Work_Kind_NodeOptionalFollowUp,
        MD_Parse_Work_Kind_NodeChildrenStyleScan,
    }
    MD_Parse_Work_Kind;
    typedef struct MD_Parse_Work_Node MD_Parse_Work_Node;
    struct MD_Parse_Work_Node
    {
        MD_Parse_Work_Node *next;
        MD_Parse_Work_Kind kind;
        MD_Node *parent;
        MD_Node *first_gathered_tag;
        MD_Node *last_gathered_tag;
        MD_Node_Flags gathered_node_flags;
        size_t counted_newlines;
    };
    MD_Parse_Work_Node first_work = STRUCT_ZERO;
    first_work.kind = MD_Parse_Work_Kind_Main;
    first_work.parent = root;
    MD_Parse_Work_Node broken_work = STRUCT_ZERO;
    broken_work.kind = MD_Parse_Work_Kind_Main;
    broken_work.parent = root;
    MD_Parse_Work_Node *work_top = &first_work;
    MD_Parse_Work_Node *work_free = NULL;
#define MD_ParseWorkPush(work_kind, work_parent) do\
    {\
        MD_Parse_Work_Node *work_node = work_free;\
        if (work_node == NULL) {work_node = arena_push(scratch.arena, MD_Parse_Work_Node, 1);}\
        else { SLLStackPop(work_free); }\
        work_node->kind   = (work_kind);\
        work_node->parent = (work_parent);\
        SLLStackPush(work_top, work_node);\
    } while(0)
#define MD_ParseWorkPop() do\
    {\
        SLLStackPop(work_top);\
        if (work_top == NULL) {work_top = &broken_work;}\
    } while(0)
    
    //- ak: parse
    MD_Token *tokens_first = tokens.v;
    MD_Token *tokens_opl = tokens_first + tokens.count;
    MD_Token *token = tokens_first;
    while (token < tokens_opl)
    {
        //- ak: unpack token
        Str8 token_string = str8_substr(string, token[0].range);
        
        //- ak: whitespace -> always no-op & inc
        if (token->flags & MD_Token_Flag_Whitespace)
        {
            token += 1;
        }
        
        //- ak: comments -> always no-op & inc
        else if (token->flags & MD_Token_Group_Comment)
        {
            token += 1;
        }
        
        //- ak: [node follow up] : following label -> work top parent has children.
        // we need to scan for explicit delimiters, else parse an implicitly
        // delimited set of children
        else if (work_top->kind == MD_Parse_Work_Kind_NodeOptionalFollowUp && str8_match(token_string, str8(":"), 0))
        {
            MD_Node *parent = work_top->parent;
            MD_ParseWorkPop();
            MD_ParseWorkPush(MD_Parse_Work_Kind_NodeChildrenStyleScan, parent);
            token += 1;
        }
        
        //- ak: [node follow up] anything but : following label -> node has no
        // children. just pop & move on
        else if (work_top->kind == MD_Parse_Work_Kind_NodeOptionalFollowUp)
        {
            MD_ParseWorkPop();
        }
        
        //- ak: [main] separators -> mark & inc
        else if (work_top->kind == MD_Parse_Work_Kind_Main &&
            token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8(","), 0) || str8_match(token_string, str8(";"), 0)))
        {
            MD_Node *parent = work_top->parent;
            if (!md_node_is_nil(parent->last))
            {
                parent->last->flags           |= MD_Node_Flag_IsBeforeComma     *!! str8_match(token_string, str8(","), 0);
                parent->last->flags           |= MD_Node_Flag_IsBeforeSemicolon *!! str8_match(token_string, str8(";"), 0);
                work_top->gathered_node_flags |= MD_Node_Flag_IsAfterComma      *!! str8_match(token_string, str8(","), 0);
                work_top->gathered_node_flags |= MD_Node_Flag_IsAfterSemicolon  *!! str8_match(token_string, str8(";"), 0);
            }
            token += 1;
        }
        
        //- ak: [main_implicit] separators -> pop
        else if (work_top->kind == MD_Parse_Work_Kind_MainImplicit && token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8(","), 0) || str8_match(token_string, str8(";"), 0)))
        {
            MD_ParseWorkPop();
        }
        
        //- ak: [main, main_implicit] unexpected reserved tokens
        else if ((work_top->kind == MD_Parse_Work_Kind_Main ||
            work_top->kind == MD_Parse_Work_Kind_MainImplicit) &&
            token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8("#"), 0) ||
            str8_match(token_string, str8("\\"), 0) ||
            str8_match(token_string, str8(":"), 0)))
        {
            MD_Node *error = md_node_push(MD_Node_Kind_ErrorMarker, 0, token_string, token_string, token->range.min, arena);
            Str8 error_string = str8f(arena, "Unexpected reserved symbol \"%.*s\".", str8_varg(token_string));
            md_msg_list_push(arena, &msgs, error, MD_Msg_Level_Error, error_string);
            token += 1;
        }
        
        //- ak: [main, main_implicit] tag signifier -> create new tag
        else if ((work_top->kind == MD_Parse_Work_Kind_Main ||
            work_top->kind == MD_Parse_Work_Kind_MainImplicit) &&
            token[0].flags & MD_Token_Flag_Reserved &&
            str8_match(token_string, str8("@"), 0))
        {
            if (token+1 >= tokens_opl || !(token[1].flags & MD_Token_Group_Label))
            {
                MD_Node *error = md_node_push(MD_Node_Kind_ErrorMarker, 0, token_string, token_string, token->range.min, arena);
                Str8 error_string = str8("Tag label expected after @ symbol.");
                md_msg_list_push(arena, &msgs, error, MD_Msg_Level_Error, error_string);
                token += 1;
            }
            else
            {
                Str8 tag_name_raw = str8_substr(string, token[1].range);
                Str8 tag_name = md_content_string_from_token_flags_string(token[1].flags, tag_name_raw);
                MD_Node *node = md_node_push(MD_Node_Kind_Tag, md_node_flags_from_token_flags(token[1].flags), tag_name, tag_name_raw, token[0].range.min, arena);
                DLLPushBack_NPZ(&md_nil_node, work_top->first_gathered_tag, work_top->last_gathered_tag, node, next, prev);
                if (token+2 < tokens_opl && token[2].flags & MD_Token_Flag_Reserved &&
                        (str8_match(str8_substr(string, token[2].range), str8("("), 0) ||
                         str8_match(str8_substr(string, token[2].range), str8("["), 0) ||
                         str8_match(str8_substr(string, token[2].range), str8("{"), 0)))
                {
                    token += 3;
                    MD_ParseWorkPush(MD_Parse_Work_Kind_Main, node);
                }
                else
                {
                    token += 2;
                }
            }
        }
        
        //- ak: [main, main_implicit] label -> create new main
        else if ((work_top->kind == MD_Parse_Work_Kind_Main ||
            work_top->kind == MD_Parse_Work_Kind_MainImplicit) &&
            token->flags & MD_Token_Group_Label)
        {
            Str8 node_string_raw = token_string;
            Str8 node_string = md_content_string_from_token_flags_string(token->flags, node_string_raw);
            MD_Node_Flags flags = md_node_flags_from_token_flags(token->flags)|work_top->gathered_node_flags;
            work_top->gathered_node_flags = 0;
            MD_Node *node = md_node_push(MD_Node_Kind_Main, flags, node_string, node_string_raw, token[0].range.min, arena);
            node->first_tag = work_top->first_gathered_tag;
            node->last_tag = work_top->last_gathered_tag;
            for (MD_Node *tag = work_top->first_gathered_tag; !md_node_is_nil(tag); tag = tag->next)
            {
                tag->parent = node;
            }
            work_top->first_gathered_tag = work_top->last_gathered_tag = &md_nil_node;
            md_node_child_push(work_top->parent, node);
            MD_ParseWorkPush(MD_Parse_Work_Kind_NodeOptionalFollowUp, node);
            token += 1;
        }
        
        //- ak: [main] {s, [s, and (s -> create new main
        else if (work_top->kind == MD_Parse_Work_Kind_Main &&
            token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8("{"), 0) ||
             str8_match(token_string, str8("["), 0) ||
             str8_match(token_string, str8("("), 0)))
        {
            MD_Node_Flags flags = md_node_flags_from_token_flags(token->flags)|work_top->gathered_node_flags;
            flags |= MD_Node_Flag_HasBraceLeft*!!str8_match(token_string, str8("{"), 0);
            flags |= MD_Node_Flag_HasBracketLeft*!!str8_match(token_string, str8("["), 0);
            flags |= MD_Node_Flag_HasParenLeft*!!str8_match(token_string, str8("("), 0);
            work_top->gathered_node_flags = 0;
            MD_Node *node = md_node_push(MD_Node_Kind_Main, flags, str8(""), str8(""), token[0].range.min, arena);
            node->first_tag = work_top->first_gathered_tag;
            node->last_tag = work_top->last_gathered_tag;
            for (MD_Node *tag = work_top->first_gathered_tag; !md_node_is_nil(tag); tag = tag->next)
            {
                tag->parent = node;
            }
            work_top->first_gathered_tag = work_top->last_gathered_tag = &md_nil_node;
            md_node_child_push(work_top->parent, node);
            MD_ParseWorkPush(MD_Parse_Work_Kind_Main, node);
            token += 1;
        }
        
        //- ak: [node children style scan] {s, [s, and (s -> explicitly
        // delimited children
        else if (work_top->kind == MD_Parse_Work_Kind_NodeChildrenStyleScan &&
            token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8("{"), 0) ||
             str8_match(token_string, str8("["), 0) ||
             str8_match(token_string, str8("("), 0)))
        {
            MD_Node *parent = work_top->parent;
            parent->flags |= MD_Node_Flag_HasBraceLeft*!!str8_match(token_string, str8("{"), 0);
            parent->flags |= MD_Node_Flag_HasBracketLeft*!!str8_match(token_string, str8("["), 0);
            parent->flags |= MD_Node_Flag_HasParenLeft*!!str8_match(token_string, str8("("), 0);
            MD_ParseWorkPop();
            MD_ParseWorkPush(MD_Parse_Work_Kind_Main, parent);
            token += 1;
        }
        
        //- ak: [node children style scan] count newlines
        else if (work_top->kind == MD_Parse_Work_Kind_NodeChildrenStyleScan && token->flags & MD_Token_Flag_Newline)
        {
            work_top->counted_newlines += 1;
            token += 1;
        }
        
        //- ak: [main_implicit] newline -> pop *all* current implicit work tasks
        else if (work_top->kind == MD_Parse_Work_Kind_MainImplicit && token->flags & MD_Token_Flag_Newline)
        {
            while (work_top->kind == MD_Parse_Work_Kind_MainImplicit)
            {
                MD_ParseWorkPop();
            }
            token += 1;
        }
        
        //- ak: [all but main_implicit] newline -> no-op & inc
        else if (work_top->kind != MD_Parse_Work_Kind_MainImplicit &&
            token->flags & MD_Token_Flag_Newline)
        {
            token += 1;
        }
        
        //- ak: [node children style scan] anything causing implicit set -> <2 newlines,
        // all good, >=2 newlines, houston we have a problem
        else if (work_top->kind == MD_Parse_Work_Kind_NodeChildrenStyleScan)
        {
            if (work_top->counted_newlines >= 2)
            {
                MD_Node *node = work_top->parent;
                MD_Node *error = md_node_push(MD_Node_Kind_ErrorMarker, 0, token_string, token_string, token->range.min, arena);
                Str8 error_string = str8f(arena, "More than two newlines following \"%.*s\", which has implicitly-delimited children, resulting in an empty list of children.", str8_varg(node->string));
                md_msg_list_push(arena, &msgs, error, MD_Msg_Level_Warn, error_string);
                MD_ParseWorkPop();
            }
            else
            {
                MD_Node *parent = work_top->parent;
                MD_ParseWorkPop();
                MD_ParseWorkPush(MD_Parse_Work_Kind_MainImplicit, parent);
            }
        }
        
        //- ak: [main] }s, ]s, and )s -> pop
        else if (work_top->kind == MD_Parse_Work_Kind_Main &&
            token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8("}"), 0) ||
             str8_match(token_string, str8("]"), 0) ||
             str8_match(token_string, str8(")"), 0)))
        {
            MD_Node *parent = work_top->parent;
            parent->flags |=   MD_Node_Flag_HasBraceRight*!!str8_match(token_string, str8("}"), 0);
            parent->flags |= MD_Node_Flag_HasBracketRight*!!str8_match(token_string, str8("]"), 0);
            parent->flags |=   MD_Node_Flag_HasParenRight*!!str8_match(token_string, str8(")"), 0);
            MD_ParseWorkPop();
            token += 1;
        }
        
        //- ak: [main implicit] }s, ]s, and )s -> pop without advancing
        else if (work_top->kind == MD_Parse_Work_Kind_MainImplicit &&
            token->flags & MD_Token_Flag_Reserved &&
            (str8_match(token_string, str8("}"), 0) ||
             str8_match(token_string, str8("]"), 0) ||
             str8_match(token_string, str8(")"), 0)))
        {
            MD_ParseWorkPop();
        }
        
        //- ak: no consumption -> unexpected token! we don't know what to do with this.
        else {
            MD_Node *error = md_node_push(MD_Node_Kind_ErrorMarker, 0, token_string, token_string, token->range.min, arena);
            Str8 error_string = str8f(arena, "Unexpected \"%.*s\" token.", str8_varg(token_string));
            md_msg_list_push(arena, &msgs, error, MD_Msg_Level_Error, error_string);
            token += 1;
        }
    }
    MD_Parse parse = STRUCT_ZERO;
    parse.root = root;
    parse.msgs = msgs;
    arena_scratch_end(scratch);
    return parse;
}

internal MD_Parse md_parse_from_string(Str8 string, Str8 filename, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    MD_Tokenize tokenize = md_tokenize_from_string(string, scratch.arena);
    MD_Parse parse = md_parse_from_string_tokens(string, tokenize.tokens, filename, arena);
    arena_scratch_end(scratch);
    return parse;
}
