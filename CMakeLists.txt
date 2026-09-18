cmake_minimum_required(VERSION 3.15)
project(ClanLib VERSION 3.0.0 LANGUAGES C CXX)

# Platform detection
if(WIN32 OR MSVC)
    set(CLANLIB_PLATFORM "Windows")
else()
    set(CLANLIB_PLATFORM "Linux")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Module options - default ON (build all modules)
set(CLANLIB_BUILD_APP OFF)
set(CLANLIB_BUILD_CORE ON)
set(CLANLIB_BUILD_DISPLAY OFF)
set(CLANLIB_BUILD_GL OFF)
set(CLANLIB_BUILD_SOUND OFF)
set(CLANLIB_BUILD_DATABASE OFF)
set(CLANLIB_BUILD_SQLITE OFF)
set(CLANLIB_BUILD_NETWORK OFF)
set(CLANLIB_BUILD_GUI OFF)
set(CLANLIB_BUILD_CSSLAYOUT OFF)
set(CLANLIB_BUILD_SCENE3D OFF)
set(CLANLIB_BUILD_PHYSICS3D OFF)
set(CLANLIB_BUILD_PHYSICS2D OFF)
set(CLANLIB_BUILD_GAMEIDE OFF)
set(CLANLIB_BUILD_SWRENDER OFF)
set(CLANLIB_BUILD_COMPUTE OFF)
set(CLANLIB_BUILD_D3D OFF)

# Parse arguments to enable modules
foreach(arg ${CMAKE_ARGV})
    string(REGEX REMOVE "^--" opt ${arg})
    string(REGEX REMOVE "=.*" opt_value ${arg})
    
    if(opt STREQUAL "CLANLIB_BUILD_ALL")
        # Enable all modules
        set(CLANLIB_BUILD_APP ON)
        set(CLANLIB_BUILD_CORE ON)
        set(CLANLIB_BUILD_DISPLAY ON)
        set(CLANLIB_BUILD_GL ON)
        set(CLANLIB_BUILD_SOUND ON)
        set(CLANLIB_BUILD_DATABASE ON)
        set(CLANLIB_BUILD_SQLITE ON)
        set(CLANLIB_BUILD_NETWORK ON)
        set(CLANLIB_BUILD_GUI ON)
        set(CLANLIB_BUILD_CSSLAYOUT ON)
        set(CLANLIB_BUILD_SCENE3D ON)
        set(CLANLIB_BUILD_PHYSICS3D ON)
        set(CLANLIB_BUILD_PHYSICS2D ON)
        set(CLANLIB_BUILD_GAMEIDE ON)
        set(CLANLIB_BUILD_SWRENDER ON)
        set(CLANLIB_BUILD_COMPUTE ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_CORE")
        set(CLANLIB_BUILD_CORE ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_DISPLAY")
        set(CLANLIB_BUILD_DISPLAY ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_GL")
        set(CLANLIB_BUILD_GL ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_SOUND")
        set(CLANLIB_BUILD_SOUND ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_DATABASE")
        set(CLANLIB_BUILD_DATABASE ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_SQLITE")
        set(CLANLIB_BUILD_SQLITE ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_NETWORK")
        set(CLANLIB_BUILD_NETWORK ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_GUI")
        set(CLANLIB_BUILD_GUI ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_CSSLAYOUT")
        set(CLANLIB_BUILD_CSSLAYOUT ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_SCENE3D")
        set(CLANLIB_BUILD_SCENE3D ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_PHYSICS3D")
        set(CLANLIB_BUILD_PHYSICS3D ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_PHYSICS2D")
        set(CLANLIB_BUILD_PHYSICS2D ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_GAMEIDE")
        set(CLANLIB_BUILD_GAMEIDE ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_SWRENDER")
        set(CLANLIB_BUILD_SWRENDER ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_COMPUTE")
        set(CLANLIB_BUILD_COMPUTE ON)
    elseif(opt STREQUAL "CLANLIB_BUILD_APP")
        set(CLANLIB_BUILD_APP ON)
    elseif(opt MATCHES "CLANLIB_BUILD_=(.+)")
        set_module(${CMAKE_MATCH_1})
        set(CLANLIB_BUILD_${CMAKE_MATCH_1} ON)
    endif()
endforeach()

# If CLANLIB_BUILD_ALL is set, enable all modules
if(CLANLIB_BUILD_ALL)
    set(CLANLIB_BUILD_APP ON)
    set(CLANLIB_BUILD_CORE ON)
    set(CLANLIB_BUILD_DISPLAY ON)
    set(CLANLIB_BUILD_GL ON)
    set(CLANLIB_BUILD_SOUND ON)
    set(CLANLIB_BUILD_DATABASE ON)
    set(CLANLIB_BUILD_SQLITE ON)
    set(CLANLIB_BUILD_NETWORK ON)
    set(CLANLIB_BUILD_GUI ON)
    set(CLANLIB_BUILD_CSSLAYOUT ON)
    set(CLANLIB_BUILD_SCENE3D ON)
    set(CLANLIB_BUILD_PHYSICS3D ON)
    set(CLANLIB_BUILD_PHYSICS2D ON)
    set(CLANLIB_BUILD_GAMEIDE ON)
    set(CLANLIB_BUILD_SWRENDER ON)
    set(CLANLIB_BUILD_COMPUTE ON)
    # D3D is Windows-only (requires d3d11.h); Scene3D needs it on Windows.
    if(WIN32)
        set(CLANLIB_BUILD_D3D ON)
    endif()
endif()

# Core is always on
mark_as_advanced(CLANLIB_BUILD_CORE)
set(CORE_ALWAYS_ON TRUE)

# Module dependency handling
# GUI depends on CSSLayout and Display
if(NOT CLANLIB_BUILD_DISPLAY)
    set(CLANLIB_BUILD_GUI OFF)
    set(CLANLIB_BUILD_CSSLAYOUT OFF)
    set(CLANLIB_BUILD_SCENE3D OFF)
    set(CLANLIB_BUILD_PHYSICS3D OFF)
    set(CLANLIB_BUILD_PHYSICS2D OFF)
    set(CLANLIB_BUILD_GAMEIDE OFF)
    set(CLANLIB_BUILD_SWRENDER OFF)
endif()

if(NOT CLANLIB_BUILD_GUI)
    set(CLANLIB_BUILD_CSSLAYOUT OFF)
endif()

if(CLANLIB_BUILD_CSSLAYOUT AND NOT CLANLIB_BUILD_DISPLAY)
    set(CLANLIB_BUILD_CSSLAYOUT OFF)
endif()

if(CLANLIB_BUILD_GL)
    set(CLANLIB_BUILD_COMPUTE ON CACHE BOOL "clanCompute depends on clanGL" FORCE)
endif()

# Set the list of modules to build
set(CLANLIB_MODULES "")
if(CLANLIB_BUILD_APP)
	list(APPEND CLANLIB_MODULES "App")
endif()
if(CLANLIB_BUILD_CORE)
	list(APPEND CLANLIB_MODULES "Core")
endif()
if(CLANLIB_BUILD_DISPLAY)
	list(APPEND CLANLIB_MODULES "Display")
endif()
if(CLANLIB_BUILD_D3D AND WIN32)
	list(APPEND CLANLIB_MODULES "D3D")
endif()
if(CLANLIB_BUILD_GL)
	list(APPEND CLANLIB_MODULES "GL")
endif()
if(CLANLIB_BUILD_SOUND)
	list(APPEND CLANLIB_MODULES "Sound")
endif()
if(CLANLIB_BUILD_DATABASE)
	list(APPEND CLANLIB_MODULES "Database")
endif()
if(CLANLIB_BUILD_SQLITE)
	list(APPEND CLANLIB_MODULES "Sqlite")
endif()
if(CLANLIB_BUILD_NETWORK)
	list(APPEND CLANLIB_MODULES "Network")
endif()
if(CLANLIB_BUILD_GUI)
	list(APPEND CLANLIB_MODULES "GUI")
endif()
if(CLANLIB_BUILD_CSSLAYOUT)
	list(APPEND CLANLIB_MODULES "CSSLayout")
endif()
if(CLANLIB_BUILD_SCENE3D)
	list(APPEND CLANLIB_MODULES "Scene3D")
endif()
if(CLANLIB_BUILD_PHYSICS3D)
	list(APPEND CLANLIB_MODULES "Physics3D")
endif()
if(CLANLIB_BUILD_PHYSICS2D)
	list(APPEND CLANLIB_MODULES "Physics2D")
endif()
if(CLANLIB_BUILD_GAMEIDE)
	list(APPEND CLANLIB_MODULES "GameIDE")
endif()
if(CLANLIB_BUILD_SWRENDER)
	list(APPEND CLANLIB_MODULES "SWRender")
endif()
if(CLANLIB_BUILD_COMPUTE)
	list(APPEND CLANLIB_MODULES "Compute")
endif()

# Create output directories
message(STATUS "ClanLib modules to build: ${CLANLIB_MODULES}")

# Set up pkg-config paths
set(CLANLIB_PKG_CONFIG_DIR "${CMAKE_CURRENT_BINARY_DIR}/pkgconfig")
file(MAKE_DIRECTORY ${CLANLIB_PKG_CONFIG_DIR})

# Configure pkg-config template
set(CLANLIB_VERSION_MAJOR 3)
set(CLANLIB_VERSION_MINOR 0)
set(CLANLIB_VERSION_PATCH 0)

# Generate pkg-config files for each module
# ... will be generated per-module

# Windows DLL mode: Sources/API/*/api_*.h selects dllimport/dllexport via
# CL_API_DLL, exporting only when the matching <MODULE>_EXPORT macro is
# defined while building that module itself (dllexport), with consumers
# seeing dllimport. dllimport is required for cross-DLL *data* imports
# (e.g. HalfFloat tables); plain/empty views cannot link data (LNK2019).
# Template classes that are fully defined in headers (Rectx, ComPtr,
# signals) carry no CL_API macro at all, since MSVC rejects dllimport
# member definitions (C2491) and they need no exporting.
if(WIN32)
    add_compile_definitions(CL_API_DLL)
endif()

# MSVC: neutralize legacy autotools `#pragma comment(lib, "clan*-*.lib")`
# auto-link directives in Sources/API/*.h. MSVC auto-defines _DLL with /MD
# (CMake default), which makes those headers request e.g.
# clanCore-static-mtdll.lib -- a file the CMake build never produces
# (LNK1104). Our targets link explicitly via target_link_libraries, so
# ignore every legacy auto-link library name here.
# NOTE: this MUST come before add_subdirectory below: add_link_options only
# affects targets created after the call.
if(MSVC)
    set(CLANLIB_LEGACY_AUTOLINK_LIBS
        Core App Display GL Sound Database Sqlite Network GUI CSSLayout
        Scene3D Physics3D Physics2D GameIDE SWRender Compute D3D MySQL GL1)
    foreach(_clanlib ${CLANLIB_LEGACY_AUTOLINK_LIBS})
        foreach(_variant
            "-dll.lib" "-static-mtdll.lib" "-static-mt.lib"
            "-dll-debug.lib" "-static-mtdll-debug.lib" "-static-mt-debug.lib")
            add_link_options(/NODEFAULTLIB:"clan${_clanlib}${_variant}")
        endforeach()
    endforeach()
    # gui.h also auto-links clanCSSLayout under the same variants (covered
    # above via CSSLayout) and the "CSSLayout" prefix variant:
    foreach(_variant
        "-dll.lib" "-static-mtdll.lib" "-static-mt.lib"
        "-dll-debug.lib" "-static-mtdll-debug.lib" "-static-mt-debug.lib")
        add_link_options(/NODEFAULTLIB:"clanCSSLayout${_variant}")
    endforeach()
endif()

# Add subdirectories for each module
foreach(module ${CLANLIB_MODULES})
    if(EXISTS "${CMAKE_SOURCE_DIR}/Sources/${module}/CMakeLists.txt")
        add_subdirectory("${CMAKE_SOURCE_DIR}/Sources/${module}")
    else()
        message(WARNING "CMakeLists.txt not found for module: ${module}")
    endif()
endforeach()

# Per-module export macros for the Windows DLL build: the module being
# built sees its own classes as dllexport (and therefore produces an
# import library for dependents); everyone else sees dllimport.
if(WIN32)
    set(CLANLIB_EXPORT_MAP
        "App:APP_EXPORT"
        "Core:CORE_EXPORT"
        "Display:DISPLAY_EXPORT"
        "D3D:D3D_EXPORT"
        "GL:GL_EXPORT"
        "Sound:SOUND_EXPORT"
        "Database:DATABASE_EXPORT"
        "Sqlite:SQLITE_EXPORT"
        "Network:NETWORK_EXPORT"
        "GUI:GUI_EXPORT"
        "CSSLayout:CSSLAYOUT_EXPORT"
        "Scene3D:SCENE_EXPORT"
        "Physics3D:SCENE_EXPORT"
        "Physics2D:PHYSIC_EXPORT"
        "SWRender:SWRender_EXPORT"
        "Compute:CL_COMPUTE_EXPORT"
    )
    foreach(pair ${CLANLIB_EXPORT_MAP})
        string(REPLACE ":" ";" pair_list "${pair}")
        list(GET pair_list 0 _mod)
        list(GET pair_list 1 _def)
        if(TARGET clan30${_mod})
            target_compile_definitions(clan30${_mod} PRIVATE ${_def})
        endif()
    endforeach()
endif()



# Setup documentation option
option(CLANLIB_BUILD_DOCS "Build API documentation" OFF)

# Install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
    set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")
endif()

# Export compile commands for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
