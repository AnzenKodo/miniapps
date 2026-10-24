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

internal Str8_List cmd_line_from_string(Arena *arena, Str8 cmd)
{
    Str8_List list = {0};
    uint8_t *ptr = cmd.cstr;
    uint8_t *opl = cmd.cstr + cmd.length;
    
    while(ptr < opl)
    {
        // ak: skip whitespace
        while(ptr < opl && (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n'))
        {
            ptr += 1;
        }
        if(ptr >= opl) break;
        
        size_t max_len = (size_t)(opl - ptr);
        uint8_t *token_buf = arena_push(arena, uint8_t, max_len + 1);
        size_t token_len = 0;
        
        bool saw_quote = false;
        bool in_single_quote = false;
        bool in_double_quote = false;
        
        while(ptr < opl)
        {
            uint8_t c = *ptr;
            if(c == '\\' && !in_single_quote && (ptr + 1 < opl))
            {
                ptr += 1;
                token_buf[token_len++] = *ptr;
                ptr += 1;
            }
            else if(c == '\'' && !in_double_quote)
            {
                saw_quote = true;
                in_single_quote = !in_single_quote;
                ptr += 1;
            }
            else if(c == '"' && !in_single_quote)
            {
                saw_quote = true;
                in_double_quote = !in_double_quote;
                ptr += 1;
            }
            else if(c == '>' && !in_single_quote && !in_double_quote)
            {
                if(token_len == 1 && token_buf[0] == '2' && !saw_quote)
                {
                    token_buf[token_len++] = c;
                    ptr += 1;
                    break;
                }
                if(token_len > 0 || saw_quote)
                {
                    break;
                }
                token_buf[token_len++] = c;
                ptr += 1;
                break;
            }
            else if((c == ' ' || c == '\t' || c == '\r' || c == '\n') && !in_single_quote && !in_double_quote)
            {
                break;
            }
            else
            {
                token_buf[token_len++] = c;
                ptr += 1;
            }
        }
        if(token_len > 0 || saw_quote)
        {
            token_buf[token_len] = 0;
            Str8 token = str8_init(token_buf, token_len);
            str8_list_push(arena, &list, token);
        }
    }
    return list;
}

internal Process process_launch_from_cmd(Str8 cmd)
{
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    Str8_List cmd_list = {0};
    str8_list_push(scratch.arena, &cmd_list, cmd);
    Process process = process_launch_from_cmd_list(cmd_list);
    arena_scratch_end(scratch);
    return process;
}

internal Process process_launch_from_cmdf(char *format, ...)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    va_list args;
    va_start(args, format);
    Str8 cmd = str8fv(scratch.arena, format, args);
    Process result = process_launch_from_cmd(cmd);
    va_end(args);
    arena_scratch_end(scratch);
    return result;
}

internal Process process_launch_from_cmd_list(Str8_List cmd_list)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Process process = {0};
    if(cmd_list.length != 0)
    {
        // ak: normalize arguments (split any nodes that contain multiple tokens/spaces/quotes)
        Str8_List normalized_cmd_list = {0};
        for(Str8_Node *n = cmd_list.first; n != 0; n = n->next)
        {
            Str8_List parts = cmd_line_from_string(scratch.arena, n->str);
            for(Str8_Node *p = parts.first; p != 0; p = p->next)
            {
                str8_list_push(scratch.arena, &normalized_cmd_list, p->str);
            }
        }
        
        if(normalized_cmd_list.length != 0)
        {
            // ak: find redirections
            Str8_Node *first_redir_n = 0;
            Str8 stdout_path = {0};
            Str8 stderr_path = {0};
            for(Str8_Node *n = normalized_cmd_list.first; n != 0; n = n->next)
            {
                if(str8_match(n->str, str8(">"), 0) && n->next)
                {
                    if(first_redir_n == 0) first_redir_n = n;
                    stdout_path = n->next->str;
                }
                else if(str8_match(n->str, str8("2>"), 0) && n->next)
                {
                    if(first_redir_n == 0) first_redir_n = n;
                    stderr_path = n->next->str;
                }
            }
            
            // ak: open stdout handle
            Fs_File stdout_handle = {0};
            if(stdout_path.length != 0)
            {
                Fs_File file = fs_file_open(stdout_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Read);
                fs_file_close(file);
                stdout_handle = fs_file_open(stdout_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Append|Fs_File_Access_Flag_ShareRead|Fs_File_Access_Flag_ShareWrite|Fs_File_Access_Flag_Inherited);
            }
            
            // ak: open stderr handle
            Fs_File stderr_handle = {0};
            if(stderr_path.length != 0)
            {
                Fs_File file = fs_file_open(stderr_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Read);
                fs_file_close(file);
                stderr_handle = fs_file_open(stderr_path, Fs_File_Access_Flag_Write|Fs_File_Access_Flag_Append|Fs_File_Access_Flag_ShareRead|Fs_File_Access_Flag_ShareWrite|Fs_File_Access_Flag_Inherited);
            }
            
            // ak: form command line
            Str8_List cmdline = {0};
            for(Str8_Node *n = normalized_cmd_list.first; n != first_redir_n && n != 0; n = n->next)
            {
                str8_list_push(scratch.arena, &cmdline, n->str);
            }
            
            // ak: launch
            Process_Launch_Params params = {0};
            params.cmd_line = cmdline;
            params.inherit_env = 1;
            params.stdout_file = stdout_handle;
            params.stderr_file = stderr_handle;
            process = process_init(&params);
            
            // ak: close redirected handles
            if(stdout_path.length != 0)
            {
                fs_file_close(stdout_handle);
            }
            if(stderr_path.length != 0)
            {
                fs_file_close(stderr_handle);
            }
        }
    }
    arena_scratch_end(scratch);
    return process;
}
