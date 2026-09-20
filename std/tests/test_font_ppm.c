// To compile run this: cc src/test/test.c && ./a.out

#include "../base/base_include.h"
#include "../os/os_include.h"
#include "../font/font.h"

#include "../base/base_include.c"
#include "../os/os_include.c"
#include "../font/font.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char* load_file(const char* filename, int* out_size) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
        printf("Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char* buffer = (unsigned char*)malloc(sz);
    if (!buffer) {
        fclose(f);
        return NULL;
    }
    fread(buffer, 1, sz, f);
    fclose(f);
    if (out_size) *out_size = (int)sz;
    return buffer;
}

void save_ppm(const char* filename, const unsigned char* pixels, int w, int h) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        printf("Error: Cannot write to file '%s'\n", filename);
        return;
    }
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (int i = 0; i < w * h; ++i) {
        unsigned char val = 255 - pixels[i];  // Invert: black text on white background
        fputc(val, f);
        fputc(val, f);
        fputc(val, f);
    }
    fclose(f);
    printf("Saved font atlas to '%s'\n", filename);
}

void entry_point(void) {
    const char* ttf_file = "./assets/font/VendSans-Regular.ttf";
    const char* ppm_file = "./build/img.ppm";
    float pixel_height = 100;
    int bitmap_size = 800;

    if (pixel_height <= 0 || bitmap_size <= 0 || bitmap_size > 16384) {
        printf("Error: Invalid pixel_height or bitmap_size\n");
        // return 1;
    }

    int ttf_size;
    unsigned char* ttf_buffer = load_file(ttf_file, &ttf_size);
    if (!ttf_buffer) {
        // return 1;
    }

    unsigned char* bitmap = (unsigned char*)calloc(bitmap_size * bitmap_size, 1);
    if (!bitmap) {
        printf("Error: Out of memory\n");
        free(ttf_buffer);
        // return 1;
    }

    stbtt_bakedchar cdata[96];  // For characters 32 to 127
    int res = stbtt_BakeFontBitmap(ttf_buffer, 0, pixel_height, bitmap,
                                   bitmap_size, bitmap_size,
                                   32, 96, cdata);

    free(ttf_buffer);

    if (res <= 0) {
        printf("Error: Font baking failed.\n");
        if (res < 0) {
            printf("Only %d characters fitted before running out of space.\n", -res);
            printf("Try a larger bitmap_size.\n");
        }
        free(bitmap);
        // return 1;
    }

    printf("Success: All 96 characters baked, using %d pixel rows.\n", res);
    save_ppm(ppm_file, bitmap, bitmap_size, bitmap_size);
    free(bitmap);

    // return 0;
}
