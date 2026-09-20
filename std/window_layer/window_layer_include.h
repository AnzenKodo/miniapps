#ifndef WINDOW_LAYER_INCLUDE_H
#define WINDOW_LAYER_INCLUDE_H

#include "window_layer_core.h"

#if OS_LINUX
#   include "window_layer_x11.h"
#elif OS_WINDOWS
#   include "window_layer_win32.h"
#else
#   error OS window layer not implemented for this operating system.
#endif


#endif // WINDOW_LAYER_CORE_H
