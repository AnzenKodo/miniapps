// ak: headers
#include "std/base/base_include.h"
#include "std/os/os_include.h"

// ak: implementation
#include "std/base/base_include.c"
#include "std/os/os_include.c"

internal void print_help_message()
{
    term_style_start(OS_STDOUT, TERM_UNDERLINE);
    fmt_println("USAGE:");
    term_style_end();
    Str8 *program_name = os_program_path_get();
    fmt_printfln("   %s [OPTIONS]", program_name->cstr);
    term_style_start(OS_STDOUT, TERM_UNDERLINE);
    fmt_println("OPTIONS:");
    term_style_end();
    flags_print_help();
    term_style_start(OS_STDOUT, TERM_UNDERLINE);
    fmt_println("VERSION:");
    term_style_end();
    fmt_println("   v1.3");
}

internal void base_main(void)
{
    flags_init();
    Flags_Option *option = NULL;
    bool help = false;
    option = flags_option_bool(str8("help"), &help, help, str8("Print help message"));
    flags_add_option_shortname(option, str8("h"));
    bool version = false;
    option = flags_option_bool(str8("version"), &version, version, str8("Print version message"));
    flags_add_option_shortname(option, str8("v"));

    // ak: Testing String Array Args
    Str8_Array str_arr = STRUCT_ZERO;
    flags_arg_str_arr(&str_arr, &str_arr);
    // NOTE(ak): more options and args can't be assigned after arg array
    // option = flags_option_bool(str8("a"), &version, version, str8("Print version message"));

    Str8_Array *args = term_args_get();
    if (!flags_parse(args))
    {
        flags_print_error();
        print_help_message();
        flags_clean();
        os_exit(1);
    }
    
    if (help)
    {
        print_help_message();
        flags_clean();
        os_exit(0);
    }
    if (version)
    {
        fmt_print("v1.3");
        flags_clean();
        os_exit(0);
    }
}
