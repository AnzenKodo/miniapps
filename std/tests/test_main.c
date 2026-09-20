#define GL_GLEXT_PROTOTYPES

#include "main.h"

typedef struct Raw_Position Raw_Position;
struct Raw_Position {
    int32_t tilemap_x;
    int32_t tilemap_y;
    float x;
    float y;
};

typedef struct Canonical_Position Canonical_Position;
struct Canonical_Position {
    int32_t tilemap_x;
    int32_t tilemap_y;
    int32_t tile_x;
    int32_t tile_y;
    float tile_rel_x;
    float tile_rel_y;
};

typedef struct Game_State Game_State;
struct Game_State
{
    int32_t player_tilemap_x;
    int32_t player_tilemap_y;
    float player_x;
    float player_y;
};

typedef struct Tilemap Tilemap;
struct Tilemap
{
    uint32_t *tiles;
};

typedef struct World World;
struct World
{
    Tilemap *tilemaps;
    int32_t tilemap_count_x;
    int32_t tilemap_count_y;
    int32_t count_x;
    int32_t count_y;
    float upper_left_x;
    float upper_left_y;
    float tile_width;
    float tile_height;

    float tile_side_in_meters;
    int32_t tile_side_in_pixels;
};

internal int32_t floor_f32_to_i32(float a) 
{
    return cast(int32_t)floor_f32(a);
}

internal uint32_t tilemap_get_tile_value_unchecked(
    World *world,Tilemap *tilemap, int32_t tile_x, int32_t tile_y
)
{
    Assert(tilemap);
    Assert((tile_x >= 0) && (tile_x < world->length_x) &&
        (tile_y >= 0) && (tile_y < world->length_y));
    uint32_t tilemap_value = tilemap->tiles[tile_y*world->length_x + tile_x];
    return tilemap_value;
}

internal bool tilemap_point_is_empty(
    World *world, Tilemap *tilemap, int32_t test_tile_x, int32_t test_tile_y
)
{
    bool empty = false;

    if (tilemap) {
        if (
            (test_tile_x >= 0) && (test_tile_x < world->length_x) &&
            (test_tile_y >= 0) && (test_tile_y < world->length_y)
        ) {
            int32_t tilemap_value = tilemap_get_tile_value_unchecked(
                world, tilemap, test_tile_x, test_tile_y
            );
            empty = (tilemap_value == 0);
        }
    }

    return empty;
}

internal Tilemap *tilemap_get(World *world, int32_t tilemap_x, int32_t tilemap_y)
{
    Tilemap *tilemap = 0;
    if (
        (tilemap_x >= 0) && (tilemap_x < world->tilemap_count_x) &&
        (tilemap_y >= 0) && (tilemap_y < world->tilemap_count_y)
    ) {
        tilemap = &world->tilemaps[tilemap_y*world->tilemap_count_x + tilemap_x];
    }

    return tilemap;
}

internal Canonical_Position get_canonical_position(World *world, Raw_Position pos)
{
    Canonical_Position result;

    result.tilemap_x = pos.tilemap_x;
    result.tilemap_y = pos.tilemap_y;

    float x = pos.x - world->upper_left_x;
    float y = pos.y - world->upper_left_y;
    result.tile_x = floor_f32_to_i32(x / world->tile_width);
    result.tile_y = floor_f32_to_i32(y / world->tile_height);

    result.tile_rel_x = x - result.tile_x*world->tile_width;
    result.tile_rel_y = y - result.tile_y*world->tile_height;

    Assert(result.tile_rel_x >= 0);
    Assert(result.tile_rel_y >= 0);
    Assert(result.tile_rel_x < world->tile_width);
    Assert(result.tile_rel_y < world->tile_height);

    if (result.tile_x < 0)
    {
        result.tile_x = world->length_x + result.tile_x;
        --result.tilemap_x;
    }
    if (result.tile_y < 0)
    {
        result.tile_y = world->length_y + result.tile_y;
        --result.tilemap_y;
    }
    if (result.tile_x >= world->length_x)
    {
        result.tile_x = world->length_x - result.tile_x;
        ++result.tilemap_x;
    }
    if (result.tile_y >= world->length_y)
    {
        result.tile_y = world->length_y - result.tile_y;
        ++result.tilemap_y;
    }

    return result;
}

internal bool world_point_is_empty(
    World *world, Raw_Position test_pos
) {
    bool empty = false;
    Canonical_Position can_pos = get_canonical_position(world, test_pos);
    Tilemap *tilemap = tilemap_get(world, can_pos.tilemap_x, can_pos.tilemap_y);
    empty = tilemap_point_is_empty(
        world, tilemap, can_pos.tile_x, can_pos.tile_y
    );

    return empty;
}

int main(void)
{
    TCTX tctx;
    Arena *arena = arena_alloc();

    wl_window_open(str8_comp("Scuttle"), 960, 540);
    // wl_set_window_icon((const uint8_t *)SCUTTLEICON);

    // float monitor_refresh_rate = 60;
    // float game_update_hz = monitor_refresh_rate / 2.0f;
    // float target_sec_per_frame = 10.0f / game_update_hz;

    Draw_Buffer *buffer = render_init();

    // Game Start ======================================================
#define TILEMAP_COUNT_X 15
#define TILEMAP_COUNT_Y 10
    int32_t tiles00[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] = {
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0 },
        { 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    };
    int32_t tiles01[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    };
    int32_t tiles10[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    };
    int32_t tiles11[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },
        { 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    };

    Tilemap tilemaps[2][2];
    tilemaps[0][0].tiles = (uint32_t *)tiles00;
    tilemaps[0][1].tiles = (uint32_t *)tiles10;
    tilemaps[1][0].tiles = (uint32_t *)tiles01;
    tilemaps[1][1].tiles = (uint32_t *)tiles11;


    World world;
    world.count_x = TILEMAP_COUNT_X;
    world.count_y = TILEMAP_COUNT_Y;
    world.upper_left_x = -30;
    world.upper_left_y = 0;
    world.tile_width = 30;
    world.tile_height = 30;
    world.tilemap_count_x = 2;
    world.tilemap_count_y = 2;
    world.tilemaps = (Tilemap *)tilemaps;
    world.tile_side_in_meters = 1.4f;
    world.tile_side_in_pixels = 60;
    float player_width = 0.75f * world.tile_width;
    float player_height = world.tile_height;

    Game_State *game_state = arena_push(arena, Game_State, 1);
    game_state->player_x = 50;
    game_state->player_y = 260;

    Tilemap *tilemap = tilemap_get(
        &world, game_state->player_tilemap_x, game_state->player_tilemap_y
    );
    Assert(tilemap);

    // =================================================================

    while (!wl_should_window_close()) {
        // wl_enforce_frame_rate(60);
        // float frame_rate = wl_get_current_frame_rate();
        wl_update_events();
        if (wl_is_key_pressed(Wl_Key_Esc))
        {
            wl_set_window_close();
        }

        render_begin();
        {
            draw_fill(buffer, ORANGE);

            float d_player_x = 0.0f;
            float d_player_y = 0.0f;
            if (wl_is_key_pressed(Wl_Key_W)) {
                d_player_y = -1.0f;
            } else if (wl_is_key_pressed(Wl_Key_S)) {
                d_player_y = 1.0f;
            } else if (wl_is_key_pressed(Wl_Key_A)) {
                d_player_x = -1.0f;
            } else if (wl_is_key_pressed(Wl_Key_D)) {
                d_player_x = 1.0f;
            }
            d_player_x *= 210.0f;
            d_player_y *= 210.0f;

            // float target_sec_per_frame = (1.0f /frame_rate);
            float target_sec_per_frame = (1.0f /60);
            float new_player_x = game_state->player_x + target_sec_per_frame * d_player_x;
            float new_player_y = game_state->player_y + target_sec_per_frame * d_player_y;

            Raw_Position player_pos = {
                game_state->player_tilemap_x, game_state->player_tilemap_y,
                new_player_x, new_player_y
            };
            Raw_Position player_pos_left = player_pos;
            player_pos_left.x -= 0.5f*player_width;
            Raw_Position player_pos_right = player_pos;
            player_pos_right.x += 0.5f*player_width;

            if (
                world_point_is_empty(&world, player_pos)
                && world_point_is_empty(&world, player_pos_left)
                && world_point_is_empty( &world, player_pos_right)
            ) {
                Canonical_Position con_pos = get_canonical_position(&world, player_pos);
                game_state->player_tilemap_x = con_pos.tilemap_x;
                game_state->player_tilemap_y = con_pos.tilemap_y;
                game_state->player_x = world.upper_left_x + world.tile_width *con_pos.tile_x + con_pos.tile_rel_x;
                game_state->player_y = world.upper_left_y + world.tile_height*con_pos.tile_y + con_pos.tile_rel_y;
            }

            for (int32_t row = 0; row < world.count_y; ++row) {
                for (int32_t col = 0; col < world.count_x; ++col) {
                    int32_t tile_id =  tilemap_get_tile_value_unchecked(&world, tilemap, col, row);

                    Rgba color = GREEN;
                    if (tile_id == 1) {
                        color = YELLOW;
                    }

                    float rect_x = world.upper_left_x + cast(float)col * world.tile_width;
                    float rect_y = world.upper_left_y + cast(float)row * world.tile_height;
                    float rect_width  = rect_x + world.tile_width;
                    float rect_height = rect_y + world.tile_height;
                    draw_rect(buffer, (Rect){
                        rect_x, rect_y, rect_width, rect_height
                    }, color);
                }
            }

            float player_left = game_state->player_x - (0.5f * player_width);
            float player_top = game_state->player_y - player_height;

            draw_rect(buffer, (Rect) {
                player_left, player_top,
                player_left + player_width, player_top + player_height
            }, BLUE);

            // Draw tile dot
            draw_rect(buffer, (Rect) {
                new_player_x, new_player_y,
                new_player_x + 2, new_player_y + 2
            }, BLACK);
        }

        render_end();
    }

    render_close();
    wl_window_close();
    arena_free(arena);
    // tctx_release();
    return 0;
}
