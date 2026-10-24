//-----------------------------------------------------------------------------
// Reference:
// - By Epic Games Tools & Ryan Fleury:
//      - https://github.com/EpicGames/raddebugger/blob/77bacd640af62933a3cab61f0e8ae2902aefd9f0/src/base/base_processes.h
//      - https://github.com/EpicGames/raddebugger/blob/77bacd640af62933a3cab61f0e8ae2902aefd9f0/src/base/base_processes.c
//-----------------------------------------------------------------------------

#ifndef BASE_PROCESSES_H
#define BASE_PROCESSES_H

typedef struct Process_Info Process_Info;
struct Process_Info
{
  uint32_t pid;
  bool large_pages_allowed;
  Str8 binary_file_path;
  Str8 binary_path;
  Str8 initial_path;
  Str8 user_program_config_data_path;
  Str8 user_program_cache_data_path;
  Str8 user_program_logs_data_path;
  Str8_List module_load_paths;
  Str8_List environment;
};

typedef struct Process Process;
struct Process
{
  uint64_t id[1];
};

typedef struct Process_Group Process_Group;
struct Process_Group
{
  uint64_t id[1];
};

typedef struct Process_Node Process_Node;
struct Process_Node
{
  Process_Node *next;
  Process v;
};

typedef struct Process_List Process_List;
struct Process_List
{
  Process_Node *first;
  Process_Node *last;
  size_t count;
};

typedef struct Process_Launch_Params Process_Launch_Params;
struct Process_Launch_Params
{
  Str8_List cmd_line;
  Str8 path;
  Str8_List env;
  bool inherit_env;
  bool debug_subprocesses;
  bool consoleless;
  bool new_console;
  Process_Group process_group;
  Fs_File stdout_file;
  Fs_File stderr_file;
  Fs_File stdin_file;
};

//~ ak: Handle Type Functions =================================================

internal bool process_match(Process a, Process b);
internal bool process_is_zero(Process p);
internal void process_list_push(Arena *arena, Process_List *list, Process p);

//~ ak: Process Launcher Helpers ==============================================

internal Str8_List cmd_line_from_string(Arena *arena, Str8 cmd);


//~ ak: Process Info ==========================================================

internal Process_Info *process_info_get(void);
internal Str8 current_path_get(Arena *arena);
internal uint32_t process_start_time_unix_get(void);

//~ ak: Child Processes =======================================================

internal Process process_init(Process_Launch_Params *params);
internal Process process_launch_from_cmd(Str8 cmd);
internal Process process_launch_from_cmdf(char *format, ...);
internal Process process_launch_from_cmd_list(Str8_List cmd_list);
internal uint64_t pid_from_process(Process process);
internal bool process_poll(Process process, uint64_t *exit_code_out);
internal bool process_is_active(Process process);
internal bool process_join(Process process, uint64_t endt_us, uint64_t *exit_code_out);
internal void process_detach(Process process);
internal bool process_kill(Process process);
internal bool process_send_ctrl_c(Process process);

internal Process_Group process_group_make(bool kill_on_close);
internal bool process_group_add(Process_Group group, Process process);
internal void process_group_close(Process_Group group);

#endif // BASE_PROCESSES_H
