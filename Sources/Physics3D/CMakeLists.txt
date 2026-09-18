# Physics3D module CMakeLists.txt

add_library(clan30Physics3D SHARED)

set(PHYSICS3D_SOURCES
    physics3d_world.cpp
    physics3d_world_impl.h
    physics3d_shape.cpp
    physics3d_shape_impl.h
    physics3d_ray_test.cpp
    physics3d_ray_test_impl.h
    physics3d_object.cpp
    physics3d_object_impl.h
    physics3d_sweep_test.cpp
    physics3d_sweep_test_impl.h
    physics3d_contact_test.cpp
    physics3d_contact_test_impl.h
    physics3d_contact_pair_test.cpp
    physics3d_contact_pair_test_impl.h
    precomp.cpp
)

# Bundled Bullet physics library. Verified: the on-disk Bullet/*.cpp set is
# exactly the set listed in Sources/Physics3D/Makefile.am (127 files).
file(GLOB_RECURSE BULLET_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Bullet/*.cpp"
)

target_sources(clan30Physics3D PRIVATE ${PHYSICS3D_SOURCES} ${BULLET_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Physics3D PRIVATE -DWIN32 -DUNICODE -DPHYSICS3D_SYSTEM=WIN32)
    # MSVC has no X11; use the system OpenGL library name (valid for both
    # MSVC and MinGW) instead of the imported OpenGL::OpenGL target.
    target_link_libraries(clan30Physics3D PRIVATE opengl32 clan30Core)
else()
    target_compile_definitions(clan30Physics3D PRIVATE -DHAVE_POSIX -DPHYSICS3D_SYSTEM=LINUX)
    find_package(OpenGL REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(clan30Physics3D PRIVATE
        OpenGL::OpenGL
        X11::X11
        clan30Core
    )
endif()

# Include directories
target_include_directories(clan30Physics3D PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Physics3D"
    "${CMAKE_SOURCE_DIR}/Sources/Physics3D/Bullet"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Physics3D PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)