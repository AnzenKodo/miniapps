#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define PROJECT_NAME "ctimer"
#define PROJECT_VERSION "Version: 0.1"

const char *help_message = "Timer and Stopwatch writtern in C.\n\n"
"Usage: "PROJECT_NAME" [OPTIONS] [FILE]\n"
"Options:\n"
"   -h --help               Print help\n"
"   -v --version            Print version\n\n"
PROJECT_VERSION
" | SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)\n";

int main(int argc, char *argv[]) {
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
