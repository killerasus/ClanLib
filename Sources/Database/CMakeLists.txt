# Database module CMakeLists.txt

add_library(clan30Database SHARED)

set(DATABASE_SOURCES
    db_connection.cpp
    db_connection_impl.h
    db_command.cpp
    db_command_impl.h
    db_reader.cpp
    db_reader_impl.h
    db_transaction.cpp
    db_transaction_impl.h
    db_value.cpp
    precomp.cpp
)

target_sources(clan30Database PRIVATE ${DATABASE_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Database PRIVATE -DWIN32 -DUNICODE)
    target_link_libraries(clan30Database PRIVATE clan30Core)
else()
    target_compile_definitions(clan30Database PRIVATE -DHAVE_POSIX)
    find_package(Threads REQUIRED)
    target_link_libraries(clan30Database PRIVATE Threads::Threads)
endif()

# Include directories
target_include_directories(clan30Database PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Database"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Database PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)