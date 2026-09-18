# Network module CMakeLists.txt

add_library(clan30Network SHARED)

set(NETWORK_SOURCES
    precomp.cpp
    NetGame/client.cpp
    NetGame/connection.cpp
    NetGame/connection_impl.cpp
    NetGame/event.cpp
    NetGame/event_value.cpp
    NetGame/network_data.cpp
    NetGame/server.cpp
    Web/http_request_handler.cpp
    Web/http_request_handler_impl.cpp
    Web/http_server_connection.cpp
    Web/http_server_connection_impl.cpp
    Web/http_server.cpp
    Web/http_server_impl.cpp
    Web/ring_buffer.cpp
    Web/web_request.cpp
    Web/web_response.cpp
    TLS/tls_connection.cpp
    TLS/iodevice_provider_tls_connection.cpp
    Socket/dns_packet.cpp
    Socket/dns_resolver.cpp
    Socket/dns_resolver_impl.cpp
    Socket/dns_resource_record.cpp
    Socket/iodevice_provider_tcp_connection.cpp
    Socket/socket_error.cpp
    Socket/socket_name.cpp
    Socket/tcp_connection.cpp
    Socket/tcp_listen.cpp
    Socket/tcp_listen_impl.cpp
    Socket/udp_socket.cpp
    Socket/udp_socket_impl.cpp
)

if(WIN32)
    list(APPEND NETWORK_SOURCES
        Socket/event_provider_win32socket.cpp
        Socket/win32_socket.cpp
        setupnetwork_win32.cpp
    )
else()
    list(APPEND NETWORK_SOURCES
        Socket/unix_socket.cpp
        setupnetwork_unix.cpp
    )
endif()

target_sources(clan30Network PRIVATE ${NETWORK_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Network PRIVATE -DWIN32 -DUNICODE)
    if(MSVC)
        # MSVC: use library names without -l prefix (dns_resolver.cpp needs
        # GetNetworkParams from iphlpapi)
        target_link_libraries(clan30Network PRIVATE WS2_32 iphlpapi clan30Core)
    else()
        # MinGW / Linux
        target_compile_definitions(clan30Network PRIVATE -DHAVE_POSIX)
        # pthread for Linux
        find_package(Threads REQUIRED)
        target_link_libraries(clan30Network PRIVATE Threads::Threads)
        target_link_libraries(clan30Network PRIVATE -lws2_32 -liphlpapi clan30Core)
    endif()
else()
    target_compile_definitions(clan30Network PRIVATE -DHAVE_POSIX)
    # pthread for Linux
    find_package(Threads REQUIRED)
    target_link_libraries(clan30Network PRIVATE Threads::Threads)
    target_link_libraries(clan30Network PRIVATE clan30Core)
endif()

# Include directories
target_include_directories(clan30Network PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Network"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Network PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)