#include "std/base/base_include.h"
#include "std/os/os_include.h"
#include "std/audio/audio.h"

#include "std/base/base_include.c"
#include "std/os/os_include.c"
#include "std/audio/audio.c"

internal void base_main(void)
{
    Arena *arena = arena_alloc();
    audio_init();
    Audio_Sound sound = audio_sound_load(str8("resources/sound.wav"), arena);
    audio_uninit();
    arena_free(arena);
}
