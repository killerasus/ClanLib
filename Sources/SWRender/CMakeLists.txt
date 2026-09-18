# SWRender module CMakeLists.txt

add_library(clan30SWRender SHARED)

set(SWRENDER_SOURCES
    precomp.cpp
)

target_sources(clan30SWRender PRIVATE ${SWRENDER_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30SWRender PRIVATE -DWIN32 -DUNICODE)
    target_link_libraries(clan30SWRender PRIVATE clan30Core clan30Display)
else()
    target_compile_definitions(clan30SWRender PRIVATE -DHAVE_POSIX)
endif()

# Include directories
target_include_directories(clan30SWRender PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/SWRender"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30SWRender PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)