#ifndef AUDIO_h
#define AUDIO_h

// ak: External Includes
//=============================================================================

#define MA_NO_JACK
#define MA_NO_WAV
#define MA_NO_FLAC
#define MA_NO_MP3
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MA_COINIT_VALUE 2
#include "./external/miniaudio.h"

// #define DRWAV_MALLOC malloc
// #define DRWAV_REALLOC realloc
// #define DRWAV_FREE free
#include "./external/dr_wav.h"

// #define DRMP3_MALLOC malloc
// #define DRMP3_REALLOC realloc
// #define DRMP3_FREE free
#include "./external/dr_mp3.h"

#define QOA_MALLOC malloc
#define QOA_FREE free
#include "./external/qoa.h"

// #define DRFLAC_MALLOC malloc
// #define DRFLAC_REALLOC realloc
// #define DRFLAC_FREE free
#define DR_FLAC_NO_WIN32_IO
#include "external/dr_flac.h"

// ak: Types
//=============================================================================

typedef enum
{
    Audio_Format_Unknown = 0,     // Mainly used for indicating an error, but also used as the default for the output format for decoders.
    Audio_Format_U8      = 1,
    Audio_Format_S16     = 2,     // Seems to be the most widely supported format.
    Audio_Format_S24     = 3,     // Tightly packed. 3 bytes per sample. */
    Audio_Format_S32     = 4,
    Audio_Format_F32     = 5,
    Audio_Format_COUNT
} Audio_Format;

typedef enum
{
    Audio_Buffer_Usage_Static = 0,
    Audio_Buffer_Usage_Stream
} Audio_Buffer_Usage;

typedef enum
{
    Audio_Music_Context_Type_None = 0,
    Audio_Music_Context_Type_Wav,
    Audio_Music_Context_Type_Ogg,
    Audio_Music_Context_Type_Mp3,
    Audio_Music_Context_Type_Qoa,
    Audio_Music_Context_Type_Flac,
} Audio_Music_Context_Type;

typedef void (*Audio_Callback)(void *bufferData, uint64_t frames);

typedef struct Audio_Processor Audio_Processor;
struct Audio_Processor {
    Audio_Callback process;          // Processor callback function
    Audio_Processor *next;          // Next audio processor on the list
    Audio_Processor *prev;          // Previous audio processor on the list
};

typedef struct Audio_Buffer Audio_Buffer;
struct Audio_Buffer
{
    Audio_Buffer *next; // Next audio buffer on the list
    Audio_Buffer *prev; // Previous audio buffer on the list
    
    float volume;                   // Audio buffer volume
    float pitch;                    // Audio buffer pitch
    float pan;                      // Audio buffer pan (-1.0f to 1.0f)
    
    bool playing; // Audio buffer state: AUDIO_PLAYING
    bool paused;  // Audio buffer state: AUDIO_PAUSED
    bool looping; // Audio buffer looping, default to true for AudioStreams
    
    ma_data_converter converter; // Audio data converter
    uint8_t *converter_residual; // Cached residual input frames for use by the converter
    size_t converter_residual_count; // The number of valid frames sitting in converter_residual
    
    Audio_Callback callback;    // Audio buffer callback for buffer filling on audio threads
    Audio_Processor *processor; // Audio processor
    uint64_t frames_processed;   // Total frames processed in this buffer (required for play timing)
    uint64_t size_in_frames;      // Total buffer size in frames
    uint64_t frame_cursor_pos;    // Frame cursor position
    bool is_sub_buffer_processed[2];   // SubBuffer processed (virtual double buffer)
    
    Audio_Buffer_Usage usage;                      // Audio buffer usage mode: STATIC or STREAM
    uint8_t *data;            // Data buffer, on music stream keeps filling
};

typedef struct Audio_Wave Audio_Wave;
struct Audio_Wave
{
    uint64_t frame_count;   // Total number of frames (considering channels)
    uint32_t sample_rate;   // Frequency (samples per second)
    uint32_t sample_size;
    uint32_t channels;      // Number of channels (1-mono, 2-stereo, ...)
    void *data;             // Buffer data pointer
};

typedef struct Audio_Stream Audio_Stream;
struct Audio_Stream
{
    Audio_Buffer *buffer;       // Pointer to internal data used by the audio system
    Audio_Processor *processor; // Pointer to internal data processor, useful for audio effects
    uint32_t sample_rate; // Frequency (samples per second)
    uint32_t sample_size; // Bit depth (bits per sample): 8, 16, 32 (24 not supported)
    uint32_t channels;    // Number of channels (1-mono, 2-stereo, ...)
};

typedef struct Audio_Sound Audio_Sound;
struct Audio_Sound
{
    Audio_Stream stream;   // Audio stream
    size_t frame_count;    // Total number of frames (considering channels)
};

typedef struct Audio_Music Audio_Music;
struct Audio_Music
{
    Audio_Stream stream;
    uint64_t frame_count;
    bool looping;
    Audio_Music_Context_Type context_type;
    void *context_data;
};

typedef struct _Audio_State _Audio_State;
struct _Audio_State
{
    Arena *arena;
    struct {
        ma_context context;
        ma_device device;
        ma_mutex lock;
        bool is_ready;
        struct {
            Arena *arena;
            size_t buffer_size;
            void *buffer;
        } pcm;
    } system;
    struct
    {
        Audio_Buffer *first;
        Audio_Buffer *last;
        Audio_Buffer *free;
        size_t default_size;
    } buffer;
    Audio_Processor *processor_mixed;
    Audio_Processor *processor_free;
};

// ak: Defines
//=============================================================================

#ifndef AUDIO_DEVICE_FORMAT
    #define AUDIO_DEVICE_FORMAT Audio_Format_F32    // Device output format (float-32bit)
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

// ak: Helper =================================================================

internal void _audio_ma_log(void *user_data, ma_uint32 level, const char *message);
internal void _audio_mix_frames(float *frame_out, const float *frame_in, ma_uint32 frame_count, Audio_Buffer *buffer);
internal void _audio_on_send_audio_data_to_device(ma_device *device, void *frame_out, const void *frame_input, ma_uint32 frame_count);

// ak: Initialization and Deinitialization Audio Device and State =============

internal void audio_init(void);
internal void audio_uninit(void);
internal Str8 audio_file_export_to_str8(Str8 file_name, Str8 name, size_t wrap_size, Arena *arena);

// ak: Wave Management ========================================================

internal Audio_Wave audio_wave_load(Str8 file_name, Arena *arena);
internal Audio_Wave audio_wave_load_from_memory(Str8 file_type, U8Array file_data, Arena *arena);
internal bool audio_wave_is_valid(Audio_Wave wave);
internal bool audio_wave_export(Audio_Wave wave, Str8 file_name);
internal void audio_wave_format_convert(Audio_Wave *wave, uint32_t sample_rate, uint32_t sample_size, uint32_t channels, Arena *arena);
internal Audio_Wave audio_wave_copy(Audio_Wave wave, Arena *arena);
internal void audio_wave_crop(Audio_Wave *wave, uint64_t init_frame, uint64_t final_frame);
internal Str8 audio_wave_export_to_str8(Audio_Wave wave, Str8 name, size_t wrap_size, Arena *arena);

// ak: Samples Management =====================================================
// Load samples data from wave as a floats array
// Returned sample values are normalized to range [-1..1]

internal F32Array audio_samples_from_wave_load(Audio_Wave wave, Arena *arena);

// ak: Sound Management =======================================================
// The entire file is loaded to memory to be played (no-streaming)

internal Audio_Sound audio_sound_load(Str8 file_name, Arena *arena);
Audio_Sound audio_sound_load_from_wave(Audio_Wave wave, Arena *arena);
internal void audio_sound_unload(Audio_Sound sound);
Audio_Sound audio_sound_alias_load(Audio_Sound source, Arena *arnea);
internal void audio_sound_alias_unload(Audio_Sound alias);
internal bool audio_sound_is_valid(Audio_Sound sound);
internal void audio_sound_update(Audio_Sound sound, const void *data, size_t frame_count);
internal void audio_sound_play(Audio_Sound sound);
internal void audio_sound_pause(Audio_Sound sound);
internal void audio_sound_resume(Audio_Sound sound);
internal void audio_sound_stop(Audio_Sound sound);
internal bool audio_sound_is_playing(Audio_Sound sound);
internal void audio_sound_volume_set(Audio_Sound sound, float volume);
internal void audio_sound_pitch_set(Audio_Sound sound, float pitch);
internal void audio_sound_pan_set(Audio_Sound sound, float pan);

// ak: Music Management =======================================================

internal Audio_Music audio_music_load(Str8 file_name, Arena *arena);
internal Audio_Music audio_music_from_memory_load(Str8 file_type, const uint8_t *data, size_t data_size, Arena *arena);
internal void audio_music_unload(Audio_Music music);
internal bool audio_music_is_valid(Audio_Music music);
internal void audio_music_play(Audio_Music music);
internal void audio_music_pause(Audio_Music music);
internal void audio_music_resume(Audio_Music music);
internal void audio_music_stop(Audio_Music music);
internal void audio_music_seek(Audio_Music music, float position);
internal void audio_music_update(Audio_Music music);
internal bool audio_music_is_playing(Audio_Music music);
internal void audio_music_volume_set(Audio_Music music, float volume);
internal void audio_music_pitch_set(Audio_Music music, float pitch);
internal void audio_music_pan_set(Audio_Music music, float pan);
internal float audio_music_duration_get(Audio_Music music);
internal float audio_music_time_played_get(Audio_Music music);

// ak: Audio Stream Management
//=============================================================================

Audio_Stream audio_stream_load(uint32_t sample_rate, uint32_t sample_size, uint32_t channels, Arena *arena);
internal void audio_stream_unload(Audio_Stream stream);
internal bool audio_stream_is_valid(Audio_Stream stream);
internal void audio_stream_update(Audio_Stream stream, const void *data, size_t frame_count);
internal void audio_stream_update_in_locked_state(Audio_Stream stream, const void *data, size_t frame_count);
bool audio_stream_is_processed(Audio_Stream stream);
internal void audio_stream_play(Audio_Stream stream);
internal void audio_stream_pause(Audio_Stream stream);
internal void audio_stream_resume(Audio_Stream stream);
internal bool audio_stream_is_playing(Audio_Stream stream);
internal void audio_stream_stop(Audio_Stream stream);
internal void audio_stream_volume_set(Audio_Stream stream, float volume);
internal void audio_stream_pitch_set(Audio_Stream stream, float pitch);
internal void audio_stream_pan_set(Audio_Stream stream, float pan);
internal void audio_stream_buffer_size_default_set(size_t size);
internal void audio_stream_callback_set(Audio_Stream stream, Audio_Callback callback);

// ak: Process

internal void audio_stream_process_attach(Audio_Stream stream, Audio_Callback process);
internal void audio_stream_process_detach(Audio_Stream stream, Audio_Callback process);

// ak: Audio Processor ========================================================

internal void audio_processor_mixed_attach(Audio_Callback process);
internal void audio_processor_mixed_detach(Audio_Callback process);

// ak: Audio Buffer Managment =================================================

internal Audio_Buffer *audio_buffer_alloc(Audio_Format format, uint32_t channels, uint32_t sample_rate, ma_uint64 size_in_frames, Audio_Buffer_Usage usage, Arena *arena);
internal void audio_buffer_free(Audio_Buffer *audio_buffer);

internal void audio_buffer_track(Audio_Buffer *buffer);
internal void audio_buffer_untrack(Audio_Buffer *buffer);
internal void audio_buffer_play(Audio_Buffer *buffer);
internal bool audio_buffer_is_playing(Audio_Buffer *buffer);
internal bool audio_buffer_is_playing_in_locked_state(Audio_Buffer *buffer);
internal void audio_buffer_stop(Audio_Buffer *buffer);
internal void audio_buffer_stop_in_locked_state(Audio_Buffer *buffer);
internal void audio_buffer_pause(Audio_Buffer *buffer);
internal void audio_buffer_resume(Audio_Buffer *buffer);
internal void audio_buffer_volume_set(Audio_Buffer *buffer, float volume);
internal void audio_buffer_pitch_set(Audio_Buffer *buffer, float pitch);
internal void audio_buffer_pan_set(Audio_Buffer *buffer, float pan);

internal uint32_t audio_buffer_read_frames_in_internal_format(Audio_Buffer *audio_buffer, void *frames_out, uint32_t frame_count);
internal uint32_t audio_buffer_read_frames_in_mixing_format(Audio_Buffer *audio_buffer, float *frames_out, uint32_t frame_count);

// ak: Globals
//=============================================================================

global _Audio_State *_audio_state = 0;

#endif // AUDIO_H
