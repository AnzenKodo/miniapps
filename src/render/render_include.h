#ifndef RENDER_INCLUDE_H
#define RENDER_INCLUDE_H

// ak: Backend Constants
//=============================================================================

#define RENDER_BACKEND_STUB   0
#define RENDER_BACKEND_OPENGL 1

// ak: Decide On Backend
//=============================================================================

#ifndef RENDER_BACKEND
#   if OS_LINUX
#        define RENDER_BACKEND RENDER_BACKEND_OPENGL
#   elif OS_WINDOWS
#        define RENDER_BACKEND RENDER_BACKEND_OPENGL
#   endif
#endif


// ak: Main Includes
//=============================================================================

#include "render_core.h"

// ak: Backend Includes
//=============================================================================

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#   if OS_LINUX
#       include "render_egl.h"
#   elif OS_WINDOWS
#       include "render_wgl.h"
#   else
#       error no OpenGL render layer for this platform
#   endif
#   include "render_opengl.h"
#else
#   error no render layer for this platform
#endif

#endif // RENDER_INCLUDE_H
