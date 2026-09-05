#ifndef BASE_FLAGS_H
#define BASE_FLAGS_H

// ak: Types
//=============================================================================

typedef enum _Flags_Error_Kind
{
    _Flags_Error_Kind_MissingValue,
    _Flags_Error_Kind_UnknownOption,
    _Flags_Error_Kind_NoFlagPrefix,
    _Flags_Error_Kind_DuplicateOption,
    _Flags_Error_Kind_SingleValue,
    _Flags_Error_Kind_InvalidBool,
    _Flags_Error_Kind_OutIndexArg,
    // ak: Option Enums
    _Flags_Error_Kind_RequireOption,
    _Flags_Error_Kind_InvalidIntOption,
    _Flags_Error_Kind_UIntMinusOption,
    _Flags_Error_Kind_InvalidFloatOption,
    // ak: Arg Enums
    _Flags_Error_Kind_RequireArg,
    _Flags_Error_Kind_InvalidIntArg,
    _Flags_Error_Kind_UIntMinusArg,
    _Flags_Error_Kind_InvalidFloatArg,
} _Flags_Error_Kind;

typedef enum _Flags_Option_Kind
{
    _Flags_Option_Kind_Str,
    _Flags_Option_Kind_Int,
    _Flags_Option_Kind_UInt,
    _Flags_Option_Kind_Float,
    _Flags_Option_Kind_Bool,
    _Flags_Option_Kind_StrArr,
    _Flags_Option_Kind_IntArr,
    _Flags_Option_Kind_UIntArr,
    _Flags_Option_Kind_FloatArr,
} _Flags_Option_Kind;

typedef enum _Flags_Arg_Kind
{
    _Flags_Arg_Kind_Str,
    _Flags_Arg_Kind_Int,
    _Flags_Arg_Kind_UInt,
    _Flags_Arg_Kind_Float,
} _Flags_Arg_Kind;

typedef struct _Flags_Error _Flags_Error;
struct _Flags_Error
{
    _Flags_Error *next;
    _Flags_Error_Kind kind;
    Str8 flag_name;
    Str8 value;
    size_t arg_index;
};

typedef struct Flags_Option Flags_Option;
struct Flags_Option
{
    Flags_Option *next;
    Str8 name;
    Str8 shortname;
    Str8 description;
    Str8 value_hint;
    union
    {
        Str8 str_value;
        bool bool_value;
        int64_t int_value;
        uint64_t uint_value;
        double float_value;
        Str8_Array *str_value_arr;
        I64Array *int_value_arr;
        U64Array *uint_value_arr;
        F64Array *float_value_arr;
    } default_value;
    union
    {
        Str8 *str_value;
        bool *bool_value;
        int64_t *int_value;
        uint64_t *uint_value;
        double *float_value;
        Str8_Array *str_value_arr;
        I64Array *int_value_arr;
        U64Array *uint_value_arr;
        F64Array *float_value_arr;
    } result_value;
    _Flags_Option_Kind kind;
    bool assigned;
    bool required;
};

typedef struct Flags_Arg Flags_Arg;
struct Flags_Arg
{
    Flags_Arg *next;
    _Flags_Arg_Kind kind;
    size_t index;
    union
    {
        Str8 str_value;
        bool bool_value;
        int64_t int_value;
        uint64_t uint_value;
        double float_value;
    } default_value;
    union
    {
        Str8 *str_value;
        bool *bool_value;
        int64_t *int_value;
        uint64_t *uint_value;
        double *float_value;
    } result_value;
    bool assigned;
    bool required;
};

typedef struct _Flags_State _Flags_State;
struct _Flags_State
{
    Arena *arena;
    Flags_Option *first_option;
    Flags_Option *last_option;
    _Flags_Error *first_error;
    _Flags_Error *last_error;
    Flags_Arg *first_arg;
    Flags_Arg *last_arg;
    size_t index_arg;
    bool has_error;
    bool has_program_name;
    Log_Context log_context;
};

// ak: Functions
//=============================================================================

// ak: Private functions ======================================================

internal Flags_Option *_flags_get_options(Str8 name);
internal void _flags_add_option(Flags_Option *option);
internal Flags_Arg *_flags_get_arg(size_t index);
internal void _flags_add_arg(Flags_Arg *arg);
internal void _flags_add_option_error(_Flags_Error_Kind kind, Str8 name);
internal void _flags_add_option_error_value(_Flags_Error_Kind kind, Str8 name, Str8 value);
internal void _flags_add_error_arg(_Flags_Error_Kind kind, size_t index, Str8 value);
internal bool _flags_is_arg_option(Str8 arg);
internal Str8 _flags_get_options_from_arg(Str8 arg);
internal uint64_t _flags_get_values_count(Str8_Array *args, uint64_t index);

// ak: Flags core functions ===================================================

internal void flags_begin(void);
internal void flags_end(void);
#define FlagsScope(arena) DeferLoop(flags_begin(arena), flags_end())
internal bool flags_parse(Str8_Array *args);
internal void flags_print_error(void);
internal void flags_print_help(void);

// ak: Flags config functions =================================================

internal void flags_has_program_name(bool has_name);
internal void flags_add_option_shortname(Flags_Option *option, Str8 shortname);
internal void flags_add_option_value_hint(Flags_Option *option, Str8 value_hint);
internal void flags_make_option_required(Flags_Option *option);
internal void flags_make_arg_required(Flags_Arg *arg);

// ak: Add option flag ========================================================

internal Flags_Option *flags_option_str(Str8 name, Str8 *result_value, Str8 default_value, Str8 description);
internal Flags_Option *flags_option_int(Str8 name, int64_t *result_value, int64_t default_value, Str8 description);
internal Flags_Option *flags_option_uint(Str8 name, uint64_t *result_value, uint64_t default_value, Str8 description);
internal Flags_Option *flags_option_float(Str8 name, double *result_value, double default_value, Str8 description);
internal Flags_Option *flags_option_bool(Str8 name, bool *result_value, bool default_value, Str8 description);

internal Flags_Option *flags_option_str_arr(Str8 name, Str8_Array *result_value, Str8_Array *default_value, Str8 description);
internal Flags_Option *flags_option_int_arr(Str8 name, I64Array *result_value, I64Array *default_value, Str8 description);
internal Flags_Option *flags_option_uint_arr(Str8 name, U64Array *result_value, U64Array *default_value, Str8 description);
internal Flags_Option *flags_option_float_arr(Str8 name, F64Array *result_value, F64Array *default_value, Str8 description);

// ak: Add value flag =========================================================

internal Flags_Arg *flags_arg_str(Str8 *result_value, Str8 default_value);
internal Flags_Arg *flags_arg_int(int64_t *result_value, int64_t default_value);
internal Flags_Arg *flags_arg_uint(uint64_t *result_value, uint64_t default_value);
internal Flags_Arg *flags_arg_float(double *result_value, double default_value);

// ak: Generic Macros =========================================================

#if LANGUAGE_CPP
    inline Flags_Option *flags_option(Str8 name, Str8* val, Str8 def, Str8 desc) { return flags_option_str(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, int64_t* val, int64_t def, Str8 desc) { return flags_option_int(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, uint64_t* val, uint64_t def, Str8 desc) { return flags_option_uint(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, double* val, double def, Str8 desc) { return flags_option_float(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, bool* val, bool def, Str8 desc) { return flags_option_bool(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, Str8_Array* val, Str8_Array* def, Str8 desc) { return flags_option_str_arr(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, I64Array* val, I64Array* def, Str8 desc) { return flags_option_int_arr(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, U64Array* val, U64Array* def, Str8 desc) { return flags_option_uint_arr(name, val, def, desc); }
    inline Flags_Option *flags_option(Str8 name, F64Array* val, F64Array* def, Str8 desc) { return flags_option_float_arr(name, val, def, desc); }
    
    inline Flags_Arg *flags_arg(Str8* val, Str8 def) { return flags_arg_str(val, def); }
    inline Flags_Arg *flags_arg(int64_t* val, int64_t def) { return flags_arg_int(val, def); }
    inline Flags_Arg *flags_arg(uint64_t* val, uint64_t def) { return flags_arg_uint(val, def); }
    inline Flags_Arg *flags_arg(double* val, double def) { return flags_arg_float(val, def); }
#else
    #define flags_option(name, result_value, default_value, description) _Generic((result_value), \
        Str8*:        flags_option_str,       \
        int64_t*:     flags_option_int,       \
        uint64_t*:    flags_option_uint,      \
        double*:      flags_option_float,     \
        bool*:        flags_option_bool,      \
        Str8_Array*:  flags_option_str_arr,   \
        I64Array*:    flags_option_int_arr,   \
        U64Array*:    flags_option_uint_arr,  \
        F64Array*:    flags_option_float_arr  \
    )(name, result_value, default_value, description)
    
    #define flags_arg(result_value, default_value) _Generic((result_value), \
        Str8*:        flags_arg_str,     \
        int64_t*:     flags_arg_int,     \
        uint64_t*:    flags_arg_uint,    \
        double*:      flags_arg_float    \
    )(result_value, default_value)
#endif

// ak: Globals
//=============================================================================

global _Flags_State *_flags_state = 0;

#endif // BASE_FLAGS_H
