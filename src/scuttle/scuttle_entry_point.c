#define RENDER_BACKEND RENDER_BACKEND_OPENGL

#include "../base/base_include.h"
#include "../os/os_include.h"
#include "../window_layer/window_layer_include.h"
#include "../draw/draw_include.h"
#include "../render/render_include.h"
#include "../scuttle/scuttle_include.h"

#include "../base/base_include.c"
#include "../os/os_include.c"
#include "../window_layer/window_layer_include.c"
#include "../draw/draw_include.c"
#include "../render/render_include.c"
#include "../scuttle/scuttle_include.c"

#include <GLES2/gl2ext.h>

internal void
gen_next(int32_t *tilemap, int32_t width, int32_t height)
{
    for (int32_t w = 0; w < width; w++)
    {
        for (int32_t h = 0; h < height; h++)
        {
            int32_t alive_count = 0;
            for (int32_t k = -1; k <= 1; k++)
            {
                for (int32_t l = -1; l <= 1; l++)
                {
                    if (k == 0 && l == 0) continue;
                    if (w + k < height && w + k >= 0 && h + l < width && h + l >= 0)
                    {
                        if (tilemap[(w+k)*width+(h+l)] == 1)
                        {
                            alive_count++;
                        }
                    }
                }
            }

            switch (alive_count)
            {
                case 0:
                case 1: {
                    tilemap[w*width+h] = 0;
                    break;
                }
                case 2:
                case 3: {
                    if (tilemap[w*width+h] == 0 && alive_count == 3)
                    {
                        tilemap[w*width+h] = 1;
                    }
                    break;
                }
                default: {
                    tilemap[w*width+h] = 0;
                    break;
                }
            }
        } // for h
    } // for w
}

internal void
entry_point(char *argv[])
{
    // Program Init ===========================================================
    wl_window_open(str8("Scuttle"), vec2i32(750, 750));
    wl_window_icon_set(cast(uint32_t *)ICON, ICON_WIDTH, ICON_HEIGHT);
    uint64_t size = MB(10);
    void *buffer = os_mem_alloc(size);
    Arena *arena = alloc_arena_init(buffer, size);
    Draw_Buffer draw_buffer = render_init(arena);

    // Tilemap Init ===========================================================
    uint32_t tile_height = 20;
    uint32_t tile_width = 20;
    int32_t tilemap[TILEMAP_COUNT_Y][TILEMAP_COUNT_X];
    mem_set(tilemap, 0, sizeof(tilemap));
    for (int32_t i = 0; i < TILEMAP_COUNT_X/5; i++)
    {
        for (int32_t j = 0; j < TILEMAP_COUNT_X/5; j++)
        {
            tilemap[i][j] = 1;
        }
    }

    while (!wl_should_window_close())
    {
        wl_set_fps(60);
        wl_update_events();
        if (wl_is_key_pressed(Wl_Key_Esc) ||
            wl_is_event_happen(Wl_EventType_WindowClose))
        {
            wl_set_window_close();
        }

        render_begin();
        {
            // Draw Tile oop ==================================================
            gen_next(&tilemap, TILEMAP_COUNT_Y, TILEMAP_COUNT_X);
            for (int32_t row = 0; row < TILEMAP_COUNT_Y; ++row) 
            {
                for (int32_t col = 0; col < TILEMAP_COUNT_X; ++col) 
                {
                    Draw_Rgba color = DRAW_BLUE;
                    if (tilemap[row][col] == 1)
                    {
                       color = DRAW_YELLOW;
                    }

                    float rect_x = cast(float)col * tile_width;
                    float rect_y = cast(float)row * tile_height;
                    float rect_width  = rect_x + tile_width;
                    float rect_height = rect_y + tile_height;
                    draw_rect(draw_buffer, (Draw_Rect){
                       rect_x, rect_y, rect_width, rect_height
                    }, color);
                } // for col
            } // for row

            uint32_t width = wl_get_window_width();
            uint32_t height = wl_get_window_height();
            glViewport(0, 0, width, height);

            GLuint texture_handle = 0;
            glGenTextures(1, &texture_handle);
            glBindTexture(GL_TEXTURE_2D, texture_handle);
            glTexImage2D(
                GL_TEXTURE_2D, 0, GL_RGBA,
                draw_buffer.width, draw_buffer.height,
                0, GL_BGRA_EXT, GL_UNSIGNED_BYTE,
                draw_buffer.memory
            );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glEnable(GL_TEXTURE_2D);

            glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            // Main loop:
            renderTriangles();
            glDrawArrays(GL_TRIANGLES, 0, 6);

            eglSwapBuffers(render_opengl_state.display, render_opengl_state.surface);
        }
        render_end();
    }

    // Free Everything ========================================================
    render_deinit();
    os_mem_release(buffer, size);
    wl_window_close();
}
