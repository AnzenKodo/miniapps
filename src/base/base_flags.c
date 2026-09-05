// ak: Private functions
//=============================================================================

internal Flags_Option *_flags_get_option(Str8 name)
{
    Flags_Option *result = NULL;
    for (Flags_Option *option = _flags_state->first_option; option != NULL; option = option->next)
    {
        if (str8_match(name, option->name, Str_Match_Flag_None))
        {
            result = option;
        }
        if (str8_match(name, option->shortname, Str_Match_Flag_None))
        {
            result = option;
        }
    }
    return result;
}
internal void _flags_add_option(Flags_Option *option)
{
    // ak: Error on finding dublicate flags
    Assert(_flags_get_option(option->name) == NULL);
    SLLQueuePush(_flags_state->first_option, _flags_state->last_option, option);
}

internal Flags_Arg *_flags_get_arg(size_t index)
{
    Flags_Arg *result = NULL;
    for (Flags_Arg *farg = _flags_state->first_arg; farg != NULL; farg = farg->next)
    {
        if (farg->index == index)
        {
            result = farg;
        }
    }
    return result;
}
internal void _flags_add_arg(Flags_Arg *farg)
{
    Assert(_flags_get_arg(farg->index) == NULL);
    SLLQueuePush(_flags_state->first_arg, _flags_state->last_arg, farg);
    farg->index = _flags_state->index_arg++;
}

internal void _flags_add_option_error(_Flags_Error_Kind kind, Str8 name)
{
    _Flags_Error *error = arena_push(_flags_state->arena, _Flags_Error, 1);
    error->kind = kind;
    error->flag_name = name;
    SLLQueuePush(_flags_state->first_error, _flags_state->last_error, error);
    _flags_state->has_error = true;
}
internal void _flags_add_option_error_value(_Flags_Error_Kind kind, Str8 name, Str8 value)
{
    _Flags_Error *error = arena_push(_flags_state->arena, _Flags_Error, 1);
    error->kind = kind;
    error->flag_name = name;
    error->value = value;
    SLLQueuePush(_flags_state->first_error, _flags_state->last_error, error);
    _flags_state->has_error = true;
}
internal void _flags_add_error_arg(_Flags_Error_Kind kind, size_t index, Str8 value)
{
    _Flags_Error *error = arena_push(_flags_state->arena, _Flags_Error, 1);
    error->kind = kind;
    error->value = value;
    error->arg_index = index;
    SLLQueuePush(_flags_state->first_error, _flags_state->last_error, error);
    _flags_state->has_error = true;
}

internal bool _flags_is_arg_option(Str8 arg)
{
    return _flags_get_options_from_arg(arg).length > 0 ? true : false;
}

internal Str8 _flags_get_options_from_arg(Str8 arg)
{
    Str8 result = STRUCT_ZERO;
    if (str8_match(str8_prefix(arg, 2), str8("--"), Str_Match_Flag_None))
    {
        result = str8_skip(arg, 2);
    }
    else if (str8_match(str8_prefix(arg, 1), str8("-"), Str_Match_Flag_None))
    {
        result = str8_skip(arg, 1);
    }
    else if (Context_Os_CURRENT == Context_Os_Windows &&
            str8_match(str8_prefix(arg, 1), str8("/"), Str_Match_Flag_None))
    {
        result = str8_skip(arg, 1);
    }
    return result;
}

internal uint64_t _flags_get_values_count(Str8_Array *args, uint64_t index)
{
    uint64_t count = 0;
    for (uint64_t i = index; i < args->length; i++)
    {
        Str8 arg = args->v[i];
        if (_flags_is_arg_option(arg)) break;
        count++;
    }
    return count;
}

// ak: Flags core functions ===================================================

internal void flags_begin(void)
{
    Arena *arena = arena_alloc();
    _flags_state = arena_push(arena, _Flags_State, 1);
    _flags_state->arena = arena;
    _flags_state->has_program_name = true;
    _flags_state->log_context = log_init();
}

internal void flags_end(void)
{
    arena_free(_flags_state->arena);
}

internal bool flags_parse(Str8_Array *args)
{
    bool has_passthrough_option = false;
    Flags_Option *option = NULL;
    for (uint32_t index = _flags_state->has_program_name ? 1 : 0; index < args->length; index++)
    {
        Str8 arg = args->v[index];
        Base base = Base_10;
        if (str8_match(arg, str8("--"), Str_Match_Flag_None))
        {
            has_passthrough_option = 1;
            Unused(has_passthrough_option);
            break;
        }
        if (_flags_is_arg_option(arg))
        {
            Str8 option_name = _flags_get_options_from_arg(arg);
            option = _flags_get_option(option_name);
            if (option == NULL)
            {
                _flags_add_option_error(_Flags_Error_Kind_UnknownOption, option_name);
            }
            else
            {
                if (option->assigned)
                {
                    _flags_add_option_error(_Flags_Error_Kind_DuplicateOption, option_name);
                }
                Str8 arg_next = STRUCT_ZERO;
                if (args->length < index+1)
                {
                    arg_next = array_get(args, index+1);
                }
                bool is_arg_next_option = _flags_is_arg_option(arg_next);
                if ((is_arg_next_option || arg_next.length == 0) && option->kind == _Flags_Option_Kind_Bool)
                {
                    *option->result_value.bool_value = true;
                    option->assigned = true;
                }
                if (is_arg_next_option && option->kind != _Flags_Option_Kind_Bool)
                {
                    _flags_add_option_error(_Flags_Error_Kind_MissingValue, option_name);
                }
            }
        }
        else if (option != NULL)
        {
            if (option->assigned)
            {
                _flags_add_option_error(_Flags_Error_Kind_SingleValue, option->name);
            }
            option->assigned = true;
            switch (option->kind)
            {
                case _Flags_Option_Kind_Str:
                {
                        *option->result_value.str_value = arg;
                }
                break;
                case _Flags_Option_Kind_Int:
                {
                    if (str8_is_integer(arg, base))
                    {
                        *option->result_value.int_value = i64_from_str8(arg, base);
                    }
                    else
                    {
                        _flags_add_option_error_value(_Flags_Error_Kind_InvalidIntOption, option->name, arg);
                    }
                }
                break;
                case _Flags_Option_Kind_UInt:
                {
                    if (str8_is_integer(arg, base))
                    {
                        if (str8_is_integer_unsigned(arg, base))
                        {
                            *option->result_value.uint_value = u64_from_str8(arg, base);
                        }
                        else
                        {
                            _flags_add_option_error_value(_Flags_Error_Kind_UIntMinusOption, option->name, arg);
                        }
                    }
                    else
                    {
                        _flags_add_option_error_value(_Flags_Error_Kind_InvalidIntOption, option->name, arg);
                    }
                }
                break;
                case _Flags_Option_Kind_Float:
                {
                    if (str8_is_float(arg))
                    {
                        *option->result_value.float_value = f64_from_str8(arg);
                    }
                    else
                    {
                        _flags_add_option_error_value(_Flags_Error_Kind_InvalidFloatOption, option->name, arg);
                    }
                }
                break;
                case _Flags_Option_Kind_Bool:
                {
                    if (str8_is_bool(arg))
                    {
                        *option->result_value.bool_value = bool_from_str8(arg);
                    }
                    else
                    {
                        _flags_add_option_error_value(_Flags_Error_Kind_InvalidBool, option->name, arg);
                    }
                }
                break;
                case _Flags_Option_Kind_StrArr:
                {
                    Str8_Array array = STRUCT_ZERO;
                    uint64_t items_count = _flags_get_values_count(args, index);
                    array.v = arena_push(_flags_state->arena, Str8, items_count);
                    for (uint64_t i = 0; i < items_count; i++)
                    {
                        Str8 array_arg = args->v[index];
                        array.v[array.length++] = array_arg;
                        index++;
                    }
                    index--;
                    *option->result_value.str_value_arr = array;
                }
                break;
                case _Flags_Option_Kind_IntArr:
                {
                    I64Array array = STRUCT_ZERO;
                    uint64_t items_count = _flags_get_values_count(args, index);
                    array.v = arena_push(_flags_state->arena, int64_t, items_count);
                    for (uint64_t i = 0; i < items_count; i++)
                    {
                        Str8 array_arg = args->v[index];
                        if (str8_is_integer(array_arg, base))
                        {
                            array.v[array.length++] = i64_from_str8(array_arg, base);
                        }
                        else
                        {
                            _flags_add_option_error_value(_Flags_Error_Kind_InvalidIntOption, option->name, array_arg);
                        }
                        index++;
                    }
                    index--;
                    *option->result_value.int_value_arr = array;
                }
                break;
                case _Flags_Option_Kind_UIntArr:
                {
                    U64Array array = STRUCT_ZERO;
                    uint64_t items_count = _flags_get_values_count(args, index);
                    array.v = arena_push(_flags_state->arena, uint64_t, index);
                    for (uint64_t i = 0; i < items_count; i++)
                    {
                        Str8 array_arg = args->v[index];
                        if (str8_is_integer(array_arg, base))
                        {
                            if (str8_is_integer_unsigned(array_arg, base))
                            {
                                array.v[array.length++] = u64_from_str8(array_arg, base);
                            }
                            else
                            {
                                _flags_add_option_error_value(_Flags_Error_Kind_UIntMinusOption, option->name, array_arg);
                            }
                        }
                        else
                        {
                            _flags_add_option_error_value(_Flags_Error_Kind_InvalidIntOption, option->name, array_arg);
                        }
                        index++;
                    }
                    index--;
                    *option->result_value.uint_value_arr = array;
                }
                break;
                case _Flags_Option_Kind_FloatArr:
                {
                    F64Array array = STRUCT_ZERO;
                    uint64_t items_count = _flags_get_values_count(args, index);
                    array.v = arena_push(_flags_state->arena, double, items_count);
                    for (uint64_t i = 0; i < items_count; i++)
                    {
                        Str8 array_arg = args->v[index];
                        if (str8_is_float(array_arg))
                        {
                            array.v[array.length++] = f64_from_str8(array_arg);
                        }
                        else
                        {
                            _flags_add_option_error_value(_Flags_Error_Kind_InvalidIntOption, option->name, array_arg);
                        }
                        index++;
                    }
                    index--;
                    *option->result_value.float_value_arr = array;
                }
                break;
            }
        }
        else
        {
            Flags_Arg *farg = _flags_get_arg(index - (_flags_state->has_program_name ? 1: 0));
            if (farg != NULL)
            {
                farg->assigned = true;
                switch (farg->kind)
                {
                    case _Flags_Arg_Kind_Str:
                    {
                        *farg->result_value.str_value = arg;
                    }
                    break;
                    case _Flags_Arg_Kind_Int:
                    {
                        if (str8_is_integer(arg, base))
                        {
                            *farg->result_value.int_value = i64_from_str8(arg, base);
                        }
                        else
                        {
                            _flags_add_error_arg(_Flags_Error_Kind_InvalidIntArg, index, arg);
                        }
                    }
                    break;
                    case _Flags_Arg_Kind_UInt:
                    {
                        if (str8_is_integer(arg, base))
                        {
                            if (str8_is_integer_unsigned(arg, base))
                            {
                                *farg->result_value.uint_value = u64_from_str8(arg, base);
                            }
                            else
                            {
                                _flags_add_error_arg(_Flags_Error_Kind_UIntMinusArg, index, arg);
                            }
                        }
                        else
                        {
                            _flags_add_error_arg(_Flags_Error_Kind_InvalidIntArg, index, arg);
                        }
                    }
                    break;
                    case _Flags_Arg_Kind_Float:
                    {
                        if (str8_is_float(arg))
                        {
                            *farg->result_value.float_value = f64_from_str8(arg);
                        }
                        else
                        {
                            _flags_add_error_arg(_Flags_Error_Kind_InvalidFloatArg, index, arg);
                        }
                    }
                    break;
                }
            }
            else
            {
                _flags_add_error_arg(_Flags_Error_Kind_OutIndexArg, index, arg);
            }
        }
    }
    // ak: Assign defaults and handle require
    for (Flags_Option *foption = _flags_state->first_option; foption != NULL; foption = foption->next)
    {
        if (!foption->assigned)
        {
            if (foption->required)
            {
                _flags_add_option_error(_Flags_Error_Kind_RequireOption, foption->name);
            }
            else
            {
                switch (foption->kind)
                {
                    case _Flags_Option_Kind_Str:
                    {
                        *foption->result_value.str_value = foption->default_value.str_value;
                    }
                    break;
                    case _Flags_Option_Kind_Bool:
                    {
                        *foption->result_value.bool_value = foption->default_value.bool_value;
                    }
                    break;
                    case _Flags_Option_Kind_Int:
                    {
                        *foption->result_value.int_value = foption->default_value.int_value;
                    }
                    break;
                    case _Flags_Option_Kind_UInt:
                    {
                        *foption->result_value.uint_value = foption->default_value.uint_value;
                    }
                    break;
                    case _Flags_Option_Kind_Float:
                    {
                        *foption->result_value.float_value = foption->default_value.float_value;
                    }
                    break;
                    case _Flags_Option_Kind_StrArr:
                    {
                        foption->result_value.str_value_arr = foption->default_value.str_value_arr;
                    }
                    break;
                    case _Flags_Option_Kind_IntArr:
                    {
                        foption->result_value.int_value_arr = foption->default_value.int_value_arr;
                    }
                    break;
                    case _Flags_Option_Kind_UIntArr:
                    {
                        foption->result_value.uint_value_arr = foption->default_value.uint_value_arr;
                    }
                    break;
                    case _Flags_Option_Kind_FloatArr:
                    {
                        foption->result_value.float_value_arr = foption->default_value.float_value_arr;
                    }
                    break;
                }
            }
        }
    }
    for (Flags_Arg *farg = _flags_state->first_arg; farg != NULL; farg = farg->next)
    {
        if (!farg->assigned)
        {
            if (farg->required)
            {
                _flags_add_error_arg(_Flags_Error_Kind_RequireArg, farg->index, str8(""));
            }
            else
            {
                switch (farg->kind)
                {
                    case _Flags_Arg_Kind_Str:
                    {
                        *farg->result_value.str_value = farg->default_value.str_value;
                    } break;
                    case _Flags_Arg_Kind_Int:
                    {
                        *farg->result_value.int_value = farg->default_value.int_value;
                    } break;
                    case _Flags_Arg_Kind_UInt:
                    {
                        *farg->result_value.uint_value = farg->default_value.uint_value;
                    } break;
                    case _Flags_Arg_Kind_Float:
                    {
                        *farg->result_value.float_value = farg->default_value.float_value;
                    } break;
                }
            }
        }
    }
    return !_flags_state->has_error;
}

internal void flags_print_error(void)
{
    for (_Flags_Error *error = _flags_state->first_error; error != NULL; error = error->next)
    {
        switch (error->kind)
        {
            case _Flags_Error_Kind_MissingValue:
            {
                log_errorfln(&_flags_state->log_context,
                    "opiton '%.*s' requires a value. Example: '--%.*s <value>'.",
                    str8_varg(error->flag_name), str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_UnknownOption:
            {
                log_errorfln(&_flags_state->log_context, "opiton '%.*s' is invalid.", str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_NoFlagPrefix:
            {
                log_errorfln(&_flags_state->log_context,
                    "'%.*s' is not recognized as a opiton. Option must start with '-', '--', or '/' (Windows only). Examples: '-%.*s', '--%.*s', '/%.*s'.",
                    str8_varg(error->flag_name), str8_varg(error->flag_name), str8_varg(error->flag_name), str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_DuplicateOption:
            {
                log_errorfln(&_flags_state->log_context, "option '%.*s' was specified multiple times.", str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_RequireOption:
            {
                log_errorfln(&_flags_state->log_context, "missing required option '--%.*s'.", str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_InvalidIntOption:
            {
                log_errorfln(&_flags_state->log_context,
                    "option '%.*s' expects an integer value, but '%.*s' was given. Examples: '--%.*s 42', '--%.*s -7', '--%.*s 123'.",
                    str8_varg(error->flag_name), str8_varg(error->value), str8_varg(error->flag_name), str8_varg(error->flag_name), str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_UIntMinusOption:
            {
                log_errorfln(&_flags_state->log_context,
                    "option '%.*s' does not accept negative values (got '%.*s'). Use a positive integer instead.",
                    str8_varg(error->flag_name), str8_varg(error->value));
            }
            break;
            case _Flags_Error_Kind_InvalidFloatOption:
            {
                log_errorfln(&_flags_state->log_context,
                    "option '%.*s' expects a floating-point number, but '%.*s' was given. Examples: '--%.*s .14', '--%.*s -0.5', '--%.*s 2', '--%.*s 2.0'.",
                    str8_varg(error->flag_name), str8_varg(error->value), str8_varg(error->flag_name), str8_varg(error->flag_name), str8_varg(error->flag_name), str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_InvalidBool:
            {
                log_errorfln(&_flags_state->log_context,
                    "option '%.*s' expects a boolean value, but '%.*s' was given. Enter 'true', 'false' or no value for true.",
                    str8_varg(error->flag_name), str8_varg(error->value));
            }
            break;
            case _Flags_Error_Kind_SingleValue:
            {
                log_errorfln(&_flags_state->log_context,
                    "option '%.*s' only accepts single value.",
                    str8_varg(error->flag_name));
            }
            break;
            case _Flags_Error_Kind_OutIndexArg:
            {
                if (_flags_state->index_arg > 0)
                {
                    log_errorfln(&_flags_state->log_context,
                        "command only accepts >= (less equal to) %zu arguments without options.",
                        _flags_state->index_arg);
                }
                else
                {
                    log_errorfln(&_flags_state->log_context,
                        "command don't accepts any arguments without options.");
                }
            }
            break;
            case _Flags_Error_Kind_RequireArg:
            {
                log_errorfln(&_flags_state->log_context,
                    "at least %zu argument without option is required.",
                    error->arg_index + 1);
            }
            break;
            case _Flags_Error_Kind_InvalidIntArg:
            {
                log_errorfln(&_flags_state->log_context,
                    "%zu argument without option should be integer. Examples: '42', '-7', '123'.",
                    error->arg_index + 1);
            }
            break;
            case _Flags_Error_Kind_UIntMinusArg:
            {
                log_errorfln(&_flags_state->log_context,
                    "%zu argument without option does not accept negative values (got '%.*s'). Use a positive integer instead.",
                    error->arg_index + 1, str8_varg(error->value));
            }
            break;
            case _Flags_Error_Kind_InvalidFloatArg:
            {
                log_errorfln(&_flags_state->log_context,
                    "%zu argument without option expects a floating-point number, but '%.*s' was given. Examples: '.14', '-0.5', '2', '2.0'.",
                    error->arg_index + 1, str8_varg(error->value));
            }
            break;
        }
    }
}
internal void flags_print_help(void)
{
    for (Flags_Option *option = _flags_state->first_option; option != NULL; option = option->next)
    {
        term_style_start(OS_STDOUT, TERM_BOLD);
        fmt_printf("    %s%s%s%s--%.*s",
            option->shortname.length > 0 ? "-" : "",
            option->shortname.length > 0 ? (char *)option->shortname.cstr : "",
            option->shortname.length > 0 ? "," : "",
            option->shortname.length  ? " " : "",
            str8_varg(option->name));
        if (option->value_hint.length > 0)
        {
            fmt_printf(" %.*s", str8_varg(option->value_hint));
        }
        if (option->required)
        {
            fmt_printf(" (required)");
        }
        uint8_t desc_spacing = 8;
        const char *default_syntex = "(default: ";
        fmt_printfln("\n%-*s%.*s", desc_spacing, "", str8_varg(option->description));
        switch (option->kind)
        {
            case _Flags_Option_Kind_Str:
            {
                if (option->default_value.str_value.length > 0)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    fmt_printfln("\"%.*s\")", str8_varg(option->default_value.str_value));
                }
            }
            break;
            case _Flags_Option_Kind_Int:
            {
                if (option->default_value.int_value != 0)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    fmt_printfln("%ld)", option->default_value.int_value);
                }
            }
            break;
            case _Flags_Option_Kind_UInt:
            {
                if (option->default_value.uint_value != 0)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    fmt_printfln("%lu)", option->default_value.uint_value);
                }
            }
            break;
            case _Flags_Option_Kind_Float:
            {
                if (option->default_value.float_value != 0)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    fmt_printfln("%f)", option->default_value.float_value);
                }
            }
            break;
            case _Flags_Option_Kind_Bool:
            {
            }
            break;
            case _Flags_Option_Kind_StrArr:
            {
                Str8_Array *default_array = option->default_value.str_value_arr;
                if (default_array != NULL)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    for (uint32_t i = 0; i < default_array->length; i++)
                    {
                        fmt_printf("\"%.*s\"", str8_varg(default_array->v[i]));
                        if (default_array->length-1 != i)
                        {
                            fmt_print(" ");
                        }
                    }
                    fmt_printfln(")");
                }
            }
            break;
            case _Flags_Option_Kind_IntArr:
            {
                I64Array *default_array = option->default_value.int_value_arr;
                if (default_array != NULL)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    for (uint32_t i = 0; i < default_array->length; i++)
                    {
                        fmt_printf("%ld", default_array->v[i]);
                        if (default_array->length-1 != i)
                        {
                            fmt_print(" ");
                        }
                    }
                    fmt_printfln(")");
                }
            }
            break;
            case _Flags_Option_Kind_UIntArr:
            {
                U64Array *default_array = option->default_value.uint_value_arr;
                if (default_array != NULL)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    for (uint32_t i = 0; i < default_array->length; i++)
                    {
                        fmt_printf("%lu", default_array->v[i]);
                        if (default_array->length-1 != i)
                        {
                            fmt_print(" ");
                        }
                    }
                    fmt_printfln(")");
                }
            }
            break;
            case _Flags_Option_Kind_FloatArr:
            {
                F64Array *default_array = option->default_value.float_value_arr;
                if (default_array != NULL)
                {
                    fmt_printf("%*.s%s", desc_spacing, "", default_syntex);
                    for (uint32_t i = 0; i < default_array->length; i++)
                    {
                        fmt_printf("%f", default_array->v[i]);
                        if (default_array->length-1 != i)
                        {
                            fmt_print(" ");
                        }
                    }
                    fmt_printfln(")");
                }
            }
            break;
        }
    }
}

// ak: Flags config functions
//=============================================================================

internal void flags_has_program_name(bool has_name)
{
    _flags_state->has_program_name = has_name;
}

internal void flags_add_option_shortname(Flags_Option *option, Str8 shortname)
{
    option->shortname = shortname;
}
internal void flags_add_option_value_hint(Flags_Option *option, Str8 value_hint)
{
    option->value_hint = value_hint;
}
internal void flags_make_option_required(Flags_Option *option)
{
    option->required = true;
}
internal void flags_make_arg_required(Flags_Arg *farg)
{
    farg->required = true;
}

// ak: Add option =============================================================

internal Flags_Option *flags_option_str(Str8 name, Str8 *result_value, Str8 default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_Str;
    option->name = name;
    option->default_value.str_value = default_value;
    option->description = description;
    option->result_value.str_value = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_int(Str8 name, int64_t *result_value, int64_t default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_Int;
    option->name = name;
    option->default_value.int_value = default_value;
    option->description = description;
    option->result_value.int_value = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_uint(Str8 name, uint64_t *result_value, uint64_t default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_UInt;
    option->name = name;
    option->default_value.uint_value = default_value;
    option->description = description;
    option->result_value.uint_value = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_float(Str8 name, double *result_value, double default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_Float;
    option->name = name;
    option->default_value.float_value = default_value;
    option->description = description;
    option->result_value.float_value = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_bool(Str8 name, bool *result_value, bool default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_Bool;
    option->name = name;
    option->default_value.bool_value = default_value;
    option->description = description;
    option->result_value.bool_value = result_value;
    _flags_add_option(option);
    return option;
}

internal Flags_Option *flags_option_str_arr(Str8 name, Str8_Array *result_value, Str8_Array *default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_StrArr;
    option->name = name;
    option->default_value.str_value_arr = default_value;
    option->description = description;
    option->result_value.str_value_arr = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_int_arr(Str8 name, I64Array *result_value, I64Array *default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_IntArr;
    option->name = name;
    option->default_value.int_value_arr = default_value;
    option->description = description;
    option->result_value.int_value_arr = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_uint_arr(Str8 name, U64Array *result_value, U64Array *default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_UIntArr;
    option->name = name;
    option->default_value.uint_value_arr = default_value;
    option->description = description;
    option->result_value.uint_value_arr = result_value;
    _flags_add_option(option);
    return option;
}
internal Flags_Option *flags_option_float_arr(Str8 name, F64Array *result_value, F64Array *default_value, Str8 description)
{
    Flags_Option *option = arena_push(_flags_state->arena, Flags_Option, 1);
    option->kind = _Flags_Option_Kind_FloatArr;
    option->name = name;
    option->default_value.float_value_arr = default_value;
    option->description = description;
    option->result_value.float_value_arr = result_value;
    _flags_add_option(option);
    return option;
}

// ak: Add value flag
//=============================================================================

internal Flags_Arg *flags_arg_str(Str8 *result_value, Str8 default_value)
{
    Flags_Arg *farg = arena_push(_flags_state->arena, Flags_Arg, 1);
    farg->kind = _Flags_Arg_Kind_Str;
    farg->result_value.str_value = result_value;
    farg->default_value.str_value = default_value;
    _flags_add_arg(farg);
    return farg;
}
internal Flags_Arg *flags_arg_int(int64_t *result_value, int64_t default_value)
{
    Flags_Arg *farg = arena_push(_flags_state->arena, Flags_Arg, 1);
    farg->kind = _Flags_Arg_Kind_Int;
    farg->result_value.int_value = result_value;
    farg->default_value.int_value = default_value;
    _flags_add_arg(farg);
    return farg;
}
internal Flags_Arg *flags_arg_uint(uint64_t *result_value, uint64_t default_value)
{
    Flags_Arg *farg = arena_push(_flags_state->arena, Flags_Arg, 1);
    farg->kind = _Flags_Arg_Kind_UInt;
    farg->result_value.uint_value = result_value;
    farg->default_value.uint_value = default_value;
    _flags_add_arg(farg);
    return farg;
}
internal Flags_Arg *flags_arg_float(double *result_value, double default_value)
{
    Flags_Arg *farg = arena_push(_flags_state->arena, Flags_Arg, 1);
    farg->kind = _Flags_Arg_Kind_Float;
    farg->result_value.float_value = result_value;
    farg->default_value.float_value = default_value;
    _flags_add_arg(farg);
    return farg;
}

