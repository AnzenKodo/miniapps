#include "render_core.c"

#if RENDER_BACKEND == RENDER_BACKEND_OPENGL
#   include "render_opengl.c"
#   if OS_LINUX
#       include "render_egl.c"
#   elif OS_WINDOWS
#       include "render_wgl.c"
#   else
#       error no OpenGL render layer for this platform
#   endif
#elif RENDER_BACKEND == RENDER_BACKEND_VULKAN
#   include "render_vulkan.c"
#elif RENDER_BACKEND == RENDER_BACKEND_WEBGPU
#   include "render_webgpu.c"
#elif RENDER_BACKEND == RENDER_BACKEND_SOKOL
#   include "render_sokol.c"
#else
#   error no render layer for this platform
#endif
