// NOTE(ak): the lib inspiration taken from:
//  - [Ryan Fleury](https://www.dgtlgrove.com)
//      - [metagen](https://github.com/EpicGamesExt/raddebugger/tree/master/src/metagen)

#ifndef METADESK_GEN_H
#define METADESK_GEN_H

//~ ak: Struct
//=============================================================================

//~ ak: Message Type ==========================================================

typedef struct MDG_Msg MDG_Msg;
struct MDG_Msg
{
    Log_Level level;
    Txt_Pt pt;
    Str8 file_path;
    Str8 string;
};

typedef struct MDG_Msg_Node MDG_Msg_Node;
struct MDG_Msg_Node
{
    MDG_Msg_Node *next;
    MDG_Msg v;
};

typedef struct MDG_Msg_List MDG_Msg_List;
struct MDG_Msg_List
{
    MDG_Msg_Node *first;
    MDG_Msg_Node *last;
    uint64_t count;
};

//~ ak: Parse Artifact Types ==================================================

typedef struct MDG_ParsedFile MDG_ParsedFile;
struct MDG_ParsedFile
{
    MD_Node *root;
};

typedef struct MDG_ParsedFile_Node MDG_ParsedFile_Node;
struct MDG_ParsedFile_Node
{
    MDG_ParsedFile_Node *next;
    MDG_ParsedFile v;
};

typedef struct MDG_ParsedFile_List MDG_ParsedFile_List;
struct MDG_ParsedFile_List
{
    MDG_ParsedFile_Node *first;
    MDG_ParsedFile_Node *last;
    size_t count;
};


//~ ak: Map Type ==============================================================

typedef struct MDG_Map_Node MDG_Map_Node;
struct MDG_Map_Node
{
    MDG_Map_Node *next;
    Str8 key;
    void *val;
};

typedef struct MDG_Map_Slot MDG_Map_Slot;
struct MDG_Map_Slot
{
    MDG_Map_Node *first;
    MDG_Map_Node *last;
};

typedef struct MDG_Map MDG_Map;
struct MDG_Map
{
    MDG_Map_Slot *slots;
    uint64_t slots_count;
};

//~ ak: String Expression Types ===============================================

typedef enum MDG_Str_Expr_Op_Kind
{
    MDG_Str_Expr_Op_Kind_Null,
    MDG_Str_Expr_Op_Kind_Prefix,
    MDG_Str_Expr_Op_Kind_Postfix,
    MDG_Str_Expr_Op_Kind_Binary,
    MDG_Str_Expr_Op_Kind_COUNT
}
MDG_Str_Expr_Op_Kind;

typedef enum MDG_Str_Expr_Op
{
    MDG_Str_Expr_Op_Null,

#define MDG_Str_Expr_Op_FirstString MDG_Str_Expr_Op_Dot
    MDG_Str_Expr_Op_Dot,
    MDG_Str_Expr_Op_ExpandIfTrue,
    MDG_Str_Expr_Op_Concat,
    MDG_Str_Expr_Op_BumpToColumn,
#define MDG_Str_Expr_Op_LastString MDG_Str_Expr_Op_BumpToColumn

#define MDG_Str_Expr_Op_FirstNumeric MDG_Str_Expr_Op_Add
    MDG_Str_Expr_Op_Add,
    MDG_Str_Expr_Op_Subtract,
    MDG_Str_Expr_Op_Multiply,
    MDG_Str_Expr_Op_Divide,
    MDG_Str_Expr_Op_Modulo,
    MDG_Str_Expr_Op_LeftShift,
    MDG_Str_Expr_Op_RightShift,
    MDG_Str_Expr_Op_BitwiseAnd,
    MDG_Str_Expr_Op_BitwiseOr,
    MDG_Str_Expr_Op_BitwiseXor,
    MDG_Str_Expr_Op_BitwiseNegate,
    MDG_Str_Expr_Op_BooleanAnd,
    MDG_Str_Expr_Op_BooleanOr,
    MDG_Str_Expr_Op_BooleanNot,
    MDG_Str_Expr_Op_Equals,
    MDG_Str_Expr_Op_DoesNotEqual,
#define MDG_Str_Expr_Op_LastNumeric MDG_Str_Expr_Op_DoesNotEqual

    MDG_Str_Expr_Op_COUNT,
}
MDG_Str_Expr_Op;

typedef struct MDG_Str_Expr MDG_Str_Expr;
struct MDG_Str_Expr
{
    MDG_Str_Expr *parent;
    MDG_Str_Expr *left;
    MDG_Str_Expr *right;
    MDG_Str_Expr_Op op;
    MD_Node *node;
};

typedef struct MDG_Str_Expr_ParseResult MDG_Str_Expr_ParseResult;
struct MDG_Str_Expr_ParseResult
{
    MDG_Str_Expr *root;
    MD_Node *next_node;
    MD_Msg_List msgs;
};

//~ ak: Table Generation Types ================================================

typedef struct MDG_Node_Array MDG_Node_Array;
struct MDG_Node_Array
{
    MD_Node **v;
    uint64_t count;
};

typedef struct MDG_Node_Grid MDG_Node_Grid;
struct MDG_Node_Grid
{
    uint64_t x_stride;
    uint64_t y_stride;
    MDG_Node_Array cells;
    MDG_Node_Array row_parents;
};

typedef enum MDG_Column_Kind
{
    MDG_Column_Kind_DirectCell,
    MDG_Column_Kind_CheckForTag,
    MDG_Column_Kind_TagChild,
    MDG_Column_Kind_COUNT
}
MDG_Column_Kind;

typedef struct MDG_Column_Desc MDG_Column_Desc;
struct MDG_Column_Desc
{
    Str8 name;
    MDG_Column_Kind kind;
    Str8 tag_name;
};

typedef struct MDG_Column_Desc_Array MDG_Column_Desc_Array;
struct MDG_Column_Desc_Array
{
    uint64_t count;
    MDG_Column_Desc *v;
};

typedef struct MDG_TableExpand_Task MDG_TableExpand_Task;
struct MDG_TableExpand_Task
{
    MDG_TableExpand_Task *next;
    Str8 expansion_label;
    MDG_Node_Grid *grid;
    MDG_Column_Desc_Array column_descs;
    uint64_t count;
    uint64_t index;
};

typedef struct MDG_TableExpand_Info MDG_TableExpand_Info;
struct MDG_TableExpand_Info
{
    MDG_TableExpand_Task *first_expand_task;
    Str8 missing_value_fallback;
};

//~ ak: Main Output Path Types ================================================

typedef struct MDG_Layer MDG_Layer;
struct MDG_Layer
{
    Str8 key;
    Str8 src_path;
    bool is_library;
    Str8 gen_folder_name;
    Str8 h_name_override;
    Str8 c_name_override;
    Str8_List enums;
    Str8_List structs;
    Str8_List h_functions;
    Str8_List h_tables;
    Str8_List h_catchall;
    Str8_List h_header;
    Str8_List h_footer;
    Str8_List c_functions;
    Str8_List c_tables;
    Str8_List c_catchall;
    Str8_List c_header;
    Str8_List c_footer;
};

typedef struct MDG_Layer_Node MDG_Layer_Node;
struct MDG_Layer_Node
{
    MDG_Layer_Node *next;
    MDG_Layer v;
};

typedef struct MDG_Layer_Slot MDG_Layer_Slot;
struct MDG_Layer_Slot
{
    MDG_Layer_Node *first;
    MDG_Layer_Node *last;
};

typedef struct MDG_State MDG_State;
struct MDG_State
{
    size_t slots_count;
    MDG_Layer_Slot *slots;
};

//~ ak: Functions
//=============================================================================

//~ ak: Helper Functions ======================================================

internal uint64_t mdg_hash_from_string(Str8 string);

//~ ak: Message Lists =========================================================

internal void mdg_msg_list_push(MDG_Msg_List *msgs, MDG_Msg *msg, Arena *arena);

// ak: C-String-Izing =======================================================

internal Str8 mdg_c_string_literal_from_multiline_string(Str8 string, Arena *arena);
internal Str8 mdg_c_array_literal_contents_from_string(Str8 string, Arena *arena);

//~ ak: Map Functions =========================================================

internal MDG_Map mdg_map_push(size_t slot_count, Arena *arena);
internal void *mdg_map_ptr_from_string(MDG_Map *map, Str8 string);
internal void mdg_map_insert_ptr(MDG_Map *map, Str8 string, void *val, Arena *arena);

//~ ak: String Expression Parsing =============================================

internal MDG_Str_Expr *mdg_str_expr_push(Arena *arena, MDG_Str_Expr_Op op, MD_Node *node);
internal MDG_Str_Expr_ParseResult mdg_str_expr_parse_from_first_opl_min_prec(Arena *arena, MD_Node *first, MD_Node *opl, char min_prec);
internal MDG_Str_Expr_ParseResult mdg_str_expr_parse_from_root(MD_Node *root, Arena *arena);

//~ ak: Table Generation Functions ============================================

internal MDG_Node_Array mdg_node_array_make(size_t count, Arena *arena);
internal MDG_Node_Grid mdg_node_grid_make_from_node(MD_Node *root, Arena *arena);
internal MDG_Column_Desc_Array mdg_column_desc_array_from_tag(MD_Node *tag, Arena *arena);
internal size_t mdg_column_index_from_name(MDG_Column_Desc_Array descs, Str8 name);
internal Str8 mdg_string_from_row_desc_index(MD_Node *row_parent, MDG_Column_Desc_Array descs, size_t index);
internal int64_t mdg_eval_table_expand_expr_numeric(MDG_Str_Expr *expr, MDG_TableExpand_Info *info);
internal void mdg_eval_table_expand_expr_string(MDG_Str_Expr *expr, MDG_TableExpand_Info *info, Str8_List *out, Arena *arena);
internal void mdg_loop_table_column_expansion(Str8 strexpr, MDG_TableExpand_Info *info, MDG_TableExpand_Task *task, Str8_List *out, Arena *arena);
internal Str8_List mdg_str_list_from_table_gen(MDG_Map grid_name_map, MDG_Map grid_column_desc_map, Str8 fallback, MD_Node *gen, Arena *arena);

//~ ak: Layer Lookup Functions ================================================

internal Str8 mdg_layer_key_from_path(Str8 path, Str8 skip_str, Arena *arena);
internal MDG_Layer *mdg_layer_from_key(MDG_State *state, Str8 key, Arena *arena);

//~ ak: Main Output Path Functions ============================================

internal MDG_State *mdg_state_init(size_t count, Arena *arena);

//~ ak: Globals
//=============================================================================

PragmaNoWarnMissingFieldInitPush()

read_only global MDG_Str_Expr mdg_str_expr_nil = {
    &mdg_str_expr_nil,
    &mdg_str_expr_nil,
    &mdg_str_expr_nil
};

PragmaPop()

read_only global Str8 mdg_str_expr_op_symbol_string_table[MDG_Str_Expr_Op_COUNT] =
{
    str8(""),
    str8("."),  // MDG_Str_Expr_Op_Dot
    str8("->"), // MDG_Str_Expr_Op_ExpandIfTrue
    str8(".."), // MDG_Str_Expr_Op_Concat
    str8("=>"), // MDG_Str_Expr_Op_BumpToColumn
    str8("+"),  // MDG_Str_Expr_Op_Add
    str8("-"),  // MDG_Str_Expr_Op_Subtract
    str8("*"),  // MDG_Str_Expr_Op_Multiply
    str8("/"),  // MDG_Str_Expr_Op_Divide
    str8("%"),  // MDG_Str_Expr_Op_Modulo
    str8("<<"), // MDG_Str_Expr_Op_LeftShift
    str8(">>"), // MDG_Str_Expr_Op_RightShift
    str8("&"),  // MDG_Str_Expr_Op_BitwiseAnd
    str8("|"),  // MDG_Str_Expr_Op_BitwiseOr
    str8("^"),  // MDG_Str_Expr_Op_BitwiseXor
    str8("~"),  // MDG_Str_Expr_Op_BitwiseNegate
    str8("&&"), // MDG_Str_Expr_Op_BooleanAnd
    str8("||"), // MDG_Str_Expr_Op_BooleanOr
    str8("!"),  // MDG_Str_Expr_Op_BooleanNot
    str8("=="), // MDG_Str_Expr_Op_Equals
    str8("!="), // MDG_Str_Expr_Op_DoesNotEqual
};

read_only global MDG_Str_Expr_Op_Kind mdg_str_expr_op_kind_table[MDG_Str_Expr_Op_COUNT] =
{
    MDG_Str_Expr_Op_Kind_Null,
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Dot
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_ExpandIfTrue
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Concat
    MDG_Str_Expr_Op_Kind_Prefix, // MDG_Str_Expr_Op_BumpToColumn
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Add
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Subtract
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Multiply
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Divide
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Modulo
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_LeftShift
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_RightShift
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_BitwiseAnd
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_BitwiseOr
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_BitwiseXor
    MDG_Str_Expr_Op_Kind_Prefix, // MDG_Str_Expr_Op_BitwiseNegate
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_BooleanAnd
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_BooleanOr
    MDG_Str_Expr_Op_Kind_Prefix, // MDG_Str_Expr_Op_BooleanNot
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_Equals
    MDG_Str_Expr_Op_Kind_Binary, // MDG_Str_Expr_Op_DoesNotEqual
};

read_only global int8_t mdg_str_expr_op_precedence_table[MDG_Str_Expr_Op_COUNT] =
{
    0,
    20, // MDG_Str_Expr_Op_Dot
    1,  // MDG_Str_Expr_Op_ExpandIfTrue
    2,  // MDG_Str_Expr_Op_Concat
    12, // MDG_Str_Expr_Op_BumpToColumn
    5,  // MDG_Str_Expr_Op_Add
    5,  // MDG_Str_Expr_Op_Subtract
    6,  // MDG_Str_Expr_Op_Multiply
    6,  // MDG_Str_Expr_Op_Divide
    6,  // MDG_Str_Expr_Op_Modulo
    7,  // MDG_Str_Expr_Op_LeftShift
    7,  // MDG_Str_Expr_Op_RightShift
    8,  // MDG_Str_Expr_Op_BitwiseAnd
    10, // MDG_Str_Expr_Op_BitwiseOr
    9,  // MDG_Str_Expr_Op_BitwiseXor
    11, // MDG_Str_Expr_Op_BitwiseNegate
    3,  // MDG_Str_Expr_Op_BooleanAnd
    3,  // MDG_Str_Expr_Op_BooleanOr
    11, // MDG_Str_Expr_Op_BooleanNot
    4,  // MDG_Str_Expr_Op_Equals
    4,  // MDG_Str_Expr_Op_DoesNotEqual
};

#endif // METADESK_GEN_H
