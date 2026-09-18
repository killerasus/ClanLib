# D3D module CMakeLists.txt (Windows-only: requires d3d11.h)
#
# Note: upstream has no Makefile.am for D3D (it was built only by the
# Windows configure.exe generator). All on-disk *.cpp files are built;
# Shaders/ contains embedded HLSL headers only.
# Scene3D requires this module on Windows (GPUTimer uses D3DTarget).

add_library(clan30D3D SHARED)

file(GLOB_RECURSE D3D_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(clan30D3D PRIVATE ${D3D_SOURCES})

target_compile_definitions(clan30D3D PRIVATE -DWIN32 -DUNICODE)

# Plain library names work for both MSVC and MinGW (-l prefix breaks MSVC)
target_link_libraries(clan30D3D PRIVATE
    d3d11
    d3dcompiler
    dxgi
    clan30Core
    clan30Display
)

# Include directories
target_include_directories(clan30D3D PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/D3D"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30D3D PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)
