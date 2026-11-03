cmake_minimum_required(VERSION 3.22)
project(Zenith VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

if(DEFINED ENV{CI})
    set(ZENITH_COPY FALSE)   # sur GitHub Actions : pas de copie dans les dossiers systeme
else()
    set(ZENITH_COPY TRUE)
endif()

if(APPLE)
    set(ZENITH_FORMATS VST3 AU Standalone)
else()
    set(ZENITH_FORMATS VST3 Standalone)
endif()

juce_add_plugin(Zenith
    COMPANY_NAME "Foudre Audio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Fdre
    PLUGIN_CODE ZeMb
    FORMATS ${ZENITH_FORMATS}
    PRODUCT_NAME "Zenith"
    COPY_PLUGIN_AFTER_BUILD ${ZENITH_COPY})

target_sources(Zenith PRIVATE
    Source/MasterProcessor.cpp
    Source/MasterEditor.cpp
    Source/UiKit.cpp)

target_compile_definitions(Zenith PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Zenith
    PRIVATE
        juce::juce_audio_utils
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
