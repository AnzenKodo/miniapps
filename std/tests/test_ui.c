// ak: headers
#include "./app.h"
#include "../base/base_include.h"
#include "../os/os_include.h"
#include "../window_layer/window_layer_include.h"
#include "../render/render_include.h"
#include "../ui/ui_include.h"

// ak: implementation
#include "../base/base_include.c"
#include "../os/os_include.c"
#include "../window_layer/window_layer_include.c"
#include "../render/render_include.c"
#include "../ui/ui_include.c"

internal void base_main(void)
{
    Arena *arena = arena_alloc();
    
    // ak: Program Init =======================================================
    unsigned int width = 150;
    unsigned int height = 300;
    // unsigned int row_height = 25;
    wl_window_open(str8(APP_NAME), width, height);
    wl_window_border_set(false);
    wl_window_pos_set(wl_display_width_get()-width-30, 0);
    render_init();
    UI_State *ui = ui_state_alloc();
    
    // ak: Program Loop =======================================================
    while (!wl_should_window_close())
    {
        render_begin();
        wl_set_fps(60);
        wl_update_events();
        if ((wl_is_key_pressed(Wl_Key_Esc)) ||
            wl_is_event_happen(Wl_EventType_WindowClose))
        {
            wl_set_window_close();
        }
        
        // ak: build ui
        UI_Build(ui)
        {
            UI_Pref_Width(ui_px(150.f, 1.f))
            UI_Pref_Height(ui_px(30.f, 1.f))
            {
                ui_button(str8("Here is a panel."));
            }
            ui_pref_width_pop();
            ui_pref_height_pop();
        }
        
        // ak: render ui
        UI_Box_Rec rec;
        for (UI_Box *box = ui_root_from_state(ui); !ui_box_is_nil(box); box = rec.next)
        {
            if (box->flags & UI_Box_Flag_DrawBackground)
            {
                render_draw_rect_push(box->rect, (Render_Color){ 212, 190, 152, 255 });
            }
            rec = ui_box_rec_df_post(box, &ui_box_nil);
        }
        render_end();
    }
    
    // ak: Free Everything ====================================================
    wl_window_close();
    render_deinit();
}
