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
    list(APPEND SOUND_SOURCES
        Unix/soundoutput_oss.cpp
        # ALSA support - enabled if ALSA found
        # Unix/soundoutput_alsa.cpp
    )
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
    # OSS fallback
    # find_package(OSS REQUIRED) # may not be available
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