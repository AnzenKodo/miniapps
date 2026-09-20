//- GENERATED CODE

#ifndef RENDER_META_H
#define RENDER_META_H

typedef enum Render_Tex_2D_Format
{
    Render_Tex_2D_Format_R8,
    Render_Tex_2D_Format_RG8,
    Render_Tex_2D_Format_RGBA8,
    Render_Tex_2D_Format_BGRA8,
    Render_Tex_2D_Format_R16,
    Render_Tex_2D_Format_RGBA16,
    Render_Tex_2D_Format_R32,
    Render_Tex_2D_Format_RG32,
    Render_Tex_2D_Format_RGBA32,
    Render_Tex_2D_Format_COUNT,
}
Render_Tex_2D_Format;

typedef enum Render_Resource_Kind
{
    Render_Resource_Kind_Static,
    Render_Resource_Kind_Dynamic,
    Render_Resource_Kind_Stream,
    Render_Resource_Kind_COUNT,
}
Render_Resource_Kind;

typedef enum Render_Tex_2D_Sample_Kind
{
    Render_Tex_2D_Sample_Kind_Nearest,
    Render_Tex_2D_Sample_Kind_Linear,
    Render_Tex_2D_Sample_Kind_COUNT,
}
Render_Tex_2D_Sample_Kind;

typedef enum Render_Pass_Kind
{
    Render_Pass_Kind_UI,
    Render_Pass_Kind_COUNT,
}
Render_Pass_Kind;

C_LINKAGE_BEGIN
extern Str8 _render_tex2d_format_display_string_table[9];
extern uint8_t _render_tex2d_format_bytes_per_pixel_table[9];
extern Str8 _render_resource_kind_display_string_table[3];
extern Str8 _render_tex2d_sample_kind_display_string_table[2];
extern Str8 _render_pass_kind_display_string_table[1];
extern uint8_t _render_pass_kind_batch_table[1];

C_LINKAGE_END

#endif // RENDER_META_H
