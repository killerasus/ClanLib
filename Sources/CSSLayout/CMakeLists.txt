# CSSLayout module CMakeLists.txt

add_library(clan30CSSLayout SHARED)

# Full source list mirrors Sources/CSSLayout/Makefile.am (autotools build),
# plus HTML/html_parser.cpp (present on disk, newer than the Makefile.am).
# Verified: disk set == Makefile.am set + that one file.
file(GLOB_RECURSE CSSTLAYOUT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

target_sources(clan30CSSLayout PRIVATE ${CSSTLAYOUT_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30CSSLayout PRIVATE -DWIN32 -DUNICODE)
    target_link_libraries(clan30CSSLayout PRIVATE clan30Core clan30Display)
else()
    target_compile_definitions(clan30CSSLayout PRIVATE -DHAVE_POSIX)
    target_link_libraries(clan30CSSLayout PRIVATE clan30Core clan30Display)
endif()

# Include directories
target_include_directories(clan30CSSLayout PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/CSSLayout"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30CSSLayout PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)