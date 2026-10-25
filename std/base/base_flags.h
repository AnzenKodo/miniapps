//-----------------------------------------------------------------------------
// Reference:
// - By Krzysztof Gabis: http://github.com/kgabis/kgflags
// - By Go Authors: https://pkg.go.dev/flag
//-----------------------------------------------------------------------------

#ifndef BASE_FLAGS_H
#define BASE_FLAGS_H

//~ ak: Types
//=============================================================================

typedef enum Flags_Option_Kind
{
    Flags_Option_Kind_Str,
    Flags_Option_Kind_Int,
    Flags_Option_Kind_UInt,
    Flags_Option_Kind_Float,
    Flags_Option_Kind_Bool,
    Flags_Option_Kind_StrArr,
    Flags_Option_Kind_IntArr,
    Flags_Option_Kind_UIntArr,
    Flags_Option_Kind_FloatArr,
} Flags_Option_Kind;

typedef enum Flags_Arg_Kind
{
    Flags_Arg_Kind_Str,
    Flags_Arg_Kind_Int,
    Flags_Arg_Kind_UInt,
    Flags_Arg_Kind_Float,
    Flags_Arg_Kind_StrArr,
    Flags_Arg_Kind_IntArr,
    Flags_Arg_Kind_UIntArr,
    Flags_Arg_Kind_FloatArr,
} Flags_Arg_Kind;

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
    Flags_Option_Kind kind;
    bool assigned;
    bool required;
};

typedef struct Flags_Arg Flags_Arg;
struct Flags_Arg
{
    Flags_Arg *next;
    Flags_Arg_Kind kind;
    size_t index;
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
    bool assigned;
    bool required;
};

//~ ak: Functions
//=============================================================================

//- ak: Core ==================================================================

internal void flags_init(void);
internal void flags_uninit(void);
internal bool flags_parse(Str8_Array *args);

//- ak: Print Flags ============================================================

internal void flags_print_error(void);
internal void flags_print_help(Str8 description, Str8 usage);
internal void flags_print_version(void);
internal void flags_print_options(void);

//- ak: Config Flags ==========================================================

internal void flags_add_option_shortname(Flags_Option *option, Str8 shortname);
internal void flags_add_option_value_hint(Flags_Option *option, Str8 value_hint);
internal void flags_make_option_required(Flags_Option *option);
internal void flags_make_arg_required(Flags_Arg *arg);

//- ak: Options Flags =========================================================

internal Flags_Option *flags_option_str(Str8 name, Str8 *result_value, Str8 default_value, Str8 description);
internal Flags_Option *flags_option_int(Str8 name, int64_t *result_value, int64_t default_value, Str8 description);
internal Flags_Option *flags_option_uint(Str8 name, uint64_t *result_value, uint64_t default_value, Str8 description);
internal Flags_Option *flags_option_float(Str8 name, double *result_value, double default_value, Str8 description);
internal Flags_Option *flags_option_bool(Str8 name, bool *result_value, bool default_value, Str8 description);

internal Flags_Option *flags_option_str_arr(Str8 name, Str8_Array *result_value, Str8_Array *default_value, Str8 description);
internal Flags_Option *flags_option_int_arr(Str8 name, I64Array *result_value, I64Array *default_value, Str8 description);
internal Flags_Option *flags_option_uint_arr(Str8 name, U64Array *result_value, U64Array *default_value, Str8 description);
internal Flags_Option *flags_option_float_arr(Str8 name, F64Array *result_value, F64Array *default_value, Str8 description);

//- ak: Arguments Flags =======================================================

internal Flags_Arg *flags_arg_str(Str8 *result_value, Str8 default_value);
internal Flags_Arg *flags_arg_int(int64_t *result_value, int64_t default_value);
internal Flags_Arg *flags_arg_uint(uint64_t *result_value, uint64_t default_value);
internal Flags_Arg *flags_arg_float(double *result_value, double default_value);

internal Flags_Arg *flags_arg_str_arr(Str8_Array *result_value, Str8_Array *default_value);
internal Flags_Arg *flags_arg_int_arr(I64Array *result_value, I64Array *default_value);
internal Flags_Arg *flags_arg_uint_arr(U64Array *result_value, U64Array *default_value);
internal Flags_Arg *flags_arg_float_arr(F64Array *result_value, F64Array *default_value);

//~ ak: Generic Macros
//=============================================================================

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
    inline Flags_Option *flags_arg(Str8_Array* val, Str8_Array* def) { return flags_arg_str_arr(val, def); }
    inline Flags_Option *flags_arg(I64Array* val, I64Array* def) { return flags_arg_int_arr(val, def); }
    inline Flags_Option *flags_arg(U64Array* val, U64Array* def) { return flags_arg_uint_arr(val, def); }
    inline Flags_Option *flags_arg(F64Array* val, F64Array* def) { return flags_arg_float_arr(val, def); }
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
        Str8*:        flags_arg_str,      \
        int64_t*:     flags_arg_int,      \
        uint64_t*:    flags_arg_uint,     \
        double*:      flags_arg_float     \
        Str8_Array*:  flags_arg_str_arr,  \
        I64Array*:    flags_arg_int_arr,  \
        U64Array*:    flags_arg_uint_arr, \
        F64Array*:    flags_arg_float_arr \
    )(result_value, default_value)
#endif

#endif // BASE_FLAGS_H
