// ak: Helpers
//=============================================================================

internal Vec2_F32 game_random_pos_get(void)
{
    uint32_t time = os_now_unix();
    float    x    = rand_u32(time)      % (uint32_t)game_state->cells.x;
    float    y    = rand_u32(time+time) % (uint32_t)game_state->cells.y;
    Vec2_F32 pos  = (Vec2_F32){x, y};
    return pos;
}

internal void game_entity_pos_add(Game_Entity *entity, Vec2_F32 pos)
{
    if (entity->pos_array.length+1 <= entity->pos_array.capacity)
    {
        entity->pos_array.length++;
    }
    entity->pos_array.v[entity->pos_array.length-1] = pos;
}

internal Rng2_F32 game_calc_rect_from_pos(Vec2_F32 pos)
{
    Vec2_F32 cell_pos = scale_vec2(pos, game_state->cell_size);
    Vec2_F32 cell_dim = add_vec2(cell_pos, game_state->rect.playground.v[0]);
    Vec2_F32 cell_size = add_vec2(cell_dim, ((Vec2_F32){ (float)game_state->cell_size, (float)game_state->cell_size }));
    Rng2_F32 rect = rng2(cell_dim, cell_size);
    return rect;
}

internal void game_draw_text(Str8 text, float size, Vec2_F32 position)
{
    draw_text(game_state->font, game_state->cell_size*size, 0.f, 4.f, Font_Raster_Flag_Smooth, position, game_state->color.foreground, text);
}

internal void game_draw_text_in_center(Str8 text, float size, float adjust_y)
{
    Vec2_F32 dim = font_dim_from_tag_size_string(game_state->font, size, 0.f, 4.f, text);
    Vec2_F32 pos = STRUCT_ZERO;
    pos.x = game_state->center.x - (dim.x / 2.f);
    pos.y = game_state->center.y - ((dim.y - (game_state->cell_size * adjust_y)) / 2.f);
    game_draw_text(text, size, pos);
}

// ak: Core
//=============================================================================

internal void game_snake_reset(void)
{
    Game_Entity *entity = &game_state->entities[Game_Entity_Type_Snake];
    entity->pos_array.length = 0;
    
    Vec2_F32 center_cell = scale_vec2(game_state->cells, 0.5f);
    game_entity_pos_add(entity, center_cell);
    center_cell.y--;
    game_entity_pos_add(entity, center_cell);
    center_cell.y--;
    game_entity_pos_add(entity, center_cell);
    center_cell.y--;
    game_entity_pos_add(entity, center_cell);
}

internal void game_init(void)
{
    Arena *arena = arena_alloc();
    game_state = arena_push(arena, Game_State, 1);
    game_state->arena = arena;
    // ak: init colors
    /*
        NOTE(ak): Nintendo Gameboy (bgb)
        - Link: https://lospec.com/palette-list/nintendo-gameboy-bgb
        - Colors:
            #081820
            #346856
            #88c070
            #e0f8d0
    */
    game_state->color.background = linear_from_srgba(rgba_from_u32(APP_BACKGROUND_COLOR));
    game_state->color.foreground = linear_from_srgba(rgba_from_u32(APP_FOREGROUND_COLOR));
    game_state->color.playground = linear_from_srgba(rgba_from_u32(0x346856ff));
    game_state->cells            = (Vec2_F32){ 80, 50 };
    
    for EachEnumVal(Game_Entity_Type, type)
    {
        Game_Entity *entity = &game_state->entities[type];
        switch (type)
        {
            case Game_Entity_Type_Apple:
            {
                entity->color              = linear_from_srgba(rgba_from_u32(0xb41c39ff));
                entity->pos_array.capacity = 1;
                entity->pos_array.v        = arena_push(arena, Vec2_F32, entity->pos_array.capacity);
                game_entity_pos_add(entity, game_random_pos_get());
            } break;
            case Game_Entity_Type_Snake:
            {
                entity->color              = linear_from_srgba(rgba_from_u32(0x88c070ff));
                entity->pos_array.capacity = game_state->cells.x*game_state->cells.y;
                entity->pos_array.v        = arena_push(arena, Vec2_F32, entity->pos_array.capacity);
                Vec2_F32 center_cell       = scale_vec2(game_state->cells, 0.5f);
                game_entity_pos_add(entity, center_cell);
                center_cell.y--;
                game_entity_pos_add(entity, center_cell);
                center_cell.y--;
                game_entity_pos_add(entity, center_cell);
                center_cell.y--;
                game_entity_pos_add(entity, center_cell);
            } break;
            case Game_Entity_Type_COUNT: break;
        }
    }
}

internal void game_loop(Arena *arena)
{
    if (game_state->event.window_resize)
    {
        {
            game_state->cell_size = game_state->rect.canvas.x1/90;
        }
        
        {
            game_state->center = center_rng2(game_state->rect.canvas);
            float half_width = game_state->cell_size * 40.f;
            float half_height = game_state->cell_size * 25.f;
            game_state->rect.playground = rng2p(
                game_state->center.x - half_width, game_state->center.y - half_height,
                game_state->center.x + half_width, game_state->center.y + half_height
            );
        }
    }
    
    {
        Game_Entity *snake_entity = &game_state->entities[Game_Entity_Type_Snake];
        Game_Entity *apple_entity = &game_state->entities[Game_Entity_Type_Apple];
        Vec2_F32 snake_head = snake_entity->pos_array.v[0];
        Vec2_F32 snake_tail = snake_entity->pos_array.v[snake_entity->pos_array.length-1];
        Vec2_F32 apple_pos = apple_entity->pos_array.v[0];
        
        // ak: update snake head according to movement direction
        switch (game_state->event.direction)
        {
            case Game_Direction_Up:
            {
                snake_head.y--;
            } break;
            case Game_Direction_Down:
            {
                snake_head.y++;
            } break;
            case Game_Direction_Left:
            {
                snake_head.x--;
            } break;
            case Game_Direction_Right:
            {
                snake_head.x++;
            } break;
        }
        if (snake_head.y >= game_state->cells.y)
        {
            snake_head.y = 0;
        }
        if (snake_head.y < 0)
        {
            snake_head.y = game_state->cells.y-1;
        }
        if (snake_head.x >= game_state->cells.x)
        {
            snake_head.x = 0;
        }
        if (snake_head.x < 0)
        {
            snake_head.x = game_state->cells.x-1;
        }
        
        // ak: update sanke body according to head
        for (size_t i = snake_entity->pos_array.length - 1; i > 0; i--)
        {
            snake_entity->pos_array.v[i] = snake_entity->pos_array.v[i - 1];
        }
        
        // ak: check for body collision
        for (size_t i = 0; i < snake_entity->pos_array.length; i++)
        {
            if (snake_entity->pos_array.v[i].x == snake_head.x &&
                snake_entity->pos_array.v[i].y == snake_head.y &&
                game_state->score.current != 0)
            {
                game_snake_reset();
                game_state->game_over = true;
                snake_head = snake_entity->pos_array.v[0];
            }
        }
        
        // ak: check if snake eaten has apple, if yes then get new pos for apple
        if (snake_head.x == apple_pos.x && snake_head.y == apple_pos.y)
        {
            apple_entity->pos_array.v[0] = game_random_pos_get();
            game_entity_pos_add(snake_entity, snake_tail);
            game_state->score.current++;
            if (audio_handle_is_valid(game_state->sound_eat))
            {
                audio_play(game_state->sound_eat, audio_play_params_default());
            }
        }
        
        // ak: update snake head
        snake_entity->pos_array.v[0] = snake_head;
    }
    
    // ak: Draw ===============================================================
    
    // ak: draw playground
    draw_rect(game_state->rect.playground, game_state->color.playground, 0.f, 0.f, 0.f);
    
    Str8 score_text = str8f(arena, "Score: %zu / Max Score: %zu", game_state->score.current, game_state->score.max);
    if (game_state->game_over)
    {
        game_draw_text_in_center(str8("Game Over"), 3.f, 0);
        game_draw_text_in_center(score_text, 1.1f, 4.f);
        game_draw_text_in_center(str8("Press 'Enter' to restart"), 1.3f, 15.f);
    }
    else
    {
        draw_grid(game_state->rect.playground, game_state->cell_size, 1, game_state->color.background);
        
        // ak: draw entities
        for EachEnumVal(Game_Entity_Type, type)
        {
            Game_Entity entity = game_state->entities[type];
            for EachIndex(i, entity.pos_array.length)
            {
                Rng2_F32 rect = game_calc_rect_from_pos(entity.pos_array.v[i]);
                draw_rect(rect, entity.color, 0.f, 0.f, 0.f);
            }
        }
        
        Vec2_F32 text_pos = STRUCT_ZERO;
        text_pos.x = game_state->rect.playground.x0,
        text_pos.y = game_state->rect.playground.y0-game_state->cell_size;
        game_draw_text(score_text, 1, text_pos);
    }
}
