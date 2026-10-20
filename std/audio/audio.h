//-----------------------------------------------------------------------------
// Reference:
// - By Ramon Santamaria:
//      - https://github.com/raysan5/raudio/blob/711c86eae17db9a94af575f7a5b496244b48b22d/src/raudio.h
//      - https://github.com/raysan5/raudio/tree/811c150fe97a2abb8ec67857dc52e022bfaa4e43
//-----------------------------------------------------------------------------

#ifndef AUDIO_h
#define AUDIO_h

// ak: Types
//=============================================================================

typedef enum
{
    Audio_Device_Format_Unknown = 0, // Mainly used for indicating an error, but also used as the default for the output format for decoders.
    Audio_Device_Format_U8      = 1,
    Audio_Device_Format_S16     = 2, // Seems to be the most widely supported format.
    Audio_Device_Format_S24     = 3, // Tightly packed. 3 bytes per sample. */
    Audio_Device_Format_S32     = 4,
    Audio_Device_Format_F32     = 5,
    Audio_Device_Format_COUNT
} Audio_Device_Format;

typedef enum
{
    Audio_Buffer_Usage_Static = 0,
    Audio_Buffer_Usage_Stream
} Audio_Buffer_Usage;

typedef enum
{
    Audio_Format_Unknown = 0,
    Audio_Format_Wav,
    Audio_Format_Ogg,
    Audio_Format_Mp3,
    Audio_Format_Qoa,
    Audio_Format_Flac,
} Audio_Format;

typedef void (*Audio_Callback)(void *bufferData, uint64_t frames);

typedef struct Audio_Processor Audio_Processor;
struct Audio_Processor {
    Audio_Callback process;
    Audio_Processor *next;
    Audio_Processor *prev;
};

typedef struct Audio_Buffer Audio_Buffer;
struct Audio_Buffer
{
    uint64_t id[1];
};

typedef struct Audio_Wave Audio_Wave;
struct Audio_Wave
{
    Arena *arena;
    uint64_t frame_count;   // Total number of frames (considering channels)
    uint32_t sample_rate;   // Frequency (samples per second)
    uint32_t sample_size;
    uint32_t channels;      // Number of channels (1-mono, 2-stereo, ...)
    void *data;
};

typedef struct Audio_Stream Audio_Stream;
struct Audio_Stream
{
    Audio_Buffer buffer;       // Pointer to internal data used by the audio system
    Audio_Processor *processor; // Pointer to internal data processor, useful for audio effects
    uint32_t sample_rate; // Frequency (samples per second)
    uint32_t sample_size; // Bit depth (bits per sample): 8, 16, 32 (24 not supported)
    uint32_t channels;    // Number of channels (1-mono, 2-stereo, ...)
};

typedef struct Audio_Sound Audio_Sound;
struct Audio_Sound
{
    Audio_Stream stream;
    size_t frame_count;    // Total number of frames (considering channels)
};

typedef struct Audio_Music Audio_Music;
struct Audio_Music
{
    Arena *arena;
    Audio_Stream stream;
    uint64_t frame_count;
    bool looping;
    Audio_Format format;
    void *context_data;
};

// ak: Defines
//=============================================================================

#ifndef AUDIO_DEVICE_FORMAT
    #define AUDIO_DEVICE_FORMAT Audio_Device_Format_F32    // Device output format (float-32bit)
#endif
#ifndef AUDIO_DEVICE_CHANNELS
    #define AUDIO_DEVICE_CHANNELS              2    // Device output channels: stereo
#endif
#ifndef AUDIO_DEVICE_SAMPLE_RATE
    #define AUDIO_DEVICE_SAMPLE_RATE           0    // Device output sample rate
#endif
#ifndef AUDIO_DEVICE_PERIOD_SIZE_IN_FRAMES
    #define AUDIO_DEVICE_PERIOD_SIZE_IN_FRAMES 0    // Device latency. 0 defaults to 10ms
#endif
#ifndef AUDIO_BUFFER_RESIDUAL_CAPACITY
    #define AUDIO_BUFFER_RESIDUAL_CAPACITY     8    // In PCM frames, for resampling and pitch shifting
#endif

// ak: Functions
//=============================================================================

// Helper =====================================================================

internal Audio_Format audio_format_from_string(Str8 string);

// ak: Initialization and Deinitialization Audio Device and State =============

internal void audio_init(void);
internal void audio_uninit(void);
internal Str8 audio_file_export_to_str8(Str8 file_name, Str8 name, size_t wrap_size, Arena *arena);

// ak: Wave Management ========================================================

// ak: wave load, validate and unload
internal Audio_Wave audio_wave_load(Str8 file_name);
internal Audio_Wave audio_wave_load_from_memory(Str8 file_type, U8Array file_data);
internal bool audio_wave_is_valid(Audio_Wave wave);

// ak: wave modification
internal void audio_wave_format_convert(Audio_Wave *wave, uint32_t sample_rate, uint32_t sample_size, uint32_t channels);
internal Audio_Wave audio_wave_copy(Audio_Wave wave);
internal void audio_wave_crop(Audio_Wave *wave, uint64_t init_frame, uint64_t final_frame);

// ak: wave export
internal bool audio_wave_export_to_file(Audio_Wave wave, Str8 file_name);
internal Str8 audio_wave_export_to_str8(Audio_Wave wave, Str8 name, size_t wrap_size, Arena *arena);

// ak: Samples Management =====================================================
// Load samples data from wave as a floats array
// Returned sample values are normalized to range [-1..1]

internal F32Array audio_samples_from_wave_load(Audio_Wave wave, Arena *arena);

// ak: Sound Management =======================================================
// The entire file is loaded to memory to be played (no-streaming)

// ak: sound load, validate and unload
internal Audio_Sound audio_sound_load(Str8 file_name);
internal Audio_Sound audio_sound_load_from_wave(Audio_Wave wave);
internal bool audio_sound_is_valid(Audio_Sound sound);
internal void audio_sound_unload(Audio_Sound sound);
// ak: Load sound alias, clone sound from existing sound data,
internal Audio_Sound audio_sound_alias_load(Audio_Sound source);
internal void audio_sound_alias_unload(Audio_Sound alias);

// ak: sound playback control
internal void audio_sound_play(Audio_Sound sound);
internal void audio_sound_pause(Audio_Sound sound);
internal void audio_sound_resume(Audio_Sound sound);
internal void audio_sound_stop(Audio_Sound sound);
internal void audio_sound_update(Audio_Sound sound, const void *data, size_t frame_count);

// ak: sound playback query
internal bool audio_sound_is_playing(Audio_Sound sound);

// ak: sound property set
internal void audio_sound_volume_set(Audio_Sound sound, float volume);
internal void audio_sound_pitch_set(Audio_Sound sound, float pitch);
internal void audio_sound_pan_set(Audio_Sound sound, float pan);

// ak: Music Management =======================================================

// ak: music load, validate and unload
internal Audio_Music audio_music_load(Str8 file_name);
internal Audio_Music audio_music_from_memory_load(Str8 file_type, const uint8_t *data, size_t data_size);
internal bool audio_music_is_valid(Audio_Music music);
internal void audio_music_unload(Audio_Music music);

// ak: music playback control
internal void audio_music_play(Audio_Music music);
internal void audio_music_pause(Audio_Music music);
internal void audio_music_resume(Audio_Music music);
internal void audio_music_stop(Audio_Music music);
internal void audio_music_seek(Audio_Music music, float position);
internal void audio_music_update(Audio_Music music);

// ak: music playback query
internal bool audio_music_is_playing(Audio_Music music);

// ak: music property set
internal void audio_music_volume_set(Audio_Music music, float volume);
internal void audio_music_pitch_set(Audio_Music music, float pitch);
internal void audio_music_pan_set(Audio_Music music, float pan);

// ak: music property get
internal float audio_music_duration_get(Audio_Music music);
internal float audio_music_time_played_get(Audio_Music music);

// ak: Audio Stream Management ================================================

// ak: audio stream load, validate and unload
internal Audio_Stream audio_stream_load(uint32_t sample_rate, uint32_t sample_size, uint32_t channels);
internal bool audio_stream_is_valid(Audio_Stream stream);
internal void audio_stream_unload(Audio_Stream stream);

// ak: audio stream playback control
internal void audio_stream_play(Audio_Stream stream);
internal void audio_stream_pause(Audio_Stream stream);
internal void audio_stream_resume(Audio_Stream stream);
internal void audio_stream_stop(Audio_Stream stream);
internal void audio_stream_update(Audio_Stream stream, const void *data, size_t frame_count);
internal void audio_stream_update_in_locked_state(Audio_Stream stream, const void *data, size_t frame_count);

// ak: audio stream playback query
internal bool audio_stream_is_playing(Audio_Stream stream);
internal bool audio_stream_is_processed(Audio_Stream stream);

// ak: audio stream property set
internal void audio_stream_volume_set(Audio_Stream stream, float volume);
internal void audio_stream_pitch_set(Audio_Stream stream, float pitch);
internal void audio_stream_pan_set(Audio_Stream stream, float pan);
internal void audio_stream_buffer_size_default_set(size_t size);
internal void audio_stream_callback_set(Audio_Stream stream, Audio_Callback callback);

// ak: audio stream process management
internal void audio_stream_process_attach(Audio_Stream stream, Audio_Callback process);
internal void audio_stream_process_detach(Audio_Stream stream, Audio_Callback process);

// ak: Audio Processor ========================================================
// Useful to apply effects to an Audio_Buffer

internal void audio_processor_mixed_attach(Audio_Callback process);
internal void audio_processor_mixed_detach(Audio_Callback process);

// ak: Audio Buffer Management ================================================

// ak: audio buffer type management
internal inline bool audio_buffer_match(Audio_Buffer a, Audio_Buffer b);
internal inline bool audio_buffer_is_zero(Audio_Buffer buffer);

// ak: audio buffer alloc and free
internal Audio_Buffer audio_buffer_alloc(Audio_Device_Format format, uint32_t channels, uint32_t sample_rate, uint64_t size_in_frames, Audio_Buffer_Usage usage);
internal void audio_buffer_free(Audio_Buffer audio_buffer);

// ak: audio buffer playback control
internal void audio_buffer_play(Audio_Buffer buffer);
internal void audio_buffer_pause(Audio_Buffer buffer);
internal void audio_buffer_resume(Audio_Buffer buffer);
internal void audio_buffer_stop(Audio_Buffer buffer);
internal void audio_buffer_track(Audio_Buffer buffer);
internal void audio_buffer_untrack(Audio_Buffer buffer);

// ak: audio buffer playback query
internal bool audio_buffer_is_playing(Audio_Buffer buffer);

// ak: audio buffer property set
internal void audio_buffer_volume_set(Audio_Buffer buffer, float volume);
internal void audio_buffer_pitch_set(Audio_Buffer buffer, float pitch);
internal void audio_buffer_pan_set(Audio_Buffer buffer, float pan);

#endif // AUDIO_H
