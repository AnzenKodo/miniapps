//-----------------------------------------------------------------------------
// Reference:
// - By Epic Games Tools & Ryan Fleury
//      - https://github.com/EpicGames/raddebugger/blob/e28a543573277315ffd32a3e983c8171570bb946/src/base/base_files.h
//      - https://github.com/EpicGames/raddebugger/blob/e28a543573277315ffd32a3e983c8171570bb946/src/base/base_files.c
//-----------------------------------------------------------------------------

#ifndef BASE_FILE_H
#define BASE_FILE_H

//~ ak: Types
//=============================================================================

typedef uint64_t Fs_File;

typedef uint32_t Fs_File_Access_Flags;
enum
{
    Fs_File_Access_Flag_Read        = (1<<0),
    Fs_File_Access_Flag_Write       = (1<<1),
    Fs_File_Access_Flag_Execute     = (1<<2),
    Fs_File_Access_Flag_Append      = (1<<3),
    Fs_File_Access_Flag_ShareRead   = (1<<4),
    Fs_File_Access_Flag_ShareWrite  = (1<<5),
    Fs_File_Access_Flag_Inherited   = (1<<6),
};

typedef uint32_t Fs_Property_Flags;
enum
{
    Fs_Property_Flag_IsFolder = (1 << 0),
};

typedef struct Fs_Properties Fs_Properties;
struct Fs_Properties
{
    uint64_t size;
    DenseTime modified;
    DenseTime created;
    Fs_Property_Flags flags;
};

typedef struct Fs_Info Fs_Info;
struct Fs_Info
{
    Str8 name;
    Fs_Properties props;
};

typedef uint32_t Fs_Walk_Flags;
enum
{
    Fs_Walk_Flag_SkipFolders     = (1 << 0),
    Fs_Walk_Flag_SkipFiles       = (1 << 1),
    Fs_Walk_Flag_SkipHiddenFiles = (1 << 2),
    Fs_Walk_Flag_Done            = (1 << 31),
};

typedef struct Fs_Walk Fs_Walk;
struct Fs_Walk
{
    Fs_Walk_Flags flags;
    uint8_t memory[800];
};

//~ ak: Functions
//=============================================================================

// ak: Open and Close file
internal Fs_File fs_file_open(Str8 path, Fs_File_Access_Flags flags);
internal void fs_file_close(Fs_File file);

// ak: File Read
internal size_t fs_file_read_raw(Fs_File file, Rng1_U64 range, void *out_data);
internal U8Array fs_file_read(Fs_File file, Rng1_U64 range, Arena *arena);
internal U8Array fs_file_read_full(Fs_File file, Arena *arena);
internal U8Array fs_file_path_read(Str8 path, Rng1_U64 range, Arena *arena);
internal U8Array fs_file_path_read_full(Str8 path, Arena *arena);

// ak: File Write
internal size_t fs_file_write(Fs_File file, void *data, Rng1_U64 rng);
internal size_t fs_file_write_append(Fs_File file, void *data, size_t size);

// ak: Properties
internal Fs_Properties fs_properties_from_file(Fs_File file);
internal Fs_Properties fs_properties_from_file_path(Str8 path);

// ak: Walk
internal Fs_Walk *fs_walk_begin(Arena *arena, Str8 path, Fs_Walk_Flags flags);
internal bool fs_walk_next(Arena *arena, Fs_Walk *walk, Fs_Info *info_out);
internal void fs_walk_end(Fs_Walk *walk);

// ak: Directory Operations
internal bool fs_is_dir_exist(Str8 path);
internal bool fs_dir_make(Str8 path);
internal bool fs_dir_ensure(Str8 path);

// ak: Exists
internal bool fs_dir_path_exists(Str8 path);
internal bool fs_file_path_exists(Str8 path);

#endif // BASE_FILE_H
