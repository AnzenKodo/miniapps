//~ ak: Includes
//=============================================================================

//- ak: headers
#include "../base/base_include.h"
#include "../os/os_include.h"
#include "./metadesk.h"
#include "./metadesk_gen.h"
#include "./metadesk_app.h"

//- ak: implementation
#include "../base/base_include.c"
#include "../os/os_include.c"
#include "./metadesk.c"
#include "./metadesk_gen.c"
#include <stdio.h>

internal void print_help_message(void)
{
    fmt_println("DESCRIPTION:");
    fmt_println("    "MDA_NAME" - "MDA_DESCRIPTION);
    fmt_println("USAGE:");
    fmt_printfln("   "MDA_CMD_NAME" [PATH] [OPTIONS]");
    fmt_println("OPTIONS:");
    flags_print_help();
    fmt_println("VERSION:");
    fmt_println("    "MDA_VERSION);
}

void base_main(void)
{
    Arena *arena = arena_alloc();
    Str8 src_path = str8("./src");
    Str8 defulat_gen_dirname = str8("generated");
    
    //- ak: Command Line ======================================================
    FlagsScope()
    {
        Flags_Option *option = NULL;
        Flags_Arg *arg = flags_arg_str(&src_path, src_path);
        flags_make_arg_required(arg);
        option = flags_option_str(str8("gen-dir"), &defulat_gen_dirname, defulat_gen_dirname, str8("Set generated directory name"));
        bool help = false;
        option = flags_option_bool(str8("help"), &help, help, str8("Print help message"));
        flags_add_option_shortname(option, str8("h"));
        bool version = false;
        option = flags_option_bool(str8("version"), &version, version, str8("Print version message"));
        flags_add_option_shortname(option, str8("v"));
        Str8_Array *args = os_args_get();
        if (!flags_parse(args))
        {
            flags_print_error();
            fmt_print("\n");
            print_help_message();
            os_exit(1);
        }
        if (help)
        {
            print_help_message();
            os_exit(0);
        }
        if (version)
        {
            fmt_print("v"MDA_VERSION);
            os_exit(0);
        }
    }
    
    //- ak: initialization ====================================================
    if (str8_ends_with(src_path, str8("/")) ||
        str8_ends_with(src_path, str8("\\")))
    {
        src_path = str8_chop_last_slash(src_path);
    }
    Str8 ext_name = str8("mdesk");
    MDG_Msg_List msgs = STRUCT_ZERO;
    MDG_State *state = mdg_state_init(256, arena);
    Log_Context log = log_init();
    
    //- ak: collect file paths ================================================
    Str8_List file_paths = STRUCT_ZERO;
    log_infof(&log, "Searching %s8...", src_path);
    {
        typedef struct Dir Dir;
        struct Dir
        {
            Dir *next;
            Str8 src_path;
        };
        Dir start_dir = {0, src_path};
        Dir *first_dir = &start_dir;
        Dir *last_dir = &start_dir;
        for (Dir *dir = first_dir; dir != NULL; dir = dir->next)
        {
            Os_File_Walk *walk = os_file_walk_begin(arena, dir->src_path, 0);
            for (Os_File_Info info = STRUCT_ZERO; os_file_walk_next(arena, walk, &info);)
            {
                Str8 file_path = str8f(arena, "%.*s/%.*s", str8_varg(dir->src_path), str8_varg(info.name));
                if (info.props.flags & Os_File_Property_Flag_IsFolder)
                {
                    Dir *next_dir = arena_push(arena, Dir, 1);
                    SLLQueuePush(first_dir, last_dir, next_dir);
                    next_dir->src_path = file_path;
                }
                else
                {
                    str8_list_push(arena, &file_paths, file_path);
                }
            }
            os_file_walk_end(walk);
        }
    }
    fmt_fprintfln(log.file, " %zu directory found", file_paths.length);
    
    //- ak: parse all metatable files =========================================
    log_info(&log, "Parsing metatable...");
    MDG_ParsedFile_List parses = STRUCT_ZERO;
    {
        for (Str8_Node *node = file_paths.first; node != NULL; node = node->next)
        {
            Str8 file_path = node->str;
            Str8 file_ext = str8_skip_last_dot(file_path);
            if (str8_match(file_ext, ext_name, 0))
            {
                Str8 source = os_path_read_str_full(file_path, arena);
                MD_Parse parse = md_parse_from_string(source, file_path, arena);
                for (MD_Msg *m = parse.msgs.first; m != NULL; m = m->next)
                {
                    Log_Level msg_level = Log_Level_None;
                    switch (m->level)
                    {
                        case MD_Msg_Level_None:  {}break;
                        case MD_Msg_Level_Info:  {msg_level = Log_Level_Info;}break;
                        case MD_Msg_Level_Debug: {msg_level = Log_Level_Debug;}break;
                        case MD_Msg_Level_Warn:  {msg_level = Log_Level_Warn;}break;
                        case MD_Msg_Level_Error: {msg_level = Log_Level_Error;}break;
                    }
                    Txt_Pt pt = str8_offset_to_txt_pt(source, m->node->src_offset);
                    MDG_Msg dst_m = {msg_level, pt, file_path, m->string};
                    mdg_msg_list_push(&msgs, &dst_m, arena);
                }
                MDG_ParsedFile_Node *parse_n = arena_push(arena, MDG_ParsedFile_Node, 1);
                SLLQueuePush(parses.first, parses.last, parse_n);
                parse_n->v.root = parse.root;
                parses.count += 1;
            }
        }
    }
    fmt_fprintfln(log.file, " %zu .%.*s files parsed", parses.count, str8_varg(ext_name));
    
    //- ak: gather tables =====================================================
    MDG_Map table_grid_map = mdg_map_push(1024, arena);
    MDG_Map table_col_map = mdg_map_push(1024, arena);
    size_t table_count = 0;
    log_info(&log, "Gathering tables...");
    {
        for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
        {
            MD_Node *file = node->v.root;
            for (MD_Node *n = file->first; !md_node_is_nil(n); n = n->next)
            {
                MD_Node *table_tag = md_tag_from_string(n, str8("table"), 0);
                if (!md_node_is_nil(table_tag))
                {
                    MDG_Node_Grid *table = arena_push(arena, MDG_Node_Grid, 1);
                    MDG_Column_Desc_Array *col_descs = arena_push(arena, MDG_Column_Desc_Array, 1);
                    *table = mdg_node_grid_make_from_node(n, arena);
                    *col_descs = mdg_column_desc_array_from_tag(table_tag, arena);
                    mdg_map_insert_ptr(&table_grid_map, n->string, table, arena);
                    mdg_map_insert_ptr(&table_col_map, n->string, col_descs, arena);
                    table_count += 1;
                }
            }
        }
    }
    fmt_fprintfln(log.file, " %zu tables found", table_count);
    
    //- ak: gather layer options ==============================================
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
        MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
        layer->src_path = file->string;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            if (md_node_has_tag(md_node, str8("option"), 0))
            {
                if (str8_match(md_node->string, str8("library"), 0))
                {
                    layer->is_library = 1;
                }
            }
            if (md_node_has_tag(md_node, str8("gen_folder"), 0))
            {
                layer->gen_folder_name = md_node->string;
            }
            if (md_node_has_tag(md_node, str8("h_name"), 0))
            {
                layer->h_name_override = md_node->string;
            }
            if (md_node_has_tag(md_node, str8("c_name"), 0))
            {
                layer->c_name_override = md_node->string;
            }
            if (md_node_has_tag(md_node, str8("h_header"), 0))
            {
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_push(arena, &layer->h_header, n->str);
                    str8_list_push(arena, &layer->h_header, str8("\n"));
                }
            }
            if (md_node_has_tag(md_node, str8("h_footer"), 0))
            {
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_push(arena, &layer->h_footer, n->str);
                    str8_list_push(arena, &layer->h_footer, str8("\n"));
                }
            }
            if (md_node_has_tag(md_node, str8("c_header"), 0))
            {
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_push(arena, &layer->c_header, n->str);
                    str8_list_push(arena, &layer->c_header, str8("\n"));
                }
            }
            if (md_node_has_tag(md_node, str8("c_footer"), 0))
            {
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_push(arena, &layer->c_footer, n->str);
                    str8_list_push(arena, &layer->c_footer, str8("\n"));
                }
            }
        }
    }
    
    //- ak: generate enums
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            MD_Node *tag = md_tag_from_string(md_node, str8("enum"), 0);
            if (!md_node_is_nil(tag))
            {
                Str8 enum_name = md_node->string;
                Str8 enum_member_prefix = enum_name;
                if (str8_match(str8_postfix(enum_name, 5), str8("Flags"), 0))
                {
                    enum_member_prefix = str8_chop(enum_name, 1);
                }
                Str8 enum_base_type_name = tag->first->string;
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                if (enum_base_type_name.size == 0)
                {
                    str8_list_pushf(arena, &layer->enums, "typedef enum %.*s\n{\n", str8_varg(enum_name));
                }
                else
                {
                    str8_list_pushf(arena, &layer->enums, "typedef %.*s %.*s;\n", str8_varg(enum_base_type_name), str8_varg(enum_name));
                    str8_list_pushf(arena, &layer->enums, "typedef enum %.*sEnum\n{\n", str8_varg(enum_name));
                }
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_pushf(arena, &layer->enums, "    %.*s_%.*s,\n", str8_varg(enum_member_prefix), str8_varg(n->str));
                }
                if (enum_base_type_name.size == 0)
                {
                    str8_list_pushf(arena, &layer->enums, "}\n%.*s;\n\n", str8_varg(enum_name));
                }
                else
                {
                    str8_list_pushf(arena, &layer->enums, "}\n%.*sEnum;\n\n", str8_varg(enum_name));
                }
            }
        }
    }
    
    //- ak: generate xlists
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            MD_Node *tag = md_tag_from_string(md_node, str8("xlist"), 0);
            if (!md_node_is_nil(tag))
            {
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                str8_list_pushf(arena, &layer->enums, "#define %.*s \\\n", str8_varg(md_node->string));
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_pushf(arena, &layer->enums, "X(%.*s)\\\n", str8_varg(n->str));
                }
                str8_list_push(arena, &layer->enums, str8("\n"));
            }
        }
    }
    
    //- ak: generate structs
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            if (md_node_has_tag(md_node, str8("struct"), 0))
            {
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                str8_list_pushf(arena, &layer->structs, "typedef struct %.*s %.*s;\n",
                    str8_varg(md_node->string), str8_varg(md_node->string));
                str8_list_pushf(arena, &layer->structs, "struct %.*s\n{\n", str8_varg(md_node->string));
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_pushf(arena, &layer->structs, "%.*s;\n", str8_varg(n->str));
                }
                str8_list_pushf(arena, &layer->structs, "};\n\n");
            }
        }
    }
    
    //- ak: generate data tables
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            MD_Node *tag = md_tag_from_string(md_node, str8("data"), 0);
            if (!md_node_is_nil(tag))
            {
                Str8 element_type = tag->first->string;
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                if (!md_node_has_tag(md_node, str8("c_file"), 0))
                {
                    str8_list_pushf(arena, &layer->h_tables, "extern %.*s %.*s[%zu];\n",
                        str8_varg(element_type), str8_varg(md_node->string), gen_strings.length);
                }
                str8_list_pushf(arena, &layer->c_tables, "%.*s %.*s[%zu] =\n{\n",
                    str8_varg(element_type), str8_varg(md_node->string), gen_strings.length);
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_pushf(arena, &layer->c_tables, "    %.*s,\n", str8_varg(n->str));
                }
                str8_list_push(arena, &layer->c_tables, str8("};\n\n"));
            }
        }
    }
            
    //- ak: generate enum -> string mapping functions
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            MD_Node *tag = md_tag_from_string(md_node, str8("enum2string_switch"), 0);
            if (!md_node_is_nil(tag))
            {
                Str8 enum_type = tag->first->string;
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                str8_list_pushf(arena, &layer->h_functions, "internal Str8 %.*s(%.*s v);\n",
                    str8_varg(md_node->string), str8_varg(enum_type));
                str8_list_pushf(arena, &layer->c_functions, "internal Str8\n%.*s(%.*s v)\n{\n",
                    str8_varg(md_node->string), str8_varg(enum_type));
                str8_list_pushf(arena, &layer->c_functions, "Str8 result = str8(\"<Unknown %.*s>\");\n", str8_varg(enum_type));
                str8_list_pushf(arena, &layer->c_functions, "switch(v)\n");
                str8_list_pushf(arena, &layer->c_functions, "{\n");
                str8_list_pushf(arena, &layer->c_functions, "default:{}break;\n");
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    str8_list_pushf(arena, &layer->c_functions, "%.*s;\n", str8_varg(n->str));
                }
                str8_list_pushf(arena, &layer->c_functions, "}\n");
                str8_list_pushf(arena, &layer->c_functions, "return result;\n");
                str8_list_pushf(arena, &layer->c_functions, "}\n\n");
            }
        }
    }
    
    //- ak: generate catch-all generations
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            MD_Node *tag = md_tag_from_string(md_node, str8("gen"), 0);
            if (!md_node_is_nil(tag))
            {
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                bool prefer_c_file = md_node_has_tag(md_node, str8("c_file"), 0);
                Str8_List *out = prefer_c_file ? &layer->c_catchall : &layer->h_catchall;
                if (tag->first->string.size == 0)
                {
                }
                else if (str8_match(tag->first->string, str8("enums"), 0))
                {
                    out = &layer->enums;
                }
                else if (str8_match(tag->first->string, str8("structs"), 0))
                {
                    out = &layer->structs;
                }
                else if (str8_match(tag->first->string, str8("functions"), 0))
                {
                    out = prefer_c_file ? &layer->c_functions : &layer->h_functions;
                }
                else if (str8_match(tag->first->string, str8("tables"), 0))
                {
                    out = prefer_c_file ? &layer->c_tables : &layer->h_tables;
                }
                Str8_List gen_strings = mdg_str_list_from_table_gen(table_grid_map, table_col_map, str8(""), md_node, arena);
                for (Str8_Node *n = gen_strings.first; n != NULL; n = n->next)
                {
                    Str8 trimmed = str8_skip_chop_whitespace(n->str);
                    str8_list_push(arena, out, trimmed);
                    str8_list_push(arena, out, str8("\n"));
                }
            }
        }
    }
    
    //- ak: gather & generate all embeds
    for (MDG_ParsedFile_Node *node = parses.first; node != NULL; node = node->next)
    {
        MD_Node *file = node->v.root;
        for (MD_Node *md_node = file->first; !md_node_is_nil(md_node); md_node = md_node->next)
        {
            if (md_node_has_tag(md_node, str8("embed_string"), 0))
            {
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8 embed_string = mdg_c_string_literal_from_multiline_string(md_node->first->string, arena);
                str8_list_pushf(arena, &layer->h_tables,
                    "read_only global Str8 %.*s =\nstr8_comp(\n", str8_varg(md_node->string));
                str8_list_push (arena, &layer->h_tables, embed_string);
                str8_list_pushf(arena, &layer->h_tables, ");\n\n");
            }
            if (md_node_has_tag(md_node, str8("embed_file"), 0))
            {
                Str8 layer_key = mdg_layer_key_from_path(file->string, src_path, arena);
                MDG_Layer *layer = mdg_layer_from_key(state, layer_key, arena);
                Str8 string = os_path_read_str_full(md_node->first->string, arena);
                Str8 embed_string = mdg_c_array_literal_contents_from_string(string, arena);
                str8_list_pushf(arena, &layer->h_tables,
                    "read_only global uint8_t %.*s__data[] =\n{\n", str8_varg(md_node->string));
                str8_list_push (arena, &layer->h_tables, embed_string);
                str8_list_pushf(arena, &layer->h_tables, "};\n\n");
                str8_list_pushf(arena, &layer->h_tables,
                    "read_only global Str8 %.*s = {%.*s__data, sizeof(%.*s__data), sizeof(%.*s__data)};\n",
                    str8_varg(md_node->string), str8_varg(md_node->string), str8_varg(md_node->string), str8_varg(md_node->string));
            }
        }
    }
    
    //- ak: write all layer output files ======================================
    log_infoln(&log, "Generating layer code:");
    for (size_t slot_idx = 0; slot_idx < state->slots_count; slot_idx += 1)
    {
        MDG_Layer_Slot *slot = &state->slots[slot_idx];
        for (MDG_Layer_Node *mdg_layer_node = slot->first; mdg_layer_node != NULL; mdg_layer_node = mdg_layer_node->next)
        {
            MDG_Layer *layer = &mdg_layer_node->v;
            Str8 layer_generated_folder = STRUCT_ZERO;
            if (layer->gen_folder_name.size != 0)
            {
                Str8 gen_folder = layer->gen_folder_name;
                layer_generated_folder = str8f(arena, "%.*s/%.*s", str8_varg(src_path), str8_varg(gen_folder));
            }
            else
            {
                Str8 gen_folder = defulat_gen_dirname;
                layer_generated_folder = str8f(arena, "%.*s/%.*s/%.*s", str8_varg(src_path), str8_varg(layer->key), str8_varg(gen_folder));
            }
            if (os_dir_ensure(layer_generated_folder))
            {
                Str8_List layer_key_parts = str8_split_path(arena, layer->key);
                Str_Join join = STRUCT_ZERO;
                join.sep = str8("_");
                Str8 layer_key_filename = str8_list_join(arena, &layer_key_parts, &join);
                Str8 layer_key_filename_upper = upper_from_str8(layer_key_filename, arena);
                Str8 h_path = str8f(arena, "%.*s/%.*s.meta.h", str8_varg(layer_generated_folder), str8_varg(layer_key_filename));
                Str8 c_path = str8f(arena, "%.*s/%.*s.meta.c", str8_varg(layer_generated_folder), str8_varg(layer_key_filename));
                if (layer->h_name_override.size != 0)
                {
                    h_path = str8f(arena, "%.*s/%.*s", str8_varg(layer_generated_folder), str8_varg(str8_skip_last_slash(layer->h_name_override)));
                }
                if (layer->c_name_override.size != 0)
                {
                    c_path = str8f(arena, "%.*s/%.*s", str8_varg(layer_generated_folder), str8_varg(str8_skip_last_slash(layer->c_name_override)));
                }
                {
                    Os_File h_file = os_file_open(h_path, Os_AccessFlag_Write);
                    if (layer->h_header.first == NULL)
                    {
                        fmt_fprintfln(h_file, "//- GENERATED CODE\n");
                        fmt_fprintfln(h_file, "#ifndef %.*s_META_H", str8_varg(layer_key_filename_upper));
                        fmt_fprintfln(h_file, "#define %.*s_META_H\n", str8_varg(layer_key_filename_upper));
                    }
                    else for (Str8_Node *n = layer->h_header.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(h_file, n->str.cstr, n->str.size);
                    }
                    for (Str8_Node *n = layer->enums.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(h_file, n->str.cstr, n->str.size);
                    }
                    for (Str8_Node *n = layer->structs.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(h_file, n->str.cstr, n->str.size);
                    }
                    for (Str8_Node *n = layer->h_catchall.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(h_file, n->str.cstr, n->str.size);
                    }
                    for (Str8_Node *n = layer->h_functions.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(h_file, n->str.cstr, n->str.size);
                    }
                    if (layer->h_tables.first != NULL)
                    {
                        if (!layer->is_library)
                        {
                            fmt_fprintf(h_file, "C_LINKAGE_BEGIN\n");
                        }
                        for (Str8_Node *n = layer->h_tables.first; n != NULL; n = n->next)
                        {
                            os_file_write_append(h_file, n->str.cstr, n->str.size);
                        }
                        fmt_fprintf(h_file, "\n");
                        if (!layer->is_library)
                        {
                            fmt_fprintf(h_file, "C_LINKAGE_END\n\n");
                        }
                    }
                    if (layer->h_footer.first == NULL)
                    {
                        fmt_fprintf(h_file, "#endif // %.*s_META_H\n", str8_varg(layer_key_filename_upper));
                    }
                    else for (Str8_Node *n = layer->h_footer.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(h_file, n->str.cstr, n->str.size);
                    }
                    os_file_close(h_file);
                }
                {
                    Os_File c_file = os_file_open(c_path, Os_AccessFlag_Write);
                    if (layer->c_header.first == NULL)
                    {
                        fmt_fprintfln(c_file, "//- GENERATED CODE\n");
                    }
                    else for (Str8_Node *n = layer->c_header.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(c_file, n->str.cstr, n->str.size);
                    }
                    for (Str8_Node *n = layer->c_catchall.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(c_file, n->str.cstr, n->str.size);
                    }
                    if (layer->c_tables.first != NULL)
                    {
                        if (!layer->is_library)
                        {
                            fmt_fprintfln(c_file, "C_LINKAGE_BEGIN");
                        }
                        for (Str8_Node *n = layer->c_tables.first; n != NULL; n = n->next)
                        {
                            os_file_write_append(c_file, n->str.cstr, n->str.size);
                        }
                        if (!layer->is_library)
                        {
                            fmt_fprintfln(c_file, "C_LINKAGE_END\n");
                        }
                    }
                    for (Str8_Node *n = layer->c_functions.first; n != NULL; n = n->next)
                    {
                        os_file_write_append(c_file, n->str.cstr, n->str.size);
                    }
                    if (layer->c_footer.first != NULL)
                    {
                        for (Str8_Node *n = layer->c_footer.first; n != NULL; n = n->next)
                        {
                            os_file_write_append(c_file, n->str.cstr, n->str.size);
                        }
                    }
                    os_file_close(c_file);
                }
                log_infofln(&log, "    %.*s", str8_varg(layer->src_path));
                log_infofln(&log, "        ├─ %.*s", str8_varg(c_path));
                log_infofln(&log, "        └─ %.*s", str8_varg(h_path));
            }
        }
    }
    
    //- ak: write out all messages to stderr ==================================
    const char *file_info_color = log_get_file_info_color(&log);
    const char *restart_color   = log_get_reset_color(&log);
    for (MDG_Msg_Node *node = msgs.first; node != NULL; node = node->next)
    {
        MDG_Msg *msg = &node->v;
        fmt_eprintf("%s%.*s:%ld:%ld%s ", file_info_color, str8_varg(msg->file_path), msg->pt.line, msg->pt.column, restart_color);
        log_print_color_level(&log, msg->level);
        fmt_eprintf("%.*s\n", str8_varg(msg->string));
    }
}
