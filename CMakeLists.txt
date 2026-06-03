cmake_minimum_required (VERSION 3.22)

project (QuadCorePrototype VERSION 0.1.0 LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory (JUCE)

juce_add_gui_app (QuadCorePrototype
    PRODUCT_NAME "QuadCore Prototype"
    COMPANY_NAME "Open Source"
    BUNDLE_ID "com.opensource.quadcoreprototype")

target_sources (QuadCorePrototype
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/AudioEngine.cpp
        Source/GainProcessor.cpp)

target_include_directories (QuadCorePrototype
    PRIVATE
        Source)

target_compile_definitions (QuadCorePrototype
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:QuadCorePrototype,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:QuadCorePrototype,JUCE_VERSION>")

target_compile_features (QuadCorePrototype PRIVATE cxx_std_20)

target_link_libraries (QuadCorePrototype
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)