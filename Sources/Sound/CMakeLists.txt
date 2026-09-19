# Sound module CMakeLists.txt

add_library(clan30Sound SHARED)

set(SOUND_SOURCES
    Mixer/sound_format_conversion.cpp
    soundbuffer_session.cpp
    sound.cpp
    SoundProviders/soundprovider_raw.cpp
    SoundProviders/soundprovider_vorbis.cpp
    SoundProviders/soundprovider_raw_session.cpp
    SoundProviders/soundprovider_factory.cpp
    SoundProviders/soundprovider.cpp
    SoundProviders/soundprovider_session.cpp
    SoundProviders/soundprovider_vorbis_session.cpp
    SoundProviders/soundprovider_type.cpp
    SoundProviders/soundprovider_wave_session.cpp
    SoundProviders/soundprovider_wave.cpp
    Resources/xml_sound_cache.cpp
    Resources/sound_cache.cpp
    setupsound.cpp
    soundoutput_impl.cpp
    soundfilter.cpp
    soundbuffer_impl.cpp
    cd_drive.cpp
    SoundFilters/echofilter.cpp
    SoundFilters/fadefilter_provider.cpp
    SoundFilters/fadefilter.cpp
    SoundFilters/inverse_echofilter.cpp
    SoundFilters/echofilter_provider.cpp
    SoundFilters/inverse_echofilter_provider.cpp
    AudioWorld/audio_object.cpp
    AudioWorld/audio_world.cpp
    AudioWorld/audio_definition.cpp
    soundbuffer_session_impl.cpp
    soundbuffer.cpp
    soundoutput_description.cpp
    sound_sse.cpp
    soundoutput.cpp
)

if(WIN32)
    list(APPEND SOUND_SOURCES
        Win32/soundoutput_directsound.cpp
        Win32/init_guids.cpp
        Win32/soundoutput_win32.cpp
    )
else()
    # ALSA backend: always built on non-Windows (tried first at runtime,
    # see soundoutput.cpp). ALSA itself is required below.
    list(APPEND SOUND_SOURCES
        Unix/soundoutput_alsa.cpp
    )
    # OSS backend: only a fallback, and only when the kernel OSS headers
    # exist. Modern distros (e.g. Ubuntu) ship none of these, and /dev/dsp
    # is obsolete - mirrors autotools AC_CHECK_HEADERS([machine/soundcard.h
    # sys/soundcard.h soundcard.h]).
    include(CheckIncludeFile)
    check_include_file("sys/soundcard.h" HAVE_SYS_SOUNDCARD_H)
    check_include_file("soundcard.h" HAVE_SOUNDCARD_H)
    check_include_file("machine/soundcard.h" HAVE_MACHINE_SOUNDCARD_H)
    if(HAVE_SYS_SOUNDCARD_H OR HAVE_SOUNDCARD_H OR HAVE_MACHINE_SOUNDCARD_H)
        list(APPEND SOUND_SOURCES Unix/soundoutput_oss.cpp)
    else()
        message(STATUS "OSS soundcard.h not found - clanSound will use ALSA only")
    endif()
endif()

target_sources(clan30Sound PRIVATE ${SOUND_SOURCES})

# Platform-specific defines and flags
if(WIN32)
    target_compile_definitions(clan30Sound PRIVATE -DWIN32 -DUNICODE)
    if(MSVC)
        # MSVC: use library names without -l prefix
        target_link_libraries(clan30Sound PRIVATE
            dsound
            dxguid
            winmm
            clan30Core
        )
    else()
        # MinGW / Linux
        target_compile_definitions(clan30Sound PRIVATE -DHAVE_POSIX)
        # ALSA audio
        find_package(ALSA REQUIRED)
        target_link_libraries(clan30Sound PRIVATE
            ALSA::ALSA
            clan30Core
        )
        # OSS fallback
        # find_package(OSS REQUIRED) # may not be available
    endif()
else()
    target_compile_definitions(clan30Sound PRIVATE -DHAVE_POSIX)
    # ALSA audio
    find_package(ALSA REQUIRED)
    target_link_libraries(clan30Sound PRIVATE
        ALSA::ALSA
        clan30Core
    )
    # Use <alsa/asoundlib.h> (Debian/Ubuntu layout) in soundoutput_alsa.h.
    target_compile_definitions(clan30Sound PRIVATE -DHAVE_ALSA_ASOUNDLIB_H)
    # Enable the OSS fallback in soundoutput.cpp only when its headers were
    # found above (HAVE_OSS_BACKEND guards the SoundOutput_OSS references).
    if(HAVE_SYS_SOUNDCARD_H OR HAVE_SOUNDCARD_H OR HAVE_MACHINE_SOUNDCARD_H)
        target_compile_definitions(clan30Sound PRIVATE -DHAVE_OSS_BACKEND)
        foreach(_oss HAVE_SYS_SOUNDCARD_H HAVE_SOUNDCARD_H HAVE_MACHINE_SOUNDCARD_H)
            if(${_oss})
                target_compile_definitions(clan30Sound PRIVATE -D${_oss})
            endif()
        endforeach()
    endif()
endif()

# Include directories
target_include_directories(clan30Sound PRIVATE
    "${CMAKE_SOURCE_DIR}/Sources/API"
    "${CMAKE_SOURCE_DIR}/Sources/Sound"
    "${CMAKE_SOURCE_DIR}/Sources"
)

# Version information
set_target_properties(clan30Sound PROPERTIES
    VERSION 3.0.0
    SOVERSION 3.0
)