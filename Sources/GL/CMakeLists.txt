# GL module CMakeLists.txt

add_library(clan30GL SHARED)

# Minimal source file for OpenGL wrapper
target_sources(clan30GL PRIVATE
    gl_dummy.cpp
)

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30GL PRIVATE -DWIN32 -DUNICODE -DOPENGL32)
    target_link_libraries(clan30GL PRIVATE -lopengl32)
else()
    target_compile_definitions(clan30GL PRIVATE -DHAVE_POSIX -DGL_GLEXT_PROTOTYPES)
    # OpenGL is typically available on Linux without extra linking
    # But we may need GL, GLU, X11, Xrandr, Xfixes, etc.
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(clan30GL PRIVATE
        OpenGL::OpenGL
        X11::X11
    )
endif()

# Include directories
target_include_directories(clan30GL PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/GL"
)

# Version information
set_target_properties(clan30GL PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)