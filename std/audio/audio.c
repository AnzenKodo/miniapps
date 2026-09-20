// #define MINIAUDIO_IMPLEMENTATION
// #include "./external/miniaudio.h"

// ak: Helpers
//=============================================================================

internal Audio_Sound audio_sound_zero(void)
{
    return StructZeroType(Audio_Sound);
}

internal Audio_Sound _audio_handle_from_sound(_Audio_Sound *_sound)
{
    Audio_Sound sound = {(uintptr_t)_sound};
    return sound;
}

internal _Audio_Sound *_audio_sound_from_handle(Audio_Sound sound)
{
    _Audio_Sound *_sound = (_Audio_Sound *)sound.u64[0];
    return _sound;
}

internal _Audio_Voice *_audio_voice_from_handle(Audio_Voice voice)
{
    _Audio_Voice *_voice = (_Audio_Voice *)voice.u64[0];
    return _voice;
}

internal Audio_Voice audio_voice_zero(void)
{
    return StructZeroType(Audio_Voice);
}

internal Audio_Voice _audio_voice_handle_from_voice(_Audio_Voice *_voice)
{
    Audio_Voice voice = {(uintptr_t)_voice};
    return voice;
}

internal bool audio_handle_is_valid(Audio_Sound sound)
{
    return _audio_sound_from_handle(sound) != NULL;
}

internal void _audio_callback(ma_device* device, void* output, const void* input, ma_uint32 frame_count)
{
    uint32_t channels = _audio_state->decoder.channels;
    float *out_buf = (float *)output;
    mem_set(out_buf, 0, frame_count * channels * sizeof(float));
    
    ma_mutex_lock(&_audio_state->mutex);
    
    _Audio_Voice *voice = _audio_state->active_voices_first;
    while(voice != NULL)
    {
        _Audio_Voice *next_voice = voice->next;
        
        if(voice->state == _Audio_Voice_State_Playing && voice->decoder_valid)
        {
            uint32_t bus_idx = voice->params.bus;
            if(bus_idx >= AUDIO_MAX_BUSES)
            {
                bus_idx = 0;
            }
            _Audio_Bus *bus = &_audio_state->buses[bus_idx];
            
            if(!bus->paused)
            {
                #define MIX_BUFFER_SIZE 1024
                float mix_buf[MIX_BUFFER_SIZE * 8];
                
                uint32_t out_channels = channels;
                if (out_channels > 8) out_channels = 8;
                
                ma_uint32 frames_mixed = 0;
                while(frames_mixed < frame_count)
                {
                    ma_uint32 chunk_frames = frame_count - frames_mixed;
                    if(chunk_frames > MIX_BUFFER_SIZE)
                    {
                        chunk_frames = MIX_BUFFER_SIZE;
                    }
                    
                    ma_uint64 frames_read = 0;
                    ma_result result = ma_decoder_read_pcm_frames(&voice->decoder, mix_buf, chunk_frames, &frames_read);
                    
                    float volume = voice->params.volume * bus->volume;
                    if(volume < 0.f) volume = 0.f;
                    
                    float left_gain = volume;
                    float right_gain = volume;
                    if(out_channels == 2)
                    {
                        float pan = voice->params.pan + bus->pan;
                        if(pan < -1.f) pan = -1.f;
                        if(pan > 1.f)  pan = 1.f;
                        
                        if(pan < 0.f)
                        {
                            right_gain *= (1.f + pan);
                        }
                        else if(pan > 0.f)
                        {
                            left_gain *= (1.f - pan);
                        }
                    }
                    
                    for(ma_uint32 i = 0; i < frames_read; i++)
                    {
                        for(uint32_t c = 0; c < out_channels; c++)
                        {
                            float sample = mix_buf[i * out_channels + c];
                            if(out_channels == 2)
                            {
                                sample *= (c == 0) ? left_gain : right_gain;
                            }
                            else
                            {
                                sample *= volume;
                            }
                            out_buf[(frames_mixed + i) * channels + c] += sample;
                        }
                    }
                    
                    frames_mixed += (ma_uint32)frames_read;
                    
                    if(result != MA_SUCCESS || frames_read < chunk_frames)
                    {
                        if(voice->params.loop)
                        {
                            ma_decoder_seek_to_pcm_frame(&voice->decoder, 0);
                        }
                        else
                        {
                            voice->state = _Audio_Voice_State_Finished;
                            break;
                        }
                    }
                }
                #undef MIX_BUFFER_SIZE
            }
        }
        
        if(voice->state == _Audio_Voice_State_Finished)
        {
            DLLRemove(_audio_state->active_voices_first, _audio_state->active_voices_last, voice);
            if(voice->decoder_valid)
            {
                ma_decoder_uninit(&voice->decoder);
                voice->decoder_valid = false;
            }
            voice->state = _Audio_Voice_State_Inactive;
            SLLStackPush(_audio_state->free_voice, voice);
        }
        
        voice = next_voice;
    }
    
    ma_mutex_unlock(&_audio_state->mutex);
    (void)input;
    (void)device;
}

// ak: Play Parameters
//=============================================================================

internal Audio_Play_Params audio_play_params_default(void)
{
    return (Audio_Play_Params) {
        .volume = 1.f,
        .pitch  = 1.f,
        .pan    = 0.f,
        .bus    = 0,
        .loop   = false,
    };
}

// ak: Lifetime
//=============================================================================

internal bool audio_init(uint32_t sample_rate, uint32_t channel_count)
{
    bool result = false;
    
    // ak: initialize sate
    Arena *arena = arena_alloc();
    _audio_state = arena_push(arena, _Audio_State, 1);
    _audio_state->arena = arena;
    _audio_state->decoder.format = ma_format_f32;
    _audio_state->decoder.channels = channel_count;
    _audio_state->decoder.sample_rate = sample_rate;
    
    // ak: initialize buses
    for(uint32_t i = 0; i < AUDIO_MAX_BUSES; i++)
    {
        _audio_state->buses[i].volume = 1.0f;
        _audio_state->buses[i].pitch  = 1.0f;
        _audio_state->buses[i].pan    = 0.0f;
        _audio_state->buses[i].paused = false;
    }
    
    // ak: initialize device config
    ma_device_config config  = ma_device_config_init(ma_device_type_playback);
    // Set to ma_format_unknown to use the device's native format.
    config.playback.format   = _audio_state->decoder.format;
    // Set to 0 to use the device's native channel count.
    config.playback.channels = _audio_state->decoder.channels;
    // Set to 0 to use the device's native sample rate.
    config.sampleRate        = _audio_state->decoder.sample_rate;
    config.dataCallback      = _audio_callback;

    // ak: initialize device
    ma_result init_result = ma_device_init(NULL, &config, &_audio_state->device);
    if (init_result == MA_SUCCESS)
    {
        ma_result mutex_result = ma_mutex_init(&_audio_state->mutex);
        if (mutex_result == MA_SUCCESS)
        {
            ma_result start_result = ma_device_start(&_audio_state->device);
            if(start_result == MA_SUCCESS)
            {
                result = true;
            }
            else
            {
                ma_mutex_uninit(&_audio_state->mutex);
                ma_device_uninit(&_audio_state->device);
            }
        }
        else
        {
            ma_device_uninit(&_audio_state->device);
        }
    }
    
    return result;
}

internal void audio_cleanup(void)
{
    if (_audio_state != 0)
    {
        ma_device_uninit(&_audio_state->device);
        ma_mutex_uninit(&_audio_state->mutex);
        arena_free(_audio_state->arena);
        _audio_state = 0;
    }
}

// ak: Loading
//=============================================================================

internal Audio_Sound audio_load_from_memory(U8Array data, Audio_Load_Flags flags)
{
    if (data.v == NULL || data.size == 0) { return audio_sound_zero(); }
    
    // ak: allocate sound record
    _Audio_Sound *_sound = _audio_state->free_sound;
    if (_sound)
    {
        SLLStackPop(_audio_state->free_sound);
    }
    else
    {
        _sound = arena_push(_audio_state->arena, _Audio_Sound, 1);
    }
    
    // ak: fill sound data
    _sound->in_use = true;
    _sound->data   = data;
    _sound->path   = StructZeroType(Str8);
    _sound->flags  = flags;
    
    // ak: bundle & return
    Audio_Sound result = _audio_handle_from_sound(_sound);
    return result;
}

internal Audio_Sound audio_load_from_path(Arena *arena, Str8 path, Audio_Load_Flags flags)
{
    if (path.length == 0) { return audio_sound_zero(); }
    
    Audio_Sound result = audio_sound_zero();
    if (flags & Audio_Load_Flag_Stream)
    {
        _Audio_Sound *_sound = _audio_state->free_sound;
        if (_sound)
        {
            SLLStackPop(_audio_state->free_sound);
        }
        else
        {
            _sound = arena_push(_audio_state->arena, _Audio_Sound, 1);
        }
        
        _sound->in_use = true;
        _sound->data   = StructZeroType(U8Array);
        _sound->path   = str8_copy(_audio_state->arena, path);
        _sound->flags  = flags;
        
        result = _audio_handle_from_sound(_sound);
    }
    else
    {
        U8Array data = fs_file_path_read_full(path, arena);
        result = audio_load_from_memory(data, flags);
    }
    return result;
}

internal void audio_unload(Audio_Sound sound)
{
    _Audio_Sound *_sound = _audio_sound_from_handle(sound);
    if(_sound != 0)
    {
        ma_mutex_lock(&_audio_state->mutex);
        _Audio_Voice *voice = _audio_state->active_voices_first;
        while(voice != 0)
        {
            _Audio_Voice *next_voice = voice->next;
            if(voice->sound.u64[0] == sound.u64[0])
            {
                DLLRemove(_audio_state->active_voices_first, _audio_state->active_voices_last, voice);
                if(voice->decoder_valid)
                {
                    ma_decoder_uninit(&voice->decoder);
                    voice->decoder_valid = false;
                }
                voice->state = _Audio_Voice_State_Inactive;
                SLLStackPush(_audio_state->free_voice, voice);
            }
            voice = next_voice;
        }
        
        _sound->in_use = false;
        _sound->data = StructZeroType(U8Array);
        _sound->path = StructZeroType(Str8);
        _sound->flags = 0;
        SLLStackPush(_audio_state->free_sound, _sound);
        ma_mutex_unlock(&_audio_state->mutex);
    }
}

// ak: Voice Controls
//=============================================================================

internal Audio_Voice audio_play_voice_from_sound(Audio_Sound sound, Audio_Play_Params params)
{
    _Audio_Sound *_sound = _audio_sound_from_handle(sound);
    if(_sound == NULL || !_sound->in_use) { return audio_voice_zero(); }
    
    if (params.bus >= AUDIO_MAX_BUSES)
    {
        params.bus = 0;
    }
    if (_sound->flags & Audio_Load_Flag_Loop)
    {
        params.loop = true;
    }
    
    ma_mutex_lock(&_audio_state->mutex);
    
    _Audio_Voice *voice = _audio_state->free_voice;
    if (voice)
    {
        SLLStackPop(_audio_state->free_voice);
    }
    else
    {
        voice = arena_push(_audio_state->arena, _Audio_Voice, 1);
    }
    
    ma_decoder_config config = ma_decoder_config_init(ma_format_f32, _audio_state->decoder.channels, _audio_state->decoder.sample_rate);
    
    ma_result decode_result;
    if ((_sound->flags & Audio_Load_Flag_Stream) && _sound->path.cstr != NULL)
    {
        decode_result = ma_decoder_init_file((const char *)_sound->path.cstr, &config, &voice->decoder);
    }
    else
    {
        decode_result = ma_decoder_init_memory(_sound->data.v, _sound->data.length, &config, &voice->decoder);
    }
    
    if(decode_result == MA_SUCCESS)
    {
        voice->decoder_valid = true;
        voice->sound         = sound;
        voice->params        = params;
        voice->state         = _Audio_Voice_State_Playing;
            
        DLLPushBack(_audio_state->active_voices_first, _audio_state->active_voices_last, voice);
    }
    else
    {
        voice->decoder_valid = false;
        SLLStackPush(_audio_state->free_voice, voice);
        voice = NULL;
    }
    
    ma_mutex_unlock(&_audio_state->mutex);
    
    return voice ? _audio_voice_handle_from_voice(voice) : audio_voice_zero();
}

internal bool audio_voice_is_alive(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return false; }
    
    ma_mutex_lock(&_audio_state->mutex);
    bool found = false;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            found = (v->state != _Audio_Voice_State_Inactive && v->decoder_valid);
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return found;
}

internal bool audio_voice_is_playing(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return false; }
    
    ma_mutex_lock(&_audio_state->mutex);
    bool playing = false;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            playing = (v->state == _Audio_Voice_State_Playing);
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return playing;
}

internal bool audio_voice_is_paused(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return false; }
    
    ma_mutex_lock(&_audio_state->mutex);
    bool paused = false;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            paused = (v->state == _Audio_Voice_State_Paused);
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return paused;
}

internal void audio_voice_pause(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            if (v->state == _Audio_Voice_State_Playing)
            {
                v->state = _Audio_Voice_State_Paused;
            }
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_voice_resume(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            if (v->state == _Audio_Voice_State_Paused)
            {
                v->state = _Audio_Voice_State_Playing;
            }
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_voice_toggle_pause(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            if (v->state == _Audio_Voice_State_Playing)
            {
                v->state = _Audio_Voice_State_Paused;
            }
            else if (v->state == _Audio_Voice_State_Paused)
            {
                v->state = _Audio_Voice_State_Playing;
            }
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_voice_restart(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice && v->decoder_valid)
        {
            ma_decoder_seek_to_pcm_frame(&v->decoder, 0);
            v->state = _Audio_Voice_State_Playing;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_voice_set_volume(Audio_Voice voice, float volume)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    if (volume < 0.0f) volume = 0.0f;
    if (volume > 1.0f) volume = 1.0f;
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            v->params.volume = volume;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal float audio_voice_get_volume(Audio_Voice voice)
{
    if (_audio_state == 0) { return 0.0f; }
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return 0.0f; }
    
    ma_mutex_lock(&_audio_state->mutex);
    float vol = 0.0f;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            vol = v->params.volume;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return vol;
}

internal double audio_voice_get_position_seconds(Audio_Voice voice)
{
    if (_audio_state == 0) { return 0.0; }
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return 0.0; }
    
    ma_mutex_lock(&_audio_state->mutex);
    double pos = 0.0;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice && v->decoder_valid)
        {
            ma_uint64 cursor = 0;
            if (ma_decoder_get_cursor_in_pcm_frames(&v->decoder, &cursor) == MA_SUCCESS)
            {
                ma_uint32 sample_rate = v->decoder.outputSampleRate ? v->decoder.outputSampleRate : _audio_state->decoder.sample_rate;
                if (sample_rate > 0)
                {
                    pos = (double)cursor / (double)sample_rate;
                }
            }
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return pos;
}

internal double audio_voice_get_duration_seconds(Audio_Voice voice)
{
    if (_audio_state == 0) { return 0.0; }
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return 0.0; }
    
    ma_mutex_lock(&_audio_state->mutex);
    double dur = 0.0;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice && v->decoder_valid)
        {
            ma_uint64 length = 0;
            if (ma_decoder_get_length_in_pcm_frames(&v->decoder, &length) == MA_SUCCESS)
            {
                ma_uint32 sample_rate = v->decoder.outputSampleRate ? v->decoder.outputSampleRate : _audio_state->decoder.sample_rate;
                if (sample_rate > 0)
                {
                    dur = (double)length / (double)sample_rate;
                }
            }
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return dur;
}

internal bool audio_voice_seek_seconds(Audio_Voice voice, double seconds)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return false; }
    
    ma_mutex_lock(&_audio_state->mutex);
    bool success = false;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice && v->decoder_valid)
        {
            ma_uint32 sample_rate = v->decoder.outputSampleRate ? v->decoder.outputSampleRate : _audio_state->decoder.sample_rate;
            ma_uint64 total_frames = 0;
            ma_decoder_get_length_in_pcm_frames(&v->decoder, &total_frames);
            
            if (seconds < 0.0) seconds = 0.0;
            ma_uint64 target_frame = (ma_uint64)(seconds * (double)sample_rate);
            if (total_frames > 0 && target_frame >= total_frames)
            {
                target_frame = (total_frames > 0) ? total_frames - 1 : 0;
            }
            
            ma_result res = ma_decoder_seek_to_pcm_frame(&v->decoder, target_frame);
            success = (res == MA_SUCCESS);
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return success;
}

internal void audio_voice_set_pitch(Audio_Voice voice, float pitch)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    if (pitch < 0.0f) pitch = 0.0f;
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            v->params.pitch = pitch;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal float audio_voice_get_pitch(Audio_Voice voice)
{
    if (_audio_state == 0) { return 1.0f; }
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return 1.0f; }
    
    ma_mutex_lock(&_audio_state->mutex);
    float pitch = 1.0f;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            pitch = v->params.pitch;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return pitch;
}

internal void audio_voice_set_pan(Audio_Voice voice, float pan)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f)  pan = 1.0f;
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            v->params.pan = pan;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal float audio_voice_get_pan(Audio_Voice voice)
{
    if (_audio_state == 0) { return 0.0f; }
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return 0.0f; }
    
    ma_mutex_lock(&_audio_state->mutex);
    float pan = 0.0f;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            pan = v->params.pan;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return pan;
}

internal void audio_voice_set_bus(Audio_Voice voice, uint32_t bus)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0 || bus >= AUDIO_MAX_BUSES) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            v->params.bus = bus;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal uint32_t audio_voice_get_bus(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return 0; }
    
    ma_mutex_lock(&_audio_state->mutex);
    uint32_t bus = 0;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            bus = v->params.bus;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return bus;
}


internal void audio_voice_set_play_params(Audio_Voice voice, Audio_Play_Params params)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            v->params = params;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal Audio_Play_Params audio_voice_get_play_params(Audio_Voice voice)
{
    _Audio_Voice *_voice = _audio_voice_from_handle(voice);
    if (_voice == 0) { return StructZeroType(Audio_Play_Params); }
    
    ma_mutex_lock(&_audio_state->mutex);
    Audio_Play_Params params = STRUCT_ZERO;
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v == _voice)
        {
            params = v->params;
            break;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
    return params;
}

// ak: Bus Controls
//=============================================================================

internal void audio_bus_set_volume(uint32_t bus, float volume)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    if (volume < 0.0f) volume = 0.0f;
    
    ma_mutex_lock(&_audio_state->mutex);
    _audio_state->buses[bus].volume = volume;
    ma_mutex_unlock(&_audio_state->mutex);
}

internal float audio_bus_get_volume(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return 0.0f; }
    
    ma_mutex_lock(&_audio_state->mutex);
    float vol = _audio_state->buses[bus].volume;
    ma_mutex_unlock(&_audio_state->mutex);
    return vol;
}

internal void audio_bus_set_pitch(uint32_t bus, float pitch)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    if (pitch < 0.0f) pitch = 0.0f;
    
    ma_mutex_lock(&_audio_state->mutex);
    _audio_state->buses[bus].pitch = pitch;
    ma_mutex_unlock(&_audio_state->mutex);
}

internal float audio_bus_get_pitch(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return 1.0f; }
    
    ma_mutex_lock(&_audio_state->mutex);
    float pitch = _audio_state->buses[bus].pitch;
    ma_mutex_unlock(&_audio_state->mutex);
    return pitch;
}

internal void audio_bus_set_pan(uint32_t bus, float pan)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    if (pan < -1.0f) pan = -1.0f;
    if (pan > 1.0f)  pan = 1.0f;
    
    ma_mutex_lock(&_audio_state->mutex);
    _audio_state->buses[bus].pan = pan;
    ma_mutex_unlock(&_audio_state->mutex);
}

internal float audio_bus_get_pan(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return 0.0f; }
    
    ma_mutex_lock(&_audio_state->mutex);
    float pan = _audio_state->buses[bus].pan;
    ma_mutex_unlock(&_audio_state->mutex);
    return pan;
}

internal void audio_bus_pause(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    _audio_state->buses[bus].paused = true;
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_bus_resume(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    _audio_state->buses[bus].paused = false;
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_bus_toggle_pause(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    _audio_state->buses[bus].paused = !_audio_state->buses[bus].paused;
    ma_mutex_unlock(&_audio_state->mutex);
}

internal bool audio_bus_is_paused(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return false; }
    
    ma_mutex_lock(&_audio_state->mutex);
    bool paused = _audio_state->buses[bus].paused;
    ma_mutex_unlock(&_audio_state->mutex);
    return paused;
}

internal void audio_bus_stop(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    _Audio_Voice *voice = _audio_state->active_voices_first;
    while(voice != 0)
    {
        _Audio_Voice *next_voice = voice->next;
        if(voice->params.bus == bus)
        {
            DLLRemove(_audio_state->active_voices_first, _audio_state->active_voices_last, voice);
            if(voice->decoder_valid)
            {
                ma_decoder_uninit(&voice->decoder);
                voice->decoder_valid = false;
            }
            voice->state = _Audio_Voice_State_Inactive;
            SLLStackPush(_audio_state->free_voice, voice);
        }
        voice = next_voice;
    }
    ma_mutex_unlock(&_audio_state->mutex);
}

internal void audio_bus_restart(uint32_t bus)
{
    if (bus >= AUDIO_MAX_BUSES) { return; }
    
    ma_mutex_lock(&_audio_state->mutex);
    for (_Audio_Voice *v = _audio_state->active_voices_first; v != 0; v = v->next)
    {
        if (v->params.bus == bus && v->decoder_valid)
        {
            ma_decoder_seek_to_pcm_frame(&v->decoder, 0);
            v->state = _Audio_Voice_State_Playing;
        }
    }
    ma_mutex_unlock(&_audio_state->mutex);
}
