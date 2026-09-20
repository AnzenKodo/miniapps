#define STB_TRUETYPE_IMPLEMENTATION
#include "../font/external/stb_truetype.h"

#define KB_TEXT_SHAPE_IMPLEMENTATION
#include "../font/external/kb_text_shape.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

unsigned char* load_file(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    *out_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buffer = (unsigned char*)malloc(*out_size);
    if (buffer) fread(buffer, 1, *out_size, f);
    fclose(f);
    return buffer;
}

int main(int argc, char** argv) {
    const char* font_path = "./assets/font/VendSans-Regular.ttf";
    const char* text = (argc >= 3) ? argv[2] : "Hello World(:-)^_^)";  // Text with potential ligatures (fi, fl, etc.)

    size_t ttf_size = 0;
    unsigned char* ttf_buffer = load_file(font_path, &ttf_size);
    if (!ttf_buffer) {
        printf("Failed to load font file\n");
        return 1;
    }

    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, ttf_buffer, 0)) {
        printf("Failed to initialize font\n");
        free(ttf_buffer);
        return 1;
    }

    float font_pixel_height = 96.0f;
    float scale = stbtt_ScaleForPixelHeight(&info, font_pixel_height);

    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);

    const int img_w = 1280;
    const int img_h = 300;
    unsigned char* rgb = (unsigned char*)calloc(img_w * img_h * 3, 1);  // White background (0xff would be white, but calloc gives 0; we'll treat as white)

    // Fill with white
    memset(rgb, 255, img_w * img_h * 3);

    kbts_shape_context* ctx = kbts_CreateShapeContext(0, 0);  // Default allocator (malloc/free)

    kbts_ShapePushFontFromMemory(ctx, ttf_buffer, (int)ttf_size, 0);

    kbts_ShapeBegin(ctx, KBTS_DIRECTION_LTR, KBTS_LANGUAGE_DONT_KNOW);
    kbts_ShapeUtf8(ctx, text, (int)strlen(text), KBTS_USER_ID_GENERATION_MODE_CODEPOINT_INDEX);
    kbts_ShapeEnd(ctx);

    // Compute total width for centering
    float total_width = 0.0f;
    kbts_run run;
    while (kbts_ShapeRun(ctx, &run)) {
        kbts_glyph* glyph;
        kbts_glyph_iterator iter = run.Glyphs;
        while (kbts_GlyphIteratorNext(&iter, &glyph)) {
            total_width += (float)glyph->AdvanceX * scale;
        }
    }

    float start_x = (img_w - total_width) / 2.0f;
    float pos_x = start_x;
    float baseline_y = img_h / 2.0f + (ascent * scale);

    // Second pass: render (reshape to reset runs)
    kbts_ShapeBegin(ctx, KBTS_DIRECTION_LTR, KBTS_LANGUAGE_DONT_KNOW);
    kbts_ShapeUtf8(ctx, text, (int)strlen(text), KBTS_USER_ID_GENERATION_MODE_CODEPOINT_INDEX);
    kbts_ShapeEnd(ctx);

    while (kbts_ShapeRun(ctx, &run)) {
        kbts_glyph* glyph;
        kbts_glyph_iterator iter = run.Glyphs;
        while (kbts_GlyphIteratorNext(&iter, &glyph)) {
            int gid = glyph->Id;
            if (gid == 0) continue;  // Missing glyph

            float gx = pos_x + (float)glyph->OffsetX * scale;
            float gy = baseline_y + (float)glyph->OffsetY * scale;

            int gw, gh, gxoff, gyoff;
            unsigned char* bitmap = stbtt_GetGlyphBitmap(&info, scale, scale, gid, &gw, &gh, &gxoff, &gyoff);

            if (bitmap && gw > 0 && gh > 0) {
                for (int y = 0; y < gh; ++y) {
                    for (int x = 0; x < gw; ++x) {
                        int px = (int)floorf(gx + gxoff + x + 0.5f);
                        int py = (int)floorf(gy + gyoff + y + 0.5f);
                        if (px >= 0 && px < img_w && py >= 0 && py < img_h) {
                            unsigned char alpha = bitmap[y * gw + x];
                            if (alpha > 0) {
                                float a = alpha / 255.0f;
                                int idx = (py * img_w + px) * 3;
                                for (int c = 0; c < 3; ++c) {
                                    rgb[idx + c] = (unsigned char)(rgb[idx + c] * (1.0f - a));
                                }
                            }
                        }
                    }
                }
                stbtt_FreeBitmap(bitmap, NULL);
            }
            pos_x += (float)glyph->AdvanceX * scale;
        }
    }
    // Write PPM (P6 binary)
    FILE* out = fopen("./build/img.ppm", "wb");
    if (out) {
        fprintf(out, "P6\n%d %d\n255\n", img_w, img_h);
        fwrite(rgb, 1, img_w * img_h * 3, out);
        fclose(out);
        printf("Rendered text to output.ppm\n");
    } else {
        printf("Failed to write output.ppm\n");
    }
    free(rgb);
    free(ttf_buffer);
    kbts_DestroyShapeContext(ctx);
    return 0;
}
