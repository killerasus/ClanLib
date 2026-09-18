# Scene3D module CMakeLists.txt

add_library(clan30Scene3D SHARED)

# Full source list mirrors Sources/Scene3D/Makefile.am (autotools build).
# Verified: the on-disk *.cpp set matches the Makefile.am list exactly.
file(GLOB_RECURSE SCENE3D_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(clan30Scene3D PRIVATE ${SCENE3D_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Scene3D PRIVATE -DWIN32 -DUNICODE -DSCENE3D_SYSTEM=WIN32)
    target_link_libraries(clan30Scene3D PRIVATE opengl32 clan30Core clan30Display clan30GL)
    # GPUTimer uses D3DTarget on Windows - link the D3D module when built.
    if(TARGET clan30D3D)
        target_link_libraries(clan30Scene3D PRIVATE clan30D3D)
    endif()
else()
    target_compile_definitions(clan30Scene3D PRIVATE -DHAVE_POSIX -DSCENE3D_SYSTEM=LINUX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(clan30Scene3D PRIVATE
        OpenGL::OpenGL
        X11::X11
    )
endif()

# Include directories
target_include_directories(clan30Scene3D PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Scene3D"
    "${CMAKE_SOURCE_DIR}/Sources/Scene3D/Model"
    "${CMAKE_SOURCE_DIR}/Sources/Scene3D/Passes"
    "${CMAKE_SOURCE_DIR}/Sources/Scene3D/Culling"
    "${CMAKE_SOURCE_DIR}/Sources/Scene3D/Level"
    "${CMAKE_SOURCE_DIR}/Sources/Scene3D/Framework"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Scene3D PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)