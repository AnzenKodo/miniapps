//- GENERATED CODE

C_LINKAGE_BEGIN
Str8 _render_tex2d_format_display_string_table[9] =
{
    str8("R8"),
    str8("RG8"),
    str8("RGBA8"),
    str8("BGRA8"),
    str8("R16"),
    str8("RGBA16"),
    str8("R32"),
    str8("RG32"),
    str8("RGBA32"),
};

uint8_t _render_tex2d_format_bytes_per_pixel_table[9] =
{
    1,
    2,
    4,
    4,
    2,
    8,
    4,
    8,
    16,
};

Str8 _render_resource_kind_display_string_table[3] =
{
    str8("Static"),
    str8("Dynamic"),
    str8("Stream"),
};

Str8 _render_tex2d_sample_kind_display_string_table[2] =
{
    str8("Nearest"),
    str8("Linear"),
};

Str8 _render_pass_kind_display_string_table[1] =
{
    str8("UI"),
};

uint8_t _render_pass_kind_batch_table[1] =
{
    1,
};

uint64_t _render_pass_kind_params_size_table[1] =
{
    sizeof(Render_Pass_Params_UI),
};

C_LINKAGE_END

