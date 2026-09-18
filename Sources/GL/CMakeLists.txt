# GL module CMakeLists.txt

add_library(clan30GL SHARED)

# Full source list mirrors Sources/GL/Makefile.am (autotools build).
set(GL_SOURCES
    setup_gl.cpp
    State/opengl_blend_state.cpp
    State/opengl_depth_stencil_state.cpp
    State/opengl_rasterizer_state.cpp
    GL3/gl3_pixel_buffer_provider.cpp
    GL3/gl3_frame_buffer_provider.cpp
    GL3/gl3_occlusion_query_provider.cpp
    GL3/gl3_standard_programs.cpp
    GL3/gl3_vertex_array_buffer_provider.cpp
    GL3/gl3_element_array_buffer_provider.cpp
    GL3/gl3_buffer_object_provider.cpp
    GL3/gl3_storage_buffer_provider.cpp
    GL3/gl3_graphic_context_provider.cpp
    GL3/gl3_uniform_buffer_provider.cpp
    GL3/gl3_render_buffer_provider.cpp
    GL3/gl3_texture_provider.cpp
    GL3/gl3_transfer_buffer_provider.cpp
    GL3/gl3_primitives_array_provider.cpp
    GL3/gl3_program_object_provider.cpp
    GL3/gl3_shader_object_provider.cpp
    opengl.cpp
    opengl_target.cpp
    precomp.cpp
    GL1/pbuffer.cpp
    GL1/gl1_frame_buffer_provider.cpp
    GL1/gl1_vertex_array_buffer_provider.cpp
    GL1/gl1_transfer_buffer_provider.cpp
    GL1/gl1_render_buffer_provider.cpp
    GL1/gl1_uniform_buffer_provider.cpp
    GL1/gl1_program_object_provider.cpp
    GL1/gl1_graphic_context_provider.cpp
    GL1/gl1_texture_provider.cpp
    GL1/gl1_primitives_array_provider.cpp
    opengl_target_provider.cpp
    opengl_graphic_context.cpp
    opengl_window_description.cpp
)

if(WIN32)
    list(APPEND GL_SOURCES
        WGL/opengl_creation_helper.cpp
        WGL/opengl_window_provider_wgl.cpp
        WGL/pbuffer_impl.cpp
    )
else()
    list(APPEND GL_SOURCES
        GLX/opengl_window_provider_glx.cpp
        GLX/pbuffer_impl.cpp
    )
endif()

target_sources(clan30GL PRIVATE ${GL_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    # NOTE: no -DNOMINMAX/-DWIN32_LEAN_AND_MEAN here: GL/precomp.h already
    # defines them, and command-line duplicates cause C4005 warnings.
    target_compile_definitions(clan30GL PRIVATE -DWIN32 -DUNICODE)
    # Plain library names work for both MSVC and MinGW (-l prefix breaks MSVC: LNK4044)
    target_link_libraries(clan30GL PRIVATE opengl32 gdi32 user32 clan30Core clan30Display)
else()
    target_compile_definitions(clan30GL PRIVATE -DHAVE_POSIX -DGL_GLEXT_PROTOTYPES)
    # OpenGL is typically available on Linux without extra linking
    # But we may need GL, GLU, X11, Xrandr, Xfixes, etc.
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(clan30GL PRIVATE
        OpenGL::OpenGL
        X11::X11
        clan30Core
        clan30Display
    )
endif()

# Include directories
target_include_directories(clan30GL PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/GL"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30GL PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)