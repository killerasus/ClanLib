# Core module CMakeLists.txt

add_library(clan30Core SHARED)

set(CORE_SOURCES
    Text/console_logger.cpp
    Text/utf8_reader.cpp
    Text/string_format.cpp
    Text/file_logger.cpp
    Text/logger.cpp
    Text/console.cpp
    Text/string_help.cpp
    Resources/xml_resource_node.cpp
    Resources/xml_resource_manager.cpp
    Resources/xml_resource_document.cpp
    Resources/resource_manager.cpp
    ErrorReporting/crash_reporter.cpp
    ErrorReporting/detect_hang.cpp
    ErrorReporting/exception_dialog.cpp
    Crypto/random.cpp
    Crypto/aes128_decrypt.cpp
    Crypto/x509_impl.cpp
    Crypto/aes192_encrypt.cpp
    Crypto/aes192_encrypt_impl.cpp
    Crypto/asn1.cpp
    Crypto/sha512_256.cpp
    Crypto/sha.cpp
    Crypto/sha1_impl.cpp
    Crypto/aes192_encrypt_impl.cpp
    Crypto/md5_impl.cpp
    Crypto/secret.cpp
    Crypto/sha256.cpp
    Crypto/aes128_encrypt.cpp
    Crypto/sha512_224.cpp
    Crypto/sha1.cpp
    Crypto/sha224.cpp
    Crypto/aes256_encrypt.cpp
    Crypto/aes256_decrypt_impl.cpp
    Crypto/sha512_impl.cpp
    Crypto/aes256_decrypt.cpp
    Crypto/x509.cpp
    precomp.cpp
    IOData/iodevice_provider_memory.cpp
    IOData/iodevice_memory.cpp
    IOData/file_system_provider_zip.cpp
    IOData/pipe_listen_impl.cpp
    IOData/pipe_listen.cpp
    IOData/file_help.cpp
    IOData/html_url.cpp
    IOData/file_system.cpp
    IOData/directory.cpp
    IOData/file_system_provider_file.cpp
    IOData/directory_listing_entry.cpp
    IOData/iodevice_provider_file.cpp
    IOData/endianess.cpp
    IOData/directory_scanner.cpp
    IOData/pipe_connection.cpp
    IOData/iodevice.cpp
    IOData/file.cpp
    IOData/iodevice_provider_pipe_connection.cpp
    JSON/json_value.cpp
    System/datetime.cpp
    System/runnable.cpp
    System/setup_core.cpp
    System/service_impl.cpp
    System/event_impl.cpp
    System/thread.cpp
    System/game_time.cpp
    System/mutex.cpp
    System/keep_alive.cpp
    System/timer.cpp
    System/console_window_generic.cpp
    System/exception.cpp
    System/databuffer.cpp
    System/system.cpp
    System/block_allocator.cpp
    System/registry_key.cpp
    System/console_window.cpp
    System/disposable_object.cpp
    System/event.cpp
    System/thread_local_storage.cpp
    System/detect_cpu_ext.cpp
    System/service.cpp
    System/thread_local_storage_impl.cpp
    System/work_queue.cpp
)

if(WIN32)
    list(APPEND CORE_SOURCES
        System/Win32/service_win32.cpp
        System/Win32/init_win32.cpp
        System/Win32/event_provider_win32.cpp
        System/Win32/thread_win32.cpp
        IOData/Win32/directory_scanner_win32.cpp
    )
else()
    list(APPEND CORE_SOURCES
        IOData/Unix/directory_scanner_unix.cpp
        System/Unix/init_linux.cpp
        System/Unix/service_unix.cpp
        System/Unix/event_provider_socketpair.cpp
        System/Unix/thread_unix.cpp
    )
endif()

target_sources(clan30Core PRIVATE ${CORE_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Core PRIVATE -DWIN32 -DUNICODE)
    target_link_libraries(clan30Core PRIVATE
        -lz
        -lmsvcrt
        -lshell32
        -lshlwapi
        -limagehlp
        -lmingwex
    )
else()
    # Linux - pthread
    find_package(Threads REQUIRED)
    target_link_libraries(clan30Core PRIVATE Threads::Threads)
endif()

# Include directories - match original autotools: -I\$(top_srcdir)/Sources
target_include_directories(clan30Core PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Core PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
    PUBLIC_HEADER "${CMAKE_SOURCE_DIR}/Sources/API/core.h"
)

# Export variables for pkg-config
set(CLANLIB_CORE_VERSION "3.0.0")
set(CLANLIB_CORE_RELEASE "@CLANLIB_RELEASE@")