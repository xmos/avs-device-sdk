add_subdirectory("Authorization")

cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(VALID FALSE)

if (PORTAUDIO AND (GSTREAMER_MEDIA_PLAYER OR CUSTOM_MEDIA_PLAYER))
    set(VALID TRUE)
elseif (ANDROID_MEDIA_PLAYER AND ANDROID_MICROPHONE)
    set(VALID TRUE)
elseif (AUDIO_INJECTION AND (GSTREAMER_MEDIA_PLAYER OR ANDROID_MEDIA_PLAYER OR CUSTOM_MEDIA_PLAYER))
    set(VALID TRUE)
endif()

if (VALID)
    project(SampleApp LANGUAGES CXX)
    include("${AVS_CORE}/build/BuildDefaults.cmake")

    add_subdirectory("src")
else()
    message("To build the Sample App, please enable microphone and media player modules.")
endif()