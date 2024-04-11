/********************************************************************************
 * `build.c`: A C file that builds C programs.
 * Taken inspiration from
 *  - https://github.com/tsoding/nobuild by Alexey Kutepov
 * -----------------------------------------------------------------------------
 * MIT License
 *
 * Copyright 2024 AnzenKodo
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#ifndef _WIN32
#   include <sys/stat.h>
#else
#   define WIN32_LEAN_AND_MEAN
#   define NOCOMM
#   include <windows.h>
#endif // _WIN32

#if _MSC_VER
#	include <stdarg.h>
#endif // _MSC_VER

#define PROJECT_TITLE "example"
#define MAIN_FILE "examples.c"
#define BUILD_DIR "build"

const char *HELP_MESSAGE = "build.c: A C file that build's C files\n"
"Version: 0.1"
"Usage:\n"
"	CMD: build.exe [options]\n"
"	PowerShell: ./build.exe [options]\n"
"	Bash & Others: ./build [options]\n"
"Options:\n"
"	b	Build\n"
"	br	Build and Run project\n"
"	r	Only run program\n"
// "	c	Only compile\n"
"	h	Print help\n";

char cmd[100] = "";

typedef enum {
	BUILD_LOG_ALL,
	BUILD_LOG_DEBUG,
    BUILD_LOG_INFO,
    BUILD_LOG_WARNING,
    BUILD_LOG_ERROR,
    BUILD_LOG_NONE
} Build_Log_Levels;

static Build_Log_Levels BUILD_LOG_LEVEL = BUILD_LOG_INFO;

extern void build_cmd_append(char *cmd, char *src, ...);
extern void build_cmd_finish(char *cmd);
extern void build_dir_make(char *path);
extern void build_log(Build_Log_Levels level, char *fmt, ...);

void build_proj_compile(char *cmd) {
	build_log(BUILD_LOG_INFO, "Compiling Project:\n");
	build_cmd_append(cmd, "cc "MAIN_FILE); 					  	      // Main
	// build_cmd_append(cmd, " -c");							          // Don't link
	// build_cmd_append(cmd, " -Iincludes");							  // Includes
	build_cmd_append(cmd, " -o "BUILD_DIR"/"PROJECT_TITLE); 	  // Object file
	build_cmd_append(cmd, " -g3");									  // Debug
	build_cmd_append(cmd, " -Wall -Wextra");							      // Warning
	// Security
	// build_cmd_append(cmd, " -GS -EHa /guard:cf -sdl -Qspectre -fsanitize=address");
	build_cmd_finish(cmd);
}

void build_proj_link(char *cmd) {
	build_log(BUILD_LOG_INFO, "Linking Project:\n");
	build_cmd_append(cmd, "ld "BUILD_DIR"/"PROJECT_TITLE".o");          // Main
	// build_cmd_append(cmd, " -LIBPATH:libs");									// Libs
	// build_cmd_append(cmd, " -DEBUG:FULL");				   				 	// Debug
	build_cmd_append(cmd, " -o "BUILD_DIR"/"PROJECT_TITLE);			 	        // Output
	// build_cmd_append(cmd, " -CETCOMPAT -DYNAMICBASE -HIGHENTROPYVA");	 		// Security
	build_cmd_finish(cmd);
}

void build_proj_run(char *cmd) {
	build_log(BUILD_LOG_INFO, "Running Project:\n");
	build_cmd_append(cmd, BUILD_DIR"/"PROJECT_TITLE);
	build_cmd_finish(cmd);
}

int main(int argc, char *argv[]) {
    (void)argc;
	char *args = argv[1];

	build_dir_make(BUILD_DIR);

	if (!args) {
		printf(HELP_MESSAGE);
	} else if (strcmp(args, "h") == 0) {
		printf(HELP_MESSAGE);
	} else if (strcmp(args, "b") == 0) {
		build_proj_compile(cmd);
		// build_proj_link(cmd);
	} else if (strcmp(args, "br") == 0) {
		build_proj_compile(cmd);
		// build_proj_link(cmd);
		build_proj_run(cmd);
	} else if (strcmp(args, "r") == 0) {
		build_proj_run(cmd);
	}
	// else if (strcmp(args, "c") == 0) {
		// build_proj_compile(cmd);
	// }
	else {
		build_log(BUILD_LOG_ERROR, "Wrong argument provided: %s\n%s", args, HELP_MESSAGE);
		return 1;
	}

	return 0;
}

void build_cmd_append(char *cmd, char *src, ...) {
	strcat(cmd, src);
}

void build_cmd_run(char *cmd) {
	build_log(BUILD_LOG_DEBUG, "%s\n", cmd);
	int err = system(cmd);
	if (err) {
		build_log(BUILD_LOG_ERROR, strerror(err), "\n");
		exit(err);
	}
}

void build_cmd_finish(char *cmd) {
	build_cmd_run(cmd);
	memset(cmd, 0, strlen(cmd));
}


int build_file_exists(char *path) {
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributes(path);
    return (dwAttrib != INVALID_FILE_ATTRIBUTES);
#else
    struct stat statbuf = {0};
    if (stat(path, &statbuf) < 0) {
        if (errno == ENOENT) {
            errno = 0;
            return 0;
        }

        build_log(BUILD_LOG_ERROR, "could not retrieve information about file %s: %s", path, strerror(errno));
    }
    return 1;
#endif
}

void build_dir_make(char *path) {
	if (build_file_exists(BUILD_DIR)) return;

#ifndef _WIN32
    int result = mkdir(path,0700);
#else
    int result = mkdir(path);
#endif
    if (result < 0) {
		build_log(BUILD_LOG_ERROR, "Unable to create directory `%s`: %s\n", path, strerror(errno));
        exit(1);
    }
    build_log(BUILD_LOG_INFO, "Created directory: %s\n", path);
}

void build_log(Build_Log_Levels level, char *fmt, ...)
{
	if (level < BUILD_LOG_LEVEL) return;
    va_list args;
    va_start(args, fmt);
    switch (level) {
    	case BUILD_LOG_DEBUG:
    		fprintf(stdout, "[DEBUG] ");
    		vfprintf(stdout, fmt, args);
    		break;
    	case BUILD_LOG_INFO:
    	    fprintf(stdout, "[INFO] ");
    		vfprintf(stdout, fmt, args);
    	    break;
    	case BUILD_LOG_WARNING:
    	    fprintf(stderr, "[WARNING] ");
    		vfprintf(stderr, fmt, args);
    	    break;
    	case BUILD_LOG_ERROR:
    	    fprintf(stderr, "[ERROR] ");
    		vfprintf(stderr, fmt, args);
    	    break;
    	default:
			build_log(BUILD_LOG_ERROR, "Error: Wrong BUILD_LOG_LEVEL enterd in build_log");
			exit(1);
			break;
    }
    va_end(args);
}
