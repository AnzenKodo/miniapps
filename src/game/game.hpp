#ifndef GAME_HPP
#define GAME_HPP

// ak: Types
//=============================================================================

typedef enum Game_Entity_Type
{
    Game_Entity_Type_Snake,
    Game_Entity_Type_Apple,
    Game_Entity_Type_COUNT,
}
Game_Entity_Type;

typedef struct Game_Entity Game_Entity;
struct Game_Entity
{
    Vec4_F32 color;
    struct {
        Vec2_F32 *v;
        size_t length;
        size_t capacity;
    } pos_array;
};

typedef enum Game_Direction
{
    Game_Direction_Up,
    Game_Direction_Down,
    Game_Direction_Left,
    Game_Direction_Right,
}
Game_Direction;

typedef struct Game_State Game_State;
struct Game_State
{
    Arena *arena;
    Font_Tag font;
    Audio_Handle sound_eat;
    
    size_t cell_size;
    Vec2_F32 cells;
    Vec2_F32 center;
    
    struct {
        bool window_resize;
        Game_Direction direction;
    } event;
    
    struct {
        Vec4_F32 background;
        Vec4_F32 playground;
        Vec4_F32 foreground;
        Vec4_F32 red;
    } color;
    
    struct {
        Rng2_F32 canvas;
        Rng2_F32 playground;
    } rect;
    
    struct {
        size_t current;
        size_t max;
    } score;
    
    Game_Entity entities[Game_Entity_Type_COUNT];
    bool game_over;
};

// ak: Globals
//=============================================================================

global Game_State *game_state = NULL;

#endif // GAME_HPP
