# App module CMakeLists.txt

add_library(clan30App SHARED)

if(WIN32)
    set(APP_SOURCES
        Win32/clanapp.cpp
    )
else()
    set(APP_SOURCES
        Unix/clanapp.cpp
    )
endif()

target_sources(clan30App PRIVATE ${APP_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30App PRIVATE -DWIN32 -DUNICODE)
    # MSVC uses library names without -l prefix; MinGW uses -l prefix
    if(MSVC)
        target_link_libraries(clan30App PRIVATE gdi32 user32 kernel32 clan30Core)
    else()
        target_link_libraries(clan30App PRIVATE gdi32 user32 kernel32 clan30Core)
        target_link_options(clan30App PRIVATE -mwindows)
    endif()
else()
    target_compile_definitions(clan30App PRIVATE -DUNIX)
    target_link_libraries(clan30App PRIVATE -lgdi32 -luser32 -lkernel32 clan30Core)
endif()

# Include directories - match original autotools: -I\$(top_srcdir)/Sources
target_include_directories(clan30App PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30App PROPERTIES
    VERSION 3.0.0
)