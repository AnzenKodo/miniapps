//~ ak: Handle Type Functions =================================================

internal bool process_match(Process a, Process b)
{
    bool result = MemMatchStruct(&a, &b);
    return result;
}

internal bool process_is_zero(Process p)
{
    bool result = process_match(p, StructZeroType(Process));
    return result;
}

internal void process_list_push(Arena *arena, Process_List *list, Process p)
{
    Process_Node *n = arena_push(arena, Process_Node, 1);
    SLLQueuePush(list->first, list->last, n);
    n->v = p;
    list->count += 1;
}

//~ ak: Process Launcher Helpers ==============================================

internal Process process_launch_from_cmd(Str8 string)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    
    // ak: split args
    Str8_List parts = STRUCT_ZERO;
    size_t i = 0;
    while (i < string.size)
    {
        // ak: skip leading spaces
        while (i < string.size && char_is_space(string.cstr[i]))
        {
            i += 1;
        }
        if (i >= string.size) break;
        
        // ak: collect one argument
        uint8_t *buf = arena_push(scratch.arena, uint8_t, string.size - i + 1);
        size_t len = 0;
        bool in_single = false;
        bool in_double = false;

        while (i < string.size)
        {
            uint8_t c = string.cstr[i];

            if (c == '\'' && !in_double)
            {
                in_single = !in_single;
                i += 1;
            }
            else if (c == '"' && !in_single)
            {
                in_double = !in_double;
                i += 1;
            }
            else if (c == '\\' && !in_single && (i + 1 < string.size))
            {
                // ak: escaped character: consume '\' and append next char literally
                i += 1;
                buf[len++] = string.cstr[i++];
            }
            else if (char_is_space(c) && !in_single && !in_double)
            {
                // ak: space outside quotes ends this token
                break;
            }
            else
            {
                buf[len++] = c;
                i += 1;
            }
        }
        buf[len] = 0;
        str8_list_push(scratch.arena, &parts, str8_init(buf, len));
    }
    
    // ak: initialize process
    Process process = STRUCT_ZERO;
    if(parts.length != 0)
    {
        // ak: find stdout delimiter
        Str8_Node *stdout_delimiter_n = 0;
        for(Str8_Node *n = parts.first; n != 0; n = n->next)
        {
            if(str8_match(n->str, str8(">"), 0))
            {
                stdout_delimiter_n = n;
                break;
            }
        }

        // ak: read stdout path
        Str8 stdout_path = STRUCT_ZERO;
        if(stdout_delimiter_n && stdout_delimiter_n->next)
        {
            stdout_path = stdout_delimiter_n->next->str;
        }

        // ak: open stdout handle
        Fs_File stdout_handle = STRUCT_ZERO;
        if(stdout_path.length != 0)
        {
            Fs_File file = fs_file_open(stdout_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Read);
            fs_file_close(file);
            stdout_handle = fs_file_open(stdout_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Append|Fs_File_Access_Flag_ShareRead|Fs_File_Access_Flag_ShareWrite|Fs_File_Access_Flag_Inherited);
        }

        // ak: form command line
        Str8_List cmdline = STRUCT_ZERO;
        for(Str8_Node *n = parts.first; n != stdout_delimiter_n && n != 0; n = n->next)
        {
            str8_list_push(scratch.arena, &cmdline, n->str);
        }

        // ak: launch
        Process_Launch_Params params = STRUCT_ZERO;
        params.cmd_line = cmdline;
        params.inherit_env = 1;
        params.stdout_file = stdout_handle;
        process = process_init(&params);

        // ak: close stdout handle
        if(stdout_path.length != 0)
        {
            fs_file_close(stdout_handle);
        }
    }
    arena_scratch_end(scratch);
    return process;
}

internal Process process_launch_from_cmdf(char *fmt, ...)
{
  Arena_Temp scratch = arena_scratch_begin(NULL, 0);
  va_list args;
  va_start(args, fmt);
  Str8 string = str8fv(scratch.arena, fmt, args);
  Process result = process_launch_from_cmd(string);
  va_end(args);
  arena_scratch_end(scratch);
  return result;
}

