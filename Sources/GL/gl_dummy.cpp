// Minimal OpenGL wrapper for ClanLib CMake build
// This file is needed to satisfy CMake's requirement for source files
// The actual OpenGL functionality is provided by the Display module and user code

#include <GL/gl.h>

namespace clan
{
    // Empty namespace - OpenGL functions are loaded dynamically
}