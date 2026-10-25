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
    Build_Flag_Zig          = (1<<5),
};

typedef struct Build_Info Build_Info;
struct Build_Info
{
    Arena *arena;
    Str8_List cmd;
    Str8 name;
    Str8 entry_point;
    Str8 args;
    Str8 zig_args;
    Build_Type type;
    Build_Flags flags;
};



// ak: Functions
//=============================================================================

internal uint64_t build_run_list(Str8_List cmd_list);
internal uint64_t build_run_string(Str8 cmd);
internal Str8 build_path(Build_Info *info);
internal Str8 build_version_from_type(Build_Type type, Arena *arena);
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
    bool is_arm = (str8_find_substr(info->zig_args, 0, str8("aarch64"), 0) < info->zig_args.length ||
                   str8_find_substr(info->zig_args, 0, str8("arm"), 0) < info->zig_args.length);
    if (is_arm)
    {
        str8_list_pushf(info->arena, &info->cmd, " -fstack-protector");
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd, " -mshstk -fcf-protection=full -fstack-protector");
    }
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
    str8_list_pushf(info->arena, &info->cmd, " -DBUILD_VERSION='\"%s8\"'", build_version_from_type(info->type, info->arena));
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

internal void build_compile_zig(Build_Info *info)
{
    if (info->flags & Build_Flag_Cpp)
    {
        str8_list_pushf(info->arena, &info->cmd, "zig c++");
    }
    else
    {
        str8_list_pushf(info->arena, &info->cmd, "zig cc");
    }
    if (info->zig_args.length > 0)
    {
        str8_list_pushf(info->arena, &info->cmd, " %s8", info->zig_args);
    }
    build_compile_gcc_style_flags(info);
}

internal void build_compile(Build_Info *info)
{
    if (info->flags & Build_Flag_Zig)
    {
        build_compile_zig(info);
    }
    else if (Context_Compiler_CURRENT == Context_Compiler_Gcc)
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
    if (info->args.length > 0)
    {
        str8_list_pushf(info->arena, &list, " %.*s", str8_varg(info->args));
    }
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
    Arena *orig_arena = info->arena;
    fmt_println(str8("# Compiling Program --------------------------------------------------------- #"));
    uint64_t result = 0;
    info->entry_point = str8f(scratch.arena, "src/%s8.c", info->name);
    info->arena = scratch.arena;
    info->cmd = StructZeroType(Str8_List);
    build_compile(info);
    if (!(info->flags & Build_Flag_DryRun))
    {
        // ak: libs
        if ((info->flags & Build_Flag_MingW) || Context_Os_CURRENT == Context_Os_Windows)
        {
            str8_list_pushf(scratch.arena, &info->cmd, " -lopengl32 -luser32 -lgdi32");
        }
        else if (info->flags & Build_Flag_Zig)
        {
            str8_list_pushf(scratch.arena, &info->cmd, " -DMINIAUDIO_IMPLEMENTATION -lm -lpthread -ldl");
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
    info->cmd = StructZeroType(Str8_List);
    info->arena = orig_arena;
    arena_scratch_end(scratch);
    return result;
}

// ak: Build Entry Point ======================================================

global Str8 help_message = str8(
"DESCRIPTION:\n"
"   Build driver for compiling and running C projects.\n\n"
"USAGE:\n"
"   build [COMMAND] [TARGET] [CONFIG] [-- <args>]\n\n"
"COMMANDS:\n"
"   build                  Compile target program(s) (default)\n"
"   run                    Run target program (builds first if not present)\n"
"   build-run              Compile and run target program\n"
"   build-dry              Check syntax without producing output files\n"
"   build-debugger         Compile with full debug information (-ggdb -g3)\n"
"   gen-meta               Generate code using MetaDesk metaprogram\n"
"   help, -h, --help       Print this help message\n\n"
"   version, -v, --version Print this help message\n\n"
"ARGUMENTS:\n"
"   [TARGET]             Name of program in src/<name>.c (e.g. 'listen').\n"
"                        Defaults to 'all' (builds every program in src/).\n"
"   [CONFIG]             Build configuration: 'dev' (default) or 'release'.\n"
"   [-- <args>]          Pass all trailing arguments directly to target program.\n\n"
"EXAMPLES:\n"
"   ./build build                         Build all programs (dev)\n"
"   ./build build listen                  Build 'listen' program (dev)\n"
"   ./build build listen release          Build 'listen' program (optimized release)\n"
"   ./build run listen                    Run 'listen' program\n"
"   ./build run listen -- --help          Run 'listen' with arguments\n"
"   ./build build-run listen              Build and run 'listen'\n"
"   ./build build-run listen -- --help    Build and run 'listen' with arguments\n"
"   ./build build-debugger listen         Build 'listen' for debugger\n"
);

internal uint64_t base_main(void)
{
    Arena *arena = arena_alloc();
    
    Build_Info info = STRUCT_ZERO;
    info.arena = arena;
    
    bool should_print_help = false;
    bool should_print_version = false;
    bool run_program = false;
    bool only_run_program = false;
    bool gen_meta_program = false;
    uint64_t exit_code = 0;
    
    Str8 cmd_arg = STRUCT_ZERO;
    Str8 target_arg = STRUCT_ZERO;
    Str8 config_arg = STRUCT_ZERO;
    Str8 custom_args = STRUCT_ZERO;
    Str8 zig_args = STRUCT_ZERO;
    
    Str8_Array *args = term_args_get();
    for (uint64_t i = 1; i < args->length; i++)
    {
        Str8 arg = args->v[i];
        
        // ak: pass all trailing arguments directly with --
        if (str8_match(arg, str8("--"), 0))
        {
            for (uint64_t j = i + 1; j < args->length; j++)
            {
                if (custom_args.length == 0)
                {
                    custom_args = args->v[j];
                }
                else
                {
                    custom_args = str8f(arena, "%s8 %s8", custom_args, args->v[j]);
                }
            }
            break;
        }
        
        // ak: check for zig flag
        if (str8_match(arg, str8("--zig"), 0))
        {
            info.flags |= Build_Flag_Zig;
            for (uint64_t j = i + 1; j < args->length; j++)
            {
                if (zig_args.length == 0)
                {
                    zig_args = args->v[j];
                }
                else
                {
                    zig_args = str8f(arena, "%s8 %s8", zig_args, args->v[j]);
                }
            }
            break;
        }

        if (str8_match(arg, str8("--mingw"), 0))
        {
            info.flags |= Build_Flag_MingW;
            continue;
        }
        
        if (cmd_arg.length == 0)
        {
            cmd_arg = arg;
        }
        else if (target_arg.length == 0)
        {
            target_arg = arg;
        }
        else if (config_arg.length == 0)
        {
            config_arg = arg;
        }
        else
        {
            BuildLog(Error, "unexpected argument `%s8`.", arg);
            exit_code = 1;
        }
    }
    
    if (exit_code == 0)
    {
        if (cmd_arg.length == 0 ||
            str8_match(cmd_arg, str8("help"), 0) ||
            str8_match(cmd_arg, str8("--help"), 0) ||
            str8_match(cmd_arg, str8("-h"), 0))
        {
            should_print_help = true;
        }
        else if (cmd_arg.length == 0 ||
            str8_match(cmd_arg, str8("version"), 0) ||
            str8_match(cmd_arg, str8("--version"), 0) ||
            str8_match(cmd_arg, str8("-v"), 0))
        {
            should_print_version = true;
        }
        else
        {
            info.name = (target_arg.length > 0) ? target_arg : str8("all");
            
            if (str8_match(cmd_arg, str8("build"), 0))
            {
                if (str8_match(config_arg, str8("release"), 0))
                {
                    info.type = Build_Type_Release;
                }
                else
                {
                    info.type = Build_Type_Dev;
                }
            }
            else if (str8_match(cmd_arg, str8("build-run"), 0))
            {
                if (str8_match(config_arg, str8("release"), 0))
                {
                    info.type = Build_Type_Release;
                }
                else
                {
                    info.type = Build_Type_Dev;
                }
                run_program = true;
            }
            else if (str8_match(cmd_arg, str8("run"), 0))
            {
                if (str8_match(config_arg, str8("release"), 0))
                {
                    info.type = Build_Type_Release;
                }
                else
                {
                    info.type = Build_Type_Dev;
                }
                only_run_program = true;
            }
            else if (str8_match(cmd_arg, str8("build-dry"), 0))
            {
                info.type = Build_Type_Dev;
                info.flags |= Build_Flag_DryRun;
            }
            else if (str8_match(cmd_arg, str8("build-debugger"), 0))
            {
                info.type = Build_Type_Debug;
            }
            else if (str8_match(cmd_arg, str8("gen-meta"), 0))
            {
                gen_meta_program = true;
            }
            else
            {
                fmt_print(help_message);
                BuildLog(Error, "wrong option provided `%s8`.", cmd_arg);
                exit_code = 1;
            }
        }
    }
    
    info.args = custom_args;
    info.zig_args = zig_args;
    
    if (only_run_program || run_program)
    {
        if (str8_match(info.name, str8("all"), 0))
        {
            fmt_print(help_message);
            BuildLog(Error, "please specify a single program name.");
            exit_code = 1;
        }
        else if (only_run_program && !fs_file_path_exists(build_path(&info)))
        {
            only_run_program = false;
            run_program = true;
        }
    }
    
    if (should_print_help)
    {
        fmt_print(help_message);
    }
    else if (should_print_version)
    {
        fmt_print(str8(BUILD_VERSION));
    }
    else if (exit_code != 0)
    {
        // ak: error already reported
    }
    else if (only_run_program)
    {
        exit_code = build_run_program(&info);
    }
    else
    {
        fmt_println(str8("# Build Output ============================================================== #"));
        
        // ak: create build directory if not exists
        if (fs_dir_make(str8(BUILD_DIR)))
        {
            BuildLog(Info, "created '"BUILD_DIR"' directory.\n");
        }
        
        // ak: build miniaudio lib (skip when compiling with zig)
        Build_Info ma_info = STRUCT_ZERO;
        if (!(info.flags & Build_Flag_Zig))
        {
            ma_info.name = str8("libminiaudio");
            if (fs_file_path_exists(str8("std/audio/vendor/miniaudio.h")))
            {
                ma_info.entry_point = str8("std/audio/vendor/miniaudio.h");
            }
            else
            {
                ma_info.entry_point = str8("std/audio/external/miniaudio.h");
            }
            ma_info.type = info.type;
            ma_info.flags = info.type == Build_Type_Release ? Build_Flag_Static_Lib : Build_Flag_Dynamic_Lib;
            ma_info.arena = arena;
            exit_code = build_compile_miniaudio(&ma_info);
            if (exit_code == 0)
            {
                exit_code = build_compile_miniaudio(&ma_info);
            }
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
                fmt_print(help_message);
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

internal Str8 build_type_string_from_type(Build_Type type)
{
    Str8 result = STRUCT_ZERO;
    switch (type)
    {
        case Build_Type_Dev:
        {
            result = str8("dev");
        }
        break;
        case Build_Type_Debug:
        {
            result = str8("debug");
        }
        break;
        case Build_Type_Release:
        {
            result = str8("release");
        }
        break;
    }
    return result;
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
    
    if (info->type != Build_Type_Release)
    {
        str8_list_pushf(info->arena, &list, "_%s8", build_type_string_from_type(info->type));
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
    } else if (Context_Os_CURRENT == Context_Os_Windows || (info->flags & Build_Flag_MingW))
    {
        str8_list_pushf(info->arena, &list, ".exe");
    }
    
    Str8 result = str8_list_join(info->arena, &list, NULL);
    return result;
}

internal Str8 build_version_from_type(Build_Type type, Arena *arena)
{
    return str8f(arena, "%s8_%s", build_type_string_from_type(type), BUILD_VERSION);
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
