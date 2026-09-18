// Minimal OpenGL wrapper for ClanLib CMake build
// This file is needed to satisfy CMake's requirement for source files
// The actual OpenGL functionality is provided by the Display module and user code

#ifdef WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif
#include <GL/gl.h>

namespace clan
{
    // Empty namespace - OpenGL functions are loaded dynamically
}