#define MINIAUDIO_IMPLEMENTATION

// #define MA_DEBUG_OUTPUT
#define MA_NO_FLAC
#define MA_NO_RESOURCE_MANAGER
#define MA_NO_NODE_GRAPH
#define MA_NO_ENGINE
#define MA_NO_GENERATION
#define MA_NO_AVX2
#define MA_NO_NEON

#include "./miniaudio.h"

#define PROJECT_NAME "audio-capture"
#define PROJECT_VERSION "Version: 0.1"

const char *help_message = "A command line audio capture utility.\n\n"
"Usage: "PROJECT_NAME" [OPTIONS] [FILE]\n"
"Options:\n"
"   -s --show-devices       Show capture devices\n"
"   -i --index              Capture from device index\n"
"   -h --help               Print help\n"
"   -v --version            Print version\n\n"
PROJECT_VERSION
" | SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)\n";

void count(void) {
    unsigned int hour = 0;
    unsigned int minute = 0;
    unsigned int second = 0;

    while(1) {
        printf("\r%02d:%02d:%02d", hour, minute, second);
        fflush(stdout);

        second++;
        if (second == 60) {
           minute += 1;
           second = 0;
        }
        if(minute == 60) {
            hour += 1;
            minute = 0;
        }
        sleep(1);
    }
}

void audio_handle_err(ma_result result, char *str, ...) {
    if (result == MA_SUCCESS) return;

    fprintf(stderr, "Error: '%s' ", ma_result_description(result));
    va_list args;
    va_start(args, str);
        fprintf(stderr, str, va_arg(args, char*));
    va_end(args);
    fprintf(stderr, "\n");
    exit(1);
}

void data_callback(
    ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount
) {
    ma_encoder* pEncoder = (ma_encoder*)pDevice->pUserData;
    MA_ASSERT(pEncoder != NULL);

    ma_encoder_write_pcm_frames(pEncoder, pInput, frameCount, NULL);

    (void)pOutput;
}

void record(
    char *filename, ma_device_id *device_id,
    unsigned int channels, int sample_rate
) {
    ma_encoder_config encoderConfig;
    ma_encoder encoder;
    ma_device device;

    if (channels > MA_MAX_CHANNELS) {
        fprintf(
            stderr, "Error: channels (which is %d) can't be more then %d.\n",
            channels, MA_MAX_CHANNELS
        );
        exit(1);
    }

    if (sample_rate < 0) {
        fprintf(
            stderr, "Error: sample-rate (which is %d) can't be negative.\n",
            sample_rate
        );
        exit(1);
    }

    encoderConfig = ma_encoder_config_init(
        ma_encoding_format_wav, ma_format_f32, channels, sample_rate
    );

    audio_handle_err(
        ma_encoder_init_file(filename, &encoderConfig, &encoder),
        "Failed to initialize output file `%s`.", filename
    );

    ma_device_config config = ma_device_config_init(ma_device_type_capture);
    config.playback.pDeviceID = device_id;
    config.capture.format     = encoder.config.format;
    config.capture.channels   = encoder.config.channels;
    config.sampleRate         = encoder.config.sampleRate;
    config.dataCallback       = data_callback;
    config.pUserData          = &encoder;

    audio_handle_err(
        ma_device_init(NULL, &config, &device),
        "Failed to initialize capture device."
    );

    audio_handle_err(ma_device_start(&device), "Failed to start device.");

    count();

    ma_device_uninit(&device);
    ma_encoder_uninit(&encoder);
}

void audio_devices(ma_device_info **capture_infos, ma_uint32 *capture_count) {
    ma_context context;

    audio_handle_err(
        ma_context_init( NULL, 0, NULL, &context),
        "Unable initialize miniaudio context."
    );


    audio_handle_err(
        ma_context_get_devices(&context, NULL, NULL, capture_infos, capture_count),
        "Unable to get devices context."
    );

    ma_context_uninit(&context);
}

void show_devices(void) {
    ma_device_info* capture_infos;
    ma_uint32 capture_count;

    audio_devices(&capture_infos, &capture_count);

    printf("Avaliable devices:\n");
    printf("--------------------------------------------------------------------------------\n");
    printf(" Index |  Name\n");
    printf("--------------------------------------------------------------------------------\n");
    for (ma_uint32 i = 0; i < capture_count; i += 1) {
        printf("   %d   | %s\n", i, capture_infos[i].name);
    }
    printf("--------------------------------------------------------------------------------\n");
}

ma_device_id get_device_id(ma_uint32 device_index) {
    ma_device_info* capture_infos;
    ma_uint32 capture_count;

    audio_devices(&capture_infos, &capture_count);

    if (device_index < capture_count) {
        return capture_infos[device_index].id;
    } else {
        fprintf(stderr, "Error: device-index exceeds capture device count.\n");
        show_devices();
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    ma_device_id device_id = get_device_id(0);;
    unsigned int channels = 2;
    unsigned int sample_rate = 44100;
    char *filename;
    if (argc < 2) {
        printf(help_message);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(help_message);
            return 0;
        }
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf(PROJECT_VERSION);
            puts("");
            return 0;
        }
        if (strcmp(argv[i], "-s") == 0 || strcmp(argv[i], "--show-devices") == 0) {
            show_devices();
            return 0;
        }

        if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--index") == 0) {
            device_id =  get_device_id(atoi(argv[i++]));
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--channels") == 0) {
            channels = atoi(argv[i++]);
        } else if (strcmp(argv[i], "-r") == 0 || strcmp(argv[i], "--sampler-rate") == 0) {
            sample_rate = atoi(argv[i++]);
        } else {
            filename = argv[i];
        }
    }

    record(filename, &device_id, channels, sample_rate);

    return 0;
}
