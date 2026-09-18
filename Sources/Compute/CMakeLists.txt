# Compute module CMakeLists.txt

add_library(clan30Compute SHARED)

# Full source list mirrors Sources/Compute/Makefile.am (autotools build).
# Verified: the on-disk *.cpp set matches the Makefile.am list exactly.
file(GLOB_RECURSE COMPUTE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(clan30Compute PRIVATE ${COMPUTE_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Compute PRIVATE -DWIN32 -DUNICODE)
    # opengl32: raw glFlush/wglGetCurrentContext/wglGetCurrentDC calls.
    target_link_libraries(clan30Compute PRIVATE opengl32 clan30Core clan30Display clan30GL)
    # ComputeBuffer shares GPU buffers with D3D providers on Windows.
    if(TARGET clan30D3D)
        target_link_libraries(clan30Compute PRIVATE clan30D3D)
    endif()
else()
    target_compile_definitions(clan30Compute PRIVATE -DHAVE_POSIX)
endif()

# Include directories
target_include_directories(clan30Compute PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Compute"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Compute PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)