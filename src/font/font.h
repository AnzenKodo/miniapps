#ifndef FONT_H
#define FONT_H

// ak: External Includes
//=============================================================================

#define STBTT_STATIC
#define stbtt_uint8 uint8_t
typedef int8_t   stbtt_int8;
typedef uint16_t stbtt_uint16;
typedef int16_t  stbtt_int16;
typedef uint32_t stbtt_uint32;
typedef int32_t  stbtt_int32;
#define STBTT_ifloor(x)   ((int) floor_f64(x))
#define STBTT_iceil(x)    ((int) ceil_f64(x))
#define STBTT_sqrt(x)      sqrt_f64(x)
#define STBTT_pow(x,y)     pow_f64(x,y)
#define STBTT_fmod(x,y)    mod_f64(x,y)
#define STBTT_cos(x)       cos_f64(x)
#define STBTT_acos(x)      acos_f64(x)
#define STBTT_fabs(x)      Abs(x)
// TODO(ak): implement alloc api similer to malloc
#define STBTT_malloc(x,u)  ((void)(u),malloc(x))
#define STBTT_free(x,u)    ((void)(u),free(x))
#define STBTT_assert(x)    Assert(x)
#define STBTT_strlen(x)    cstr8_length((uint8_t* )x)
#define STBTT_memcpy       mem_copy
#define STBTT_memset       mem_set
#include "./external/stb_truetype.h"

#define KB_TEXT_SHAPE_STATIC
#define kbts_u64 uint64_t
#define kbts_u32 uint32_t
#define kbts_u16 uint16_t
#define kbts_u8  uint8_t
#define kbts_s64 int64_t
#define kbts_s32 int32_t
#define kbts_s16 int16_t
#define kbts_s8  int8_t
#define kbts_b32 int
#include "./external/kb_text_shape.h"

// ak: Types
//=============================================================================

// ak: Font Provider ==========================================================

typedef struct _Font_Provider_Font _Font_Provider_Font;
struct _Font_Provider_Font
{
    Arena          *arena;
    Str8           file_data;
    stbtt_fontinfo info;
    kbts_font      kb_font;
};

typedef struct Font_Handle Font_Handle;
struct Font_Handle
{
    uint64_t u64[2];
};

typedef struct Font_Raster_Result Font_Raster_Result;
struct Font_Raster_Result
{
    Vec2_I16 atlas_dim;
    void *atlas;
    float advance;
};

typedef struct Font_Shaped_Glyph Font_Shaped_Glyph;
struct Font_Shaped_Glyph
{
    uint16_t id;
    float offset_x;
    float offset_y;
    float advance_x;
    float advance_y;
};

// ak: Font Cache =============================================================

// ak: Handles & Tags

typedef struct Font_Tag Font_Tag;
struct Font_Tag
{
    uint64_t u64[2];
};

// ak: Metrics

typedef struct Font_Metrics Font_Metrics;
struct Font_Metrics
{
    float design_units_per_em;
    float ascent;
    float descent;
    float line_gap;
    float capital_height;
};

// ak: Font Path -> Handle * Metrics * Path Cache Types

typedef struct Font_Hash_Node Font_Hash_Node;
struct Font_Hash_Node
{
    Font_Hash_Node *hash_next;
    Font_Handle handle;
    Font_Tag tag;
    Font_Metrics metrics;
    Str8 path;
};

typedef struct Font_Hash_Slot Font_Hash_Slot;
struct Font_Hash_Slot
{
    Font_Hash_Node *first;
    Font_Hash_Node *last;
};

// ak: Draw Package Types (For Cache Queries)

typedef struct Font_Piece Font_Piece;
struct Font_Piece
{
    Render_Handle texture;
    Rng2_I16 subrect;
    Vec2_I16 offset;
    float advance;
    size_t decode_size;
};

typedef struct Font_Piece_Chunk_Node Font_Piece_Chunk_Node;
struct Font_Piece_Chunk_Node
{
    Font_Piece_Chunk_Node *next;
    Font_Piece *v;
    size_t count;
    size_t cap;
};

typedef struct Font_Piece_Chunk_List Font_Piece_Chunk_List;
struct Font_Piece_Chunk_List
{
    Font_Piece_Chunk_Node *first;
    Font_Piece_Chunk_Node *last;
    size_t node_count;
    size_t total_piece_count;
};

typedef struct Font_Piece_Array Font_Piece_Array;
struct Font_Piece_Array
{
    Font_Piece *v;
    size_t count;
};

typedef struct Font_Run Font_Run;
struct Font_Run
{
    Font_Piece_Array pieces;
    Vec2_F32 dim;
    float ascent;
    float descent;
};

// ak: Rasterization Flags

typedef uint32_t Font_Raster_Flags;
enum
{
    Font_Raster_Flag_Smooth  = (1<<0),
    Font_Raster_Flag_Hinted  = (1<<1),
};

// ak: Rasterization Cache Types

// ak: base glyph rasterization / dimensions cache

typedef struct Font_Raster_Cache_Info Font_Raster_Cache_Info;
struct Font_Raster_Cache_Info
{
    Rng2_I16 subrect;
    Vec2_I16 raster_dim;
    int16_t atlas_num;
    float advance;
};

typedef struct Font_Hash_To_Info_Raster_Cache_Node Font_Hash_To_Info_Raster_Cache_Node;
struct Font_Hash_To_Info_Raster_Cache_Node
{
    Font_Hash_To_Info_Raster_Cache_Node *hash_next;
    Font_Hash_To_Info_Raster_Cache_Node *hash_prev;
    uint64_t hash;
    Font_Raster_Cache_Info info;
};

typedef struct Font_Hash_To_Info_Raster_Cache_Slot Font_Hash_To_Info_Raster_Cache_Slot;
struct Font_Hash_To_Info_Raster_Cache_Slot
{
    Font_Hash_To_Info_Raster_Cache_Node *first;
    Font_Hash_To_Info_Raster_Cache_Node *last;
};

// ak: run cache (arrangements of many glyphs to represent a full string)

typedef struct Font_Run_Cache_Node Font_Run_Cache_Node;
struct Font_Run_Cache_Node
{
    Font_Run_Cache_Node *next;
    Str8 string;
    Font_Run run;
};

typedef struct Font_Run_Cache_Slot Font_Run_Cache_Slot;
struct Font_Run_Cache_Slot
{
    Font_Run_Cache_Node *first;
    Font_Run_Cache_Node *last;
};

// ak: style hash -> artifacts/metrics cache

typedef struct Font_Hash_To_Style_Raster_Cache_Node Font_Hash_To_Style_Raster_Cache_Node;
struct Font_Hash_To_Style_Raster_Cache_Node
{
    Font_Hash_To_Style_Raster_Cache_Node *hash_next;
    Font_Hash_To_Style_Raster_Cache_Node *hash_prev;
    uint64_t style_hash;
    float ascent;
    float descent;
    float column_width;
    Font_Raster_Cache_Info *utf8_class1_direct_map;
    uint64_t utf8_class1_direct_map_mask[4];
    uint64_t hash2info_slots_count;
    Font_Hash_To_Info_Raster_Cache_Slot *hash2info_slots;
    uint64_t run_slots_count;
    Font_Run_Cache_Slot *run_slots;
    uint64_t run_slots_frame_index;
};

typedef struct Font_Hash_To_Style_Raster_Cache_Slot Font_Hash_To_Style_Raster_Cache_Slot;
struct Font_Hash_To_Style_Raster_Cache_Slot
{
    Font_Hash_To_Style_Raster_Cache_Node *first;
    Font_Hash_To_Style_Raster_Cache_Node *last;
};

// ak: Atlas Types

typedef uint32_t Font_Atlas_Region_Node_Flags;
enum
{
    Font_Atlas_Region_Node_Flag_Taken = (1<<0),
};

typedef struct Font_Atlas_Region_Node Font_Atlas_Region_Node;
struct Font_Atlas_Region_Node
{
    Font_Atlas_Region_Node *parent;
    Font_Atlas_Region_Node *children[Corner_COUNT];
    Vec2_I16 max_free_size[Corner_COUNT];
    Font_Atlas_Region_Node_Flags flags;
    uint64_t num_allocated_descendants;
};

typedef struct Font_Atlas Font_Atlas;
struct Font_Atlas
{
    Font_Atlas *next;
    Font_Atlas *prev;
    Render_Handle texture;
    Vec2_I16 root_dim;
    Font_Atlas_Region_Node *root;
};

// ak: Main State Type

typedef struct _Font_State _Font_State;
struct _Font_State
{
    Arena *arena;
    Arena *raster_arena;
    Arena *frame_arena;
    size_t frame_index;
    
    // ak: font table
    size_t font_hash_table_size;
    Font_Hash_Slot *font_hash_table;
    
    // ak: hash -> raster cache table
    size_t hash2style_slots_count;
    Font_Hash_To_Style_Raster_Cache_Slot *hash2style_slots;
    
    // ak: atlas list
    Font_Atlas *first_atlas;
    Font_Atlas *last_atlas;
};

// ak: Functions
//=============================================================================

// ak: Font Provider ==========================================================

internal void _font_stb_kbts_allocator(void *data, kbts_allocator_op *op);
internal _Font_Provider_Font *_font_provider_font_from_handle(Font_Handle handle);
internal Font_Handle font_handle_zero(void);
internal bool font_handle_match(Font_Handle a, Font_Handle b);
internal Font_Handle font_handle_from_font(_Font_Provider_Font *font);
internal Font_Handle font_open(Str8 path);
internal Font_Metrics font_provider_metrics_from_font(Font_Handle handle);
internal Font_Metrics font_provider_metrics_from_tag(Font_Tag tag);
internal Font_Raster_Result font_raster(Arena *arena, Font_Handle handle, float size, Str8 string);
internal Font_Handle font_font_open_from_static_data_string(Str8 *data_ptr);

// ak: Font Cache =============================================================

// ak: Basic Functions
internal U128 font_hash_from_string(Str8 string);
internal uint64_t font_little_hash_from_string(uint64_t seed, Str8 string);
internal Vec2_I32 font_vertex_from_corner(Corner corner);

// ak: Font Tag
internal Font_Tag font_tag_zero(void);
internal bool font_tag_match(Font_Tag a, Font_Tag b);
internal Font_Tag font_tag_from_path(Str8 path);
internal Font_Tag font_tag_from_static_data_string(Str8 *data_ptr);

// ak: Metrics
internal Font_Metrics font_metrics_from_tag_size(Font_Tag tag, float size);

// ak: Atlas
internal Rng2_I16 font_atlas_region_alloc(Arena *arena, Font_Atlas *atlas, Vec2_I16 needed_size);
internal void font_atlas_region_release(Font_Atlas *atlas, Rng2_I16 region);

// ak: Piece Type Functions
internal Font_Piece_Array font_piece_array_from_chunk_list(Arena *arena, Font_Piece_Chunk_List *list);
internal Font_Piece *font_piece_chunk_list_push_new(Arena *arena, Font_Piece_Chunk_List *list, uint64_t cap);

// ak: Cache Usage

// ak: base cache lookups
internal Font_Hash_To_Style_Raster_Cache_Node * font_hash_to_style_from_tag_size_flags(Font_Tag tag, float size, Font_Raster_Flags flags);

internal Font_Run font_run_from_string(Font_Tag tag, float size, float base_align_px, float tab_size_px, Font_Raster_Flags flags, Str8 string);

// ak: helpers
internal Vec2_F32 font_dim_from_tag_size_string(Font_Tag tag, float size, float base_align_px, float tab_size_px, Str8 string);

// ak: Main Calls
internal void font_init(void);
internal void font_reset(void);
internal void font_frame(void);

// ak: Globals
//=============================================================================

global _Font_State *_font_state = 0;

#endif // FONT_H
