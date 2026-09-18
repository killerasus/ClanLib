# Sqlite module CMakeLists.txt

add_library(clan30Sqlite SHARED)

set(SQLITE_SOURCES
    sqlite_connection.cpp
    sqlite_connection_provider.h
    sqlite_connection_provider.cpp
    sqlite_command_provider.h
    sqlite_command_provider.cpp
    sqlite_reader_provider.h
    sqlite_reader_provider.cpp
    sqlite_transaction_provider.h
    sqlite_transaction_provider.cpp
    precomp.cpp
)

target_sources(clan30Sqlite PRIVATE ${SQLITE_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Sqlite PRIVATE -DWIN32 -DUNICODE -DSQLITE_OS_WIN=1)
    target_link_libraries(clan30Sqlite PRIVATE clan30Core clan30Database)
    # Windows: build the bundled sqlite3 amalgamation (see precomp.h, which
    # includes "Amalgamation/sqlite3.h" on WIN32). Do NOT use find_package
    # here: an MSYS2 sqlite3 would inject MinGW headers into the MSVC build.
    target_sources(clan30Sqlite PRIVATE Amalgamation/sqlite3.c)
else()
    target_compile_definitions(clan30Sqlite PRIVATE -DHAVE_POSIX -DSQLITE_THREADSAFE=1)
    find_package(Threads REQUIRED)
    target_link_libraries(clan30Sqlite PRIVATE Threads::Threads clan30Core clan30Database)
    # Link sqlite3 library
    find_library(SQLITE3_LIBRARY sqlite3)
    if(SQLITE3_LIBRARY)
        target_link_libraries(clan30Sqlite PRIVATE ${SQLITE3_LIBRARY})
    endif()
endif()

# Include directories
target_include_directories(clan30Sqlite PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Sqlite"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Sqlite PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)