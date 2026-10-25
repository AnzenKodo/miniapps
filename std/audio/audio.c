// ak: Vendor Includes Implementation
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
#include "./vendor/miniaudio.h"

// #define DRWAV_MALLOC malloc
// #define DRWAV_REALLOC realloc
// #define DRWAV_FREE free
#define DR_WAV_IMPLEMENTATION
#include "./vendor/dr_wav.h"

// #define DRMP3_MALLOC malloc
// #define DRMP3_REALLOC realloc
// #define DRMP3_FREE free
#define DR_MP3_IMPLEMENTATION
#include "./vendor/dr_mp3.h"

// #define QOA_MALLOC malloc
// #define QOA_FREE free
#define QOA_IMPLEMENTATION
#include "./vendor/qoa.h"           // QOA loading and saving functions
#include "./vendor/qoaplay.c"       // QOA stream playing helper functions

// #define DRFLAC_MALLOC malloc
// #define DRFLAC_REALLOC realloc
// #define DRFLAC_FREE free
#define DR_FLAC_NO_WIN32_IO
#define DR_FLAC_IMPLEMENTATION
#include "vendor/dr_flac.h"

// STB_VORBIS_NO_STDIO
// #define NULL 0
// #define malloc(s)   0
// #define free(s)     ((void) 0)
// #define realloc(s)  0
#include "./vendor/stb_vorbis.c" // for OGG

// ak: Private Types
//=============================================================================

typedef struct _Audio_Buffer _Audio_Buffer;
struct _Audio_Buffer
{
    _Audio_Buffer *next;
    _Audio_Buffer *prev;
    
    Arena *arena;
    
    float volume;
    float pitch;
    float pan;
    
    bool playing;
    bool paused;
    bool looping;
    
    ma_data_converter converter;
    uint8_t *converter_residual;     // Cached residual input frames for use by the converter
    size_t converter_residual_count; // The number of valid frames sitting in converter_residual
    
    Audio_Callback callback;         // Audio buffer callback for buffer filling on audio threads
    Audio_Processor *processor;
    uint64_t frames_processed;       // Total frames processed in this buffer (required for play timing)
    uint64_t size_in_frames;
    uint64_t frame_cursor_pos;
    bool is_sub_buffer_processed[2]; // Virtual double buffer
    
    Audio_Buffer_Usage usage;
    uint8_t *data;
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
        _Audio_Buffer *first;
        _Audio_Buffer *last;
        size_t default_size;
    } buffer;
    Audio_Processor *processor_mixed;
    Audio_Processor *processor_free;
};

// ak: Globals
//=============================================================================

global _Audio_State *_audio_state = 0;

// ak: Private Function Definition
//=============================================================================

// ak: Helper =================================================================

internal void _audio_ma_log(void *user_data, ma_uint32 level, const char *message);
internal void _audio_mix_frames(float *frame_out, const float *frame_in, ma_uint32 frame_count, _Audio_Buffer *buffer);
internal void _audio_on_send_audio_data_to_device(ma_device *device, void *frame_out, const void *frame_input, ma_uint32 frame_count);

// ak: Internal Audio Buffer Management =======================================

internal bool _audio_buffer_is_playing_in_locked_state(_Audio_Buffer *buffer);
internal void _audio_buffer_stop_in_locked_state(_Audio_Buffer *buffer);
internal uint32_t _audio_buffer_read_frames_in_internal_format(_Audio_Buffer *buffer, void *frames_out, uint32_t frame_count);
internal uint32_t _audio_buffer_read_frames_in_mixing_format(_Audio_Buffer *buffer, float *frames_out, uint32_t frame_count);

// ak: Function Implementation
//=============================================================================

// ak: Helper =================================================================

internal void _audio_ma_log(void *user_data, ma_uint32 level, const char *message)
{
    Unused(user_data);
    Str8 msg = str8_from_cstr((char *)message);
    msg = str8_chop(msg, 1);
    switch(level)
    {
        case MA_LOG_LEVEL_DEBUG:   { LogTrace(Debug, msg); } break;
        case MA_LOG_LEVEL_INFO:    { LogTrace(Info, msg); } break;
        case MA_LOG_LEVEL_WARNING: { LogTrace(Warn, msg); } break;
        case MA_LOG_LEVEL_ERROR:   { LogTrace(Error, msg); } break;
    }
}

// ak: Main mixing function, pretty simple in this project, only an accumulation
// NOTE(ak): frame_out is both an input and an output, it is initially filled
// with zeros outside of this function
internal void _audio_mix_frames(float *frame_out, const float *frame_in, ma_uint32 frame_count, _Audio_Buffer *buffer)
{
    const float local_volume = buffer->volume;
    const ma_uint32 channels = _audio_state->system.device.playback.channels;
    
    if (channels == 2) // Consider panning
    {
        const float right = (buffer->pan + 1.0f)/2.0f; // Normalize: [-1..1] -> [0..1]
        const float left = 1.0f - right;
        
        // ak: Fast sine approximation in [0..1] for pan law: y = 0.5f*x*(3 - x*x);
        const float levels[2] = { local_volume*0.5f*left*(3.0f - left*left), local_volume*0.5f*right*(3.0f - right*right) };
        float *frameOut = frame_out;
        const float *frameIn = frame_in;
        for (ma_uint32 frame = 0; frame < frame_count; frame++)
        {
            frameOut[0] += (frameIn[0]*levels[0]);
            frameOut[1] += (frameIn[1]*levels[1]);
            frameOut += 2;
            frameIn += 2;
        }
    }
    else  // Do not consider panning
    {
        for (ma_uint32 frame = 0; frame < frame_count; frame++)
        {
            for (ma_uint32 c = 0; c < channels; c++)
            {
                float *frameOut = frame_out + (frame*channels);
                const float *frameIn = frame_in + (frame*channels);
                // ak: Output accumulates input multiplied by volume to provided
                // output (usually 0)
                frameOut[c] += (frameIn[c]*local_volume);
            }
        }
    }
}

internal void _audio_on_send_audio_data_to_device(ma_device *device, void *frame_out, const void *frame_input, ma_uint32 frame_count)
{
    Unused(frame_input);
    // ak: mixing is basically an accumulation, need to initialize the output
    // buffer to 0
    mem_set(frame_out, 0, frame_count*device->playback.channels*ma_get_bytes_per_sample(device->playback.format));
    ma_mutex_lock(&_audio_state->system.lock);
    {
        for (_Audio_Buffer *_buffer = _audio_state->buffer.first; _buffer != NULL; _buffer = _buffer->next)
        {
            // ak: ignore stopped or paused sounds
            if (!_buffer->playing || _buffer->paused) continue;
            ma_uint32 frames_read = 0;
            while (true)
            {
                if (frames_read >= frame_count) break;
                // ak: read as much data as possible from the stream
                ma_uint32 frames_to_read = (frame_count - frames_read);
                while (frames_to_read > 0)
                {
                    float temp_buffer[1024] = STRUCT_ZERO; // frames for stereo
                    ma_uint32 frames_to_read_right_now = frames_to_read;
                    if (frames_to_read_right_now > sizeof(temp_buffer)/sizeof(temp_buffer[0])/AUDIO_DEVICE_CHANNELS)
                    {
                        frames_to_read_right_now = sizeof(temp_buffer)/sizeof(temp_buffer[0])/AUDIO_DEVICE_CHANNELS;
                    }
                    
                    ma_uint32 frames_just_read = _audio_buffer_read_frames_in_mixing_format(_buffer, temp_buffer, frames_to_read_right_now);
                    if (frames_just_read > 0)
                    {
                        float *frames_out = (float *)frame_out + (frames_read*_audio_state->system.device.playback.channels);
                        float *frame_in = temp_buffer;
                        
                        // ak: apply processors chain if defined
                        Audio_Processor *processor = _buffer->processor;
                        while (processor)
                        {
                            processor->process(frame_in, frames_just_read);
                            processor = processor->next;
                        }
                        _audio_mix_frames(frames_out, frame_in, frames_just_read, _buffer);
                        frames_to_read -= frames_just_read;
                        frames_read += frames_just_read;
                    }
                    
                    if (!_buffer->playing)
                    {
                        frames_read = frame_count;
                        break;
                    }
                    
                    // ak: if all the frames requested can't be read, break
                    if (frames_just_read < frames_to_read_right_now)
                    {
                        if (!_buffer->looping)
                        {
                            _audio_buffer_stop_in_locked_state(_buffer);
                            break;
                        }
                        else
                        {
                            // ak: should never get here, but for safety, move
                            // the cursor position back to the start and
                            // continue the loop
                            _buffer->frame_cursor_pos = 0;
                            continue;
                        }
                    }
                }
                // ak: if for some reason is not possible to read every frame, the
                // loop needs to be broken. Not doing this could theoretically
                // end up into an infinite loop.
                if (frames_to_read > 0) break;
            }
        }
    }
    Audio_Processor *processor = _audio_state->processor_mixed;
    while (processor)
    {
        processor->process(frame_out, frame_count);
        processor = processor->next;
    }
    ma_mutex_unlock(&_audio_state->system.lock);
}

// ak: Internal Audio Buffer Management =======================================

internal bool _audio_buffer_is_playing_in_locked_state(_Audio_Buffer *buffer)
{
    bool result = false;
    if (buffer != NULL) result = (buffer->playing && !buffer->paused);
    return result;
}

internal void _audio_buffer_stop_in_locked_state(_Audio_Buffer *buffer)
{
    if (buffer != NULL)
    {
        if (_audio_buffer_is_playing_in_locked_state(buffer))
        {
            buffer->playing = false;
            buffer->paused = false;
            buffer->frame_cursor_pos = 0;
            buffer->frames_processed = 0;
            buffer->is_sub_buffer_processed[0] = true;
            buffer->is_sub_buffer_processed[1] = true;
        }
    }
}

internal uint32_t _audio_buffer_read_frames_in_internal_format(_Audio_Buffer *buffer, void *frames_out, uint32_t frame_count)
{
    // ak: Don't read anything if the sound is not playing
    if (!buffer->playing) return 0;
    
    // ak: Using audio buffer callback
    if(buffer->callback != 0)
    {
        buffer->callback(frames_out, frame_count);
        buffer->frames_processed += frame_count;
        return frame_count;
    }
    
    ma_uint32 sub_buffer_size_in_frames = (buffer->size_in_frames > 1)? buffer->size_in_frames/2 : buffer->size_in_frames;
    ma_uint32 current_sub_buffer_index = buffer->frame_cursor_pos/sub_buffer_size_in_frames;
    
    if (current_sub_buffer_index > 1) return 0;
    
    // ak: Another thread can update the processed state of buffers, so
    // take a copy here to try and avoid potential synchronization problems
    bool is_sub_buffer_processed[2] = STRUCT_ZERO;
    is_sub_buffer_processed[0] = buffer->is_sub_buffer_processed[0];
    is_sub_buffer_processed[1] = buffer->is_sub_buffer_processed[1];
    ma_uint32 frame_size_in_bytes = ma_get_bytes_per_frame(buffer->converter.formatIn, buffer->converter.channelsIn);
    
    // Fill out every frame until a buffer that's marked as processed is found, then fill the remainder with 0
    ma_uint32 frames_read = 0;
    while (1)
    {
        // ak: Break from this loop differently depending on the buffer's usage
        //  - For static buffers, simply fill as much data as possible
        //  - For streaming buffers, only fill half of the buffer that are processed
        //    Unprocessed halves must keep their audio data in-tact
        if (buffer->usage == Audio_Buffer_Usage_Static)
        {
            if (frames_read >= frame_count) break;
        }
        else
        {
            if (is_sub_buffer_processed[current_sub_buffer_index]) break;
        }
        
        ma_uint32 total_frames_remaining = (frame_count - frames_read);
        if (total_frames_remaining == 0) break;
        
        ma_uint32 framesRemainingInOutputBuffer;
        if (buffer->usage == Audio_Buffer_Usage_Static)
        {
            framesRemainingInOutputBuffer = buffer->size_in_frames - buffer->frame_cursor_pos;
        }
        else
        {
            ma_uint32 firstFrameIndexOfThisSubBuffer = sub_buffer_size_in_frames*current_sub_buffer_index;
            framesRemainingInOutputBuffer = sub_buffer_size_in_frames - (buffer->frame_cursor_pos - firstFrameIndexOfThisSubBuffer);
        }
        
        ma_uint32 framesToRead = total_frames_remaining;
        if (framesToRead > framesRemainingInOutputBuffer) framesToRead = framesRemainingInOutputBuffer;
        
        mem_copy((uint8_t *)frames_out + (frames_read*frame_size_in_bytes), buffer->data + (buffer->frame_cursor_pos*frame_size_in_bytes), framesToRead*frame_size_in_bytes);
        buffer->frame_cursor_pos = (buffer->frame_cursor_pos + framesToRead)%buffer->size_in_frames;
        frames_read += framesToRead;
        
        // ak: If the end of the buffer is read, mark it as processed
        if (framesToRead == framesRemainingInOutputBuffer)
        {
            buffer->is_sub_buffer_processed[current_sub_buffer_index] = true;
            is_sub_buffer_processed[current_sub_buffer_index] = true;
            current_sub_buffer_index = (current_sub_buffer_index + 1)%2;
            
            // ak: Break from this loop if looping not enabled
            if (!buffer->looping)
            {
                _audio_buffer_stop_in_locked_state(buffer);
                break;
            }
        }
    }
    
    // ak: Zero-fill excess
    ma_uint32 total_frames_remaining = (frame_count - frames_read);
    if (total_frames_remaining > 0)
    {
        memset((uint8_t *)frames_out + (frames_read*frame_size_in_bytes), 0, total_frames_remaining*frame_size_in_bytes);
        
        // ak: For static buffers, fill the remaining frames with silence for
        // safety, but don't report those frames as "read"; The reason for this
        // is that the caller uses the return value to know whether a
        // non-looping sound has finished playback
        if (buffer->usage != Audio_Buffer_Usage_Static) frames_read += total_frames_remaining;
    }
    
    return frames_read;
}

// ak: Reads audio data from an Audio_Buffer object in device format, returned data will be in a format appropriate for mixing
internal uint32_t _audio_buffer_read_frames_in_mixing_format(_Audio_Buffer *buffer, float *frames_out, uint32_t frame_count)
{
    // NOTE(ak): Continuously converting data from the Audio_Buffer's internal format
    // to the mixing format, which should be defined by the output format of the
    // data converter. This is done until frame_count frames have been output.
    ma_uint32 bpf = ma_get_bytes_per_frame(buffer->converter.formatIn, buffer->converter.channelsIn);
    ma_uint8 input_buffer[4096] = STRUCT_ZERO;
    ma_uint32 inputBufferFrameCap = sizeof(input_buffer)/bpf;
    ma_uint32 total_output_frames_processed = 0;
    
    while (total_output_frames_processed < frame_count)
    {
        float *runningFramesOut = frames_out + (total_output_frames_processed*buffer->converter.channelsOut);
        ma_uint64 outputFramesToProcessThisIteration = frame_count - total_output_frames_processed;
        
        // ak: process any residual input frames from the previous read first
        if (buffer->converter_residual_count > 0)
        {
            ma_uint64 input_frames_processed_this_iteration = buffer->converter_residual_count;
            ma_uint64 output_frame_processed_this_iteration = outputFramesToProcessThisIteration;
            ma_data_converter_process_pcm_frames(&buffer->converter, buffer->converter_residual, &input_frames_processed_this_iteration, runningFramesOut, &output_frame_processed_this_iteration);
            
            // ak: Make sure the data in the cache is consumed, this can be
            // optimized to use a cursor instead of a mem_move()
            mem_move(buffer->converter_residual, buffer->converter_residual + input_frames_processed_this_iteration*bpf, (size_t)(AUDIO_BUFFER_RESIDUAL_CAPACITY - input_frames_processed_this_iteration)*bpf);
            buffer->converter_residual_count -= (ma_uint32)input_frames_processed_this_iteration; // safe cast
            total_output_frames_processed += (ma_uint32)output_frame_processed_this_iteration; // safe cast
        }
        else
        {
            // ak: Getting here means there are no residual frames from the
            // previous read. fresh data can now be pulled from the Audio_Buffer
            // and processed. A best guess needs to be used made to determine
            // how many input frames to pull from the buffer. There are three
            // possible outcomes:
            // 1) exact
            // 2) underestimated;
            // 3) overestimated.
            //
            // When the guess is exactly correct or underestimated there is
            // nothing special to handle, it'll be handled naturally by the loop
            //
            // When the guess is overestimated, that's when it gets more
            // complicated. In this case, any overflow needs to be stored in a
            // buffer for later processing by the next read
            ma_uint32 estimated_input_frame_count = (ma_uint32)(((float)buffer->converter.resampler.sampleRateIn / buffer->converter.resampler.sampleRateOut)*outputFramesToProcessThisIteration);
            if (estimated_input_frame_count == 0) estimated_input_frame_count = 1; // Make sure at least one input frame is read
            if (estimated_input_frame_count > inputBufferFrameCap) estimated_input_frame_count = inputBufferFrameCap;
            
            ma_uint32 input_frames_in_internal_format_count = _audio_buffer_read_frames_in_internal_format(buffer, input_buffer, estimated_input_frame_count);
            ma_uint64 input_frames_processed_this_iteration = input_frames_in_internal_format_count;
            ma_uint64 outputFramesProcessedThisIteration = outputFramesToProcessThisIteration;
            ma_data_converter_process_pcm_frames(&buffer->converter, input_buffer, &input_frames_processed_this_iteration, runningFramesOut, &outputFramesProcessedThisIteration);
            total_output_frames_processed += (ma_uint32)outputFramesProcessedThisIteration;
            
            if (input_frames_in_internal_format_count > input_frames_processed_this_iteration)
            {
                // ak: getting here means the estimated input frame count was
                // overestimated. The residual needs be stored for later use
                ma_uint64 residual_frame_count = input_frames_in_internal_format_count - input_frames_processed_this_iteration;
                
                // ak: a safety check to make sure the capacity of the residual cache is not exceeded
                if (residual_frame_count > AUDIO_BUFFER_RESIDUAL_CAPACITY)
                {
                    residual_frame_count = AUDIO_BUFFER_RESIDUAL_CAPACITY;
                }
                mem_copy(buffer->converter_residual, input_buffer + input_frames_processed_this_iteration*bpf, (size_t)(residual_frame_count*bpf));
            }
            
            if (input_frames_in_internal_format_count < estimated_input_frame_count) break;  // reached the end of the sound
        }
    }
    
    return total_output_frames_processed;
}

// Helper =====================================================================

internal Audio_Format audio_format_from_string(Str8 string)
{
    Audio_Format format = Audio_Format_Unknown;
    if (str8_match(string, str8("wav"), Str_Match_Flag_CaseInsensitive))
    {
        format = Audio_Format_Wav;
    }
    else if (str8_match(string, str8("ogg"), Str_Match_Flag_CaseInsensitive))
    {
        format = Audio_Format_Ogg;
    }
    else if (str8_match(string, str8("mp3"), Str_Match_Flag_CaseInsensitive))
    {
        format = Audio_Format_Mp3;
    }
    else if (str8_match(string, str8("qoa"), Str_Match_Flag_CaseInsensitive))
    {
        format = Audio_Format_Qoa;
    }
    else if (str8_match(string, str8("flac"), Str_Match_Flag_CaseInsensitive))
    {
        format = Audio_Format_Flac;
    }
    return format;
}

// ak: Initialization and Deinitialization Audio Device and State =============

internal void audio_init(void)
{
    if (_audio_state != NULL) { return; }
    
    Arena *arena = arena_alloc();
    _audio_state = arena_push(arena, _Audio_State, 1);
    _audio_state->arena = arena;
    _audio_state->system.pcm.arena = arena_alloc();
    
    ma_context_config context_config = ma_context_config_init();
    ma_result result = ma_context_init(NULL, 0, &context_config, &_audio_state->system.context);
    if (result != MA_SUCCESS)
    {
        LogTraceF(Warn, "AUDIO: failed to initialize context");
    }
    else
    {
        ma_log_register_callback(ma_context_get_log(&_audio_state->system.context), ma_log_callback_init(_audio_ma_log, NULL));
        
        // ak: init audio device
        // NOTE(ak): Using the default device. Format is floating point because
        // it simplifies mixing
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.pDeviceID = NULL;        // NULL for the default playback
        config.playback.format = (ma_format)AUDIO_DEVICE_FORMAT;
        config.playback.channels = AUDIO_DEVICE_CHANNELS;
        config.sampleRate = AUDIO_DEVICE_SAMPLE_RATE;
        config.periodSizeInFrames = AUDIO_DEVICE_PERIOD_SIZE_IN_FRAMES;
        config.dataCallback = _audio_on_send_audio_data_to_device;
        config.pUserData = NULL;
        config.noPreSilencedOutputBuffer = true; // pre-silences the output buffer manually
        config.noFixedSizedCallback = true;      // does not require fixed sized callback guarantees. This bypasses an internal intermediary buffer
        
        result = ma_device_init(&_audio_state->system.context, &config, &_audio_state->system.device);
        if (result != MA_SUCCESS)
        {
            LogTraceF(Warn, "AUDIO: failed to initialize playback device");
            ma_context_uninit(&_audio_state->system.context);
        }
        else
        {
            if (ma_mutex_init(&_audio_state->system.lock) != MA_SUCCESS)
            {
                LogTraceF(Warn, "AUDIO: failed to create mutex for mixing");
                ma_device_uninit(&_audio_state->system.device);
                ma_context_uninit(&_audio_state->system.context);
            }
            else
            {
                result = ma_device_start(&_audio_state->system.device);
                if (result != MA_SUCCESS)
                {
                    LogTraceF(Warn, "AUDIO: failed to start playback device");
                    ma_device_uninit(&_audio_state->system.device);
                    ma_context_uninit(&_audio_state->system.context);
                }
                
                LogTraceF(Info, "AUDIO: Device initialized successfully");
                LogTraceF(Info, "    > Backend:       miniaudio | %s", ma_get_backend_name(_audio_state->system.context.backend));
                LogTraceF(Info, "    > Format:        %s -> %s", ma_get_format_name(_audio_state->system.device.playback.format), ma_get_format_name(_audio_state->system.device.playback.internalFormat));
                LogTraceF(Info, "    > Channels:      %d -> %d", _audio_state->system.device.playback.channels, _audio_state->system.device.playback.internalChannels);
                LogTraceF(Info, "    > Sample rate:   %d -> %d", _audio_state->system.device.sampleRate, _audio_state->system.device.playback.internalSampleRate);
                LogTraceF(Info, "    > Periods size:  %d", _audio_state->system.device.playback.internalPeriodSizeInFrames*_audio_state->system.device.playback.internalPeriods);
                
                _audio_state->system.is_ready = true;
            }
        }
    }
}

internal void audio_uninit(void)
{
    if(_audio_state != NULL && _audio_state->system.is_ready)
    {
        ma_device_uninit(&_audio_state->system.device);
        ma_mutex_uninit(&_audio_state->system.lock);
        ma_context_uninit(&_audio_state->system.context);
        
        arena_free(_audio_state->system.pcm.arena);
        _audio_state->system.pcm.buffer = NULL;
        _audio_state->system.pcm.buffer_size = 0;
        
        _audio_state->system.is_ready = false;
        arena_free(_audio_state->arena);
        _audio_state = NULL;
        
        LogTraceF(Info, "AUDIO: Device closed successfully");
    }
    else
    {
        LogTraceF(Info, "AUDIO: Device could not be closed, not currently initialized");
    }
}

internal Str8 audio_file_export_to_str8(Str8 file_name, Str8 name, size_t wrap_size, Arena *arena)
{
    Audio_Wave wave = audio_wave_load(file_name);
    Str8 result = audio_wave_export_to_str8(wave, name, wrap_size, arena);
    return result;
}

// ak: Wave Management ========================================================

// ak: wave load, validate and unload
internal Audio_Wave audio_wave_load(Str8 file_name)
{
    Audio_Wave wave = STRUCT_ZERO;
    
    Arena_Temp scratch = arena_scratch_begin(NULL, 0);
    U8Array file_data = fs_file_path_read_full(file_name, scratch.arena);
    if (file_data.v != NULL)
    {
        wave = audio_wave_load_from_memory(str8_skip_last_dot(file_name), file_data);
    }
    arena_scratch_end(scratch);
    
    return wave;
}

internal Audio_Wave audio_wave_load_from_memory(Str8 file_type, U8Array file_data)
{
    Audio_Wave wave = STRUCT_ZERO;
    wave.arena = arena_alloc();
    
    if (str8_match(file_type, str8("wav"), Str_Match_Flag_CaseInsensitive))
    {
        drwav wav = STRUCT_ZERO;
        bool success = drwav_init_memory(&wav, file_data.v, file_data.length, NULL);
        if (success)
        {
            wave.frame_count = (size_t)wav.totalPCMFrameCount;
            wave.sample_rate = wav.sampleRate;
            wave.sample_size = 16;
            wave.channels = wav.channels;
            wave.data = arena_push(wave.arena, uint16_t, (size_t)wave.frame_count*wave.channels);
            
            // NOTE(ak): Forcing conversion to 16bit sample size on reading
            drwav_read_pcm_frames_s16(&wav, wave.frame_count, (drwav_int16 *)wave.data);
        }
        else
        {
            LogTraceF(Warn, "AUDIO_WAVE: Failed to load WAV data");
        }
        drwav_uninit(&wav);
    }
    else if (str8_match(file_type, str8("ogg"), Str_Match_Flag_CaseInsensitive))
    {
        stb_vorbis *ogg_data = stb_vorbis_open_memory(file_data.v, file_data.length, NULL, NULL);
        if (ogg_data != NULL)
        {
            stb_vorbis_info info = stb_vorbis_get_info(ogg_data);
            wave.sample_rate = info.sample_rate;
            wave.sample_size = 16;       // By default, ogg data is 16 bit per sample (uint16_t)
            wave.channels = info.channels;
            wave.frame_count = stb_vorbis_stream_length_in_samples(ogg_data);  // NOTE(ak): It returns frames!
            wave.data = arena_push(wave.arena, uint16_t, (size_t)wave.frame_count*wave.channels);
            // NOTE(ak): Get the number of samples to process (be careful! asking for number of shorts, not bytes!)
            stb_vorbis_get_samples_short_interleaved(ogg_data, info.channels, (short *)wave.data, wave.frame_count*wave.channels);
            stb_vorbis_close(ogg_data);
        }
        else
        {
            LogTraceF(Warn, "AUDIO_WAVE: Failed to load OGG data");
        }
    }
    else if (str8_match(file_type, str8("mp3"), Str_Match_Flag_CaseInsensitive))
    {
        drmp3_config config = STRUCT_ZERO;
        unsigned long long total_frame_count = 0;
        // NOTE(ak): Forcing conversion to 32bit float sample size on reading
        wave.data = drmp3_open_memory_and_read_pcm_frames_f32(file_data.v, file_data.length, &config, &total_frame_count, NULL);
        wave.sample_size = 32;
        
        if (wave.data != NULL)
        {
            wave.channels = config.channels;
            wave.sample_rate = config.sampleRate;
            wave.frame_count = (size_t)total_frame_count; // WARNING(ak): Potential loss of data
        }
        else
        {
            LogTraceF(Warn, "AUDIO_WAVE: Failed to load MP3 data");
        }
    }
    else if (str8_match(file_type, str8("qoa"), Str_Match_Flag_CaseInsensitive))
    {
        qoa_desc qoa = STRUCT_ZERO;
        // NOTE(ak): Returned sample data is always 16 bit?
        wave.data = qoa_decode(file_data.v, file_data.length, &qoa);
        wave.sample_size = 16;
        
        if (wave.data != NULL)
        {
            wave.channels = qoa.channels;
            wave.sample_rate = qoa.samplerate;
            wave.frame_count = qoa.samples;
        }
        else
        {
            LogTraceF(Warn, "AUDIO_WAVE: Failed to load QOA data");
        }
    }
    else if (str8_match(file_type, str8("flac"), Str_Match_Flag_CaseInsensitive))
    {
        unsigned long long total_frame_count = 0;
        // NOTE(ak): Forcing conversion to 16bit sample size on reading
        wave.data = drflac_open_memory_and_read_pcm_frames_s16(file_data.v, file_data.length, &wave.channels, &wave.sample_rate, &total_frame_count, NULL);
        wave.sample_size = 16;
        
        if (wave.data != NULL)
        {
            wave.frame_count = (size_t)total_frame_count; // WARNING(ak): Potential loss of data
        }
        else
        {
            LogTraceF(Warn, "AUDIO_WAVE: Failed to load FLAC data");
        }
    }
    else
    {
        LogTraceF(Warn, "AUDIO_WAVE: Data format not supported");
    }
    
    LogTraceF(Info, "AUDIO_WAVE: Data loaded successfully (%i Hz, %i bit, %i channels)", wave.sample_rate, wave.sample_size, wave.channels);
    
    return wave;
}

internal bool audio_wave_is_valid(Audio_Wave wave)
{
    bool result = false;
    if ((wave.data != NULL)    && // Validate wave data available
        (wave.frame_count > 0) && // Validate frame count
        (wave.sample_rate > 0) && // Validate sample rate is supported
        (wave.sample_size > 0) && // Validate sample size is supported
        (wave.channels > 0))      // Validate number of channels supported
    {
        result = true;
    }
    return result;
}

internal void audio_wave_unload(Audio_Wave wave)
{
    arena_free(wave.arena);
}

// ak: wave modification

internal void audio_wave_format_convert(Audio_Wave *wave, uint32_t sample_rate, uint32_t sample_size, uint32_t channels)
{
    // rjf: no-op if format already matches
    if(wave->sample_rate == sample_rate &&
        wave->sample_size == sample_size &&
        wave->channels == channels)
    {
        return;
    }
    
    ma_format format_in = ((wave->sample_size == 8)? ma_format_u8 : ((wave->sample_size == 16)? ma_format_s16 : ma_format_f32));
    ma_format format_out = ((sample_size == 8)? ma_format_u8 : ((sample_size == 16)? ma_format_s16 : ma_format_f32));
    
    ma_uint32 frame_count_in = wave->frame_count;
    ma_uint32 frame_count = (ma_uint32)ma_convert_frames(NULL, 0, format_out, channels, sample_rate, NULL, frame_count_in, format_in, wave->channels, wave->sample_rate);
    
    if (frame_count != 0)
    {
        arena_clear(wave->arena);
        size_t data_size = frame_count*channels*(sample_size/8);
        void *data = arena_push(wave->arena, uint8_t, data_size);
        frame_count = (ma_uint32)ma_convert_frames(data, frame_count, format_out, channels, sample_rate, wave->data, frame_count_in, format_in, wave->channels, wave->sample_rate);
        if (frame_count == 0)
        {
            wave->frame_count = frame_count;
            wave->sample_size = sample_size;
            wave->sample_rate = sample_rate;
            wave->channels = channels;
            // TODO(ak): have to find a way to free before assigning new data
            // free(wave->data);
            wave->data = data;
        } else {
            arena_clear(wave->arena);
            LogTraceF(Warn, "AUDIO_WAVE: Failed format conversion");
        }
    } else {
        LogTraceF(Warn, "AUDIO_WAVE: Failed to get frame count for format conversion");
    }
}

internal Audio_Wave audio_wave_copy(Audio_Wave wave)
{
    Audio_Wave new_wave = STRUCT_ZERO;
    new_wave.arena = arena_alloc();
    size_t new_wave_data_size = wave.frame_count*wave.channels*wave.sample_size/8;
    new_wave.data = arena_push(new_wave.arena, uint8_t, new_wave_data_size);
    if (new_wave.data != NULL)
    {
        // NOTE(ak): Size must be provided in bytes
        mem_copy(new_wave.data, wave.data, new_wave_data_size);
        new_wave.frame_count = wave.frame_count;
        new_wave.sample_rate = wave.sample_rate;
        new_wave.sample_size = wave.sample_size;
        new_wave.channels = wave.channels;
    }
    return new_wave;
}

internal void audio_wave_crop(Audio_Wave *wave, size_t init_frame, size_t final_frame)
{
    if ((init_frame < final_frame) &&
        (final_frame <= wave->frame_count))
    {
        size_t bytes_per_frame = wave->channels * (wave->sample_size / 8);
        size_t new_frame_count = final_frame - init_frame;
        size_t data_size = new_frame_count * bytes_per_frame;
        size_t offset = init_frame * bytes_per_frame;
        mem_copy(wave->data, (uint8_t *)wave->data + offset, data_size);
        wave->frame_count = new_frame_count;
    }
    else
    {
        LogTraceF(Warn, "AUDIO_WAVE: Crop range out of bounds");
    }
}

// ak: wave export

internal bool audio_wave_export_to_file(Audio_Wave wave, Str8 file_name)
{
    bool result = false;
    Str8 file_type = str8_skip_last_dot(file_name);
    
    if (str8_match(file_type, str8("wav"), Str_Match_Flag_CaseInsensitive))
    {
        drwav wav = STRUCT_ZERO;
        drwav_data_format format = STRUCT_ZERO;
        format.container = drwav_container_riff;
        if (wave.sample_size == 32) format.format = DR_WAVE_FORMAT_IEEE_FLOAT;
        else format.format = DR_WAVE_FORMAT_PCM;
        format.channels = wave.channels;
        format.sampleRate = wave.sample_rate;
        format.bitsPerSample = wave.sample_size;
        
        U8Array file_data = STRUCT_ZERO;
        result = drwav_init_memory_write(&wav, (void**)&file_data.v, &file_data.size, &format, NULL);
        if (result) result = (int32_t)drwav_write_pcm_frames(&wav, wave.frame_count, wave.data);
        drwav_result result = drwav_uninit(&wav);
        
        if (result == DRWAV_SUCCESS)
        {
            result = fs_file_path_write(file_name, file_data);
        }
        
        drwav_free(file_data.v, NULL);
    }
    else if (str8_match(file_type, str8("qoa"), Str_Match_Flag_CaseInsensitive))
    {
        if (wave.sample_size == 16)
        {
            qoa_desc qoa = STRUCT_ZERO;
            qoa.channels = wave.channels;
            qoa.samplerate = wave.sample_rate;
            qoa.samples = wave.frame_count;
            
            int bytes_written = qoa_write((const char *)file_name.cstr, (const short *)wave.data, &qoa);
            if (bytes_written > 0) result = true;
        }
        else
        {
            LogTraceF(Warn, "AUDIO: Wave data must be 16 bit per sample for QOA format export");
        }
    }
    else if (str8_match(file_type, str8("raw"), Str_Match_Flag_CaseInsensitive))
    {
        // ak: Export raw sample data (without header)
        // NOTE(ak): It's up to the user to track wave parameters
        result = fs_file_path_write(file_name, Array((uint8_t *)wave.data, wave.frame_count*wave.channels*wave.sample_size/8));
    }
    
    if (result)
    {
        LogTraceF(Info, "AUDIO_WAVE: [%s8] Wave data exported successfully", file_name);
    }
    else
    {
        LogTraceF(Warn, "AUDIO_WAVE: [%s8] Failed to export wave data", file_name);
    }
    
    return result;
}

internal Str8 audio_wave_export_to_str8(Audio_Wave wave, Str8 name, size_t wrap_size, Arena *arena)
{
    Str8_List list = STRUCT_ZERO;
    
    size_t wave_data_size = wave.frame_count*wave.channels*wave.sample_size/8;
    
    // ak: uppercase copy of the variable name
    Str8 var_name = str8_copy(arena, name);
    for (size_t i = 0; i < var_name.size; i++)
    {
        if (var_name.cstr[i] >= 'a' && var_name.cstr[i] <= 'z') { var_name.cstr[i] -= 32; }
    }
    
    // ak: wave information
    str8_list_pushf(arena, &list, "#define %.*s_FRAME_COUNT      %lu\n",   str8_varg(var_name), wave.frame_count);
    str8_list_pushf(arena, &list, "#define %.*s_SAMPLE_RATE      %u\n",    str8_varg(var_name), wave.sample_rate);
    str8_list_pushf(arena, &list, "#define %.*s_SAMPLE_SIZE      %u\n",    str8_varg(var_name), wave.sample_size);
    str8_list_pushf(arena, &list, "#define %.*s_CHANNELS         %u\n\n",  str8_varg(var_name), wave.channels);
    
    // ak: Wave data as an array: bytes for 8/16 bit, floats for 32 bit.
    // Frame data is channel-interlaced: frame01[ch1, ch2, ...], frame02[], ...
    if (wave.sample_size == 32)
    {
        size_t count = wave_data_size/4;
        float *samples = (float *)wave.data;
        str8_list_pushf(arena, &list, "global float %.*s_DATA[%lu] = {\n    ", str8_varg(var_name), (unsigned long)count);
        for (size_t i = 0; i < count; i++)
        {
            bool last = (i == count - 1);
            bool wrap = ((i + 1)%wrap_size == 0);
            if (last)      str8_list_pushf(arena, &list, "%.4ff };\n",   samples[i]);
            else if (wrap) str8_list_pushf(arena, &list, "%.4ff,\n    ", samples[i]);
            else           str8_list_pushf(arena, &list, "%.4ff, ",      samples[i]);
        }
    }
    else
    {
        size_t count = wave_data_size;
        uint8_t *bytes = (uint8_t *)wave.data;
        str8_list_pushf(arena, &list, "global uint8_t %.*s_DATA[%lu] = {\n    ", str8_varg(var_name), (unsigned long)count);
        for (size_t i = 0; i < count; i++)
        {
            bool last = (i == count - 1);
            bool wrap = ((i + 1)%wrap_size == 0);
            if (last)      str8_list_pushf(arena, &list, "0x%x };\n",   bytes[i]);
            else if (wrap) str8_list_pushf(arena, &list, "0x%x,\n    ", bytes[i]);
            else           str8_list_pushf(arena, &list, "0x%x, ",      bytes[i]);
        }
    }
    
    return str8_list_join(arena, &list, NULL);
}

// ak: Samples Management =====================================================

internal F32Array audio_samples_from_wave_load(Audio_Wave wave, Arena *arena)
{
    size_t sample_count = wave.frame_count * wave.channels;
    F32Array samples = array_alloc(arena, F32Array, sample_count, sample_count);
    for(size_t i = 0; i < samples.length; i += 1)
    {
        if(wave.sample_size == 8)
        {
            samples.v[i] = (float)(((uint32_t *)wave.data)[i] - 128) / 128.0f;
        }
        else if(wave.sample_size == 16)
        {
            samples.v[i] = (float)(((int16_t *)wave.data)[i]) / 32768.0f;
        }
        else if(wave.sample_size == 32)
        {
            samples.v[i] = ((float *)wave.data)[i];
        }
    }
    return samples;
}

// ak: Sound Management =======================================================

// ak: sound load, validate and unload

internal Audio_Sound audio_sound_load(Str8 file_name)
{
    Audio_Wave wave = audio_wave_load(file_name);
    Audio_Sound sound = audio_sound_load_from_wave(wave);
    audio_wave_unload(wave);
    return sound;
}

internal Audio_Sound audio_sound_load_from_wave(Audio_Wave wave)
{
    Audio_Sound sound = STRUCT_ZERO;
    if (wave.data != NULL)
    {
        // ak: When using miniaudio mixing needs to b done manually.
        // To simplify this, the format of each sound needs to be converted to
        // be consistent with the format used to open the playback
        // _audio_state.device. It can be done in two ways:
        //   1) Convert the whole sound in one go at load time (here)
        //   2) Convert the audio data in chunks at mixing time
        // First option has been selected, format conversion is done on the
        // loading stage. The downside is that it uses more memory if the
        // original sound is u8 or s16
        ma_format format_in = ((wave.sample_size == 8)? ma_format_u8 : ((wave.sample_size == 16)? ma_format_s16 : ma_format_f32));
        ma_uint32 frame_count_in = wave.frame_count;
        
        ma_uint32 frame_count = (ma_uint32)ma_convert_frames(NULL, 0, (ma_format)AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, _audio_state->system.device.sampleRate, NULL, frame_count_in, format_in, wave.channels, wave.sample_rate);
        if (frame_count == 0)
        {
            LogTraceF(Warn, "AUDIO_SOUND: Failed to get frame count for format conversion");
        }
        
        Audio_Buffer buffer = audio_buffer_alloc(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, _audio_state->system.device.sampleRate, frame_count, Audio_Buffer_Usage_Static);
        if (!audio_buffer_is_zero(buffer))
        {
            _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
            
            frame_count = (ma_uint32)ma_convert_frames(_buffer->data, frame_count, (ma_format)AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, _audio_state->system.device.sampleRate, wave.data, frame_count_in, format_in, wave.channels, wave.sample_rate);
            if (frame_count == 0)
            {
                LogTraceF(Warn, "AUDIO_SOUND: Failed format conversion");
            }
            
            sound.frame_count = frame_count;
            sound.stream.sample_rate = _audio_state->system.device.sampleRate;
            sound.stream.sample_size = 32;
            sound.stream.channels = AUDIO_DEVICE_CHANNELS;
            sound.stream.buffer = buffer;
        }
        else
        {
            LogTraceF(Warn, "AUDIO_SOUND: Failed to create buffer");
        }
    }
    return sound;
}

internal bool audio_sound_is_valid(Audio_Sound sound)
{
    bool result = false;
    
    if ((sound.frame_count > 0)        && // Validate frame count
        (sound.stream.sample_rate > 0) && // Validate sample rate is supported
        (sound.stream.sample_size > 0) && // Validate sample size is supported
        (sound.stream.channels > 0)   && // Validate number of channels supported
        (!audio_buffer_is_zero(sound.stream.buffer)))  // Validate stream buffer
    {
        result = true;
    }
    
    return result;
}

internal void audio_sound_unload(Audio_Sound sound)
{
    audio_buffer_free(sound.stream.buffer);
    LogTraceF(Info, "AUDIO_SOUND: Unloaded sound data from RAM");
}

internal Audio_Sound audio_sound_alias_load(Audio_Sound source)
{
    if (audio_buffer_is_zero(source.stream.buffer))
    {
        LogTraceF(Warn, "AUDIO_BUFFER: buffer is zero");
        return StructZeroType(Audio_Sound);
    }
    
    Audio_Sound sound = STRUCT_ZERO;
    _Audio_Buffer *_source_buffer = HandleUnpack(_Audio_Buffer, source.stream.buffer);
    if (_source_buffer->data != NULL)
    {
        Audio_Buffer buffer = audio_buffer_alloc(AUDIO_DEVICE_FORMAT, AUDIO_DEVICE_CHANNELS, _audio_state->system.device.sampleRate, 0, Audio_Buffer_Usage_Static);
        if (!audio_buffer_is_zero(buffer))
        {
            _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
            _buffer->size_in_frames = _source_buffer->size_in_frames;
            _buffer->data = _source_buffer->data;
            
            // Initialize the buffer as if it was new
            _buffer->volume = 1.0f;
            _buffer->pitch = 1.0f;
            _buffer->pan = 0.0f; // Center
            
            sound.frame_count = source.frame_count;
            sound.stream.sample_rate = _audio_state->system.device.sampleRate;
            sound.stream.sample_size = 32;
            sound.stream.channels = AUDIO_DEVICE_CHANNELS;
            sound.stream.buffer = buffer;
        }
        else
        {
            LogTraceF(Warn, "AUDIO_SOUND: Failed to create buffer");
        }
    }
    
    return sound;
}

internal void audio_sound_alias_unload(Audio_Sound alias)
{
    // ak: Untrack and unload the sound buffer, not the sample data, it is shared with the source for the alias
    if (!audio_buffer_is_zero(alias.stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, alias.stream.buffer);
        audio_buffer_untrack(alias.stream.buffer);
        ma_data_converter_uninit(&_buffer->converter, NULL);
        // TODO(ak): find a way to free converterResidual
        // free(alias.stream.buffer->converterResidual);
        audio_buffer_free(alias.stream.buffer);
    }
}

// ak: sound playback control

internal void audio_sound_play(Audio_Sound sound)
{
    audio_buffer_play(sound.stream.buffer);
}

internal void audio_sound_pause(Audio_Sound sound)
{
    audio_buffer_pause(sound.stream.buffer);
}

internal void audio_sound_resume(Audio_Sound sound)
{
    audio_buffer_resume(sound.stream.buffer);
}

internal void audio_sound_stop(Audio_Sound sound)
{
    audio_buffer_stop(sound.stream.buffer);
}

// ak: Update sound buffer with new data
// PARAMS: [data], format must match sound.stream.sample_size, default 32 bit float - stereo
// PARAMS: [frame_count] must not exceed sound.frame_count
internal void audio_sound_update(Audio_Sound sound, const void *data, size_t frame_count)
{
    if (!audio_buffer_is_zero(sound.stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, sound.stream.buffer);
        audio_buffer_stop(sound.stream.buffer);
        mem_copy(_buffer->data, data, frame_count*ma_get_bytes_per_frame(_buffer->converter.formatIn, _buffer->converter.channelsIn));
    }
}

// ak: sound playback query

internal bool audio_sound_is_playing(Audio_Sound sound)
{
    return audio_buffer_is_playing(sound.stream.buffer);
}

// ak: sound property set

internal void audio_sound_volume_set(Audio_Sound sound, float volume)
{
    audio_buffer_volume_set(sound.stream.buffer, volume);
}

internal void audio_sound_pitch_set(Audio_Sound sound, float pitch)
{
    audio_buffer_pitch_set(sound.stream.buffer, pitch);
}

internal void audio_sound_pan_set(Audio_Sound sound, float pan)
{
    audio_buffer_pan_set(sound.stream.buffer, pan);
}

// ak: Music Management =======================================================

// ak: music load, validate and unload

internal Audio_Music audio_music_load(Str8 file_name)
{
    Audio_Music music = STRUCT_ZERO;
    bool music_loaded = false;
    Str8 file_type = str8_skip_last_dot(file_name);
    music.arena = arena_alloc();
    
    if (str8_match(file_type, str8("wav"), Str_Match_Flag_CaseInsensitive))
    {
        drwav *context = arena_push(music.arena, drwav, 1);
        bool success = drwav_init_file(context, (const char *)file_name.cstr, NULL);
        
        if (success)
        {
            music.format = Audio_Format_Wav;
            music.context_data = context;
            uint32_t sample_size = context->bitsPerSample;
            if (context->bitsPerSample == 24) sample_size = 16; // Forcing conversion to s16 on audio_music_update()
            music.stream = audio_stream_load(context->sampleRate, sample_size, context->channels);
            music.frame_count = (size_t)context->totalPCMFrameCount;
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            arena_clear(music.arena);
        }
    }
    else if (str8_match(file_type, str8("ogg"), Str_Match_Flag_CaseInsensitive))
    {
        stb_vorbis *context = stb_vorbis_open_filename((const char *)file_name.cstr, NULL, NULL);
        if (context != NULL)
        {
            music.format = Audio_Format_Ogg;
            music.context_data = context;
            stb_vorbis_info info = stb_vorbis_get_info((stb_vorbis *)music.context_data);
            // ak: OGG bit rate defaults to 16 bit, it's enough for compressed format
            music.stream = audio_stream_load(info.sample_rate, 16, info.channels);
            // WARNING(ak): It seems this function returns length in frames, not samples, so multiply by channels
            music.frame_count = stb_vorbis_stream_length_in_samples((stb_vorbis *)music.context_data);
            music.looping = true; // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            stb_vorbis_close(context);
        }
    }
    else if (str8_match(file_type, str8("mp3"), Str_Match_Flag_CaseInsensitive))
    {
        drmp3 *context = arena_push(music.arena, drmp3, 1);
        drmp3_bool32 result = drmp3_init_file(context, (const char *)file_name.cstr, NULL);
        if (result > 0)
        {
            music.format = Audio_Format_Mp3;
            music.context_data = context;
            music.stream = audio_stream_load(context->sampleRate, 32, context->channels);
            music.frame_count = (size_t)drmp3_get_pcm_frame_count(context);
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            arena_clear(music.arena);
        }
    }
    else if (str8_match(file_type, str8("qoa"), Str_Match_Flag_CaseInsensitive))
    {
        qoaplay_desc *context = qoaplay_open((const char *)file_name.cstr);
        if (context != NULL)
        {
            music.format = Audio_Format_Qoa;
            music.context_data = context;
            // NOTE(ak): Loading samples as 32bit float normalized data, so,
            // configure the output audio stream to also use float 32bit
            music.stream = audio_stream_load(context->info.samplerate, 32, context->info.channels);
            music.frame_count = context->info.samples;
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            //ak: no uninit required
        }
    }
    else if (str8_match(file_type, str8("flac"), Str_Match_Flag_CaseInsensitive))
    {
        drflac *context = drflac_open_file((const char *)file_name.cstr, NULL);
        if (context != NULL)
        {
            music.format = Audio_Format_Flac;
            music.context_data = context;
            uint32_t sample_size = context->bitsPerSample;
            if (context->bitsPerSample == 24) sample_size = 16;   // Forcing conversion to s16 on audio_music_update()
            music.stream = audio_stream_load(context->sampleRate, sample_size, context->channels);
            music.frame_count = (size_t)context->totalPCMFrameCount;
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            drflac_free(context, NULL);
        }
    }
    else
    {
        LogTraceF(Warn, "AUDIO_STREAM: [%s8] File format not supported", file_name);
    }
    
    if (!music_loaded)
    {
        LogTraceF(Warn, "AUDIO_STREAM: [%s8] Music file could not be opened", file_name);
    }
    else
    {
        // Show some music stream info
        LogTraceF(Info, "AUDIO_MUSIC: [%s8] Music file loaded successfully", file_name);
        LogTraceF(Info, "    > Sample rate:   %i Hz", music.stream.sample_rate);
        LogTraceF(Info, "    > Sample size:   %i bits", music.stream.sample_size);
        LogTraceF(Info, "    > Channels:      %i (%s)", music.stream.channels, (music.stream.channels == 1)? "Mono" : (music.stream.channels == 2)? "Stereo" : "Multi");
        LogTraceF(Info, "    > Total frames:  %i", music.frame_count);
    }
    
    return music;
}

internal Audio_Music audio_music_from_memory_load(Str8 file_type, const uint8_t *data, size_t data_size)
{
    Audio_Music music = STRUCT_ZERO;
    music.arena = arena_alloc();
    bool music_loaded = false;
    
    if (str8_match(file_type, str8("wav"), Str_Match_Flag_CaseInsensitive))
    {
        drwav *context = arena_push(music.arena, drwav, 1);
        bool success = drwav_init_memory(context, (const void *)data, data_size, NULL);
        
        if (success)
        {
            music.format = Audio_Format_Wav;
            music.context_data = context;
            uint32_t sample_size = context->bitsPerSample;
            if (context->bitsPerSample == 24) sample_size = 16;   // Forcing conversion to s16 on audio_music_update()
            music.stream = audio_stream_load(context->sampleRate, sample_size, context->channels);
            music.frame_count = (size_t)context->totalPCMFrameCount;
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            drwav_uninit(context);
            arena_clear(music.arena);
        }
    }
    else if (str8_match(file_type, str8("ogg"), Str_Match_Flag_CaseInsensitive))
    {
        stb_vorbis *context = stb_vorbis_open_memory(data, data_size, NULL, NULL);

        if (context != NULL)
        {
            music.format = Audio_Format_Ogg;
            music.context_data = context;
            stb_vorbis_info info = stb_vorbis_get_info((stb_vorbis *)music.context_data);  // Get Ogg file info

            // ak: OGG bit rate defaults to 16 bit, it's enough for compressed format
            music.stream = audio_stream_load(info.sample_rate, 16, info.channels);

            // WARNING(ak): It seems this function returns length in frames, not samples, so multiply by channels
            music.frame_count = stb_vorbis_stream_length_in_samples((stb_vorbis *)music.context_data);
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            stb_vorbis_close(context);
        }
    }
    else if (str8_match(file_type, str8("mp3"), Str_Match_Flag_CaseInsensitive))
    {
        drmp3 *context = arena_push(music.arena, drmp3, 1);
        drmp3_bool32 success = drmp3_init_memory(context, (const void *)data, data_size, NULL);

        if (success)
        {
            music.format = Audio_Format_Mp3;
            music.context_data = context;
            music.stream = audio_stream_load(context->sampleRate, 32, context->channels);
            music.frame_count = (size_t)drmp3_get_pcm_frame_count(context);
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            drmp3_uninit(context);
            arena_clear(music.arena);
        }
    }
    else if (str8_match(file_type, str8("qoa"), Str_Match_Flag_CaseInsensitive))
    {
        qoaplay_desc *context = NULL;
        if ((data != NULL) && (data_size > 0))
        {
            context = qoaplay_open_memory(data, data_size);
        }
        if (context != NULL)
        {
            music.format = Audio_Format_Qoa;
            music.context_data = context;
            
            // NOTE(ak): Loading samples are 32bit float normalized data, so,
            // configure the output audio stream to also use float 32bit
            music.stream = audio_stream_load(context->info.samplerate, 32, context->info.channels);
            music.frame_count = context->info.samples;
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else{
            // ak: no uninit required
        }
    }
    else if (str8_match(file_type, str8("flac"), Str_Match_Flag_CaseInsensitive))
    {
        drflac *context = drflac_open_memory((const void *)data, data_size, NULL);

        if (context != NULL)
        {
            music.format = Audio_Format_Flac;
            music.context_data = context;
            uint32_t sample_size = context->bitsPerSample;
            if (context->bitsPerSample == 24) sample_size = 16;   // Forcing conversion to s16 on audio_music_update()
            music.stream = audio_stream_load(context->sampleRate, sample_size, context->channels);
            music.frame_count = (size_t)context->totalPCMFrameCount;
            music.looping = true;   // Looping enabled by default
            music_loaded = true;
        }
        else
        {
            drflac_free(context, NULL);
        }
    }
    else LogTraceF(Warn, "AUDIO_STREAM: Data format not supported");

    if (!music_loaded)
    {
        LogTraceF(Warn, "AUDIO_STREAM: Music data could not be loaded");
    }
    else
    {
        LogTraceF(Info, "AUDIO_MUSIC: Music data loaded successfully");
        LogTraceF(Info, "    > Sample rate:   %i Hz", music.stream.sample_rate);
        LogTraceF(Info, "    > Sample size:   %i bits", music.stream.sample_size);
        LogTraceF(Info, "    > Channels:      %i (%s)", music.stream.channels, (music.stream.channels == 1)? "Mono" : (music.stream.channels == 2)? "Stereo" : "Multi");
        LogTraceF(Info, "    > Total frames:  %i", music.frame_count);
    }
    
    return music;
}

internal bool audio_music_is_valid(Audio_Music music)
{
    return ((music.context_data != NULL)   && // Validate context loaded
            (music.frame_count > 0)        && // Validate audio frame count
            (music.stream.sample_rate > 0) && // Validate sample rate is supported
            (music.stream.sample_size > 0) && // Validate sample size is supported
            (music.stream.channels > 0));     // Validate number of channels supported
}

internal void audio_music_unload(Audio_Music music)
{
    if (audio_music_is_playing(music)) { audio_music_stop(music); }
    audio_stream_unload(music.stream);
    if (music.context_data != NULL)
    {
        if (music.format == Audio_Format_Wav)
        {
            drwav_uninit((drwav *)music.context_data);
        }
        else if (music.format == Audio_Format_Ogg)
        {
            stb_vorbis_close((stb_vorbis *)music.context_data);
        }
        else if (music.format == Audio_Format_Mp3)
        {
            drmp3_uninit((drmp3 *)music.context_data);
        }
        else if (music.format == Audio_Format_Qoa)
        {
            qoaplay_close((qoaplay_desc *)music.context_data);
        }
        else if (music.format == Audio_Format_Flac)
        {
            drflac_close((drflac *)music.context_data);
        }
    }
    arena_free(music.arena);
}

// ak: music playback control

internal void audio_music_play(Audio_Music music)
{
    audio_stream_play(music.stream);
}

internal void audio_music_pause(Audio_Music music)
{
    audio_stream_pause(music.stream);
}

internal void audio_music_resume(Audio_Music music)
{
    audio_stream_resume(music.stream);
}

internal void audio_music_stop(Audio_Music music)
{
    audio_stream_stop(music.stream);

    switch (music.format)
    {
        case Audio_Format_Unknown: break;
        case Audio_Format_Wav:  drwav_seek_to_first_pcm_frame((drwav *)music.context_data); break;
        case Audio_Format_Ogg:  stb_vorbis_seek_start((stb_vorbis *)music.context_data); break;
        case Audio_Format_Mp3:  drmp3_seek_to_start_of_stream((drmp3 *)music.context_data); break;
        case Audio_Format_Qoa:  qoaplay_rewind((qoaplay_desc *)music.context_data); break;
        case Audio_Format_Flac: drflac__seek_to_first_frame((drflac *)music.context_data); break;
    }
}

internal void audio_music_seek(Audio_Music music, float position)
{
    uint32_t position_in_frames = (uint32_t)(position*music.stream.sample_rate);
    
    switch (music.format)
    {
        case Audio_Format_Unknown: break;
        case Audio_Format_Wav: drwav_seek_to_pcm_frame((drwav *)music.context_data, position_in_frames); break;
        case Audio_Format_Ogg: stb_vorbis_seek_frame((stb_vorbis *)music.context_data, position_in_frames); break;
        case Audio_Format_Mp3: drmp3_seek_to_pcm_frame((drmp3 *)music.context_data, position_in_frames); break;
        case Audio_Format_Qoa:
        {
            int32_t qoaFrame = position_in_frames/QOA_FRAME_LEN;
            qoaplay_seek_frame((qoaplay_desc *)music.context_data, qoaFrame); // Seeks to QOA frame, not PCM frame

            // ak: Compute QOA frame number and update position_in_frames
            position_in_frames = ((qoaplay_desc *)music.context_data)->sample_position;
        } break;
        case Audio_Format_Flac: drflac_seek_to_pcm_frame((drflac *)music.context_data, position_in_frames); break;
    }
    
    if (!audio_buffer_is_zero(music.stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, music.stream.buffer);
        ma_mutex_lock(&_audio_state->system.lock);
        _buffer->frames_processed = position_in_frames;
        _buffer->is_sub_buffer_processed[0] = true;
        _buffer->is_sub_buffer_processed[1] = true;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_music_update(Audio_Music music)
{
    if (audio_buffer_is_zero(music.stream.buffer)) return;
    
    _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, music.stream.buffer);
    if (!_buffer->playing) return;
    
    ma_mutex_lock(&_audio_state->system.lock);
    
    uint32_t sub_buffer_size_in_frames = _buffer->size_in_frames/2;
    
    // ak: on first call of this function, lazily pre-allocated a temp buffer to read audio files/memory data in
    size_t frame_size = music.stream.channels*music.stream.sample_size/8;
    size_t pcm_size = sub_buffer_size_in_frames*frame_size;
    
    if (_audio_state->system.pcm.buffer_size < pcm_size && _audio_state->system.pcm.arena != NULL)
    {
        arena_clear(_audio_state->system.pcm.arena);
        _audio_state->system.pcm.buffer = arena_push(_audio_state->system.pcm.arena, uint8_t, pcm_size);
        _audio_state->system.pcm.buffer_size = pcm_size;
    }
    
    // ak: check both sub-buffers to check if they require refilling
    for (size_t i = 0; i < 2; i++)
    {
        uint32_t framesLeft = music.frame_count - _buffer->frames_processed;  // Frames left to be processed
        uint32_t framesToStream = 0;                 // Total frames to be streamed
        
        if ((framesLeft >= sub_buffer_size_in_frames) || music.looping) framesToStream = sub_buffer_size_in_frames;
        else framesToStream = framesLeft;
        
        if (framesToStream == 0)
        {
            // ak: check if both buffers have been processed
            if (_buffer->is_sub_buffer_processed[0] && _buffer->is_sub_buffer_processed[1])
            {
                ma_mutex_unlock(&_audio_state->system.lock);
                audio_music_stop(music);
                return;
            }
            ma_mutex_unlock(&_audio_state->system.lock);
            return;
        }
        
        if (!_buffer->is_sub_buffer_processed[i]) continue; // No refilling required, move to next sub-buffer
        
        size_t frame_count_still_needed = framesToStream;
        size_t frame_count_read_total = 0;
        
        switch (music.format)
        {
            case Audio_Format_Unknown: {}break;
            case Audio_Format_Wav:
            {
                if (music.stream.sample_size == 16)
                {
                    while (true)
                    {
                        size_t frame_count_read = (size_t)drwav_read_pcm_frames_s16((drwav *)music.context_data, frame_count_still_needed, (short *)((char *)_audio_state->system.pcm.buffer + frame_count_read_total*frame_size));
                        frame_count_read_total += frame_count_read;
                        frame_count_still_needed -= frame_count_read;
                        if (frame_count_still_needed == 0) break;
                        else drwav_seek_to_first_pcm_frame((drwav *)music.context_data);
                    }
                }
                else if (music.stream.sample_size == 32)
                {
                    while (true)
                    {
                        size_t frame_count_read = (size_t)drwav_read_pcm_frames_f32((drwav *)music.context_data, frame_count_still_needed, (float *)((char *)_audio_state->system.pcm.buffer + frame_count_read_total*frame_size));
                        frame_count_read_total += frame_count_read;
                        frame_count_still_needed -= frame_count_read;
                        if (frame_count_still_needed == 0) break;
                        else drwav_seek_to_first_pcm_frame((drwav *)music.context_data);
                    }
                }
            } break;
            case Audio_Format_Ogg:
            {
                while (true)
                {
                    size_t frame_count_read = stb_vorbis_get_samples_short_interleaved((stb_vorbis *)music.context_data, music.stream.channels, (short *)((char *)_audio_state->system.pcm.buffer + frame_count_read_total*frame_size), frame_count_still_needed*music.stream.channels);
                    frame_count_read_total += frame_count_read;
                    frame_count_still_needed -= frame_count_read;
                    if (frame_count_still_needed == 0) break;
                    else stb_vorbis_seek_start((stb_vorbis *)music.context_data);
                }
            } break;
            case Audio_Format_Mp3:
            {
                while (true)
                {
                    size_t frame_count_read = (size_t)drmp3_read_pcm_frames_f32((drmp3 *)music.context_data, frame_count_still_needed, (float *)((char *)_audio_state->system.pcm.buffer + frame_count_read_total*frame_size));
                    frame_count_read_total += frame_count_read;
                    frame_count_still_needed -= frame_count_read;
                    if (frame_count_still_needed == 0) break;
                    else drmp3_seek_to_start_of_stream((drmp3 *)music.context_data);
                }
            } break;
            case Audio_Format_Qoa:
            {
                size_t frame_count_read = qoaplay_decode((qoaplay_desc *)music.context_data, (float *)_audio_state->system.pcm.buffer, framesToStream);
                frame_count_read_total += frame_count_read;
            } break;
            case Audio_Format_Flac:
            {
                while (true)
                {
                    size_t frame_count_read = (size_t)drflac_read_pcm_frames_s16((drflac *)music.context_data, frame_count_still_needed, (short *)((char *)_audio_state->system.pcm.buffer + frame_count_read_total*frame_size));
                    frame_count_read_total += frame_count_read;
                    frame_count_still_needed -= frame_count_read;
                    if (frame_count_still_needed == 0) break;
                    else drflac__seek_to_first_frame((drflac *)music.context_data);
                }
            } break;
        }
        
        audio_stream_update_in_locked_state(music.stream, _audio_state->system.pcm.buffer, framesToStream);
    }

    ma_mutex_unlock(&_audio_state->system.lock);
}

// ak: music playback query

internal bool audio_music_is_playing(Audio_Music music)
{
    return audio_stream_is_playing(music.stream);
}

// ak: music property set

internal void audio_music_volume_set(Audio_Music music, float volume)
{
    audio_stream_volume_set(music.stream, volume);
}

internal void audio_music_pitch_set(Audio_Music music, float pitch)
{
    audio_buffer_pitch_set(music.stream.buffer, pitch);
}

internal void audio_music_pan_set(Audio_Music music, float pan)
{
    audio_buffer_pan_set(music.stream.buffer, pan);
}

// ak: music property get

internal float audio_music_duration_get(Audio_Music music)
{
    float totalSeconds = (float)music.frame_count/music.stream.sample_rate;
    return totalSeconds;
}

internal float audio_music_time_played_get(Audio_Music music)
{
    float seconds_played = 0.0f;
    if (!audio_buffer_is_zero(music.stream.buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, music.stream.buffer);
        
        uint64_t frames_processed = _buffer->frames_processed;
        size_t sub_buffer_size = _buffer->size_in_frames/2;
        uint64_t frames_in_first_buffer = _buffer->is_sub_buffer_processed[0]? 0 : sub_buffer_size;
        uint64_t frames_in_second_buffer = _buffer->is_sub_buffer_processed[1]? 0 : sub_buffer_size;
        uint64_t frames_in_buffers = frames_in_first_buffer + frames_in_second_buffer;
        if (((uint32_t)frames_in_buffers > music.frame_count) && !music.looping) frames_in_buffers = music.frame_count;
        uint64_t frames_sent_to_mix = _buffer->frame_cursor_pos%sub_buffer_size;
        int64_t frames_played = 0;
        if (music.frame_count > 0)
        {
            frames_played = ((int64_t)frames_processed - (int64_t)frames_in_buffers + (int64_t)frames_sent_to_mix) % (int64_t)music.frame_count;
            if (frames_played < 0) frames_played += (int64_t)music.frame_count;
        }
        seconds_played = (float)frames_played / (float)music.stream.sample_rate;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
    
    return seconds_played;
}

// ak: Audio Stream Management ================================================

// ak: audio stream load, validate and unload

Audio_Stream audio_stream_load(uint32_t sample_rate, uint32_t sample_size, uint32_t channels)
{
    Audio_Stream stream = STRUCT_ZERO;

    stream.sample_rate = sample_rate;
    stream.sample_size = sample_size;
    stream.channels = channels;

    Audio_Device_Format format_in = ((stream.sample_size == 8)? Audio_Device_Format_U8 : ((stream.sample_size == 16)? Audio_Device_Format_S16 : Audio_Device_Format_F32));

    // The size of a streaming buffer must be at least double the size of a period
    size_t periodSize = _audio_state->system.device.playback.internalPeriodSizeInFrames;

    // If the buffer is not set, compute one that would give us a buffer good enough for a decent frame rate at the device bit size/rate
    int32_t device_bits_per_sample = _audio_state->system.device.playback.format;
    if (device_bits_per_sample > 4)  device_bits_per_sample = 4;
    device_bits_per_sample *= _audio_state->system.device.playback.channels;

    size_t sub_buffer_size = (_audio_state->buffer.default_size == 0)? (_audio_state->system.device.sampleRate/30*device_bits_per_sample) : _audio_state->buffer.default_size;

    if (sub_buffer_size < periodSize) sub_buffer_size = periodSize;

    // Create a double audio buffer of defined size
    stream.buffer = audio_buffer_alloc(format_in, stream.channels, stream.sample_rate, sub_buffer_size*2, Audio_Buffer_Usage_Stream);

    if (!audio_buffer_is_zero(stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, stream.buffer);
        _buffer->looping = true;    // Always loop for streaming buffers
        LogTraceF(Info, "AUDIO_STREAM: Initialized successfully (%i Hz, %i bit, %s)", stream.sample_rate, stream.sample_size, (stream.channels == 1)? "Mono" : "Stereo");
    }
    else LogTraceF(Warn, "AUDIO_STREAM: Failed to load audio buffer, stream could not be created");

    return stream;
}

internal bool audio_stream_is_valid(Audio_Stream stream)
{
    return ((!audio_buffer_is_zero(stream.buffer)) && // Validate stream buffer
            (stream.sample_rate > 0) && // Validate sample rate is supported
            (stream.sample_size > 0) && // Validate sample size is supported
            (stream.channels > 0));     // Validate number of channels supported
}

internal void audio_stream_unload(Audio_Stream stream)
{
    audio_buffer_free(stream.buffer);
    LogTraceF(Info, "AUDIO_STREAM: Unloaded audio stream data from RAM");
}

// ak: audio stream playback control

internal void audio_stream_play(Audio_Stream stream)
{
    audio_buffer_play(stream.buffer);
}

internal void audio_stream_pause(Audio_Stream stream)
{
    audio_buffer_pause(stream.buffer);
}

internal void audio_stream_resume(Audio_Stream stream)
{
    audio_buffer_resume(stream.buffer);
}

internal void audio_stream_stop(Audio_Stream stream)
{
    audio_buffer_stop(stream.buffer);
}

// NOTE 1: Only updates one buffer of the stream source: dequeue -> update -> queue
// NOTE 2: To dequeue a buffer it needs to be processed: IsAudioStreamProcessed()
internal void audio_stream_update(Audio_Stream stream, const void *data, size_t frame_count)
{
    ma_mutex_lock(&_audio_state->system.lock);
    audio_stream_update_in_locked_state(stream, data, frame_count);
    ma_mutex_unlock(&_audio_state->system.lock);
}

internal void audio_stream_update_in_locked_state(Audio_Stream stream, const void *data, size_t frame_count)
{
    if (!audio_buffer_is_zero(stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, stream.buffer);
        if (_buffer->is_sub_buffer_processed[0] || _buffer->is_sub_buffer_processed[1])
        {
            ma_uint32 sub_buffer_to_update = 0;

            if (_buffer->is_sub_buffer_processed[0] && _buffer->is_sub_buffer_processed[1])
            {
                // ak: both buffers are available for updating
                // Update the first one and make sure the cursor is moved back to the front
                sub_buffer_to_update = 0;
                _buffer->frame_cursor_pos = 0;
            }
            else
            {
                // ak: update whichever sub-buffer is processed
                sub_buffer_to_update = (_buffer->is_sub_buffer_processed[0])? 0 : 1;
            }
            
            ma_uint32 suf_buffer_size_in_frames = _buffer->size_in_frames/2;
            uint8_t *sub_buffer = _buffer->data + ((suf_buffer_size_in_frames*stream.channels*(stream.sample_size/8))*sub_buffer_to_update);
            
            _buffer->frames_processed += frame_count;
            
            // ak: does this API expect a whole buffer to be updated in one go?
            // Assuming so, but if not will need to change this logic
            if (suf_buffer_size_in_frames >= (ma_uint32)frame_count)
            {
                ma_uint32 framesToWrite = (ma_uint32)frame_count;
                ma_uint32 bytesToWrite = framesToWrite*stream.channels*(stream.sample_size/8);
                memcpy(sub_buffer, data, bytesToWrite);
                // ak: Any leftover frames should be filled with zeros
                ma_uint32 leftoverFrameCount = suf_buffer_size_in_frames - framesToWrite;
                if (leftoverFrameCount > 0) memset(sub_buffer + bytesToWrite, 0, leftoverFrameCount*stream.channels*(stream.sample_size/8));

                _buffer->is_sub_buffer_processed[sub_buffer_to_update] = false;
            }
            else
            {
                LogTraceF(Warn, "AUDIO_STREAM: Attempting to write too many frames to buffer");
            }
        }
        else {
            LogTraceF(Warn, "AUDIO_STREAM: Buffer not available for updating");
        }
    }
}

// ak: audio stream playback query

internal bool audio_stream_is_playing(Audio_Stream stream)
{
    return audio_buffer_is_playing(stream.buffer);
}

// Check if any audio stream buffers requires refill
bool audio_stream_is_processed(Audio_Stream stream)
{
    bool result = false;

    if (!audio_buffer_is_zero(stream.buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, stream.buffer);
        result = _buffer->is_sub_buffer_processed[0] || _buffer->is_sub_buffer_processed[1];
        ma_mutex_unlock(&_audio_state->system.lock);
    }

    return result;
}

// ak: audio stream property set

// Set volume for audio stream (1.0 is max level)
internal void audio_stream_volume_set(Audio_Stream stream, float volume)
{
    audio_buffer_volume_set(stream.buffer, volume);
}

// Set pitch for audio stream (1.0 is base level)
internal void audio_stream_pitch_set(Audio_Stream stream, float pitch)
{
    audio_buffer_pitch_set(stream.buffer, pitch);
}

// Set pan for audio stream
internal void audio_stream_pan_set(Audio_Stream stream, float pan)
{
    audio_buffer_pan_set(stream.buffer, pan);
}

// Default size for new audio streams
internal void audio_stream_buffer_size_default_set(size_t size)
{
    _audio_state->buffer.default_size = size;
}

// Audio thread callback to request new data
internal void audio_stream_callback_set(Audio_Stream stream, Audio_Callback callback)
{
    if (!audio_buffer_is_zero(stream.buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, stream.buffer);
        _buffer->callback = callback;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

// ak: audio stream process management

// ak: Add processor to audio stream. Contrary to buffers, the order of processors is important
// The new processor must be added at the end. As there aren't supposed to be a lot of processors attached to
// a given stream, iterate through the list to find the end. That way there is no need to keep a pointer to the last element
internal void audio_stream_process_attach(Audio_Stream stream, Audio_Callback process)
{
    ma_mutex_lock(&_audio_state->system.lock);
    
    if (!audio_buffer_is_zero(stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, stream.buffer);
        Audio_Processor *processor = _audio_state->processor_free;
        if (processor)
        {
            SLLStackPop(_audio_state->processor_free);
        }
        else
        {
            processor = arena_push(_audio_state->arena, Audio_Processor, 1);
        }

        processor->process = process;
        Audio_Processor *last = _buffer->processor;
        while (last && last->next)
        {
            last = last->next;
        }

        if (last)
        {
            processor->prev = last;
            last->next = processor;
        }
        else
        {
            _buffer->processor = processor;
        }
    }
    
    ma_mutex_unlock(&_audio_state->system.lock);
}

internal void audio_stream_process_detach(Audio_Stream stream, Audio_Callback process)
{
    ma_mutex_lock(&_audio_state->system.lock);
    
    if (!audio_buffer_is_zero(stream.buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, stream.buffer);
        Audio_Processor *processor = _buffer->processor;
        while (processor)
        {
            Audio_Processor *next = processor->next;
            Audio_Processor *prev = processor->prev;
            if (processor->process == process)
            {
                if (_buffer->processor == processor) _buffer->processor = next;
                if (prev) prev->next = next;
                if (next) next->prev = prev;

                SLLStackPush(_audio_state->processor_free, processor);
            }
            processor = next;
        }
    }
    
    ma_mutex_unlock(&_audio_state->system.lock);
}

// ak: Audio Processor ========================================================

internal void audio_processor_mixed_attach(Audio_Callback process)
{
    ma_mutex_lock(&_audio_state->system.lock);
    
    Audio_Processor *processor = _audio_state->processor_free;
    if (processor)
    {
        SLLStackPop(_audio_state->processor_free);
    }
    else
    {
        processor = arena_push(_audio_state->arena, Audio_Processor, 1);
    }
    
    processor->process = process;
    Audio_Processor *last = _audio_state->processor_mixed;
    while (last && last->next)
    {
        last = last->next;
    }
    if (last)
    {
        processor->prev = last;
        last->next = processor;
    }
    else _audio_state->processor_mixed = processor;
    
    ma_mutex_unlock(&_audio_state->system.lock);
}

internal void audio_processor_mixed_detach(Audio_Callback process)
{
    ma_mutex_lock(&_audio_state->system.lock);
    
    Audio_Processor *processor = _audio_state->processor_mixed;
    while (processor)
    {
        Audio_Processor *next = processor->next;
        Audio_Processor *prev = processor->prev;
        if (processor->process == process)
        {
            if (_audio_state->processor_mixed == processor) _audio_state->processor_mixed = next;
            if (prev) prev->next = next;
            if (next) next->prev = prev;
            
            SLLStackPush(_audio_state->processor_free, processor);
        }
        processor = next;
    }
    
    ma_mutex_unlock(&_audio_state->system.lock);
}

// ak: Audio Buffer Management ================================================

// ak: audio buffer type management

internal inline bool audio_buffer_match(Audio_Buffer a, Audio_Buffer b)
{
    return a.id[0] == b.id[0];
}

internal inline bool audio_buffer_is_zero(Audio_Buffer buffer)
{
    return audio_buffer_match(buffer, StructZeroType(Audio_Buffer));
}

// ak: audio buffer alloc and free

internal Audio_Buffer audio_buffer_alloc(Audio_Device_Format format, uint32_t channels, uint32_t sample_rate, uint64_t size_in_frames, Audio_Buffer_Usage usage)
{
    Arena *arena = arena_alloc();
    _Audio_Buffer *_buffer = arena_push(arena, _Audio_Buffer, 1);
    _buffer->arena = arena_alloc();
    Audio_Buffer buffer = HandlePack(Audio_Buffer, _buffer);
    
    if (size_in_frames > 0)
    {
        _buffer->data = arena_push(_buffer->arena, uint8_t, size_in_frames*channels*ma_get_bytes_per_sample((ma_format)format));
    }
    
    // ak: Audio data runs through a format converter
    ma_data_converter_config converterConfig = ma_data_converter_config_init((ma_format)format, (ma_format)AUDIO_DEVICE_FORMAT, channels, AUDIO_DEVICE_CHANNELS, sample_rate, _audio_state->system.device.sampleRate);
    converterConfig.allowDynamicSampleRate = true;
    
    ma_result result = ma_data_converter_init(&converterConfig, NULL, &_buffer->converter);
    if (result == MA_SUCCESS)
    {
        // ak: A cache for use by the converter is necessary when resampling because
        // when generating output frames a different number of input frames will
        // be consumed. Any residual input frames need to be kept track of to
        // ensure there are no discontinuities. Since raylib supports pitch
        // shifting, which is done through resampling, a cache will always be
        // required. This will be kept relatively small to avoid too much wastage
        _buffer->converter_residual_count = 0;
        size_t bytes_per_frame = ma_get_bytes_per_frame((ma_format)format, channels);
        size_t converter_residual_size = AUDIO_BUFFER_RESIDUAL_CAPACITY*bytes_per_frame;
        _buffer->converter_residual = arena_push(_buffer->arena, uint8_t, converter_residual_size);
        
        // ak: Init audio buffer values
        _buffer->volume = 1.0f;
        _buffer->pitch = 1.0f;
        _buffer->pan = 0.0f; // Center
        
        _buffer->callback = NULL;
        _buffer->processor = NULL;
        
        _buffer->playing = false;
        _buffer->paused = false;
        _buffer->looping = false;
        
        _buffer->usage = usage;
        _buffer->frame_cursor_pos = 0;
        _buffer->frames_processed = 0;
        _buffer->size_in_frames = size_in_frames;
        
        // ak: Buffers should be marked as processed by default so that a call to
        // audio_stream_update() immediately after initialization works correctly
        _buffer->is_sub_buffer_processed[0] = true;
        _buffer->is_sub_buffer_processed[1] = true;
        
        // ak: Track audio buffer to linked list next position
        audio_buffer_track(buffer);
    }
    else
    {
        LogTraceF(Warn, "AUDIO_BUFFER: Failed to create data conversion pipeline");
        audio_buffer_free(buffer);
    }
    return buffer;
}

internal void audio_buffer_free(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        audio_buffer_untrack(buffer);
        ma_data_converter_uninit(&_buffer->converter, NULL);
        arena_free(_buffer->arena);
    }
}

// ak: audio buffer playback control

internal void audio_buffer_play(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        _buffer->playing = true;
        _buffer->paused = false;
        _buffer->frame_cursor_pos = 0;
        _buffer->frames_processed = 0;
        _buffer->is_sub_buffer_processed[0] = true;
        _buffer->is_sub_buffer_processed[1] = true;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_pause(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        _buffer->paused = true;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_resume(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        _buffer->paused = false;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_stop(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        _audio_buffer_stop_in_locked_state(_buffer);
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_track(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        if (_audio_state->buffer.first == NULL)
        {
            _audio_state->buffer.first = _buffer;
        }
        else
        {
            _audio_state->buffer.last->next = _buffer;
            _buffer->prev = _audio_state->buffer.last;
        }

        _audio_state->buffer.last = _buffer;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_untrack(Audio_Buffer buffer)
{
    if (!audio_buffer_is_zero(buffer))
    {
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        ma_mutex_lock(&_audio_state->system.lock);
        DLLRemove_NPZ(0, _audio_state->buffer.first, _audio_state->buffer.last, _buffer, next, prev);
        _buffer->prev = 0;
        _buffer->next = 0;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

// ak: audio buffer playback query

internal bool audio_buffer_is_playing(Audio_Buffer buffer)
{
    bool result = false;
    ma_mutex_lock(&_audio_state->system.lock);
    _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
    result = _audio_buffer_is_playing_in_locked_state(_buffer);
    ma_mutex_unlock(&_audio_state->system.lock);
    return result;
}

// ak: audio buffer property set

internal void audio_buffer_volume_set(Audio_Buffer buffer, float volume)
{
    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        _buffer->volume = volume;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_pitch_set(Audio_Buffer buffer, float pitch)
{
    if ((!audio_buffer_is_zero(buffer)) && (pitch > 0.0f))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        // ak: Pitching is an adjustment of the sample rate
        // Note that this changes the duration of the sound:
        //  - higher pitches will make the sound faster
        //  - lower pitches make it slower
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        ma_uint32 outputSampleRate = (ma_uint32)((float)_audio_state->system.device.sampleRate/pitch);
        ma_data_converter_set_rate(&_buffer->converter, _buffer->converter.sampleRateIn, outputSampleRate);
        _buffer->pitch = pitch;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}

internal void audio_buffer_pan_set(Audio_Buffer buffer, float pan)
{
    if (pan < -1.0f) pan = -1.0f;
    else if (pan > 1.0f) pan = 1.0f;

    if (!audio_buffer_is_zero(buffer))
    {
        ma_mutex_lock(&_audio_state->system.lock);
        _Audio_Buffer *_buffer = HandleUnpack(_Audio_Buffer, buffer);
        _buffer->pan = pan;
        ma_mutex_unlock(&_audio_state->system.lock);
    }
}
