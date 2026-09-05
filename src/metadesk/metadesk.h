// NOTE(ak): the lib inspiration taken from:
//  - [Ryan Fleury](https://www.dgtlgrove.com)
//      - [Table-Driven Code Generation](https://www.dgtlgrove.com/p/table-driven-code-generation)
//      - [Metadesk](https://github.com/ryanfleury/metadesk)
//      - [RadDebugger Metadesk](https://github.com/EpicGamesExt/raddebugger/tree/master/src/mdesk)

#ifndef METADESK_H
#define METADESK_H

//~ ak: Types
//=============================================================================

//~ ak: Messages Types ========================================================

typedef enum MD_Msg_Level
{
    MD_Msg_Level_None,
    MD_Msg_Level_Info,
    MD_Msg_Level_Debug,
    MD_Msg_Level_Warn,
    MD_Msg_Level_Error,
}
MD_Msg_Level;

typedef struct MD_Msg MD_Msg;
struct MD_Msg
{
    MD_Msg *next;
    struct MD_Node *node;
    MD_Msg_Level level;
    Str8 string;
};

typedef struct MD_Msg_List MD_Msg_List;
struct MD_Msg_List
{
    MD_Msg *first;
    MD_Msg *last;
    size_t count;
    MD_Msg_Level worst_message_level;
};

//~ ak: Tokens Types ==========================================================

typedef uint32_t MD_Token_Flags;
enum
{
    // ak: base kind info
    MD_Token_Flag_Identifier          = (1<<0),
    MD_Token_Flag_Numeric             = (1<<1),
    MD_Token_Flag_StringLiteral       = (1<<2),
    MD_Token_Flag_Symbol              = (1<<3),
    MD_Token_Flag_Reserved            = (1<<4),
    MD_Token_Flag_Comment             = (1<<5),
    MD_Token_Flag_Whitespace          = (1<<6),
    MD_Token_Flag_Newline             = (1<<7),
    
    // ak: decoration info
    MD_Token_Flag_StringSingleQuote   = (1<<8),
    MD_Token_Flag_StringDoubleQuote   = (1<<9),
    MD_Token_Flag_StringTick          = (1<<10),
    MD_Token_Flag_StringTriplet       = (1<<11),
    
    // ak: error info
    MD_Token_Flag_BrokenComment       = (1<<12),
    MD_Token_Flag_BrokenStringLiteral = (1<<13),
    MD_Token_Flag_BadCharacter        = (1<<14),
};

typedef uint32_t MD_Token_Groups;
enum
{
  MD_Token_Group_Comment    = MD_Token_Flag_Comment,
  MD_Token_Group_Whitespace = (MD_Token_Flag_Whitespace|
                              MD_Token_Flag_Newline),
  MD_Token_Group_Irregular  = (MD_Token_Group_Comment|
                              MD_Token_Group_Whitespace),
  MD_Token_Group_Regular    = ~MD_Token_Group_Irregular,
  MD_Token_Group_Label      = (MD_Token_Flag_Identifier|
                              MD_Token_Flag_Numeric|
                              MD_Token_Flag_StringLiteral|
                              MD_Token_Flag_Symbol),
  MD_Token_Group_Error      = (MD_Token_Flag_BrokenComment|
                              MD_Token_Flag_BrokenStringLiteral|
                              MD_Token_Flag_BadCharacter),
};

typedef struct MD_Token MD_Token;
struct MD_Token
{
    Rng1_U64 range;
    MD_Token_Flags flags;
};

typedef struct MD_Token_Chunk_Node MD_Token_Chunk_Node;
struct MD_Token_Chunk_Node
{
    MD_Token_Chunk_Node *next;
    MD_Token *v;
    size_t count;
    size_t cap;
};

typedef struct MD_Token_Chunk_List MD_Token_Chunk_List;
struct MD_Token_Chunk_List
{
    MD_Token_Chunk_Node *first;
    MD_Token_Chunk_Node *last;
    size_t chunk_count;
    size_t total_token_count;
};

typedef struct MD_Token_Array MD_Token_Array;
struct MD_Token_Array
{
    MD_Token *v;
    size_t count;
};

//~ ak: Node Types ============================================================

typedef enum MD_Node_Kind
{
    MD_Node_Kind_Nil,
    MD_Node_Kind_File,
    MD_Node_Kind_ErrorMarker,
    MD_Node_Kind_Main,
    MD_Node_Kind_Tag,
    MD_Node_Kind_List,
    MD_Node_Kind_Reference,
    MD_Node_Kind_COUNT
}
MD_Node_Kind;

typedef uint32_t MD_Node_Flags;
enum
{
    MD_Node_Flag_MaskSetDelimiters          = (0x3F<<0),
    MD_Node_Flag_HasParenLeft               = (1<<0),
    MD_Node_Flag_HasParenRight              = (1<<1),
    MD_Node_Flag_HasBracketLeft             = (1<<2),
    MD_Node_Flag_HasBracketRight            = (1<<3),
    MD_Node_Flag_HasBraceLeft               = (1<<4),
    MD_Node_Flag_HasBraceRight              = (1<<5),
    
    MD_Node_Flag_MaskSeparators             = (0xF<<6),
    MD_Node_Flag_IsBeforeSemicolon          = (1<<6),
    MD_Node_Flag_IsAfterSemicolon           = (1<<7),
    MD_Node_Flag_IsBeforeComma              = (1<<8),
    MD_Node_Flag_IsAfterComma               = (1<<9),
    
    MD_Node_Flag_MaskStringDelimiters       = (0xF<<10),
    MD_Node_Flag_StringSingleQuote          = (1<<10),
    MD_Node_Flag_StringDoubleQuote          = (1<<11),
    MD_Node_Flag_StringTick                 = (1<<12),
    MD_Node_Flag_StringTriplet              = (1<<13),
    
    MD_Node_Flag_MaskLabelKind              = (0xF<<14),
    MD_Node_Flag_Numeric                    = (1<<14),
    MD_Node_Flag_Identifier                 = (1<<15),
    MD_Node_Flag_StringLiteral              = (1<<16),
    MD_Node_Flag_Symbol                     = (1<<17),
};

typedef struct MD_Node MD_Node;
struct MD_Node
{
    //- ak: tree links
    MD_Node *next;
    MD_Node *prev;
    MD_Node *parent;
    MD_Node *first;
    MD_Node *last;
    
    //- ak: tag links
    MD_Node *first_tag;
    MD_Node *last_tag;
    
    //- ak: node info
    MD_Node_Kind kind;
    MD_Node_Flags flags;
    Str8 string;
    Str8 raw_string;
    
    //- ak: source code info
    size_t src_offset;
    
    //- ak: user-controlled generation number
    size_t user_gen;
    
    //- ak: extra padding to 128 bytes
    size_t _unused_[2];
};

//~ ak: Tokenize Types =============================================================

typedef struct MD_Tokenize MD_Tokenize;
struct MD_Tokenize
{
    MD_Token_Array tokens;
    MD_Msg_List msgs;
};

//~ ak: Parse Types ================================================================

typedef struct MD_Parse MD_Parse;
struct MD_Parse
{
    MD_Node *root;
    MD_Msg_List msgs;
};

//~ ak: Globals
//=============================================================================

PragmaNoWarnMissingFieldInitPush()

global read_only MD_Node md_nil_node = {
    &md_nil_node,
    &md_nil_node,
    &md_nil_node,
    &md_nil_node,
    &md_nil_node,
    &md_nil_node,
    &md_nil_node,
};

PragmaPop()

//~ ak: Functions
//=============================================================================

//~ ak: Message Functions =====================================================

internal void md_msg_list_push(Arena *arena, MD_Msg_List *msgs, MD_Node *node, MD_Msg_Level level, Str8 string);
internal void md_msg_list_pushf(Arena *arena, MD_Msg_List *msgs, MD_Node *node, MD_Msg_Level level, char *fmt, ...);
internal void md_msg_list_concat_in_place(MD_Msg_List *dst, MD_Msg_List *to_push);

//~ ak: Token Functions =======================================================

internal void md_token_chunk_list_push(MD_Token_Chunk_List *list, size_t cap, MD_Token token, Arena *arena);
internal MD_Token_Array md_token_array_from_chunk_list(Arena *arena, MD_Token_Chunk_List *chunks);
internal Str8 md_content_string_from_token_flags_string(MD_Token_Flags flags, Str8 string);

//~ ak: Node Functions ========================================================

//- ak: flag conversions
internal MD_Node_Flags md_node_flags_from_token_flags(MD_Token_Flags flags);

//- ak: nil
internal bool md_node_is_nil(MD_Node *node);

//- ak: tree building
internal MD_Node *md_node_push(MD_Node_Kind kind, MD_Node_Flags flags, Str8 string, Str8 raw_string, size_t src_offset, Arena *arena);
internal void md_node_child_push(MD_Node *parent, MD_Node *node);

// ak: tree introspection
internal MD_Node *md_node_from_chain_string(MD_Node *first, MD_Node *opl, Str8 string, Str_Match_Flags flags);
internal MD_Node *md_node_from_chain_index(MD_Node *first, MD_Node *opl, size_t index);
internal MD_Node *md_tag_from_string(MD_Node *node, Str8 tag_string, Str_Match_Flags flags);
internal MD_Node *md_root_from_node(MD_Node *node);
internal MD_Node *md_child_from_index(MD_Node *node, size_t index);
internal size_t md_child_count_from_node(MD_Node *node);
internal bool md_node_has_tag(MD_Node *node, Str8 string, Str_Match_Flags flags);

//~ ak: Tokenize Functions ====================================================

internal MD_Tokenize md_tokenize_from_string(Str8 string, Arena *arena);

//~ ak: Parse Functions =======================================================

internal MD_Parse md_parse_from_string_tokens(Str8 string, MD_Token_Array tokens, Str8 filename, Arena *arena);
internal MD_Parse md_parse_from_string(Str8 string, Str8 filename, Arena *arena);

#endif // METADESK_H
