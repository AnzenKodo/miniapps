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
#define BUILD_SRC_DIR "src"

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

global const char *help_message =
"DESCRIPTION:\n"
"   Build driver for compiling and running C projects.\n\n"
"USAGE:\n"
"   build [COMMAND] [TARGET] [CONFIG]\n\n"
"COMMANDS:\n"
"   build                Compile target program(s) (default)\n"
"   run                  Run target program (builds first if not present)\n"
"   build-run            Compile and run target program\n"
"   build-dry            Check syntax without producing output files\n"
"   build-debugger       Compile with full debug information (-ggdb -g3)\n"
"   gen-meta             Generate code using MetaDesk metaprogram\n"
"   help, -h, --help     Print this help message\n\n"
"ARGUMENTS:\n"
"   [TARGET]             Name of program in src/<name>.c (e.g. 'listen').\n"
"                        Defaults to 'all' (builds every program in src/).\n"
"   [CONFIG]             Build configuration: 'dev' (default) or 'release'.\n\n"
"EXAMPLES:\n"
"   ./build build                    Build all programs (dev)\n"
"   ./build build listen             Build 'listen' program (dev)\n"
"   ./build build listen release     Build 'listen' program (optimized release)\n"
"   ./build run listen               Run 'listen' program\n"
"   ./build build-run listen         Build and run 'listen'\n"
"   ./build build-debugger listen    Build 'listen' for debugger\n";

// ak: Functions
//=============================================================================

internal uint64_t build_run_list(Str8_List cmd_list);
internal uint64_t build_run_string(Str8 cmd);
internal Str8 build_path(Build_Info *info);
internal void build_log_tracef(Log_Level level, const char *format, ...);

#define BuildLog(level, format, ...) build_log_tracef(Log_Level_##level, (format), ##__VA_ARGS__)

// ak: Compilers functions ====================================================

internal void build_compile_gcc_style_flags(Build_Info *info)
{
    // ak: treat source code as c file not as precompiled header
    if (((info->flags & Build_Flag_Dynamic_Lib) || (info->flags & Build_Flag_Static_Lib))
        && !(info->flags &  Build_Flag_Cpp))
    {
        str8_list_pushf(info->arena, &info->cmd, " -x c");
    }
    str8_list_pushf(info->arena, &info->cmd, " %s8", info->entry_point);
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
    // ak: add version
    str8_list_pushf(info->arena, &info->cmd, " -DBUILD_VERSION='\"%s\"'", BUILD_VERSION);
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
    else
    {
        BuildLog(Error, "OS build compile is not supported.");
    }
}

// ak: Build run functions ====================================================

internal uint64_t build_run_program(Build_Info *info)
{
    fmt_println(str8("# Running ------------------------------------------------------------------- #"));
    Str8_List list = STRUCT_ZERO;
    if (info->flags & Build_Flag_MingW)
    {
        str8_list_pushf(info->arena, &list, "WINEARCH=win64 wine ");
    }
    str8_list_push(info->arena, &list, build_path(info));
    str8_list_pushf(info->arena, &list, " %.*s", str8_varg(info->args));
    return build_run_list(list);
}

// ak: Program Specific Compile Functions ======================================

internal uint64_t build_compile_miniaudio(Build_Info *info)
{
    uint64_t result = 0;
    
    // ak: create build
    Str8 path = build_path(info);
    if (!fs_file_path_exists(path))
    {
        if (info->flags & Build_Flag_Dynamic_Lib)
        {
            fmt_println(str8("# Compiling miniaudio Dynamic Library --------------------------------------- #"));
        }
        else
        {
            fmt_println(str8("# Compiling miniaudio Static Library ---------------------------------------- #"));
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
        result = build_run_list(info->cmd);
        if ((result == 0) && (info->flags & Build_Flag_Static_Lib))
        {
            result = build_run_string(str8("ar rcs build/libminiaudio.a build/libminiaudio_for_static.so"));
        }
    }
    return result;
}

internal uint64_t build_compile_run_metadesk(void)
{
    uint64_t result = 0;
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    Build_Info info = STRUCT_ZERO;
    info.type = Build_Type_Debug;
    info.name = str8(MDA_CMD_NAME);
    info.entry_point = str8("std/metadesk/metadesk_app_main.c");
    info.args = str8("std src");
    info.arena = scratch.arena;
    
    fmt_println(str8("# Compiling MetaDesk ------------------------------------------------------- #"));
    build_compile(&info);
    result = build_run_list(info.cmd);
    if (result == 0)
    {
        result = build_run_program(&info);
    }
    
    arena_scratch_end(scratch);
    return result;
}

internal uint64_t build_compile_program(Build_Info *info, Build_Info *ma_info)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    fmt_println(str8("# Compiling Program --------------------------------------------------------- #"));
    uint64_t result = 0;
    info->entry_point = str8f(scratch.arena, "src/%s8.c", info->name);
    info->arena = scratch.arena;
    build_compile(info);
    if (!(info->flags & Build_Flag_DryRun))
    {
        // ak: libs
        if ((info->flags & Build_Flag_MingW) || Context_Os_CURRENT == Context_Os_Windows)
        {
            str8_list_pushf(scratch.arena, &info->cmd, " -lopengl32 -luser32 -lgdi32");
        }
        else
        {
            str8_list_pushf(scratch.arena, &info->cmd, " -lm -lpthread");
            str8_list_pushf(scratch.arena, &info->cmd, " -lxcb -lxcb-image -lxcb-sync -lxcb-keysyms -lxcb-cursor");
            str8_list_pushf(scratch.arena, &info->cmd, " -lEGL -lGL");
            if (info->type == Build_Type_Release)
            {
                str8_list_pushf(scratch.arena, &info->cmd, " build/libminiaudio.a");
            }
            else
            {
                str8_list_pushf(scratch.arena, &info->cmd, " %s8", build_path(ma_info));
            }
        }
    }
    result = build_run_list(info->cmd);
    arena_scratch_end(scratch);
    return result;
}

// ak: Build Entry Point ======================================================

internal uint64_t base_main(void)
{
    Arena *arena = arena_alloc();
    
    Build_Info info = STRUCT_ZERO;
    
    bool should_print_help = false;
    bool run_program = false;
    bool only_run_program = false;
    bool gen_meta_program = false;
    uint64_t exit_code = 0;
    Str8 app_name = STRUCT_ZERO;
    
    Str8_Array *args = term_args_get();
    if (args->length >= 2)
    {
        Str8 arg1 = args->v[1];
        Str8 arg2 = STRUCT_ZERO;
        Str8 arg3 = STRUCT_ZERO;
        
        if (args->length >= 3)
        { info.name = args->v[2]; }
        else
        { info.name = str8("all"); }
        
        if (args->length >= 4)
        {
            arg3 = args->v[3];
        }
        if (
            str8_match(arg1, str8("help"), 0) ||
            str8_match(arg1, str8("--help"), 0) ||
            str8_match(arg1, str8("-h"), 0))
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
            fmt_printf("%s", help_message);
            BuildLog(Error, "wrong option provided `%s8`.", arg1);
            exit_code = 1;
        }
    }
    else
    {
        should_print_help = true;
    }
    if (args->length >= 3 && str8_match(args->v[2], str8("mingw"), 0))
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
        if (str8_match(info.name, str8("all"), 0))
        {
            BuildLog(Error, "please specify a single program name.");
        }
        else
        {
            exit_code = build_run_string(build_path(&info));
        }
    }
    else if (exit_code == 0)
    {
        fmt_println(str8("# Build Output ============================================================== #"));
        
        // ak: create build directory if not exists
        if (fs_dir_make(str8(BUILD_DIR)))
        {
            BuildLog(Info, "created '"BUILD_DIR"' directory.\n");
        }
        
        // ak: build miniaudio lib
        Build_Info ma_info = STRUCT_ZERO;
        ma_info.name = str8("libminiaudio");
        ma_info.entry_point = str8("std/audio/external/miniaudio.h");
        ma_info.type = info.type;
        ma_info.flags = info.type == Build_Type_Release ? Build_Flag_Static_Lib : Build_Flag_Dynamic_Lib;
        ma_info.arena = arena;
        exit_code = build_compile_miniaudio(&ma_info);
        if (exit_code == 0)
        {
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
            Str8 name = info.name;
            if (str8_match(name, str8("all"), 0))
            {
                Fs_Walk *walk = fs_walk_begin(arena, str8(BUILD_SRC_DIR), 0);
                for (Fs_Info fs_info = STRUCT_ZERO; fs_walk_next(arena, walk, &fs_info);)
                {
                    if (fs_info.props.flags & Fs_Property_Flag_IsFolder)
                    {
                        continue;
                    }
                    else
                    {
                        info.name = str8_chop_last_dot(fs_info.name);
                        exit_code = build_compile_program(&info, &ma_info);
                        if (exit_code != 0)
                        {
                            break;
                        }
                    }
                }
                fs_walk_end(walk);
            }
            else
            {
                exit_code = build_compile_program(&info, &ma_info);
            }
            if (str8_match(name, str8("all"), 0) && run_program)
            {
                fmt_printf("%s", help_message);
                BuildLog(Error, "please specify a single program name.");
                exit_code = 1;
            }
            else if (exit_code == 0 && run_program)
            {
                exit_code = build_run_program(&info);
            }
        }
    }
    arena_free(arena);
    return exit_code;
}

// ak: Build functions ========================================================

internal uint64_t build_run_list(Str8_List cmd_list)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    Str8 cmd = str8_list_join(scratch.arena, &cmd_list, 0);
    uint64_t exit_code = build_run_string(cmd);
    arena_scratch_end(scratch);
    return exit_code;
}

internal uint64_t build_run_string(Str8 cmd)
{
    uint64_t exit_code = 0;
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    fmt_printf("COMMAND: %s8\n", cmd);
    Process process = process_launch_from_cmd(cmd);
    if (process_is_zero(process))
    {
        BuildLog(Error, "failed to launch process.");
        exit_code = 1;
    }
    else
    {
        bool joined = process_join(process, max_u64, &exit_code);
        if (!joined)
        {
            BuildLog(Error, "failed waiting for process to finish.");
        }
    }
    
    arena_scratch_end(scratch);
    return exit_code;
}

internal Str8 build_path(Build_Info *info)
{
    Str8_List list = STRUCT_ZERO;
    if (Context_Os_CURRENT == Context_Os_Windows)
    {
        str8_list_pushf(info->arena, &list, BUILD_DIR"\\%s8", info->name);
    }
    else
    {
        str8_list_pushf(info->arena, &list, BUILD_DIR"/%s8", info->name);
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

internal void build_log_tracef(Log_Level level, const char *format, ...)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    va_list args;
    va_start(args, format);
    {
        Str8 level_string = log_level_string_get(level);
        Fs_File file = std_type_from_log_level(level);
        Str8 string = str8fv(scratch.arena, format, args);
        fmt_fprintf(file, "%-*s ", str8_varg(level_string));
        fmt_fprintln(file, string);
    }
    va_end(args);
    arena_scratch_end(scratch);
}
