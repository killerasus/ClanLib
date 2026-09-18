# Physics2D module CMakeLists.txt

add_library(clan30Physics2D SHARED)

set(PHYSICS2D_SOURCES

    precomp.cpp
)

target_sources(clan30Physics2D PRIVATE ${PHYSICS2D_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Physics2D PRIVATE -DWIN32 -DUNICODE -DPHYSICS2D_SYSTEM=WIN32)
    target_link_libraries(clan30Physics2D PRIVATE clan30Core)
else()
    target_compile_definitions(clan30Physics2D PRIVATE -DHAVE_POSIX -DPHYSICS2D_SYSTEM=LINUX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(clan30Physics2D PRIVATE
        OpenGL::OpenGL
        X11::X11
    )
endif()

# Include directories
target_include_directories(clan30Physics2D PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Physics2D"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Physics2D PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)