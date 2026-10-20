//-----------------------------------------------------------------------------
// Reference:
// - By Alexey Kutepov: https://github.com/tsoding/nob.h
//-----------------------------------------------------------------------------

// ak: Includes
//=============================================================================

// ak: headers
#include "std/base/base_include.h"
#include "std/os/os_include.h"
#include "std/metadesk/metadesk_app.h"

// ak: implementation
#include "std/base/base_include.c"
#include "std/os/os_include.c"

// ak: Defines
//=============================================================================

#define BUILD_DIR     "build"

// ak: Types
//=============================================================================

typedef enum Build_Type
{
    Build_Type_Dev,
    Build_Type_Debug,
    Build_Type_Release,
} Build_Type;

typedef uint32_t Build_Flags;
enum
{
    Build_Flag_MingW        = (1<<0),
    Build_Flag_DryRun       = (1<<1),
    Build_Flag_Cpp          = (1<<2),
    Build_Flag_Static_Lib   = (1<<3),
    Build_Flag_Dynamic_Lib  = (1<<4),
};

typedef struct Build_Info Build_Info;
struct Build_Info
{
    Arena *arena;
    Str8_List cmd;
    Str8 name;
    Str8 entry_point;
    Str8 args;
    Build_Type type;
    Build_Flags flags;
};

// ak: Globals
//=============================================================================

global const char *help_message = "DESCRIPTION:\n"
"   build.c - C file that build's C projects.\n"
"USAGE:\n"
"   build.c [OPTIONS]\n"
"OPTIONS:\n"
"   build                Build project\n"
"   run                  Run project\n"
"   build-run            Build and Run project\n"
"   build-dry            Build without producing any output files\n"
"   build-debugger       Build for Debugger\n"
"   gen-meta             Generate code from Metaprogram\n"
"   --help -h            Print help\n";

// ak: Functions
//=============================================================================

internal int32_t build_run(Str8 cmd);
internal Str8 build_path(Build_Info *info);

// ak: Compilers functions ====================================================

internal void build_compile_msvc(Build_Info *info)
{
    str8_list_pushf(info->arena, &info->cmd, "setup_x64.bat & cl.exe %s", info->entry_point.cstr);
    if (info->flags & Build_Flag_DryRun)
    {
        str8_list_pushf(info->arena, &info->cmd, " -Zs");
    }
    // ak: looks
    str8_list_pushf(info->arena, &info->cmd, " -nologo -diagnostics:caret");
    // ak: output
    str8_list_pushf(info->arena, &info->cmd, " -Fo:"BUILD_DIR"\\ -Fe:");
    str8_list_push(info->arena, &info->cmd, build_path(info));
    // ak: debug
    if (info->type != Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -Zi -Fd\""BUILD_DIR"\\vc140.pbd\" -DBUILD_DEBUG=1");
    }
    // ak: lock lang Version
    str8_list_pushf(info->arena, &info->cmd, " -std:c11");
    // ak: Optimization
    if (info->type == Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -Ox -wd4711"); // Enable
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd,
            " -Od"
            " -Ob1 -wd4710" // ak: Disable inline functions and it's warnings
        );
    }
    // ak: warnings
    if (info->type != Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -W4 -Wall");
    }
    // ak: disable uselss warnings
    str8_list_pushf(info->arena, &info->cmd,
        " -wd4668"                 // ak: For macros magic
        " -wd4464"                 // ak: Warning about '..' in path
        " -wd4310 -wd4146 -wd4245" // ak: Cast conversion
        " -wd4201"                 // ak: Nameless struct/union
        " -wd4820"                 // ak: Struct padding
        " -wd4061"                 // ak: Enum switch enumeration
    );
    // ak: security
    str8_list_pushf(info->arena, &info->cmd,
        " -Qspectre -wd5045"  // ak: Spectre variant 1 vulnerability
        " -GS"                // ak: Canary insertion
        " -guard:cf"          // ak: Control-flow protection
    );
    if (info->type != Build_Type_Debug && info->type != Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -fsanitize=address");
    }
}

internal void build_compile_gcc_style_flags(Build_Info *info)
{
    // ak: treat source code as c file not as precompiled header
    if (((info->flags & Build_Flag_Dynamic_Lib) || (info->flags & Build_Flag_Static_Lib))
        && !(info->flags &  Build_Flag_Cpp))
    {
        str8_list_pushf(info->arena, &info->cmd, " -x c");
    }
    str8_list_pushf(info->arena, &info->cmd, " %s", info->entry_point.cstr);
    // ak: dry run
    if (info->flags & Build_Flag_DryRun)
    {
        str8_list_pushf(info->arena, &info->cmd, " -fsyntax-only");
    }
    // ak: output
    str8_list_pushf(info->arena, &info->cmd, " -o ");
    str8_list_push(info->arena, &info->cmd, build_path(info));
    // ak: lock lang version
    if (info->flags & Build_Flag_Cpp)
    {
        str8_list_pushf(info->arena, &info->cmd, " -std=c++14");
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd, " -std=gnu99");
    }
    // ak: Optimization
    if (info->type == Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -O3");
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd, " -O0");
    }
    // ak: debug
    if (info->type == Build_Type_Debug)
    {
        str8_list_pushf(info->arena, &info->cmd, " -ggdb -g3");
    }
    if (info->type == Build_Type_Dev || info->type == Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -g0"); // no debug info
    }
    if (info->type != Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -DBUILD_DEBUG");
    }
    // ak: warning
    if (info->type != Build_Type_Release)
    {
        str8_list_pushf(info->arena, &info->cmd, " -Wall -Wextra");
    }
    // ak: disable useless warnings
    str8_list_pushf(info->arena, &info->cmd,
        " -Wno-unknown-pragmas"
        " -Wno-missing-braces"
        " -Wno-unused-function"
        " -Wno-unused-variable"
    );
    // ak: security
    str8_list_pushf(info->arena, &info->cmd, " -mshstk -fcf-protection=full -fstack-protector");
    if ((info->type != Build_Type_Debug && info->type != Build_Type_Release) && !(info->flags & Build_Flag_MingW))
    {
        str8_list_pushf(info->arena, &info->cmd, " -fsanitize=address -fno-omit-frame-pointer");
        // str8_list_pushf(info->arena, &info->cmd, " -fanalyzer");
    }
    // ak: compile as shared lib
    if (info->flags & Build_Flag_Dynamic_Lib)
    {
        str8_list_pushf(info->arena, &info->cmd, " -shared -fPIC");
    }
    else if (info->flags & Build_Flag_Static_Lib)
    {
        str8_list_pushf(info->arena, &info->cmd, " -c");
    }
}

internal void build_compile_gcc(Build_Info *info)
{
    if (info->flags & Build_Flag_MingW)
    {
        str8_list_pushf(info->arena, &info->cmd, "x86_64-w64-mingw32-gcc");
    }
    else if (info->flags & Build_Flag_Cpp)
    {
        
        str8_list_pushf(info->arena, &info->cmd, "g++");
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd, "gcc");
    }
    build_compile_gcc_style_flags(info);
}

internal void build_compile_clang(Build_Info *info)
{
    if (info->flags & Build_Flag_Cpp)
    {
        str8_list_pushf(info->arena, &info->cmd, "clang++");
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd, "clang");
    }
    build_compile_gcc_style_flags(info);
}

internal void build_compile(Build_Info *info)
{
    if (Context_Compiler_CURRENT == Context_Compiler_Gcc)
    {
        build_compile_gcc(info);
    }
    else if (Context_Compiler_CURRENT == Context_Compiler_Clang)
    {
        build_compile_clang(info);
    }
    else if (Context_Compiler_CURRENT == Context_Compiler_Msvc)
    {
        build_compile_msvc(info);
    }
    else
    {
        fmt_eprintf("Error: OS build compile is not supported.");
    }
}

// ak: Build run functions ====================================================

internal int32_t build_run_program(Build_Info *info)
{
    fmt_println("# Running ------------------------------------------------------------------- #");
    Str8_List list = STRUCT_ZERO;
    if (info->flags & Build_Flag_MingW)
    {
        str8_list_pushf(info->arena, &list, "WINEARCH=win64 wine ");
    }
    if (Context_Os_CURRENT == Context_Os_Windows)
    {
        str8_list_pushf(info->arena, &list, "setup_x64.bat & ");
    }
    str8_list_push(info->arena, &list, build_path(info));
    str8_list_pushf(info->arena, &list, " %.*s", str8_varg(info->args));
    Str8 cmd = str8_list_join(info->arena, &list, NULL);
    return build_run(cmd);
}

// ak: Program Specific Compile Functions ======================================

internal int32_t build_compile_miniaudio(Build_Info *info)
{
    int32_t result = 0;
    
    // ak: create build
    Str8 path = build_path(info);
    if (!fs_file_path_exists(path))
    {
        if (info->flags & Build_Flag_Dynamic_Lib)
        {
            fmt_println("# Compiling miniaudio Dynamic Library --------------------------------------- #");
        }
        else
        {
            fmt_println("# Compiling miniaudio Static Library ---------------------------------------- #");
        }
        build_compile(info);
        str8_list_pushf(info->arena, &info->cmd, " -DMINIAUDIO_IMPLEMENTATION");
        if (info->flags & Build_Flag_Dynamic_Lib)
        {
            str8_list_pushf(info->arena, &info->cmd, " -lm -lpthread -ldl");
        }
        if (Context_Compiler_CURRENT == Context_Compiler_Gcc)
        {
            str8_list_pushf(info->arena, &info->cmd, " -Wno-stringop-overflow");
        }
        Str8 cmd = str8_list_join(info->arena, &info->cmd, NULL);
        result = build_run(cmd);
        if ((result == 0) && (info->flags & Build_Flag_Static_Lib))
        {
            result = build_run(str8("ar rcs build/libminiaudio.a build/libminiaudio_for_static.so"));
        }
    }
    return result;
}

internal int32_t build_compile_run_metadesk(void)
{
    int32_t result = 0;
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    Build_Info info = STRUCT_ZERO;
    info.type = Build_Type_Debug;
    info.name = str8(MDA_CMD_NAME);
    info.entry_point = str8("std/metadesk/metadesk_app_main.c");
    info.args = str8("std src");
    info.arena = scratch.arena;
    
    fmt_println("# Compiling MetaDesk ------------------------------------------------------- #");
    build_compile(&info);
    Str8 cmd = str8_list_join(info.arena, &info.cmd, NULL);
    result = build_run(cmd);
    if (result == 0)
    {
        result = build_run_program(&info);
    }
    
    arena_scratch_end(scratch);
    return result;
}

internal int32_t build_compile_program(Build_Info *info, Build_Info *ma_info)
{
    fmt_println("# Compiling Program --------------------------------------------------------- #");
    int32_t result = 0;
    info->entry_point = str8f(info->arena, "src/%s8.c", info->name);
    build_compile(info);
    if (!(info->flags & Build_Flag_DryRun))
    {
        // ak: libs
        if ((info->flags & Build_Flag_MingW) || Context_Os_CURRENT == Context_Os_Windows)
        {
            str8_list_pushf(info->arena, &info->cmd, " -lopengl32 -luser32 -lgdi32");
        }
        else
        {
            str8_list_pushf(info->arena, &info->cmd, " -lm -lpthread");
            str8_list_pushf(info->arena, &info->cmd, " -lxcb -lxcb-image -lxcb-sync -lxcb-keysyms -lxcb-cursor");
            str8_list_pushf(info->arena, &info->cmd, " -lEGL -lGL");
            if (info->type == Build_Type_Release)
            {
                str8_list_pushf(info->arena, &info->cmd, " build/libminiaudio.a");
            }
            else
            {
                str8_list_pushf(info->arena, &info->cmd, " %s8", build_path(ma_info));
            }
        }
    }
    Str8 cmd = str8_list_join(info->arena, &info->cmd, NULL);
    result = build_run(cmd);
    return result;
}

// ak: Build Entry Point ======================================================

internal int32_t base_main(void)
{
    Arena *arena = arena_alloc();
    
    Build_Info info = STRUCT_ZERO;
    info.arena = arena;
    
    bool should_print_help = false;
    bool run_program = false;
    bool only_run_program = false;
    bool gen_meta_program = false;
    int32_t exit_code = 0;
    Str8 app_name = STRUCT_ZERO;
    
    Str8_Array *args = term_args_get();
    if (args->length >= 2)
    {
        Str8 arg1 = args->v[1];
        Str8 arg2 = STRUCT_ZERO;
        Str8 arg3 = STRUCT_ZERO;
        if (args->length >= 3)
        {
            info.name = args->v[2];
        }
        if (args->length >= 4)
        {
            arg3 = args->v[3];
        }
        if (str8_match(arg1, str8("--help"), 0) || str8_match(arg1, str8("-h"), 0))
        {
            should_print_help = true;
        }
        else if (str8_match(arg1, str8("build"), 0))
        {
            if (str8_match(arg3, str8("release"), 0))
            {
                info.type = Build_Type_Release;
            }
            else
            {
                info.type = Build_Type_Dev;
            }
        }
        else if (str8_match(arg1, str8("build-run"), 0))
        {
            if (str8_match(arg3, str8("release"), 0))
            {
                info.type = Build_Type_Release;
            }
            else
            {
                info.type = Build_Type_Dev;
            }
            run_program = true;
        }
        else if (str8_match(arg1, str8("build-dry"), 0))
        {
            info.type = Build_Type_Dev;
            info.flags |= Build_Flag_DryRun;
        }
        else if (str8_match(arg1, str8("build-debugger"), 0))
        {
            info.type = Build_Type_Debug;
        }
        else if (str8_match(arg1, str8("gen-meta"), 0))
        {
            gen_meta_program = true;
        }
        else if (str8_match(arg1, str8("run"), 0))
        {
            info.name = arg2;
            if (str8_match(arg3, str8("release"), 0))
            {
                info.type = Build_Type_Release;
            }
            else
            {
                info.type = Build_Type_Dev;
            }
            only_run_program = true;
        }
        else
        {
            fmt_eprintf("Error: wrong option provided `%s`.\n\n", arg1.cstr);
            should_print_help = true;
            return 1;
        }
    }
    else
    {
        should_print_help = true;
    }
    if (str8_match(args->v[2], str8("mingw"), 0))
    {
        info.flags |= Build_Flag_MingW;
    }
    
    if (only_run_program && !fs_file_path_exists(build_path(&info)))
    {
        only_run_program = false;
        run_program = true;
    }
    
    if (should_print_help)
    {
        fmt_printf("%s", help_message);
    }
    else if (only_run_program)
    {
        build_run(build_path(&info));
    }
    else
    {
        fmt_println("# Build Output ============================================================== #");
        
        // ak: create build directory if not exists
        if (fs_dir_make(str8(BUILD_DIR)))
        {
            fmt_printf("Created '"BUILD_DIR"' directory.\n");
        }
        
        // ak: build miniaudio lib
        Build_Info ma_info = STRUCT_ZERO;
        ma_info.name = str8("libminiaudio");
        ma_info.entry_point = str8("std/audio/external/miniaudio.h");
        ma_info.type = info.type;
        ma_info.flags = info.type == Build_Type_Release ? Build_Flag_Static_Lib : Build_Flag_Dynamic_Lib;
        ma_info.arena = arena;
        exit_code = build_compile_miniaudio(&ma_info);
        if (exit_code == 0) {
            exit_code = build_compile_miniaudio(&ma_info);
        }
        
        // ak: build metadesk
        if (exit_code == 0 && gen_meta_program)
        {
            exit_code = build_compile_run_metadesk();
        }
        
        // ak: build program
        else if (exit_code == 0)
        {
            exit_code = build_compile_program(&info, &ma_info);
            if (exit_code == 0 && run_program)
            {
                exit_code = build_run_program(&info);
            }
        }
    }
    arena_free(arena);
    return exit_code;
}

// ak: Build functions ========================================================

internal int32_t build_run(Str8 cmd)
{
    fmt_printf("Command: %s8\n", cmd);
    int32_t status = system((const char *)cmd.cstr);
    if (status == -1)
    {
        fmt_eprintf("\nError: %s\n", strerror(errno));
    }
    int32_t exit_code = WIFEXITED(status) ? WEXITSTATUS(status) : 1;
    return exit_code;
}

internal Str8 build_path(Build_Info *info)
{
    Str8_List list = STRUCT_ZERO;
    if (Context_Os_CURRENT == Context_Os_Windows)
    {
        str8_list_pushf(info->arena, &list, BUILD_DIR"\\%s", info->name.cstr);
    }
    else
    {
        str8_list_pushf(info->arena, &list, BUILD_DIR"/%s", info->name.cstr);
    }
    
    switch (info->type)
    {
        case Build_Type_Dev:
        {
            str8_list_pushf(info->arena, &list, "_dev");
        }
        break;
        case Build_Type_Debug:
        {
            str8_list_pushf(info->arena, &list, "_debug");
        }
        break;
        case Build_Type_Release: {}
        break;
    }
    
    if ((info->flags & Build_Flag_Static_Lib) && Context_Os_CURRENT == Context_Os_Linux)
    {
        str8_list_pushf(info->arena, &list, "_for_static.so");
    } else if ((info->flags & Build_Flag_Dynamic_Lib) && Context_Os_CURRENT == Context_Os_Linux)
    {
        str8_list_pushf(info->arena, &list, ".so");
    }
    else if ((info->flags & Build_Flag_Static_Lib) && Context_Os_CURRENT == Context_Os_Windows)
    {
        str8_list_pushf(info->arena, &list, ".lib");
    }
    else if ((info->flags & Build_Flag_Dynamic_Lib) && Context_Os_CURRENT == Context_Os_Windows)
    {
        str8_list_pushf(info->arena, &list, ".dll");
    } else if (Context_Os_CURRENT == Context_Os_Windows)
    {
        str8_list_pushf(info->arena, &list, ".exe");
    }
    
    Str8 result = str8_list_join(info->arena, &list, NULL);
    return result;
}
