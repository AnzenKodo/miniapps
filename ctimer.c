/********************************************************************************
 * Compile:
 *      GCC:    cc ctimer.c -lasound -o ctimer
 *      Clang:  cl ctimer.c -lasound -o ctimer
 *      tcc:    tcc ctimer.c -lasound -o ctimer
 *      MSVC:   cl ctimer.c -O:ctimer
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PROJECT_NAME "ctimer"
#define PROJECT_VERSION "Version: 0.1"

const char *help_message = "Timer and Stopwatch writtern in C.\n\n"
"Usage:\n"
"   "PROJECT_NAME" [OPTIONS] [FORMAT]\n"
"Format:\n"
"   0s  Seconds\n"
"   0m  Minute\n"
"   0h  Hour\n"
"Example:\n"
"   "PROJECT_NAME" 1h 5m 30s\n"
"Options:\n"
"   -d --d-beep             Duration of beep for Timer in seconds (default: 1)"
"   -n --no-beep            Disable beep for Timer"
"   -h --help               Print help\n"
"   -v --version            Print version\n\n"
PROJECT_VERSION
" | SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)\n";

//===============================================================================

/*******************************************************************************
 * Following code is taken from https://github.com/zserge/beep by Serge Zaitsev
 * Some parts of the code is modified by me.
 *
 *-------------------------------------------------------------------------------
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2020 Serge Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *******************************************************************************/

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
/* On Windows use the built-in Beep() function from <utilapiset.h> */
int beep(int freq, int ms) { return Beep(freq, ms); }
#elif __linux__
/* On Linux use alsa in synchronous mode, open "default" device in signed 8-bit
 * mode at 8kHz, mono, request for 20ms latency. Device is opened on first call
 * and never closed. */
#include <alsa/asoundlib.h>

int beep(int freq, int ms) {
  static snd_pcm_t *pcm = NULL;
  if (pcm == NULL) {
    if (snd_pcm_open(&pcm, "default", 0, 0)) {
      return -1;
    }
    snd_pcm_set_params(pcm, 1, 3, 1, 8000, 1, 20000);
  }
  unsigned char buf[2400];
  for (int i = 0; i < ms / 50; i++) {
    snd_pcm_prepare(pcm);
    for (long unsigned int j = 0; j < sizeof(buf); j++) {
      buf[j] = freq > 0 ? (500 * j * freq / 8000) : 0;
    }
    int r = snd_pcm_writei(pcm, buf, sizeof(buf));
    if (r < 0) {
      snd_pcm_recover(pcm, r, 0);
    }
  }
  return 0;
}
#elif __APPLE__
#include <AudioUnit/AudioUnit.h>

static dispatch_semaphore_t stopped, playing, done;

static int beep_freq;
static int beep_samples;
static int counter = 0;

static int initialized = 0;
static unsigned char theta = 0;

static OSStatus tone_cb(void *inRefCon,
                        AudioUnitRenderActionFlags *ioActionFlags,
                        const AudioTimeStamp *inTimeStamp, UInt32 inBusNumber,
                        UInt32 inNumberFrames, AudioBufferList *ioData) {
  unsigned int frame;
  unsigned char *buf = ioData->mBuffers[0].mData;
  unsigned long i = 0;

  for (i = 0; i < inNumberFrames; i++) {
    while (counter == 0) {
      dispatch_semaphore_wait(playing, DISPATCH_TIME_FOREVER);
      counter = beep_samples;
    }
    buf[i] = beep_freq > 0 ? (255 * theta * beep_freq / 8000) : 0;
    theta++;
    counter--;
    if (counter == 0) {
      dispatch_semaphore_signal(done);
      dispatch_semaphore_signal(stopped);
    }
  }
  return 0;
}

int beep(int freq, int ms) {
  if (!initialized) {
    AudioComponent output;
    AudioUnit unit;
    AudioComponentDescription descr;
    AURenderCallbackStruct cb;
    AudioStreamBasicDescription stream;

    initialized = 1;

    stopped = dispatch_semaphore_create(1);
    playing = dispatch_semaphore_create(0);
    done = dispatch_semaphore_create(0);

    descr.componentType = kAudioUnitType_Output,
    descr.componentSubType = kAudioUnitSubType_DefaultOutput,
    descr.componentManufacturer = kAudioUnitManufacturer_Apple,

    cb.inputProc = tone_cb;

    stream.mFormatID = kAudioFormatLinearPCM;
    stream.mFormatFlags = 0;
    stream.mSampleRate = 8000;
    stream.mBitsPerChannel = 8;
    stream.mChannelsPerFrame = 1;
    stream.mFramesPerPacket = 1;
    stream.mBytesPerFrame = 1;
    stream.mBytesPerPacket = 1;

    output = AudioComponentFindNext(NULL, &descr);
    AudioComponentInstanceNew(output, &unit);
    AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &cb, sizeof(cb));
    AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input, 0, &stream, sizeof(stream));
    AudioUnitInitialize(unit);
    AudioOutputUnitStart(unit);
  }

  dispatch_semaphore_wait(stopped, DISPATCH_TIME_FOREVER);
  beep_freq = freq;
  beep_samples = ms * 8;
  dispatch_semaphore_signal(playing);
  dispatch_semaphore_wait(done, DISPATCH_TIME_FOREVER);
  return 0;
}
#else
#error "unknown platform"
#endif
//===============================================================================

int main(int argc, char *argv[]) {
    unsigned int hour = 0, minute = 0, second = 0, time = 0;
    unsigned int max_hour = 0, max_minute = 0, max_second = 0, max_time = 0;
    char on_beep = 1;
    int d_beep = 1;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(help_message);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf(PROJECT_VERSION);
            puts("");
            return 0;
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-beep") == 0) {
            on_beep = 0;
            continue;
        } else if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--d-beep") == 0) {
            if (argv[i+1] == NULL) {
                fprintf(
                    stderr, "Error: `%s` requires integer value. No value is given.\n\n%s",
                    argv[i], help_message
                );
                exit(1);
            }

            d_beep = atoi(argv[i+1]);
            if (d_beep == 0 && strcmp(argv[i+1], "0") != 0) {
                fprintf(
                    stderr, "Error: `%s` only supports integer values. Given value: %s\n\n%s",
                    argv[i], argv[i+1], help_message
                );
                exit(1);
            }

            i++;
            continue;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: wrong argument provided `%s`\n\n%s", argv[i], help_message);
            exit(1);
        }

        size_t arg_len = strlen(argv[i]);
        char arg_end = argv[i][arg_len-1];
        char arg_num[4];
        strncpy (arg_num, argv[i], arg_len-1);

        if (arg_end == 'h') max_hour = atoi(arg_num);
        else if (arg_end == 'm') max_minute = atoi(arg_num);
        else if (arg_end == 's') max_second = atoi(arg_num);
        else {
            fprintf(stderr, "Error: wrong argument provided `%s`\n\n%s", argv[i], help_message);
            exit(1);
        }
    }

    max_time = max_hour + max_minute + max_second;
    while(1) {
        time = hour + minute + second;

        if (max_time == 0) {
            printf("\rStopwatch: %02d:%02d:%02d", hour, minute, second);
        } else {
            unsigned int progress = (time*100)/max_time;
            unsigned int i = 0;

            printf("\rTimer: %02d:%02d:%02d ", hour, minute, second);
            for (; i < progress/2; i++) printf("█");
            for (; i < 50; i++) printf("░");
            printf(" %2d%%", (time*100)/max_time);

            if (time == max_time) {
                printf("\n");
                if (on_beep) beep(10, d_beep*1000);
                break;
            }
        }
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
