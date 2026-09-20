package audio_capture

import "core:os"
import "core:os/os2"
import "core:fmt"
import "core:strings"
import "core:strconv"
import "core:path/filepath"
import "core:runtime"
import "core:time"
import ma "vendor:miniaudio"

project_name :: "audio-capture"
project_version :: "Version: 0.1"

help_message :: `A command line audio capture utility.
Usage:
    `+project_name+` [OPTIONS] [FILE]
Options:
    -s --show-devices       Show capture devices
    -i --index              Capture device index (default: 0)
    -c --channels           Capture device channels (default: 2)
    -r --sample-rate        Capture device sample rate (default: 44100)
    -d --dir                Recording save directory
                            (default: $HOME/Music/Recordings if $XDG_MUSIC_DIR is
                            not specified.)
    -h --help               Print help
    -v --version            Print version
`+project_version+` | SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)`

main :: proc() {
    device_index := 0
    channels := 2
    sample_rate := 44100
    filename := ""
    dir := ""

    if len(os.args) < 2 {
        dir = os.get_env("XDG_MUSIC_DIR")

        if dir != "" {
            dir = strings.concatenate({dir, "/Recordings/"})
        } else {
            dir = strings.concatenate({os.get_env("HOME"), "/Music/Recordings/"})
        }

        if !os.exists(dir) {
            if err := os2.make_directory(dir, os2.File_Mode_Perm); err != os2.General_Error.None {
                fmt.eprintfln("Error: `%s` Can't create directory in %s.", os2.error_string(err), dir)
                os.exit(1)
            }
            fmt.printfln("Created directory '%s' for storing audio recordings.", dir)
        }
    }

    for i := 1; i<len(os.args); i+=1 {
        switch os.args[i] {
            case "-h", "--help":
                fmt.println(help_message)
                return
            case "-v", "--version":
                fmt.println(project_version)
                return
            case "-s", "--show-devices":
                show_devices()
                return
            case "-i", "--index":
                device_index = get_arg_int(i)
                i+=1
            case "-c", "--channels":
                channels = get_arg_int(i)
                i+=1
            case "-r", "--sampler-rate":
                sample_rate = get_arg_int(i)
                i+=1
            case "-d", "--dir":
                dir = get_arg(i)
                i+=1
            case:
                filename = os.args[i]
        }
    }

    if filename == "" {
        filename = "recording-0.wav"
        for i := 0; os.exists(filename); i+=1 {
            filename = fmt.aprintf("recording-%d.wav", i)
        }
    } else {
        if filepath.ext(filename) == "" {
            fmt.printfln("Error: Filename `%s` doesn't consist `.wav` extension.", filename)
            os.exit(1)
        }
        if filepath.ext(filename) != ".wav" {
            fmt.println("Error:", project_name, "can only make .wav file. Given name:", filename)
            os.exit(1)
        }
    }

    record(strings.concatenate({dir, filename}), device_index, channels, sample_rate)
}


show_devices :: proc() {
    devices_info: [^]ma.device_info
    devices_count: u32

    audio_devices(&devices_info, &devices_count)

    fmt.println(`Available devices:
--------------------------------------------------------------------------------
 Index | Name
--------------------------------------------------------------------------------`
);
    for i in 0..=devices_count {
        fmt.printfln(" %d     | %s", i, devices_info[i].name);
    }
    fmt.println("--------------------------------------------------------------------------------");
}

time_start: time.Time

record :: proc(save_path: string, device_index: int, channels: int, sample_rate: int) {
    devices_info: [^]ma.device_info
    devices_count: u32

    audio_devices(&devices_info, &devices_count)

    if (cast(u32)device_index > devices_count) {
        fmt.eprintfln(
            "Error: device-index exceeds capture device count. Given index %d > %d\n",
            device_index, devices_count
        )
        show_devices()
        os.exit(1)
    }

    encoder: ma.encoder
    device: ma.device

    if (channels > ma.MAX_CHANNELS) {
        fmt.eprintfln(
            "Error: channels (which is %d) can't be more then %d.",
            channels, ma.MAX_CHANNELS
        )
        os.exit(1)
    }

    if (sample_rate < 0) {
        fmt.eprintfln(
            "Error: sample_rate (which is %d) can't be negative.", sample_rate
        )
        os.exit(1)
    }

    encoder_config := ma.encoder_config_init(
        .wav, .f32, cast(u32)channels, cast(u32)sample_rate
    );

    audio_handle_err(
        ma.encoder_init_file(strings.clone_to_cstring(save_path), &encoder_config, &encoder),
        "Failed to initialize output file `%s`.", save_path
    );

    config := ma.device_config_init(.capture)
    config.playback.pDeviceID   = &devices_info[device_index].id
    config.capture.format       = encoder.config.format
    config.capture.channels     = encoder.config.channels
    config.sampleRate           = encoder.config.sampleRate
    config.dataCallback         = data_callback
    config.pUserData            = &encoder

    audio_handle_err(
        ma.device_init(nil, &config, &device),
        "Failed to initialize capture device: %s", devices_info[device_index].name
    )

    audio_handle_err(
        ma.device_start(&device),
        "Failed to start device %s", devices_info[device_index].name
    )

    fmt.println("Recording: ")
    time_start = time.now()
    for {}

    ma.device_uninit(&device)
    ma.encoder_uninit(&encoder)
}

data_callback :: proc "c" (device: ^ma.device, _output, input: rawptr, frame_count: u32) {
    context = runtime.default_context()
    encoder := cast(^ma.encoder)device.pUserData

    if (encoder == nil) {
    	fmt.println("Error: Unable to encode the audio.")
        os.exit(1)
    }

    ma.encoder_write_pcm_frames(encoder, input, cast(u64)frame_count, nil)
    audio_input := (^f32)(input)

    vol: f32 = 0.0
    for i: u32 = 0; i < frame_count*2; i += 2 {
        vol = max(vol, abs(audio_input^))
    }

    fmt.printf("\r")

    time_since := time.since(time_start)
    fmt.printf(
        "%02.0f:%02.0f:%02.0f ",
        time.duration_hours(time_since),
        time.duration_minutes(time_since),
        time.duration_seconds(time_since)
    )

    for i :f32= 0; i < 30; i+=1 {
        bar_pos: f32 = i /100
        if (bar_pos <= vol) {
            fmt.printf("█")
        } else {
            fmt.printf("░")
        }
    }

    os2.flush(os2.stdin)
}

audio_devices :: proc(devices_info: ^[^]ma.device_info, devices_count: ^u32) {
    pcontext: ma.context_type

    audio_handle_err(
        ma.context_init(nil, 0, nil, &pcontext),
        "Unable initialize miniaudio context."
    )

    audio_handle_err(
        ma.context_get_devices(&pcontext, nil, nil, devices_info, devices_count),
        "Unable to get devices context."
    )

    devices_count^ -= 1
    ma.context_uninit(&pcontext)
}

audio_handle_err :: proc(result: ma.result, msg: string, args: ..any) {
    if result != .SUCCESS {
        fmt.eprint("Error: '")
        fmt.eprint(ma.result_description(result))
        fmt.eprint("' ")
        fmt.eprintln(msg, args)
    }
}

get_arg :: proc(args_i: int) -> string {
    if len(os.args)-2 < args_i {
        fmt.eprintfln("Error: `%s` requires integer value. No value is given.", os.args[args_i])
        os.exit(1)
    }

    return os.args[args_i+1]
}

get_arg_int :: proc(args_i: int) -> int {
    if len(os.args)-2 < args_i {
        fmt.eprintfln("Error: `%s` requires integer value. No value is given.", os.args[args_i])
        os.exit(1)
    }

    rel := strconv.atoi(os.args[args_i+1])

    if rel == 0 && os.args[args_i+1] != "0" {
        fmt.eprintfln(
            "Error: `%s` requires integer value. Given value: %s",
            os.args[args_i], os.args[args_i+1]
        )
        os.exit(1)
    }

    return rel
}
