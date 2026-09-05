// ak: External Includes
//=============================================================================

#define STB_TRUETYPE_IMPLEMENTATION
#include "./external/stb_truetype.h"

#define KB_TEXT_SHAPE_IMPLEMENTATION
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "./external/kb_text_shape.h"
#pragma GCC diagnostic pop

// ak: Font Provider
//=============================================================================

internal void _font_stb_kbts_allocator(void *data, kbts_allocator_op *op)
{
    Arena *arena = (Arena *)data;
    if(op->Kind == KBTS_ALLOCATOR_OP_KIND_ALLOCATE)
    {
        op->Allocate.Pointer = (op->Allocate.Size > 0) ? arena_push(arena, uint8_t, op->Allocate.Size) : 0;
    }
    // ak: FREE is a no-op for arena allocators
}

internal _Font_Provider_Font *_font_provider_font_from_handle(Font_Handle handle)
{
    _Font_Provider_Font *result = (_Font_Provider_Font *)handle.u64[0];
    return result;
}

internal Font_Handle font_handle_zero(void)
{
    return StructZeroType(Font_Handle);
}

internal bool font_handle_match(Font_Handle a, Font_Handle b)
{
    return (a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1]);
}

internal Font_Handle font_handle_from_font(_Font_Provider_Font *font)
{
    Font_Handle result = {(uint64_t)font};
    return result;
}

internal Font_Handle font_open(Str8 path)
{
    Arena *arena = arena_alloc();
    Str8 file_data = os_path_read_str_full(path, arena);
    if(file_data.length == 0)
    {
        arena_free(arena);
        return font_handle_zero();
    }
    
    _Font_Provider_Font *font = arena_push(arena, _Font_Provider_Font, 1);
    font->arena     = arena;
    font->file_data = file_data;
    
    int offset = stbtt_GetFontOffsetForIndex(file_data.cstr, 0);
    if(offset < 0) { offset = 0; }
    if(!stbtt_InitFont(&font->info, file_data.cstr, offset))
    {
        arena_free(arena);
        return font_handle_zero();
    }
    
    font->kb_font = kbts_FontFromMemory(file_data.cstr, file_data.length, 0, _font_stb_kbts_allocator, arena);
    if(!kbts_FontIsValid(&font->kb_font))
    {
        arena_free(arena);
        return font_handle_zero();
    }
    
    return font_handle_from_font(font);
}

internal Font_Metrics font_provider_metrics_from_font(Font_Handle handle)
{
    _Font_Provider_Font *font = _font_provider_font_from_handle(handle);
    Font_Metrics result = STRUCT_ZERO;
    if(font != 0)
    {
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
        
        kbts_font_info2_2 info = STRUCT_ZERO;
        info.Base.Size = sizeof(info);
        kbts_GetFontInfo2(&font->kb_font, (kbts_font_info2 *)&info);
        
        result.design_units_per_em = (float)info.UnitsPerEm;
        result.ascent              = (float)ascent;
        result.descent             = (float)(descent < 0 ? -descent : descent);
        result.line_gap            = (float)line_gap;
        result.capital_height      = (float)info.CapitalHeight;
        if(result.capital_height == 0)
        {
            result.capital_height = result.ascent;
        }
    }
    return result;
}

internal Font_Metrics font_provider_metrics_from_tag(Font_Tag tag)
{
    size_t slot_idx = tag.u64[1] % _font_state->font_hash_table_size;
    Font_Hash_Node *existing_node = 0;
    {
        for (Font_Hash_Node *n = _font_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
        {
            if(MemMatchStruct(&tag, &n->tag))
            {
                existing_node = n;
                break;
            }
        }
    }
    Font_Metrics result = STRUCT_ZERO;
    if(existing_node != 0)
    {
        result = existing_node->metrics;
    }
    return result;
}

internal NO_ASAN Font_Raster_Result font_raster(Arena *arena, Font_Handle handle, float size, Str8 string)
{
    _Font_Provider_Font *font = _font_provider_font_from_handle(handle);
    Font_Raster_Result result = STRUCT_ZERO;
    if(font != 0 && string.length > 0)
    {
        Arena_Temp scratch = arena_scratch_begin(&arena, 1);
        float scale = stbtt_ScaleForMappingEmToPixels(&font->info, (96.f/72.f) * size);
        
        if(string.length == sizeof(uint32_t))
        {
            // ak: single glyph-ID rasterization path
            uint32_t glyph_id = *(uint32_t *)string.cstr;
            int advance_raw, lsb;
            stbtt_GetGlyphHMetrics(&font->info, glyph_id, &advance_raw, &lsb);
            int x0, y0, x1, y1;
            stbtt_GetGlyphBitmapBox(&font->info, glyph_id, scale, scale, &x0, &y0, &x1, &y1);
            int gw = x1 - x0;
            int gh = y1 - y0;
            Vec2_I16 dim = {(int16_t)Max(gw, 1), (int16_t)Max(gh, 1)};
            uint64_t atlas_size = (uint64_t)dim.x * (uint64_t)dim.y * 4;
            uint8_t *atlas = arena_push(arena, uint8_t, atlas_size);
            
            if(gw > 0 && gh > 0)
            {
                uint8_t *glyph_bmp = arena_push(scratch.arena, uint8_t, gw * gh);
                stbtt_MakeGlyphBitmap(&font->info, glyph_bmp, gw, gh, gw, scale, scale, glyph_id);
                for(int32_t row = 0; row < gh; row += 1)
                {
                    for(int32_t col = 0; col < gw; col += 1)
                    {
                        uint64_t off     = ((uint64_t)row * (uint64_t)dim.x + (uint64_t)col) * 4;
                        atlas[off+0] = 255;
                        atlas[off+1] = 255;
                        atlas[off+2] = 255;
                        atlas[off+3] = glyph_bmp[row * gw + col];
                    }
                }
            }
            
            result.atlas_dim = dim;
            result.advance   = (float)advance_raw * scale;
            result.atlas     = atlas;
        }
        else
        {
            // ak: multi-character string shaping + rasterization path
            int ascent, descent, line_gap;
            stbtt_GetFontVMetrics(&font->info, &ascent, &descent, &line_gap);
            int32_t baseline = (int32_t)round_f32((float)ascent * scale);
            int32_t height   = (int32_t)round_f32((float)(ascent - descent + line_gap) * scale);
            
            // ak: shape string
            kbts_shape_context *ctx = kbts_CreateShapeContext(_font_stb_kbts_allocator, scratch.arena);
            kbts_ShapePushFont(ctx, &font->kb_font);
            kbts_ShapeBegin(ctx, KBTS_DIRECTION_DONT_KNOW, KBTS_LANGUAGE_DONT_KNOW);
            kbts_ShapeUtf8(ctx, (const char *)string.cstr, (int)string.length, KBTS_USER_ID_GENERATION_MODE_CODEPOINT_INDEX);
            kbts_ShapeEnd(ctx);
            
            // ak: collect shaped glyphs
            uint32_t glyph_count = 0;
            uint32_t glyph_cap   = 64;
            Font_Shaped_Glyph *glyphs = arena_push(scratch.arena, Font_Shaped_Glyph, glyph_cap);
            
            kbts_run run;
            while(kbts_ShapeRun(ctx, &run))
            {
                kbts_glyph *g;
                while(kbts_GlyphIteratorNext(&run.Glyphs, &g))
                {
                    if(glyph_count >= glyph_cap)
                    {
                        uint32_t new_cap = glyph_cap * 2;
                        Font_Shaped_Glyph *new_glyphs = arena_push(scratch.arena, Font_Shaped_Glyph, new_cap);
                        mem_copy(new_glyphs, glyphs, glyph_count * sizeof(Font_Shaped_Glyph));
                        glyphs   = new_glyphs;
                        glyph_cap = new_cap;
                    }
                    Font_Shaped_Glyph *dst = &glyphs[glyph_count];
                    dst->id        = g->Id;
                    dst->offset_x  = (float)g->OffsetX;
                    dst->offset_y  = (float)g->OffsetY;
                    dst->advance_x = (float)g->AdvanceX;
                    dst->advance_y = (float)g->AdvanceY;
                    glyph_count += 1;
                }
            }
            
            // ak: measure total width
            float total_width = 0.f;
            for(uint32_t i = 0; i < glyph_count; i += 1)
            {
                total_width += glyphs[i].advance_x * scale;
            }
            
            // ak: allocate atlas
            Vec2_I16 dim = {
                (int16_t)Max((int32_t)round_f32(total_width) + 1, 1),
                (int16_t)Max(height + 1, 1)
            };
            size_t atlas_size = (uint64_t)dim.x * (uint64_t)dim.y * 4;
            uint8_t *atlas = arena_push(arena, uint8_t, atlas_size);
            
            // ak: rasterize each glyph
            float cursor_x = 0.f;
            float cursor_y = 0.f;
            for(uint32_t i = 0; i < glyph_count; i += 1)
            {
                Font_Shaped_Glyph *g = &glyphs[i];
                float glyph_x = cursor_x + g->offset_x * scale;
                float glyph_y = cursor_y + (float)baseline - g->offset_y * scale;
                int x0, y0, x1, y1;
                stbtt_GetGlyphBitmapBox(&font->info, g->id, scale, scale, &x0, &y0, &x1, &y1);
                int gw = x1 - x0;
                int gh = y1 - y0;
                if(gw > 0 && gh > 0)
                {
                    uint8_t *glyph_bmp = arena_push(scratch.arena, uint8_t, gw * gh);
                    stbtt_MakeGlyphBitmap(&font->info, glyph_bmp, gw, gh, gw, scale, scale, g->id);
                    int32_t start_x = (int32_t)round_f32(glyph_x) + x0;
                    int32_t start_y = (int32_t)round_f32(glyph_y) + y0;
                    for(int32_t row = 0; row < gh; row += 1)
                    {
                        int32_t y = start_y + row;
                        if(y >= 0 && y < dim.y)
                        {
                            for(int32_t col = 0; col < gw; col += 1)
                            {
                                int32_t x = start_x + col;
                                if(x >= 0 && x < dim.x)
                                {
                                    uint64_t off      = ((uint64_t)y * (uint64_t)dim.x + (uint64_t)x) * 4;
                                    atlas[off+0] = 255;
                                    atlas[off+1] = 255;
                                    atlas[off+2] = 255;
                                    atlas[off+3] = glyph_bmp[row * gw + col];
                                }
                            }
                        }
                    }
                }
                cursor_x += g->advance_x * scale;
                cursor_y += g->advance_y * scale;
            }
            result.atlas_dim = dim;
            result.advance   = total_width;
            result.atlas     = atlas;
        }
        arena_scratch_end(scratch);
    }
    return result;
}

internal Font_Handle font_font_open_from_static_data_string(Str8 *data_ptr)
{
    Arena *arena = arena_alloc();
    _Font_Provider_Font *font = arena_push(arena, _Font_Provider_Font, 1);
    font->arena     = arena;
    font->file_data = *data_ptr;
    
    int offset = stbtt_GetFontOffsetForIndex(data_ptr->cstr, 0);
    if(offset < 0) { offset = 0; }
    if(!stbtt_InitFont(&font->info, data_ptr->cstr, offset))
    {
        arena_free(arena);
        return font_handle_zero();
    }
    
    font->kb_font = kbts_FontFromMemory(data_ptr->cstr, data_ptr->size, 0, _font_stb_kbts_allocator, arena);
    if(!kbts_FontIsValid(&font->kb_font))
    {
        arena_free(arena);
        return font_handle_zero();
    }
    
    return font_handle_from_font(font);
}

// ak: Font Cache
//=============================================================================

// ak: Basic Functions ========================================================

internal U128 font_hash_from_string(Str8 string)
{
    union
    {
        XXH128_hash_t xxhash;
        U128 u128;
    }
    hash;
    hash.xxhash = XXH3_128bits(string.cstr, string.length);
    return hash.u128;
}


internal uint64_t font_little_hash_from_string(uint64_t seed, Str8 string)
{
  uint64_t result = XXH3_64bits_withSeed(string.cstr, string.length, seed);
  return result;
}

internal Vec2_I32 font_vertex_from_corner(Corner corner)
{
    Vec2_I32 result = STRUCT_ZERO;
    switch (corner)
    {
        case Corner_TopLeft:     { result = (Vec2_I32){ 0, 0 }; } break;
        case Corner_BottomLeft:  { result = (Vec2_I32){ 0, 1 }; } break;
        case Corner_TopRight:    { result = (Vec2_I32){ 1, 0 }; } break;
        case Corner_BottomRight: { result = (Vec2_I32){ 1, 1 }; } break;
        case Corner_Invalid: case Corner_COUNT: break;
    }
    return result;
}

// ak: Font Tag ===============================================================

internal Font_Tag font_tag_zero(void)
{
  Font_Tag result = STRUCT_ZERO;
  return result;
}

internal bool font_tag_match(Font_Tag a, Font_Tag b)
{
  return a.u64[0] == b.u64[0] && a.u64[1] == b.u64[1];
}

internal Font_Tag font_tag_from_path(Str8 path)
{
    // ak: produce tag from hash of path
    Font_Tag result = STRUCT_ZERO;
    {
        U128 hash = font_hash_from_string(path);
        mem_copy(&result, &hash, sizeof(result));
        result.u64[1] |= bit64;
    }
    
    // ak: tag -> slot index
    uint64_t slot_idx = result.u64[1] % _font_state->font_hash_table_size;
    
    // ak: slot * tag -> existing node
    Font_Hash_Node *existing_node = 0;
    {
        for(Font_Hash_Node *n = _font_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
        {
            if(MemMatchStruct(&result, &n->tag))
            {
                existing_node = n;
                break;
            }
        }
    }
    
    // ak: allocate & push new node if we don't have an existing one
    if (existing_node == 0)
    {
        Font_Handle handle = font_open(path);
        Font_Hash_Slot *slot = &_font_state->font_hash_table[slot_idx];
        existing_node = arena_push(_font_state->arena, Font_Hash_Node, 1);
        existing_node->tag = result;
        existing_node->handle = handle;
        existing_node->metrics = font_provider_metrics_from_font(existing_node->handle);
        existing_node->path = str8_copy(_font_state->arena, path);
        SLLQueuePush_N(slot->first, slot->last, existing_node, hash_next);
    }
    
    // ak: tag result must be zero if this is not a valid font
    if(font_handle_match(existing_node->handle, font_handle_zero()))
    {
        MemSetZeroStruct(&result);
    }
    
    // ak: return
    return result;
}

internal Font_Tag font_tag_from_static_data_string(Str8 *data_ptr)
{
    // ak: produce tag hash of ptr
    Font_Tag result = STRUCT_ZERO;
    {
        U128 hash = font_hash_from_string(str8_init((uint8_t *)&data_ptr, sizeof(Str8 *)));
        mem_copy(&result, &hash, sizeof(result));
        result.u64[1] &= ~bit64;
    }
    
    // ak: tag -> slot index
    size_t slot_idx = result.u64[1] % _font_state->font_hash_table_size;
    
    // ak: slot * tag -> existing node
    Font_Hash_Node *existing_node = 0;
    {
        for(Font_Hash_Node *n = _font_state->font_hash_table[slot_idx].first; n != 0 ; n = n->hash_next)
        {
            if(MemMatchStruct(&result, &n->tag))
            {
                existing_node = n;
                break;
            }
        }
    }
    
    // ak: allocate & push new node if we don't have an existing one
    Font_Hash_Node *new_node = 0;
    if(existing_node == 0)
    {
        Font_Hash_Slot *slot = &_font_state->font_hash_table[slot_idx];
        new_node = arena_push(_font_state->arena, Font_Hash_Node, 1);
        new_node->tag = result;
        new_node->handle = font_font_open_from_static_data_string(data_ptr);
        new_node->metrics = font_provider_metrics_from_font(new_node->handle);
        new_node->path = str8("");
        SLLQueuePush_N(slot->first, slot->last, new_node, hash_next);
    }
    
    // ak: return
    return result;
}

// ak: Metrics ================================================================

internal Font_Metrics font_metrics_from_tag_size(Font_Tag tag, float size)
{
    Font_Metrics metrics = font_provider_metrics_from_tag(tag);
    Font_Metrics result = STRUCT_ZERO;
    {
        result.ascent   = floor_f32(size) * metrics.ascent / metrics.design_units_per_em;
        result.descent  = floor_f32(size) * metrics.descent / metrics.design_units_per_em;
        result.line_gap = floor_f32(size) * metrics.line_gap / metrics.design_units_per_em;
        result.capital_height = floor_f32(size) * metrics.capital_height / metrics.design_units_per_em;
    }
    return result;
}

// ak: Atlas ==================================================================

internal Rng2_I16 font_atlas_region_alloc(Arena *arena, Font_Atlas *atlas, Vec2_I16 needed_size)
{
    // ak: find node with best-fit size
    Vec2_I16 region_p0 = STRUCT_ZERO;
    Vec2_I16 region_sz = STRUCT_ZERO;
    Corner node_corner = Corner_Invalid;
    Font_Atlas_Region_Node *node = 0;
    {
        Vec2_I16 n_supported_size = atlas->root_dim;
        for(Font_Atlas_Region_Node *n = atlas->root, *next = 0; n != 0; n = next, next = 0)
        {
            // ak: we've traversed to a taken node.
            if(n->flags & Font_Atlas_Region_Node_Flag_Taken)
            {
                break;
            }
            
            // ak: calculate if this node can be allocated (all children are non-allocated)
            bool n_can_be_allocated = (n->num_allocated_descendants == 0);

            // ak: fill size
            if(n_can_be_allocated)
            {
                region_sz = n_supported_size;
            }
            
            // ak: calculate size of this node's children
            Vec2_I16 child_size = (Vec2_I16){ (int16_t)(n_supported_size.x/2), (int16_t)(n_supported_size.y/2) };
            
            // ak: find best next child
            Font_Atlas_Region_Node *best_child = 0;
            if(child_size.x >= needed_size.x && child_size.y >= needed_size.y)
            {
                for(Corner corner = (Corner)0; corner < Corner_COUNT; corner = (Corner)(corner+1))
                {
                    if(n->children[corner] == 0)
                    {
                        n->children[corner] = arena_push(arena, Font_Atlas_Region_Node, 1);
                        n->children[corner]->parent = n;
                        n->children[corner]->max_free_size[Corner_TopLeft] =
                        n->children[corner]->max_free_size[Corner_BottomLeft] =
                        n->children[corner]->max_free_size[Corner_TopRight] =
                        n->children[corner]->max_free_size[Corner_BottomRight] = (Vec2_I16){ (int16_t)(child_size.x/2), (int16_t)(child_size.y/2) };
                    }
                    if(n->max_free_size[corner].x >= needed_size.x &&
                            n->max_free_size[corner].y >= needed_size.y)
                    {
                        best_child = n->children[corner];
                        node_corner = corner;
                        Vec2_I32 side_vertex = font_vertex_from_corner(corner);
                        region_p0.x += side_vertex.x*child_size.x;
                        region_p0.y += side_vertex.y*child_size.y;
                        break;
                    }
                }
            }
            
            // ak: resolve node to this node if it can be allocated and children
            // don't fit, or keep going to the next best child
            if(n_can_be_allocated && best_child == 0)
            {
                node = n;
            }
            else
            {
                next = best_child;
                n_supported_size = child_size;
            }
        }
    }
    
    // ak: we're taking the subtree rooted by `node`. mark up all parents
    if(node != 0 && node_corner != Corner_Invalid)
    {
        node->flags |= Font_Atlas_Region_Node_Flag_Taken;
        if(node->parent != 0)
        {
            MemSetZeroStruct(&node->parent->max_free_size[node_corner]);
        }
        for(Font_Atlas_Region_Node *p = node->parent; p != 0; p = p->parent)
        {
            p->num_allocated_descendants += 1;
            Font_Atlas_Region_Node *parent = p->parent;
            if(parent != 0)
            {
                Corner p_corner = (p == parent->children[Corner_TopLeft] ? Corner_TopLeft :
                        p == parent->children[Corner_BottomLeft] ? Corner_BottomLeft :
                        p == parent->children[Corner_TopRight] ? Corner_TopRight :
                        p == parent->children[Corner_BottomRight] ? Corner_BottomRight :
                        Corner_Invalid);
                if(p_corner == Corner_Invalid)
                {
                    UNREACHABLE();
                }
                parent->max_free_size[p_corner].x = Max(Max(p->max_free_size[Corner_TopLeft].x,
                            p->max_free_size[Corner_BottomLeft].x),
                        Max(p->max_free_size[Corner_TopRight].x,
                            p->max_free_size[Corner_BottomRight].x));
                parent->max_free_size[p_corner].y = Max(Max(p->max_free_size[Corner_TopLeft].y,
                            p->max_free_size[Corner_BottomLeft].y),
                        Max(p->max_free_size[Corner_TopRight].y,
                            p->max_free_size[Corner_BottomRight].y));
            }
        }
    }
    
    // ak: fill rectangular region & return
    Rng2_I16 result = STRUCT_ZERO;
    result.p0 = region_p0;
    result.p1 = add_vec2(region_p0, region_sz);
    return result;
}

internal void font_atlas_region_release(Font_Atlas *atlas, Rng2_I16 region)
{
    // ak: extract region size
    Vec2_I16 region_size = (Vec2_I16){ (int16_t)(region.x1 - region.x0), (int16_t)(region.y1 - region.y0) };
    
    // ak: map region to associated node
    Vec2_I16 calc_region_size = STRUCT_ZERO;
    Font_Atlas_Region_Node *node = 0;
    Corner node_corner = Corner_Invalid;
    {
        Vec2_I16 n_p0 = (Vec2_I16){ 0, 0 };
        Vec2_I16 n_sz = atlas->root_dim;
        for(Font_Atlas_Region_Node *n = atlas->root, *next = 0; n != 0; n = next)
        {
            // ak: is the region within this node's boundaries? (either this node, or a descendant)
            if(n_p0.x <= region.p0.x && region.p0.x < n_p0.x+n_sz.x &&
                    n_p0.y <= region.p0.y && region.p0.y < n_p0.y+n_sz.y)
            {
                // ak: check the region against this node
                if(region.p0.x == n_p0.x && region.p0.y == n_p0.y &&
                        region_size.x == n_sz.x && region_size.y == n_sz.y)
                {
                    node = n;
                    calc_region_size = n_sz;
                    break;
                }
                // ak: check the region against children & iterate
                else
                {
                    Vec2_I16 r_midpoint = (Vec2_I16){ (int16_t)(region.p0.x + region_size.x/2), (int16_t)(region.p0.y + region_size.y/2) };
                    Vec2_I16 n_midpoint = (Vec2_I16){ (int16_t)(n_p0.x + n_sz.x/2), (int16_t)(n_p0.y + n_sz.y/2) };
                    Corner next_corner = Corner_Invalid;
                    if(r_midpoint.x <= n_midpoint.x && r_midpoint.y <= n_midpoint.y)
                    {
                        next_corner = Corner_TopLeft;
                    }
                    else if(r_midpoint.x <= n_midpoint.x && n_midpoint.y <= r_midpoint.y)
                    {
                        next_corner = Corner_BottomLeft;
                    }
                    else if(n_midpoint.x <= r_midpoint.x && r_midpoint.y <= n_midpoint.y)
                    {
                        next_corner = Corner_TopRight;
                    }
                    else if(n_midpoint.x <= r_midpoint.x && n_midpoint.y <= r_midpoint.y)
                    {
                        next_corner = Corner_BottomRight;
                    }
                    next = n->children[next_corner];
                    node_corner = next_corner;
                    n_sz.x /= 2;
                    n_sz.y /= 2;
                    Vec2_I32 side_vertex = font_vertex_from_corner(node_corner);
                    n_p0.x += side_vertex.x*n_sz.x;
                    n_p0.y += side_vertex.y*n_sz.y;
                }
            }
            else
            {
                break;
            }
        }
    }
    
    // ak: free node
    if(node != 0 && node_corner != Corner_Invalid)
    {
        node->flags &= ~Font_Atlas_Region_Node_Flag_Taken;
        if(node->parent != 0)
        {
            node->parent->max_free_size[node_corner] = calc_region_size;
        }
        for(Font_Atlas_Region_Node *p = node->parent; p != 0; p = p->parent)
        {
            p->num_allocated_descendants -= 1;
            Font_Atlas_Region_Node *parent = p->parent;
            if(parent != 0)
            {
                Corner p_corner = (
                    p == parent->children[Corner_TopLeft] ? Corner_TopLeft :
                    p == parent->children[Corner_BottomLeft] ? Corner_BottomLeft :
                    p == parent->children[Corner_TopRight] ? Corner_TopRight :
                    p == parent->children[Corner_BottomRight] ? Corner_BottomRight :
                    Corner_Invalid
                );
                if(p_corner == Corner_Invalid)
                {
                    UNREACHABLE();
                }
                parent->max_free_size[p_corner].x = Max(
                    Max(p->max_free_size[Corner_TopLeft].x, p->max_free_size[Corner_BottomLeft].x),
                    Max(p->max_free_size[Corner_TopRight].x, p->max_free_size[Corner_BottomRight].x)
                );
                parent->max_free_size[p_corner].y = Max(
                    Max(p->max_free_size[Corner_TopLeft].y, p->max_free_size[Corner_BottomLeft].y),
                    Max(p->max_free_size[Corner_TopRight].y, p->max_free_size[Corner_BottomRight].y)
                );
            }
        }
    }
}

// ak: Piece Type Functions ===================================================

internal Font_Piece_Array font_piece_array_from_chunk_list(Arena *arena, Font_Piece_Chunk_List *list)
{
    Font_Piece_Array array = STRUCT_ZERO;
    array.count = list->total_piece_count;
    array.v = arena_push_nz(arena, Font_Piece, array.count);
    uint64_t write_idx = 0;
    for (Font_Piece_Chunk_Node *node = list->first; node != 0; node = node->next)
    {
        mem_copy(array.v + write_idx, node->v, node->count * sizeof(Font_Piece));
        write_idx += node->count;
    }
    return array;
}

internal Font_Piece *font_piece_chunk_list_push_new(Arena *arena, Font_Piece_Chunk_List *list, uint64_t cap)
{
    Font_Piece_Chunk_Node *node = list->last;
    if(node == 0 || node->count >= node->cap)
    {
        node = arena_push(arena, Font_Piece_Chunk_Node, 1);
        node->v = arena_push_nz(arena, Font_Piece, cap);
        node->cap = cap;
        SLLQueuePush(list->first, list->last, node);
        list->node_count += 1;
    }
    Font_Piece *result = node->v + node->count;
    node->count += 1;
    list->total_piece_count += 1;
    return result;
}

// ak: Cache Usage ============================================================

// ak: base cache lookups

internal Font_Hash_To_Style_Raster_Cache_Node * font_hash_to_style_from_tag_size_flags(Font_Tag tag, float size, Font_Raster_Flags flags)
{
    // ak: tag * size -> style hash
    uint64_t style_hash = STRUCT_ZERO;
    {
        double size_f64 = size;
        uint64_t buffer[] =
        {
            tag.u64[0],
            tag.u64[1],
            *(uint64_t *)(&size_f64),
            (uint64_t)flags,
        };
        style_hash = font_little_hash_from_string(5381, str8_init((uint8_t *)buffer, sizeof(buffer)));
    }
    
    // ak: style hash -> style node
    Font_Hash_To_Style_Raster_Cache_Node *hash2style_node = 0;
    {
        size_t slot_idx = style_hash%_font_state->hash2style_slots_count;
        Font_Hash_To_Style_Raster_Cache_Slot *slot = &_font_state->hash2style_slots[slot_idx];
        for(Font_Hash_To_Style_Raster_Cache_Node *n = slot->first;
                n != 0;
                n = n->hash_next)
        {
            if(n->style_hash == style_hash)
            {
                hash2style_node = n;
                break;
            }
        }
        if(Unlikely(hash2style_node == 0))
        {
            Font_Metrics metrics = font_metrics_from_tag_size(tag, size);
            hash2style_node = arena_push(_font_state->raster_arena, Font_Hash_To_Style_Raster_Cache_Node, 1);
            DLLPushBack_NP(slot->first, slot->last, hash2style_node, hash_next, hash_prev);
            hash2style_node->style_hash = style_hash;
            hash2style_node->ascent   = metrics.ascent;
            hash2style_node->descent  = metrics.descent;
            hash2style_node->utf8_class1_direct_map = arena_push_nz(_font_state->raster_arena, Font_Raster_Cache_Info, 256);
            hash2style_node->hash2info_slots_count = 1024;
            hash2style_node->hash2info_slots = arena_push(_font_state->raster_arena, Font_Hash_To_Info_Raster_Cache_Slot, hash2style_node->hash2info_slots_count);
        }
    }
    
    return hash2style_node;
}

internal Font_Run font_run_from_string(Font_Tag tag, float size, float base_align_px, float tab_size_px, Font_Raster_Flags flags, Str8 string)
{
    // ak: map tag/size to style node
    Font_Hash_To_Style_Raster_Cache_Node *hash2style_node = font_hash_to_style_from_tag_size_flags(tag, size, flags);
    
    // ak: set up this style's run cache if needed
    if(hash2style_node->run_slots_frame_index != _font_state->frame_index)
    {
        hash2style_node->run_slots_count = 1024;
        hash2style_node->run_slots = arena_push(_font_state->frame_arena, Font_Run_Cache_Slot, hash2style_node->run_slots_count);
        hash2style_node->run_slots_frame_index = _font_state->frame_index;
    }
    
    // ak: unpack run params
    uint64_t run_hash = font_little_hash_from_string(5381, string);
    size_t run_slot_idx = run_hash%hash2style_node->run_slots_count;
    Font_Run_Cache_Slot *run_slot = &hash2style_node->run_slots[run_slot_idx];
    
    // ak: find existing run node for this string
    Font_Run_Cache_Node *run_node = 0;
    {
        for(Font_Run_Cache_Node *n = run_slot->first; n != 0; n = n->next)
        {
            if(str8_match(n->string, string, Str_Match_Flag_None))
            {
                run_node = n;
                break;
            }
        }
    }
    
    // ak: no run node? -> cache miss - compute & build & fill node if possible
    bool run_is_cacheable = 1;
    Font_Run run = STRUCT_ZERO;
    if(run_node)
    {
        run = run_node->run;
    }
    else
    {
        // ak: decode string & produce run pieces
        Font_Piece_Chunk_List piece_chunks = STRUCT_ZERO;
        Vec2_F32 dim = STRUCT_ZERO;
        bool font_handle_mapped_on_miss = 0;
        Font_Handle font_handle = STRUCT_ZERO;
        size_t piece_substring_start_idx = 0;
        size_t piece_substring_end_idx = 0;
        for(size_t idx = 0; idx <= string.length;)
        {
            // ak: decode next codepoint & get piece substring, or continuation rule
            uint8_t byte = (idx < string.length ? string.cstr[idx] : 0);
            bool need_another_codepoint = 0;
            if (byte == 0)
            {
                idx += 1;
            }
            else switch (utf8_class[byte>>3])
            {
                case 1:
                {
                    idx += 1;
                    piece_substring_end_idx += 1;
                    need_another_codepoint = 0;
                } break;
                default:
                {
                    Unicode_Decode decode = utf8_decode(string.cstr+idx, string.length-idx);
                    idx += decode.inc;
                    piece_substring_end_idx += decode.inc;
                    need_another_codepoint = 0;
                } break;
            }
            
            // ak: need another codepoint, or have no substring? -> continue
            if(need_another_codepoint || piece_substring_end_idx == piece_substring_start_idx)
            {
                continue;
            }
            
            // ak: do not need another codepoint? -> grab substring, bump piece start idx
            Str8 piece_substring = str8_substr(string, rng1(piece_substring_start_idx, piece_substring_end_idx));
            piece_substring_start_idx = idx;
            piece_substring_end_idx = idx;
            
            // ak: determine if this piece is a tab - if so, use space info to draw
            bool is_tab = (piece_substring.length == 1 && piece_substring.cstr[0] == '\t');
            if (is_tab)
            {
                run_is_cacheable = 0;
                piece_substring = str8(" ");
            }
            
            // ak: piece substring -> raster cache info
            Font_Raster_Cache_Info *info = 0;
            uint64_t piece_hash = 0;
            {
                // ak: fast path for utf8 class 1 -> direct map
                if (piece_substring.length == 1 && hash2style_node->utf8_class1_direct_map_mask[piece_substring.cstr[0]/64] & (1ull<<(piece_substring.cstr[0]%64)))
                {
                    info = &hash2style_node->utf8_class1_direct_map[piece_substring.cstr[0]];
                }
                
                // ak: more general, slower path for other glyphs
                if (piece_substring.length > 1)
                {
                    piece_hash = font_little_hash_from_string(5381, piece_substring);
                    size_t slot_idx = piece_hash%hash2style_node->hash2info_slots_count;
                    Font_Hash_To_Info_Raster_Cache_Slot *slot = &hash2style_node->hash2info_slots[slot_idx];
                    for (Font_Hash_To_Info_Raster_Cache_Node *node = slot->first; node != 0; node = node->hash_next)
                    {
                        if(node->hash == piece_hash)
                        {
                            info = &node->info;
                            break;
                        }
                    }
                }
            }
            
            // ak: no info found -> miss... fill this hash in the cache
            if (info == 0)
            {
                Arena_Temp scratch = arena_scratch_begin(0, 0);
                
                // ak: grab font handle for this tag if we don't have one already
                if(font_handle_mapped_on_miss == 0)
                {
                    font_handle_mapped_on_miss = 1;
                    
                    // ak: tag -> font slot index
                    size_t font_slot_idx = tag.u64[1] % _font_state->font_hash_table_size;
                    
                    // ak: tag * slot -> existing node
                    Font_Hash_Node *existing_node = 0;
                    {
                        for (Font_Hash_Node *n = _font_state->font_hash_table[font_slot_idx].first; n != 0 ; n = n->hash_next)
                        {
                            if(MemMatchStruct(&n->tag, &tag))
                            {
                                existing_node = n;
                                break;
                            }
                        }
                    }
                    
                    // ak: existing node -> font handle
                    if(existing_node != 0)
                    {
                        font_handle = existing_node->handle;
                    }
                }
                
                // ak: call into font provider to rasterize this substring
                Font_Raster_Result raster = STRUCT_ZERO;
                if(size > 0)
                {
                    raster = font_raster(scratch.arena, font_handle, floor_f32(size), piece_substring);
                }
                
                // ak: allocate portion of an atlas to upload the rasterization
                int16_t chosen_atlas_num = 0;
                Font_Atlas *chosen_atlas = 0;
                Rng2_I16 chosen_atlas_region = STRUCT_ZERO;
                if (raster.atlas_dim.x != 0 && raster.atlas_dim.y != 0)
                {
                    size_t num_atlases = 0;
                    for(Font_Atlas *atlas = _font_state->first_atlas;; atlas = atlas->next, num_atlases += 1)
                    {
                        // ak: create atlas if needed
                        if(atlas == 0 && num_atlases < 64)
                        {
                            atlas = arena_push(_font_state->raster_arena, Font_Atlas, 1);
                            DLLPushBack(_font_state->first_atlas, _font_state->last_atlas, atlas);
                            atlas->root_dim = (Vec2_I16){ 1024, 1024 };
                            atlas->root = arena_push(_font_state->raster_arena, Font_Atlas_Region_Node, 1);
                            atlas->root->max_free_size[Corner_TopLeft]     =
                            atlas->root->max_free_size[Corner_BottomLeft]  =
                            atlas->root->max_free_size[Corner_TopRight]    =
                            atlas->root->max_free_size[Corner_BottomRight] = (Vec2_I16){ (int16_t)(atlas->root_dim.x/2), (int16_t)(atlas->root_dim.y/2) };
                            atlas->texture = render_tex2d_alloc(Render_Resource_Kind_Dynamic, Render_Tex_2D_Format_RGBA8, (Vec2_I32){ (int32_t)atlas->root_dim.x, (int32_t)atlas->root_dim.y }, 0);
                        }
                        
                        // ak: allocate from atlas
                        if(atlas != 0)
                        {
                            Vec2_I16 needed_dimensions = (Vec2_I16){ (int16_t)(raster.atlas_dim.x + 2), (int16_t)(raster.atlas_dim.y + 2) };
                            chosen_atlas_region = font_atlas_region_alloc(_font_state->raster_arena, atlas, needed_dimensions);
                            if(chosen_atlas_region.x1 != chosen_atlas_region.x0)
                            {
                                chosen_atlas = atlas;
                                chosen_atlas_num = (int32_t)num_atlases;
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                
                // ak: upload rasterization to allocated region of atlas texture memory
                if(chosen_atlas != 0)
                {
                    Rng2_I32 subregion =
                    {
                        chosen_atlas_region.x0,
                        chosen_atlas_region.y0,
                        chosen_atlas_region.x0 + raster.atlas_dim.x,
                        chosen_atlas_region.y0 + raster.atlas_dim.y
                    };
                    render_fill_tex2d_region(chosen_atlas->texture, subregion, raster.atlas);
                }
                
                // ak: allocate & fill & push node
                {
                    if(piece_substring.length == 1)
                    {
                        info = &hash2style_node->utf8_class1_direct_map[piece_substring.cstr[0]];
                        hash2style_node->utf8_class1_direct_map_mask[piece_substring.cstr[0]/64] |= (1ull<<(piece_substring.cstr[0]%64));
                    }
                    else
                    {
                        size_t slot_idx = piece_hash%hash2style_node->hash2info_slots_count;
                        Font_Hash_To_Info_Raster_Cache_Slot *slot = &hash2style_node->hash2info_slots[slot_idx];
                        Font_Hash_To_Info_Raster_Cache_Node *node = arena_push_nz(_font_state->raster_arena, Font_Hash_To_Info_Raster_Cache_Node, 1);
                        DLLPushBack_NP(slot->first, slot->last, node, hash_next, hash_prev);
                        node->hash = piece_hash;
                        info = &node->info;
                    }
                    if(info != 0)
                    {
                        info->subrect    = chosen_atlas_region;
                        info->atlas_num  = chosen_atlas_num;
                        info->raster_dim = raster.atlas_dim;
                        info->advance    = raster.advance;
                    }
                }

                arena_scratch_end(scratch);
            }
            
            // ak: push piece for this raster portion
            if(info != 0)
            {
                // ak: find atlas
                Font_Atlas *atlas = 0;
                {
                    if(info->subrect.x1 != 0 && info->subrect.y1 != 0)
                    {
                        int32_t num = 0;
                        for(Font_Atlas *a = _font_state->first_atlas; a != 0; a = a->next, num += 1)
                        {
                            if(info->atlas_num == num)
                            {
                                atlas = a;
                                break;
                            }
                        }
                    }
                }
                
                // ak: on tabs -> expand advance
                float advance = info->advance;
                if(is_tab)
                {
                    advance = floor_f32(tab_size_px) - mod_f32(floor_f32(base_align_px), floor_f32(tab_size_px));
                }
                
                // ak: push piece
                {
                    Font_Piece *piece = font_piece_chunk_list_push_new(_font_state->frame_arena, &piece_chunks, string.length);
                    {
                        piece->texture = atlas ? atlas->texture : render_handle_zero();
                        piece->subrect = rng2p(
                            (int16_t)info->subrect.x0,
                            (int16_t)info->subrect.y0,
                            (int16_t)(info->subrect.x0 + info->raster_dim.x),
                            (int16_t)(info->subrect.y0 + info->raster_dim.y)
                        );
                        piece->advance = advance;
                        piece->decode_size = piece_substring.length;
                        piece->offset = (Vec2_I16){ 0, (int16_t)(-(hash2style_node->ascent + hash2style_node->descent)) };
                    }
                    base_align_px += advance;
                    dim.x += piece->advance;
                    dim.y = Max(dim.y, info->raster_dim.y);
                }
            }
        }
        
        // ak: tighten & fill
        {
            if(piece_chunks.node_count == 1)
            {
                run.pieces.v = piece_chunks.first->v;
                run.pieces.count = piece_chunks.first->count;
            }
            else
            {
                run.pieces = font_piece_array_from_chunk_list(_font_state->frame_arena, &piece_chunks);
            }
            run.dim = dim;
            run.ascent  = hash2style_node->ascent;
            run.descent = hash2style_node->descent;
        }
    }
    
    // ak: build node for cacheable runs
    if(run_is_cacheable)
    {
        run_node = arena_push(_font_state->frame_arena, Font_Run_Cache_Node, 1);
        SLLQueuePush(run_slot->first, run_slot->last, run_node);
        run_node->string = str8_copy(_font_state->frame_arena, string);
        run_node->run = run;
    }
    
    return run;
}

// ak: helpers

internal Vec2_F32 font_dim_from_tag_size_string(Font_Tag tag, float size, float base_align_px, float tab_size_px, Str8 string)
{
    Arena_Temp scratch = arena_scratch_begin(0, 0);
    Vec2_F32 result = STRUCT_ZERO;
    Font_Run run = font_run_from_string(tag, size, base_align_px, tab_size_px, 0, string);
    result = run.dim;
    arena_scratch_end(scratch);
    return result;
}

// ak: Main Calls =============================================================

internal void font_init(void)
{
    Arena *arena = arena_alloc();
    _font_state = arena_push(arena, _Font_State, 1);
    _font_state->arena = arena;
    _font_state->raster_arena = arena_alloc();
    _font_state->frame_arena = arena_alloc();
    _font_state->font_hash_table_size = 64;
    _font_state->font_hash_table = arena_push(_font_state->arena, Font_Hash_Slot, _font_state->font_hash_table_size);
    font_reset();
}

internal void font_reset(void)
{
    for(Font_Atlas *a = _font_state->first_atlas; a != 0; a = a->next)
    {
        render_tex2d_free(a->texture);
    }
    _font_state->first_atlas = _font_state->last_atlas = 0;
    arena_clear(_font_state->raster_arena);
    _font_state->hash2style_slots_count = 1024;
    _font_state->hash2style_slots = arena_push(_font_state->raster_arena, Font_Hash_To_Style_Raster_Cache_Slot, _font_state->hash2style_slots_count);
}

internal void font_frame(void)
{
    _font_state->frame_index += 1;
    arena_clear(_font_state->frame_arena);
}
