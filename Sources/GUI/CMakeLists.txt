# GUI module CMakeLists.txt

add_library(clan30GUI SHARED)

# Full source list mirrors Sources/GUI/Makefile.am (autotools build).
# Verified: the on-disk *.cpp set matches the Makefile.am list exactly.
file(GLOB_RECURSE GUI_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(clan30GUI PRIVATE ${GUI_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30GUI PRIVATE -DWIN32 -DUNICODE -DGUI_SYSTEM=WIN32)
    target_link_libraries(clan30GUI PRIVATE clan30Core clan30Display clan30CSSLayout)
else()
    target_compile_definitions(clan30GUI PRIVATE -DHAVE_POSIX -DGUI_SYSTEM=UNIX)
    find_package(X11 REQUIRED)
    find_package(Threads REQUIRED)
    target_link_libraries(clan30GUI PRIVATE
        X11::X11
        Threads::Threads
        clan30Core
        clan30Display
        clan30CSSLayout
    )
endif()

# Include directories
target_include_directories(clan30GUI PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/GUI"
    "${CMAKE_SOURCE_DIR}/Sources/GUI/Layout"
    "${CMAKE_SOURCE_DIR}/Sources/GUI/WindowManager"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30GUI PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
    MSVC_PCH "GUI/precomp.h.gch"
)