package audio_capture

import "core:os"
import "core:fmt"
import "core:strconv"
import "core:path/filepath"
import ma "vendor:miniaudio"

project_name :: "audio-capture"
project_version :: "Version: 0.1"

help_message :: `A command line audio capture utility.
Usage:
    `+project_name+` [OPTIONS] [FILE]
Options:
    -s --show-devices       Show capture devices
    -i --index              Capture from device index
    -h --help               Print help
    -v --version            Print version
`+project_version+` | SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)`

main :: proc() {
    index := 0
    channels := 2
    sample_rate := 44100
    filename: string
    dir := ""

    if len(os.args) < 2 {
        dir := os.get_env("XDG_MUSIC_DIR")

        if dir != "" {
            dir = fmt.aprintf("%s/Recordings", dir)
        } else {
            dir = fmt.aprintf("%s/Music/Recordings", os.get_env("HOME"))
        }

        if !os.exists(dir) {
            if err := os.make_directory(dir, os.O_CREATE); err != 0 {
                if err == 2 {
                    fmt.eprintfln(
                        "Error: `%s` doesn't exist. This is where your recodings will live.",
                        filepath.dir(dir)
                    )
                    os.exit(1)
                }
                fmt.eprintfln("Error: `%d` Can't create directory in %s.", err, dir)
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
                index = get_arg_int(i)
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
}


show_devices :: proc() {
    capture_infos: [^]ma.device_info
    capture_count: u32

    audio_devices(&capture_infos, &capture_count)

    fmt.println(`Avaliable devices:
--------------------------------------------------------------------------------
 Index | Name
--------------------------------------------------------------------------------`
);
    for i in 0..<capture_count {
        fmt.printfln(" %d     | %s", i, capture_infos[i].name);
    }
    fmt.println("--------------------------------------------------------------------------------");
}

audio_devices :: proc(capture_infos: ^[^]ma.device_info, capture_count: ^u32) {
    pcontext: ma.context_type

    audio_handle_err(
        ma.context_init(nil, 0, nil, &pcontext),
        "Unable initialize miniaudio context."
    )

    audio_handle_err(
        ma.context_get_devices(&pcontext, nil, nil, capture_infos, capture_count),
        "Unable to get devices context."
    )

    ma.context_uninit(&pcontext)
}

audio_handle_err :: proc(result: ma.result, msg: string) {
    if result != .SUCCESS {
        fmt.eprint("Error: '")
        fmt.eprint(ma.result_description(result))
        fmt.eprint("' ")
        fmt.eprintln(msg)
    }
}

get_arg :: proc(arg_i: int) -> string {
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
