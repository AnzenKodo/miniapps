// clang -c src/audio/audio_miniaudio.c -o build/miniaudio.o -O3
// ar rcs build/libminiaudio.a build/miniaudio.o
#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_ALSA
#define MA_ENABLE_PULSEAUDIO
#define MA_ENABLE_JACK
// #define MA_NO_ENGINE
#define MINIAUDIO_IMPLEMENTATION
#include "./external/miniaudio.h"
