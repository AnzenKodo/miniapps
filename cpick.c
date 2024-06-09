#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>

#define PROJECT_NAME "cpick"
#define PROJECT_VERSION "Version: 0.1"

const char *help_message = "Timer and Stopwatch writtern in C.\n\n"
"Usage:\n"
"   "PROJECT_NAME" [OPTIONS]\n"
"\n"
"   For Hex color: Left Click\n"
"   For RGB color: Right Click\n"
"Options:\n"
"   -n --no-fullscreen      Don't start fullscreen\n"
"   -h --help               Print help\n"
"   -v --version            Print version\n\n"
PROJECT_VERSION
" | SPDX-License-Identifier: MIT (https://spdx.org/licenses/MIT)\n";

typedef enum {
    Mouse_Button0,
    Mouse_Button1,
    Mouse_Button2,
    Mouse_Button3,
    Mouse_Button4,
    Mouse_Button5,
} Mouse_Buttons;

typedef struct {
    Mouse_Buttons button;
    int x;
    int y;
} Mouse_State;
Mouse_State Mouse;

typedef struct {
    Display *display;
    Window window;
    Window root_window;
    Atom delete_window;
    GC gc;
    Cursor cursor;
    int screen_width;
    int screen_height;
} Window_State;

typedef XImage Image;
typedef XColor Color;

static int WindowShouldClose = 0;
static int FullScreen = 1;

void cli_init(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf(help_message);
            exit(1);
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf(PROJECT_VERSION);
            puts("");
            exit(1);
        } else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--no-fullscreen") == 0) {
            FullScreen = 0;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: wrong argument provided `%s`\n\n%s", argv[i], help_message);
            exit(1);
        }
    }
}

void init_window(
    Window_State *state, int window_width, int window_height, const char *desc
) {
    state->display = XOpenDisplay(NULL);
    if (!state->display) {
        fprintf(stderr, "Can't open display\n");
        exit(1);
    }

    int screen = DefaultScreen(state->display);

    state->root_window = RootWindow(state->display, screen);
    state->window = XCreateSimpleWindow(
        state->display, state->root_window,
        0, 0,
        window_width, window_height,
        0, 0,
        BlackPixel(state->display, screen)
    );

    if(!state->window) {
        fprintf(stderr, "Window wasn't created properly.\n");
        exit(1);
    }

    XSelectInput(
        state->display, state->window,
        ExposureMask | KeyPressMask | ButtonPressMask
    );
    XStoreName(state->display, state->window, desc);
    XMapWindow(state->display, state->window);
    XFlush(state->display);
    XSync(state->display, 1);

    state->delete_window = XInternAtom(state->display, "WM_DELETE_WINDOW", 0);
    if (
        !XSetWMProtocols(state->display, state->window, &state->delete_window, 1)
    ) {
        fprintf(stderr, "Couldn't register WM_DELETE_WIDNOW property.\n");
        exit(1);
    }

    state->gc = XCreateGC(state->display, state->window, 0, 0);
    XSetForeground(state->display, state->gc, WhitePixel(state->display, screen));

    state->screen_width = DisplayWidth(state->display, screen);
    state->screen_height = DisplayHeight(state->display, screen);
}

void fullscreen_window(Window_State *state) {
    Atom wm_state = XInternAtom (state->display, "_NET_WM_STATE", 1);
    Atom wm_fullscreen = XInternAtom (state->display, "_NET_WM_STATE_FULLSCREEN", 1);

    XChangeProperty(
        state->display, state->window,
        wm_state, XA_ATOM, 32,
        PropModeReplace,
        (unsigned char *)&wm_fullscreen, 1
    );
}

void set_plus_cursor(Window_State *state) {
    state->cursor = XCreateFontCursor(state->display, XC_crosshair);
    XDefineCursor(state->display, state->window, state->cursor);
}

Image *get_screen_img(Window_State *state) {
    return XGetImage(
        state->display, state->root_window,
        0, 0,
        state->screen_width, state->screen_height,
        AllPlanes, ZPixmap
    );
}

void put_img(
    Window_State *state, Image *img,
    int x, int y, int width, int height
) {
    XPutImage(
        state->display, state->window,
        state->gc, img,
        0, 0,
        x, y,
        width, height
    );
}

void free_image(Image *img) {
    XDestroyImage(img);
}

void begin_drawing(Window_State *state) {
    XEvent event = {};

    XNextEvent(state->display, &event);
    switch (event.type) {
        case ButtonPress: {
            switch(event.xbutton.button) {
                case Button1: {
                    Mouse.button = Mouse_Button1;
                    Mouse.x = event.xbutton.x;
                    Mouse.y = event.xbutton.y;
                    break;
                }
                case Button2: {
                    Mouse.button = Mouse_Button2;
                    Mouse.x = event.xbutton.x;
                    Mouse.y = event.xbutton.y;
                    break;
                }
                case Button3: {
                    Mouse.button = Mouse_Button3;
                    Mouse.x = event.xbutton.x;
                    Mouse.y = event.xbutton.y;
                    break;
                }
                case Button4: {
                    Mouse.button = Mouse_Button4;
                    Mouse.x = event.xbutton.x;
                    Mouse.y = event.xbutton.y;
                    break;
                }
                case Button5: {
                    Mouse.button = Mouse_Button5;
                    Mouse.x = event.xbutton.x;
                    Mouse.y = event.xbutton.y;
                    break;
                }
            }

            break;
        }
        case MotionNotify: {
            Mouse.button = Mouse_Button0;
            Mouse.x = event.xmotion.x;
            Mouse.y = event.xmotion.y;
            break;
        }
        case KeyPress: {
            switch(event.xkey.keycode) {
                case 0x09: WindowShouldClose = 1;
            }

            return;
        }
        case DestroyNotify: {
            XDestroyWindowEvent* e = (XDestroyWindowEvent*) &event;
            if (e->window == state->window) WindowShouldClose = 1;

            return;
        }
        case ClientMessage: {
            XClientMessageEvent* e = (XClientMessageEvent*) &event;
            if ((Atom)e->data.l[0] == state->delete_window) WindowShouldClose = 1;

            return;
        }
    }
}

void close_window(Window_State *state) {
    XFreeGC(state->display, state->gc);
    XDestroyWindow(state->display, state->window);
    XCloseDisplay(state->display);
}

Color get_pixel_from_img(Window_State *state, Image *img, int x, int y) {
    Color color;
    color.pixel = XGetPixel(img, x, y);

    XQueryColor(
        state->display, XDefaultColormap(state->display,
         XDefaultScreen (state->display)), &color
    );

    color.red = color.red/256;
    color.green = color.red/256;
    color.blue = color.red/256;

    return color;
}

void print_hex_from_img(Window_State *state, Image *img, int x, int y) {
    Color color = get_pixel_from_img(state, img, x, y);
    printf("#%02X%02X%02X\n", color.red, color.green, color.blue);
}

void print_rgb_from_img(Window_State *state, Image *img, int x, int y) {
    Color color = get_pixel_from_img(state, img, x, y);
    printf("rgb(%d %d %d)\n", color.red, color.green, color.blue);
}

int main(int argc, char *argv[]) {
    cli_init(argc, argv);

    Window_State state = {0};

    init_window(&state, 800, 600, "cpick");

    if (FullScreen) {
        fullscreen_window(&state);
    }

    Image *img = get_screen_img(&state);
    set_plus_cursor(&state);

    while(!WindowShouldClose) {
        begin_drawing(&state);

        if (Mouse.button == Mouse_Button1) {
            WindowShouldClose = 1;
            print_hex_from_img(&state, img, Mouse.x, Mouse.y);
        }

        if (Mouse.button == Mouse_Button3) {
            WindowShouldClose = 1;
            print_rgb_from_img(&state, img, Mouse.x, Mouse.y);
        }

        put_img(&state, img, 0, 0, state.screen_width, state.screen_height);
    }

    free_image(img);
    close_window(&state);
    return 0;
}
