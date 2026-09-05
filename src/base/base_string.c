// ak: External Include
//=============================================================================

#if !defined(XXH_IMPLEMENTATION)
#   define XXH_IMPLEMENTATION
#   include "./external/xxhash.h"
#endif

// ak: Character Classification & Conversion Functions
//=============================================================================

internal bool char_is_space(uint8_t c)
{
    return(c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\f' || c == '\v');
}

internal bool char_is_upper(uint8_t c)
{
    return('A' <= c && c <= 'Z');
}
internal bool char_is_lower(uint8_t c)
{
    return('a' <= c && c <= 'z');
}

internal bool char_is_alpha(uint8_t c)
{
    return(char_is_upper(c) || char_is_lower(c));
}
internal bool char_is_digit(uint8_t c, uint32_t base)
{
    bool result = false;
    if (0 < base && base <= 16)
    {
        uint8_t val = integer_symbol_reverse[c];
        if (val < base)
        {
            result = true;
        }
    }
    return result;
}

internal bool char_is_slash(uint8_t c)
{
    return(c == '/' || c == '\\');
}

internal uint8_t lower_from_char(uint8_t c)
{
    if (char_is_upper(c))
    {
        c += ('a' - 'A');
    }
    return(c);
}
internal uint8_t upper_from_char(uint8_t c)
{
    if (char_is_lower(c))
    {
        c += ('A' - 'a');
    }
    return(c);
}

internal uint8_t correct_slash_from_char(uint8_t c)
{
    if (char_is_slash(c))
    {
        c = '/';
    }
    return(c);
}

// ak: C-String Measurement
//=============================================================================

internal size_t cstr8_length(uint8_t *cstr)
{
    uint8_t *p = cstr;
    for (;*p != 0; p += 1);
    return(p - cstr);
}
internal size_t cstr16_length(uint16_t *cstr)
{
    uint16_t *p = cstr;
    for (;*p != 0; p += 1);
    return(p - cstr);
}
internal size_t cstr32_length(uint32_t *cstr)
{
    uint32_t *p = cstr;
    for (;*p != 0; p += 1);
    return(p - cstr);
}

// ak: String Constructors
//=============================================================================


internal Str8 str8_zero(void)
{
    Str8 result = STRUCT_ZERO;
    return result;
}
internal Str8 str8_init(uint8_t *cstr, size_t size)
{
    return (Str8){cstr, size, size};
}
internal Str8 str8_from_cstr(char *cstr)
{
    size_t len = cstr8_length((uint8_t*)cstr);
    return (Str8){(uint8_t*)cstr, len, len};
}

internal Str16 str16_init(uint16_t *cstr, size_t size)
{
    return (Str16){cstr, size, size};
}
internal Str16 str16_from_cstr(uint16_t *cstr)
{
    size_t len = cstr16_length((uint16_t*)cstr);
    return (Str16){(uint16_t*)cstr, len, len};
}

internal Str32 str32_init(uint32_t *cstr, size_t size)
{
    return (Str32){cstr, size, size};
}
internal Str32 str32_from_cstr(uint32_t *cstr)
{
    size_t len = cstr32_length((uint32_t*)cstr);
    return (Str32){(uint32_t*)cstr, len, len};
}

internal Str8 str8_range(uint8_t *first, uint8_t *one_past_last, size_t size)
{
    return (Str8){first, (size_t)(one_past_last - first), size};
}

// ak: String Stylization
//=============================================================================

internal Str8 upper_from_str8(Str8 string, Arena *arena)
{
    string = str8_copy(arena, string);
    for (size_t index = 0; index < string.length; index += 1)
    {
        string.cstr[index] = upper_from_char(string.cstr[index]);
    }
    return string;
}

internal Str8 lower_from_str8(Str8 string, Arena *arena)
{
    string = str8_copy(arena, string);
    for (size_t index = 0; index < string.length; index += 1)
    {
        string.cstr[index] = lower_from_char(string.cstr[index]);
    }
    return string;
}

internal Str8 backslashed_from_str8(Str8 string, Arena *arena)
{
    string = str8_copy(arena, string);
    for (size_t index = 0; index < string.length; index += 1)
    {
        string.cstr[index] = char_is_slash(string.cstr[index]) ? '\\' : string.cstr[index];
    }
    return string;
}

// ak: String Matching
//=============================================================================

internal bool str8_match(Str8 a, Str8 b, Str_Match_Flags flags)
{
    bool result = false;
    if (a.length == b.length && flags == Str_Match_Flag_None)
    {
        result = MemMatch(a.cstr, b.cstr, b.length);
    }
    else if (a.length == b.length || (flags & Str_Match_Flag_RightSideSloppy))
    {
        bool case_insensitive  = (flags & Str_Match_Flag_CaseInsensitive);
        bool slash_insensitive = (flags & Str_Match_Flag_SlashInsensitive);
        size_t length               = Min(a.length, b.length);
        result = 1;
        for (size_t i = 0; i < length; i += 1)
        {
            uint8_t at = a.cstr[i];
            uint8_t bt = b.cstr[i];
            if (case_insensitive)
            {
                at = upper_from_char(at);
                bt = upper_from_char(bt);
            }
            if (slash_insensitive)
            {
                at = correct_slash_from_char(at);
                bt = correct_slash_from_char(bt);
            }
            if (at != bt)
            {
                result = false;
                break;
            }
        }
    }
    return result;
}

internal bool str8_ends_with(Str8 str, Str8 end)
{
    Str8 postfix = str8_postfix(str, end.length);
    bool is_match = str8_match(end, postfix, Str_Match_Flag_None);
    return is_match;
}

internal size_t str8_find_substr(Str8 str, size_t start_pos, Str8 substr, Str_Match_Flags flags)
{
    uint8_t *p = str.cstr + start_pos;
    size_t stop_offset = Max(str.length + 1, substr.length) - substr.length;
    uint8_t *stop_p = str.cstr + stop_offset;
    if (substr.length > 0)
    {
        uint8_t *string_opl = str.cstr + str.length;
        Str8 needle_tail = str8_skip(substr, 1);
        Str_Match_Flags adjusted_flags = (Str_Match_Flags)(flags | Str_Match_Flag_RightSideSloppy);
        uint8_t needle_first_char_adjusted = substr.cstr[0];
        if (adjusted_flags & Str_Match_Flag_CaseInsensitive)
        {
            needle_first_char_adjusted = upper_from_char(needle_first_char_adjusted);
        }
        for (;p < stop_p; p += 1)
        {
            uint8_t haystack_char_adjusted = *p;
            if (adjusted_flags & Str_Match_Flag_CaseInsensitive)
            {
                haystack_char_adjusted = upper_from_char(haystack_char_adjusted);
            }
            if (haystack_char_adjusted == needle_first_char_adjusted)
            {
                if (str8_match(str8_range(p+1, string_opl, str.length), needle_tail, adjusted_flags))
                {
                    break;
                }
            }
        }
    }
    size_t result = str.length;
    if (p < stop_p)
    {
        result = (size_t)(p - str.cstr);
    }
    return result;
}

internal size_t str8_find_substr_reverse(Str8 str, size_t start_pos, Str8 substr, Str_Match_Flags flags)
{
    size_t result = 0;
    for (int64_t i = str.length - start_pos - substr.length; i >= 0; --i)
    {
        Str8 haystack = str8_substr(str, (Rng1_U64){(uint64_t)i, (uint64_t)(i + substr.length)});
        if (str8_match(haystack, substr, flags))
        {
            result = (size_t)i + substr.length;
            break;
        }
    }
    return result;
}

// ak: String Slicing
//=============================================================================

internal Str8 str8_substr(Str8 str, Rng1_U64 range)
{
    range.min = Min(range.min, str.length);
    range.max = Min(range.max, str.length);
    str.cstr += range.min;
    str.length = dim_rng1(range);
    return(str);
}

internal Str8 str8_postfix(Str8 str, size_t length)
{
    length = Min(length, str.length);
    str.cstr = (str.cstr + str.length) - length;
    str.length = length;
    return(str);
}

internal Str8 str8_prefix(Str8 str, size_t length)
{
    str.length = Min(length, str.length);
    return str;
}

internal Str8 str8_skip(Str8 str, size_t amount)
{
    amount = Min(amount, str.length);
    str.cstr += amount;
    str.length -= amount;
    str.size = str.length;
    return str;
}

internal Str8 str8_chop(Str8 str, size_t amount)
{
    amount = Min(amount, str.length);
    str.length -= amount;
    return str;
}

internal Str8 str8_skip_chop_whitespace(Str8 string)
{
    uint8_t *first = string.cstr;
    uint8_t *opl = first + string.length;
    for (;first < opl; first += 1)
    {
        if (!char_is_space(*first))
        {
            break;
        }
    }
    while (opl > first)
    {
        opl -= 1;
        if (!char_is_space(*opl))
        {
            opl += 1;
            break;
        }
    }
    Str8 result = str8_range(first, opl, string.length);
    return result;
}

internal Str8 str8_cat(Arena *arena, Str8 s1, Str8 s2)
{
    Str8 str;
    str.length = s1.length + s2.length;
    str.cstr = arena_push(arena, uint8_t, str.length + 1);
    mem_copy(str.cstr, s1.cstr, s1.length);
    mem_copy(str.cstr + s1.length, s2.cstr, s2.length);
    str.cstr[str.length] = 0;
    return(str);
}



// ak: String Conversions
//=============================================================================

// ak: string -> integer

internal bool str8_is_integer(Str8 str, size_t radix)
{
    bool result = false;
    Str8 sign = str8_prefix(str, 1);
    if (str8_match(sign, str8("-"), Str_Match_Flag_None))
    {
        result = str8_is_integer_unsigned(str8_skip(str, 1), radix);
    }
    else
    {
        result = str8_is_integer_unsigned(str, radix);
    }
    return result;
}


internal bool str8_is_integer_unsigned(Str8 str, size_t radix)
{
    bool result = false;
    if (str.length > 0)
    {
        if (1 < radix && radix <= 16)
        {
            result = true;
            for (size_t i = 0; i < str.length; i += 1)
            {
                uint8_t c = str.cstr[i];
                if (!(c < 0x80) || integer_symbol_reverse[c] >= radix)
                {
                    result = false;
                    break;
                }
            }
        }
    }
    return result;
}

internal int64_t sign_from_str8(Str8 str, Str8 *string_tail)
{
    // ak: count negative signs
    size_t neg_count = 0;
    size_t i = 0;
    for (; i < str.length; i += 1)
    {
        if (str.cstr[i] == '-'){
            neg_count += 1;
        }
        else if (str.cstr[i] != '+'){
            break;
        }
    }
    // ak: output part of string after signs
    *string_tail = str8_skip(str, i);
    // ak: output integer sign
    int64_t sign = (neg_count & 1)?-1:+1;
    return sign;
}

internal uint64_t u64_from_str8(Str8 str, size_t radix)
{
    uint64_t x = 0;
    if (1 < radix && radix <= 16)
    {
        for (uint64_t i = 0; i < str.length; i += 1)
        {
            x *= radix;
            x += integer_symbol_reverse[str.cstr[i]&0x7F];
        }
    }
    return x;
}

internal uint32_t u32_from_str8(Str8 str, size_t radix)
{
    uint64_t x64 = u64_from_str8(str, radix);
    uint32_t x32 = safe_cast_u32(x64);
    return x32;
}

internal int64_t i64_from_str8(Str8 str, size_t radix)
{
    int64_t sign = sign_from_str8(str, &str);
    int64_t x = (int64_t)u64_from_str8(str, radix) * sign;
    return x;
}

internal int32_t i32_from_str8(Str8 str, size_t radix)
{
    int64_t x64 = i64_from_str8(str, radix);
    int32_t x32 = safe_cast_s32(x64);
    return x32;
}

internal bool try_u64_from_str8_c_rules(Str8 string, uint64_t *x)
{
    // ak: unpack radix / prefix size based on string prefix
    size_t radix = 0;
    uint64_t prefix_size = 0;
    {
        // hex
        if (str8_match(str8_prefix(string, 2), str8("0x"), Str_Match_Flag_CaseInsensitive))
        {
            radix = 0x10, prefix_size = 2;
        }
        // binary
        else if (str8_match(str8_prefix(string, 2), str8("0b"), Str_Match_Flag_CaseInsensitive))
        {
            radix = 2, prefix_size = 2;
        }
        // octal
        else if (str8_match(str8_prefix(string, 1), str8("0"), Str_Match_Flag_CaseInsensitive) && string.length > 1)
        {
            radix = 010, prefix_size = 1;
        }
        // decimal
        else
        {
            radix = 10, prefix_size = 0;
        }
    }
    // ak: convert if we can
    Str8 integer    = str8_skip(string, prefix_size);
    bool is_integer = str8_is_integer(integer, radix);
    if (is_integer)
    {
        *x = u64_from_str8(integer, radix);
    }
    return is_integer;
}

internal bool try_s64_from_str8_c_rules(Str8 string, int64_t  *x)
{
    Str8 string_tail    = STRUCT_ZERO;
    int64_t  sign       = sign_from_str8(string, &string_tail);
    uint64_t x_u64      = 0;
    bool     is_integer = try_u64_from_str8_c_rules(string_tail, &x_u64);
    *x = x_u64*sign;
    return is_integer;
}

// ak: string -> float

internal bool str8_is_float(Str8 str)
{
    if (str.length == 0)
    {
        return false;
    }
    size_t i = 0;
    if (i < str.length && (str.cstr[i] == '+' || str.cstr[i] == '-'))
    {
        i += 1;
    }
    bool has_digits = false;
    bool has_dot = false;
    bool has_exp = false;
    while (i < str.length)
    {
        uint8_t c = str.cstr[i];
        if (c >= '0' && c <= '9')
        {
            has_digits = true;
        }
        else if (c == '.' && !has_dot && !has_exp)
        {
            has_dot = true;
        }
        else if ((c == 'e' || c == 'E') && !has_exp && has_digits)
        {
            // ak: Exponent allowed only after at least one digit
            has_exp = true;
            i += 1;
            if (i >= str.length)
            {
                return false; // ak: 'e' at end is invalid
            }
            // ak: Optional exponent sign
            if (str.cstr[i] == '+' || str.cstr[i] == '-')
            {
                i += 1;
            }
            // ak: Must have at least one digit in exponent
            if (i >= str.length || str.cstr[i] < '0' || str.cstr[i] > '9')
            {
                return false;
            }
            // ak: Skip remaining exponent digits
            while (i < str.length && str.cstr[i] >= '0' && str.cstr[i] <= '9')
            {
                i += 1;
            }
            return i == str.length; // ak: Must consume all after 'e'
        }
        else
        {
            return false;
        }
        i += 1;
    }
    // ak: Valid if we saw at least one digit (somewhere before or after dot)
    // Examples: ".5" → valid, "5." → valid, "." → invalid
    return has_digits;
}

internal double f64_from_str8(Str8 str)
{
    double result = 0;
    if (str.length > 0)
    {
        // ak: Find starting pos of numeric string, as well as sign
        double sign = +1.0;
        if (str.cstr[0] == '-')
        {
            sign = -1.0;
        }
        else if (str.cstr[0] == '+')
        {
            sign = 1.0;
        }
        // ak: Gather numerics
        size_t num_valid_chars = 0;
        char buffer[64];
        bool exp = 0;
        for (size_t index = 0; index < str.length && num_valid_chars < sizeof(buffer)-1; index += 1)
        {
            if (char_is_digit(str.cstr[index], 10) || str.cstr[index] == '.' || str.cstr[index] == 'e' ||
                    (exp && (str.cstr[index] == '+' || str.cstr[index] == '-')))
            {
                buffer[num_valid_chars] = str.cstr[index];
                num_valid_chars += 1;
                exp = 0;
                exp = (str.cstr[index] == 'e');
            }
        }
        // ak: Null-terminate.
        buffer[num_valid_chars] = 0;
        // ak: Do final conversion
        result = sign * atof(buffer);
    }
    return result;
}

// ak: string -> bool

internal bool str8_is_bool(Str8 str)
{
    bool result = str8_match(str, str8("true"), Str_Match_Flag_None) || str8_match(str, str8("false"), Str_Match_Flag_None);
    return result;
}
internal bool bool_from_str8(Str8 str)
{
    bool result = str8_match(str, str8("true"), Str_Match_Flag_None) ? true : false;
    return result;
}

internal Str8 str8_from_bool(bool value)
{
    Str8 result = value ? str8("true") : str8("false");
    return result;
}

// ak: String List Construction Functions
//=============================================================================

internal Str8_Node* str8_list_push(Arena *arena, Str8_List *list, Str8 str)
{
    Str8_Node *node = arena_push(arena, Str8_Node, 1);
    SLLQueuePush(list->first, list->last, node);
    list->length += 1;
    list->size += str.length;
    node->str = str;
    return node;
}

internal Str8_Node* str8_list_pushf(Arena *arena, Str8_List *list, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  Str8 string = str8fv(arena, fmt, args);
  Str8_Node *result = str8_list_push(arena, list, string);
  va_end(args);
  return result;
}

// ak: String Arrays Construction Functions
//=============================================================================

internal Str8_Array str8_array_from_list(Arena *arena, Str8_List *list)
{
    Str8_Array array;
    array.size = list->length;
    array.length = array.size;
    array.v = arena_push(arena, Str8, array.size);
    size_t index = 0;
    for (Str8_Node *n = list->first; n != 0; n = n->next, index += 1)
    {
        array.v[index] = n->str;
    }
    return array;
}

// ak: String Path Helpers
//=============================================================================

internal Str8 str8_chop_last_slash(Str8 string)
{
    if (string.length > 0)
    {
        uint8_t *ptr = string.cstr + string.length - 1;
        for (;ptr >= string.cstr; ptr -= 1)
        {
            if (*ptr == '/' || *ptr == '\\')
            {
                break;
            }
        }
        if (ptr >= string.cstr)
        {
            string.length = (size_t)(ptr - string.cstr);
        }
        else
        {
            string.length = 0;
        }
    }
    return string;
}

internal Str8 str8_skip_last_slash(Str8 string)
{
    if (string.length > 0)
    {
        uint8_t *ptr = string.cstr + string.length - 1;
        for (;ptr >= string.cstr; ptr -= 1)
        {
            if (*ptr == '/' || *ptr == '\\')
            {
                break;
            }
        }
        if (ptr >= string.cstr)
        {
            ptr += 1;
            string.length = (size_t)(string.cstr + string.length - ptr);
            string.cstr = ptr;
        }
    }
    return string;
}

internal Str8 str8_chop_last_dot(Str8 string)
{
    Str8 result = string;
    size_t p = string.length;
    for (;p > 0;)
    {
        p -= 1;
        if (string.cstr[p] == '.')
        {
            result = str8_prefix(string, p);
            break;
        }
    }
    return result;
}

internal Str8 str8_skip_last_dot(Str8 string)
{
    Str8 result = string;
    size_t p = string.length;
    for (;p > 0;)
    {
        p -= 1;
        if (string.cstr[p] == '.')
        {
            result = str8_skip(string, p + 1);
            break;
        }
    }
    return result;
}

internal Str8_List str8_split_path(Arena *arena, Str8 string)
{
    Str8_List result = str8_split(arena, string, (uint8_t*)"/\\", 2, Str_Split_Flag_None);
    return result;
}


// ak: String Split and Join
//=============================================================================

internal Str8_List str8_split(Arena *arena, Str8 str, uint8_t *split_chars, size_t split_char_count, Str_Split_Flags flags)
{
    Str8_List list = STRUCT_ZERO;
    bool keep_empties = (flags & Str_Split_Flag_KeepEmpties);
    uint8_t *ptr = str.cstr;
    uint8_t *opl = str.cstr + str.length;
    for (;ptr < opl;)
    {
        uint8_t *first = ptr;
        for (;ptr < opl; ptr += 1)
        {
            uint8_t c = *ptr;
            bool is_split = false;
            for (size_t i = 0; i < split_char_count; i += 1)
            {
                if (split_chars[i] == c)
                {
                    is_split = true;
                    break;
                }
            }
            if (is_split)
            {
                break;
            }
        }
        Str8 node = str8_range(first, ptr, str.length);
        if (keep_empties || node.length > 0)
        {
            str8_list_push(arena, &list, node);
        }
        ptr += 1;
    }
  return list;
}

internal Str8 str8_list_join(Arena *arena, Str8_List *list, Str_Join *optional_params)
{
    Str_Join join = STRUCT_ZERO;
    if (optional_params != 0)
    {
        MemCopyStruct(&join, optional_params);
    }
    size_t sep_count = 0;
    if (list->length > 0)
    {
        sep_count = list->length - 1;
    }
    Str8 result;
    result.length = join.pre.length + join.post.length + sep_count*join.sep.length + list->size;
    result.size = result.length;
    uint8_t *ptr = result.cstr = arena_push(arena, uint8_t, result.length + 1);
    mem_copy(ptr, join.pre.cstr, join.pre.length);
    ptr += join.pre.length;
    for (Str8_Node *node = list->first; node != 0; node = node->next)
    {
        mem_copy(ptr, node->str.cstr, node->str.length);
        ptr += node->str.length;
        if (node->next != 0)
        {
            mem_copy(ptr, join.sep.cstr, join.sep.length);
            ptr += join.sep.length;
        }
    }
    mem_copy(ptr, join.post.cstr, join.post.length);
    ptr += join.post.length;
    *ptr = 0;
    return result;
}

// ak: String Formatting & Copying
//=============================================================================

internal Str8 str8_copy(Arena *arena, Str8 s)
{
    Str8 str;
    str.length = s.length;
    str.size = s.size;
    str.cstr = arena_push_nz(arena, uint8_t, str.length + 1);
    mem_copy(str.cstr, s.cstr, s.length);
    str.cstr[str.length] = 0;
    return(str);
}

internal Str8 str8fv(Arena *arena, const char *format, va_list args)
{
    Str8 result = STRUCT_ZERO;
    va_list args_copy;
    va_copy(args_copy, args);
        uint32_t needed = fmt_vsnprintf(0, 0, format, args) + 1;
        result.cstr = arena_push_nz(arena, uint8_t, needed);
        result.length = fmt_vsnprintf((char*)result.cstr, needed, format, args_copy);
        result.size = result.length;
        result.cstr[result.length] = 0;
    va_end(args_copy);
    return result;
}

internal Str8 str8f(Arena *arena, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    Str8 result = str8fv(arena, format, args);
    va_end(args);
    return result;
}

// ak: UTF-8 & UTF-16 Decoding/Encoding
//=============================================================================

internal Unicode_Decode utf8_decode(uint8_t *str, size_t max)
{
    Unicode_Decode result = {1, max_u32};
    uint8_t byte = str[0];
    uint8_t byte_class = utf8_class[byte >> 3];
    switch (byte_class)
    {
        case 1:{
            result.codepoint = byte;
        }break;
        case 2: {
            if (1 < max)
            {
                uint8_t cont_byte = str[1];
                if (utf8_class[cont_byte >> 3] == 0)
                {
                    result.codepoint = (byte & bitmask5) << 6;
                    result.codepoint |=  (cont_byte & bitmask6);
                    result.inc = 2;
                }
            }
        }break;
        case 3:
        {
            if (2 < max)
            {
                uint8_t cont_byte[2] = {str[1], str[2]};
                if (
                    utf8_class[cont_byte[0] >> 3] == 0 &&
                    utf8_class[cont_byte[1] >> 3] == 0
                )
                {
                    result.codepoint = (byte & bitmask4) << 12;
                    result.codepoint |= ((cont_byte[0] & bitmask6) << 6);
                    result.codepoint |=  (cont_byte[1] & bitmask6);
                    result.inc = 3;
                }
            }
        }break;
        case 4:
        {
            if (3 < max)
            {
                uint8_t cont_byte[3] = {str[1], str[2], str[3]};
                if (
                    utf8_class[cont_byte[0] >> 3] == 0 &&
                    utf8_class[cont_byte[1] >> 3] == 0 &&
                    utf8_class[cont_byte[2] >> 3] == 0
                )
                {
                    result.codepoint = (byte & bitmask3) << 18;
                    result.codepoint |= ((cont_byte[0] & bitmask6) << 12);
                    result.codepoint |= ((cont_byte[1] & bitmask6) <<  6);
                    result.codepoint |=  (cont_byte[2] & bitmask6);
                    result.inc = 4;
                }
            }
        }
    }
    return result;
}

internal Unicode_Decode utf16_decode(uint16_t *str, size_t max)
{
    Unicode_Decode result = {1, max_u32};
    result.codepoint = str[0];
    result.inc = 1;
    if (max > 1 && 0xD800 <= str[0] && str[0] < 0xDC00 && 0xDC00 <= str[1] && str[1] < 0xE000){
        result.codepoint = ((str[0] - 0xD800) << 10) | ((str[1] - 0xDC00) + 0x10000);
        result.inc = 2;
    }
    return result;
}

internal uint32_t utf8_encode(uint8_t *str, uint32_t codepoint)
{
    uint32_t inc = 0;
    if (codepoint <= 0x7F){
        str[0] = (uint8_t)codepoint;
        inc = 1;
    }
    else if (codepoint <= 0x7FF){
        str[0] = (uint8_t)((bitmask2 << 6)  | ((codepoint >> 6) & bitmask5));
        str[1] = (uint8_t)( bit8            | ( codepoint       & bitmask6));
        inc = 2;
    }
    else if (codepoint <= 0xFFFF){
        str[0] = (uint8_t)((bitmask3 << 5)  | ((codepoint >> 12) & bitmask4));
        str[1] = (uint8_t)( bit8            | ((codepoint >> 6)  & bitmask6));
        str[2] = (uint8_t)( bit8            | ( codepoint        & bitmask6));
        inc = 3;
    }
    else if (codepoint <= 0x10FFFF){
        str[0] = (uint8_t)((bitmask4 << 4)  | ((codepoint >> 18) & bitmask3));
        str[1] = (uint8_t)( bit8            | ((codepoint >> 12) & bitmask6));
        str[2] = (uint8_t)( bit8            | ((codepoint >>  6) & bitmask6));
        str[3] = (uint8_t)( bit8            | ( codepoint        & bitmask6));
        inc = 4;
    }
    else{
        str[0] = '?';
        inc = 1;
    }
    return(inc);
}

internal uint32_t utf16_encode(uint16_t *str, uint32_t codepoint)
{
    uint32_t inc = 1;
    if (codepoint == max_u32){
        str[0] = (uint16_t)'?';
    }
    else if (codepoint < 0x10000){
        str[0] = (uint16_t)codepoint;
    }
    else{
        uint32_t v = codepoint - 0x10000;
        str[0] = safe_cast_u16(0xD800 + (v >> 10));
        str[1] = safe_cast_u16(0xDC00 + (v & bitmask10));
        inc = 2;
    }
    return(inc);
}

internal uint32_t utf8_from_utf32_single(uint8_t *buffer, uint32_t character)
{
    return(utf8_encode(buffer, character));
}

// ak: Unicode String Conversions
//=============================================================================

internal Str8 str8_from_16(Arena *arena, Str16 in)
{
    Str8 result = STRUCT_ZERO;
    if (in.length)
    {
        size_t cap = in.length*3;
        uint8_t *str = arena_push(arena, uint8_t, cap + 1);
        uint16_t *ptr = in.cstr;
        uint16_t *opl = ptr + in.length;
        size_t length = 0;
        Unicode_Decode consume;
        for (;ptr < opl; ptr += consume.inc)
        {
            consume = utf16_decode(ptr, opl - ptr);
            length += utf8_encode(str + length, consume.codepoint);
        }
        str[length] = 0;
        arena_pop(arena, (cap - length));
        result = str8_init(str, length);
    }
    return result;
}

internal Str16 str16_from_8(Arena *arena, Str8 in)
{
    Str16 result = STRUCT_ZERO;
    if (in.length)
    {
        size_t cap = in.length*2;
        uint16_t *str = arena_push(arena, uint16_t, cap + 1);
        uint8_t *ptr = in.cstr;
        uint8_t *opl = ptr + in.length;
        size_t length = 0;
        Unicode_Decode consume;
        for (;ptr < opl; ptr += consume.inc)
        {
            consume = utf8_decode(ptr, opl - ptr);
            length += utf16_encode(str + length, consume.codepoint);
        }
        str[length] = 0;
        arena_pop(arena, (cap - length)*2);
        result = str16_init(str, length);
    }
    return result;
}

internal Str8 str8_from_32(Arena *arena, Str32 in)
{
    Str8 result = STRUCT_ZERO;
    if (in.length)
    {
        size_t cap = in.length*4;
        uint8_t *str = arena_push(arena, uint8_t, cap + 1);
        uint32_t *ptr = in.cstr;
        uint32_t *opl = ptr + in.length;
        size_t length = 0;
        for (;ptr < opl; ptr += 1)
        {
            length += utf8_encode(str + length, *ptr);
        }
        str[length] = 0;
        arena_pop(arena, (cap - length));
        result = str8_init(str, length);
    }
    return result;
}

internal Str32 str32_from_8(Arena *arena, Str8 in)
{
    Str32 result = STRUCT_ZERO;
    if (in.length)
    {
        size_t cap = in.length;
        uint32_t *str = arena_push(arena, uint32_t, cap + 1);
        uint8_t *ptr = in.cstr;
        uint8_t *opl = ptr + in.length;
        size_t length = 0;
        Unicode_Decode consume;
        for (;ptr < opl; ptr += consume.inc)
        {
            consume = utf8_decode(ptr, opl - ptr);
            str[length] = consume.codepoint;
            length += 1;
        }
        str[length] = 0;
        arena_pop(arena, (cap - length)*4);
        result = str32_init(str, length);
    }
    return result;
}

// ak: Basic Text Indentation
//=============================================================================

internal Str8 indented_from_string(Str8 string, size_t size, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    read_only local_persist uint8_t indentation_bytes[] = "                                                                                                                                ";
    Str8_List indented_strings = STRUCT_ZERO;
    int64_t depth = 0;
    int64_t next_depth = 0;
    size_t line_begin_off = 0;
    for (size_t off = 0; off <= string.length; off += 1)
    {
        uint8_t byte = off<string.length ? string.cstr[off] : 0;
        switch(byte)
        {
            default:{}break;
            case '{':case '[':case '(':
            {
                next_depth += 1; next_depth = Max(0, next_depth);
            } break;
            case '}':case ']':case ')':
            {
                next_depth -= 1; next_depth = Max(0, next_depth); depth = next_depth;
            } break;
            case '\n':
            case 0:
            {
                Str8 line = str8_skip_chop_whitespace(str8_substr(string, (Rng1_U64){line_begin_off, off}));
                if (line.length != 0)
                {
                    str8_list_pushf(scratch.arena, &indented_strings, "%.*s%.*s\n", (int)(depth*size), indentation_bytes, str8_varg(line));
                }
                if (line.length == 0 && indented_strings.length != 0 && off < string.length)
                {
                    str8_list_pushf(scratch.arena, &indented_strings, "\n");
                }
                line_begin_off = off+1;
                depth = next_depth;
            } break;
        }
    }
    Str8 result = str8_list_join(arena, &indented_strings, 0);
    arena_scratch_end(scratch);
    return result;
}

// ak: Text Escaping
//=============================================================================

internal Str8 raw_from_escaped_str8(Str8 string, Arena *arena)
{
    Arena_Temp scratch = arena_scratch_begin(&arena, 1);
    Str8_List strs = STRUCT_ZERO;
    size_t start = 0;
    for (size_t index = 0; index <= string.length; index += 1)
    {
        if (index == string.length || string.cstr[index] == '\\' || string.cstr[index] == '\r')
        {
            Str8 str = str8_substr(string, (Rng1_U64){start, index});
            if (str.length != 0)
            {
                str8_list_push(scratch.arena, &strs, str);
            }
            start = index+1;
        }
        if (index < string.length && string.cstr[index] == '\\')
        {
            uint8_t next_char = string.cstr[index+1];
            uint8_t replace_byte = 0;
            switch(next_char)
            {
                default:{}break;
                case 'a': replace_byte = 0x07; break;
                case 'b': replace_byte = 0x08; break;
                case 'e': replace_byte = 0x1b; break;
                case 'f': replace_byte = 0x0c; break;
                case 'n': replace_byte = 0x0a; break;
                case 'r': replace_byte = 0x0d; break;
                case 't': replace_byte = 0x09; break;
                case 'v': replace_byte = 0x0b; break;
                case '\\':replace_byte = '\\'; break;
                case '\'':replace_byte = '\''; break;
                case '"': replace_byte = '"';  break;
                case '?': replace_byte = '?';  break;
            }
            Str8 replace_string = str8_copy(scratch.arena, str8_init(&replace_byte, 1));
            str8_list_push(scratch.arena, &strs, replace_string);
            index += 1;
            start += 1;
        }
    }
    Str8 result = str8_list_join(arena, &strs, 0);
    arena_scratch_end(scratch);
    return result;
}

// ak: String Hash
//=============================================================================

internal uint64_t u64_hash_from_seed_str8(uint64_t seed, Str8 str)
{
    uint64_t result = XXH3_64bits_withSeed(str.cstr, str.length, seed);
    return result;
}

internal uint64_t u64_hash_from_str8(Str8 str)
{
    uint64_t result = u64_hash_from_seed_str8(5381, str);
    return result;
}

internal Txt_Pt str8_offset_to_txt_pt(Str8 string, size_t offset)
{
    Txt_Pt pt = (Txt_Pt){ 1, 1 };
    for (size_t index = 0; index < string.length && index < offset; index += 1)
    {
        if (string.cstr[index] == '\n')
        {
            pt.line += 1;
            pt.column = 1;
        }
        else
        {
            pt.column += 1;
        }
    }
    return pt;
}
