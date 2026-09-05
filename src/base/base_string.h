#ifndef BASE_STRING_H
#define BASE_STRING_H

// ak: External Include
//=============================================================================

#define XXH_INLINE_ALL
#define XXH_STATIC_LINKING_ONLY
#include "./external/xxhash.h"

// ak: Types
//=============================================================================

// ak: String Types ===========================================================

typedef struct Str8 Str8;
struct Str8
{
    uint8_t *cstr;
    size_t length;
    size_t size;
};
typedef struct Str16 Str16;
struct Str16
{
    uint16_t *cstr;
    size_t length;
    size_t size;
};
typedef struct Str32 Str32;
struct Str32
{
    uint32_t *cstr;
    size_t length;
    size_t size;
};

// ak: String List & Array Types ==============================================

typedef struct Str8_Node Str8_Node;
struct Str8_Node
{
    Str8_Node *next;
    Str8 str;
};

typedef struct Str8_List Str8_List;
struct Str8_List
{
    Str8_Node *first;
    Str8_Node *last;
    size_t length;
    size_t size;
};

typedef struct Str8_Array Str8_Array;
struct Str8_Array
{
  Str8 *v;
  size_t length;
  size_t size;
};

// ak: String Matching, Splitting, & Joining Types ============================

typedef struct Str_Join Str_Join;
struct Str_Join
{
    Str8 pre;
    Str8 sep;
    Str8 post;
};

typedef enum Str_Split_Flags
{
    Str_Split_Flag_None        = 0,
    Str_Split_Flag_KeepEmpties = (1 << 0),
}
Str_Split_Flags;

typedef enum Str_Match_Flags
{
    Str_Match_Flag_None             = 0,
    Str_Match_Flag_CaseInsensitive  = (1 << 0),
    Str_Match_Flag_RightSideSloppy  = (1 << 1),
    Str_Match_Flag_SlashInsensitive = (1 << 2),
} Str_Match_Flags;

// ak: Character Classification & Conversion Functions ========================

typedef struct Unicode_Decode Unicode_Decode;
struct Unicode_Decode
{
    uint32_t inc;
    uint32_t codepoint;
};

// ak: Functions
//=============================================================================

// ak: Character Classification & Conversion Functions ========================

internal bool    char_is_space(uint8_t c);
internal bool    char_is_upper(uint8_t c);
internal bool    char_is_lower(uint8_t c);
internal bool    char_is_alpha(uint8_t c);
internal bool    char_is_digit(uint8_t c, uint32_t base);
internal bool    char_is_slash(uint8_t c);
internal uint8_t lower_from_char(uint8_t c);
internal uint8_t upper_from_char(uint8_t c);
internal uint8_t correct_slash_from_char(uint8_t c);

// ak: C-String Measurement ===================================================

internal size_t cstr8_length(uint8_t *c);
internal size_t cstr16_length(uint16_t *c);
internal size_t cstr32_length(uint32_t *c);

// ak: String Constructors ====================================================

#define str8(str) (Str8){ (uint8_t*)(str), sizeof(str) - 1, sizeof(str) - 1 }
#define str8_varg(str) (int)((str).length), ((str).cstr)
internal Str8  str8_zero(void);
internal Str8  str8_from_cstr(char *c);
internal Str16 str16_init(uint16_t *str, size_t size);
internal Str16 str16_from_cstr(uint16_t *c);
internal Str32 str32_init(uint32_t *str, size_t size);
internal Str32 str32_from_cstr(uint32_t *c);
internal Str8  str8_range(uint8_t *first, uint8_t *one_past_last, size_t size);

// ak: String Stylization =====================================================

internal Str8 upper_from_str8(Str8 string, Arena *arena);
internal Str8 lower_from_str8(Str8 string, Arena *arena);
internal Str8 backslashed_from_str8(Str8 string, Arena *arena);

// ak: String Matching ========================================================

internal bool   str8_match(Str8 a, Str8 b, Str_Match_Flags flags);
internal bool   str8_ends_with(Str8 str, Str8 end);
internal size_t str8_find_substr(Str8 str, size_t start_pos, Str8 substr, Str_Match_Flags flags);
internal size_t str8_find_substr_reverse(Str8 str, size_t start_pos, Str8 substr, Str_Match_Flags flags);

// ak: String Slicing =========================================================

internal Str8 str8_substr(Str8 str, Rng1_U64 range);
internal Str8 str8_postfix(Str8 str, size_t length);
internal Str8 str8_prefix(Str8 str, size_t length);
internal Str8 str8_skip(Str8 str, size_t amount);
internal Str8 str8_chop(Str8 str, uint64_t amount);
internal Str8 str8_skip_chop_whitespace(Str8 string);
internal Str8 str8_cat(Arena *arena, Str8 s1, Str8 s2);

// ak: String Conversions =====================================================

// ak: string -> integer
internal bool     str8_is_integer(Str8 str, size_t radix);
internal bool     str8_is_integer_unsigned(Str8 str, size_t radix);
internal int64_t  sign_from_str8(Str8 str, Str8 *string_tail);
internal uint64_t u64_from_str8(Str8 str, size_t radix);
internal uint32_t u32_from_str8(Str8 str, size_t radix);
internal int64_t  i64_from_str8(Str8 str, size_t radix);
internal int32_t  i32_from_str8(Str8 str, size_t radix);
internal bool     try_u64_from_str8_c_rules(Str8 string, uint64_t *x);
internal bool     try_s64_from_str8_c_rules(Str8 string, int64_t  *x);

// ak: string -> float
internal bool     str8_is_float(Str8 str);
internal double   f64_from_str8(Str8 str);

// ak: string -> bool
internal bool     str8_is_bool(Str8 str);
internal bool     bool_from_str8(Str8 str);
internal Str8     str8_from_bool(bool value);

// ak: String List Construction Functions =====================================

internal Str8_Node *str8_list_push(Arena *arena, Str8_List *list, Str8 str);
internal Str8_Node *str8_list_pushf(Arena *arena, Str8_List *list, const char *fmt, ...);

// ak: String Arrays Construction Functions ===================================

internal Str8_Array str8_array_from_list(Arena *arena, Str8_List *list);

// ak: String Path Helpers ====================================================

internal Str8 str8_chop_last_slash(Str8 string);
internal Str8 str8_skip_last_slash(Str8 string);
internal Str8 str8_chop_last_dot(Str8 string);
internal Str8 str8_skip_last_dot(Str8 string);
internal Str8_List str8_split_path(Arena *arena, Str8 string);

// ak: String Split and Join ==================================================

internal Str8_List str8_split(Arena *arena, Str8 str, uint8_t *split_chars, size_t split_char_count, Str_Split_Flags flags);
internal Str8 str8_list_join(Arena *arena, Str8_List *list, Str_Join *optional_params);

// ak: String Formatting & Copying ============================================

internal Str8 str8_copy(Arena *arena, Str8 s);
internal Str8 str8fv(Arena *arena, const char *format, va_list args);
internal Str8 str8f(Arena *arena, const char *format, ...);

// ak: UTF-8 & UTF-16 Decoding/Encoding =======================================

internal Unicode_Decode utf8_decode(uint8_t *str, size_t max);
internal Unicode_Decode utf16_decode(uint16_t *str, size_t max);
internal uint32_t utf8_encode(uint8_t *str, uint32_t codepoint);
internal uint32_t utf16_encode(uint16_t *str, uint32_t codepoint);
internal uint32_t utf8_from_utf32_single(uint8_t *buffer, uint32_t character);

// ak: Unicode String Conversions =============================================

internal Str8  str8_from_16(Arena *arena, Str16 in);
internal Str16 str16_from_8(Arena *arena, Str8 in);
internal Str8  str8_from_32(Arena *arena, Str32 in);
internal Str32 str32_from_8(Arena *arena, Str8 in);

// ak: Basic Text Indentation =================================================

internal Str8 indented_from_string(Str8 string, size_t size, Arena *arena);

// ak: Text Escaping ==========================================================

internal Str8 raw_from_escaped_str8(Str8 string, Arena *arena);

// ak: String Hash ============================================================

internal uint64_t u64_hash_from_seed_str8(uint64_t seed, Str8 str);
internal uint64_t u64_hash_from_str8(Str8 str);

// ak: Globals
//=============================================================================

read_only global uint8_t utf8_class[32] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,2,2,2,2,3,3,4,5,
};

#endif // BASE_STRING_H
