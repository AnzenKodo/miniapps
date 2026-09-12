#define MA_ENABLE_ONLY_SPECIFIC_BACKENDS
#define MA_ENABLE_ALSA
#define MA_ENABLE_PULSEAUDIO
#define MA_ENABLE_JACK
#define MA_NO_ENGINE
#include "./external/miniaudio.h"

// ak: Types
//=============================================================================

#define AUDIO_MAX_BUSES 64

// ak: Types
//=============================================================================

// ak: Handles ================================================================

typedef struct Audio_Sound Audio_Sound;
struct Audio_Sound
{
    uint64_t u64[1];
};

typedef struct Audio_Voice Audio_Voice;
struct Audio_Voice
{
    uint64_t u64[1];
};

// ak: Enums ==================================================================

typedef uint32_t Audio_Load_Flags;
enum
{
    Audio_Load_Flag_Stream    = (1<<0), // decode on the fly instead of loading fully into memory
    Audio_Load_Flag_Loop      = (1<<1), // default the audio to looping playback
};

typedef struct _Audio_Sound _Audio_Sound;
struct _Audio_Sound
{
    _Audio_Sound     *next;
    U8Array          data;
    Str8             path;
    Audio_Load_Flags flags;
    bool             in_use;
};

// ak: Play Parameters ========================================================

typedef struct Audio_Play_Params Audio_Play_Params;
struct Audio_Play_Params
{
    float    volume; // 0 = silent, 1 = unity gain
    float    pitch;  // 1 = unmodified
    float    pan;    // -1 = full left, 0 = center, 1 = full right
    uint32_t bus;    // index in [0, SND_MAX_BUSES)
    bool     loop;
};

// ak: Voice ==================================================================

typedef enum _Audio_Voice_State
{
    _Audio_Voice_State_Inactive,
    _Audio_Voice_State_Playing,
    _Audio_Voice_State_Paused,
    _Audio_Voice_State_Finished,
} _Audio_Voice_State;

typedef struct _Audio_Voice _Audio_Voice;
struct _Audio_Voice
{
    _Audio_Voice       *next;
    _Audio_Voice       *prev;
    Audio_Sound        sound;
    _Audio_Voice_State state;
    Audio_Play_Params  params;
    ma_decoder         decoder;
    bool               decoder_valid;
};

// ak: Bus ====================================================================

typedef struct _Audio_Bus _Audio_Bus;
struct _Audio_Bus
{
    _Audio_Bus *next;
    float      volume;
    float      pitch;
    float      pan;
    bool       paused;
};

// ak: State ==================================================================

typedef struct _Audio_State _Audio_State;
struct _Audio_State
{
    Arena *arena;
    ma_device device;
    ma_mutex mutex;
    struct {
        ma_format format;
        ma_uint32 channels;
        ma_uint32 sample_rate;
    } decoder;
    _Audio_Sound *free_sound;
    _Audio_Voice *free_voice;
    _Audio_Voice *active_voices_first;
    _Audio_Voice *active_voices_last;
    _Audio_Bus   buses[AUDIO_MAX_BUSES];
};

// ak: Global
//=============================================================================

global _Audio_State *_audio_state = 0;

// ak: Functions
//=============================================================================

internal bool         audio_init(uint32_t sample_rate, uint32_t channel_count);
internal void         audio_cleanup(void);

internal Audio_Handle audio_load_from_memory(void *data, size_t size, Audio_Load_Flags flags);
internal Audio_Handle audio_load_from_path(Str8 path, Audio_Load_Flags flags);
internal void         audio_unload(Audio_Handle handle);

internal Audio_Play_Params audio_play_params_default(void);
internal Audio_Handle audio_play(Audio_Handle audio, Audio_Play_Params params);

internal bool         audio_handle_is_valid(Audio_Handle handle);
internal bool         audio_voice_is_alive(Audio_Handle voice_handle);
internal bool         audio_voice_is_playing(Audio_Handle voice_handle);
internal bool         audio_voice_is_paused(Audio_Handle voice_handle);
internal void         audio_voice_pause(Audio_Handle voice_handle);
internal void         audio_voice_resume(Audio_Handle voice_handle);
internal void         audio_voice_toggle_pause(Audio_Handle voice_handle);
internal void         audio_voice_set_volume(Audio_Handle voice_handle, float volume);
internal float        audio_voice_get_volume(Audio_Handle voice_handle);
internal void         audio_voice_set_pitch(Audio_Handle voice_handle, float pitch);
internal float        audio_voice_get_pitch(Audio_Handle voice_handle);
internal void         audio_voice_set_pan(Audio_Handle voice_handle, float pan);
internal float        audio_voice_get_pan(Audio_Handle voice_handle);
internal void         audio_voice_set_bus(Audio_Handle voice_handle, uint32_t bus);
internal uint32_t     audio_voice_get_bus(Audio_Handle voice_handle);
internal double       audio_voice_get_position_seconds(Audio_Handle voice_handle);
internal double       audio_voice_get_duration_seconds(Audio_Handle voice_handle);
internal bool         audio_voice_seek_seconds(Audio_Handle voice_handle, double seconds);
internal void         audio_voice_restart(Audio_Handle voice_handle);

internal void         audio_bus_set_volume(uint32_t bus, float volume);
internal float        audio_bus_get_volume(uint32_t bus);
internal void         audio_bus_set_pitch(uint32_t bus, float pitch);
internal float        audio_bus_get_pitch(uint32_t bus);
internal void         audio_bus_set_pan(uint32_t bus, float pan);
internal float        audio_bus_get_pan(uint32_t bus);
internal void         audio_bus_pause(uint32_t bus);
internal void         audio_bus_resume(uint32_t bus);
internal void         audio_bus_toggle_pause(uint32_t bus);
internal bool         audio_bus_is_paused(uint32_t bus);
internal void         audio_bus_stop(uint32_t bus);
internal void         audio_bus_restart(uint32_t bus);
