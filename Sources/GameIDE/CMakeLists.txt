# GameIDE module CMakeLists.txt

add_library(clan30GameIDE SHARED)

set(GAMEIDE_SOURCES
    precomp.cpp
)

target_sources(clan30GameIDE PRIVATE ${GAMEIDE_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30GameIDE PRIVATE -DWIN32 -DUNICODE)
    target_link_libraries(clan30GameIDE PRIVATE clan30Core clan30Display)
else()
    target_compile_definitions(clan30GameIDE PRIVATE -DHAVE_POSIX)
    target_link_libraries(clan30GameIDE PRIVATE Threads::Threads)
endif()

# Include directories
target_include_directories(clan30GameIDE PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/GameIDE"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30GameIDE PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)