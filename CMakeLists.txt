cmake_minimum_required (VERSION 3.22)

project (NoTracDSP VERSION 0.1.0 LANGUAGES CXX)

set (CMAKE_CXX_STANDARD 20)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory (JUCE)

juce_add_gui_app (NoTracDSP
    PRODUCT_NAME "NoTrac DSP Prototype"
    COMPANY_NAME "Open Source"
    BUNDLE_ID "com.opensource.NoTracDSP")

target_sources (NoTracDSP
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/AudioEngine.cpp
        Source/DelayProcessor.cpp
        Source/PluginScannerCoordinator.cpp
        Source/PluginListBox.cpp
        )

target_include_directories (NoTracDSP
    PRIVATE
        Source)

target_compile_definitions (NoTracDSP
    PUBLIC
        JUCE_ASIO=1
    PRIVATE
        SCANNER_WORKER_PATH="$<TARGET_FILE:PluginScannerWorkerMain>"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NoTracDSP,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:NoTracDSP,JUCE_VERSION>"
        $<$<CONFIG:Debug>:JUCE_DEBUG=1>
        $<$<CONFIG:Debug>:JUCE_CHECK_MEMORY_LEAKS=1>)

target_compile_features (NoTracDSP PRIVATE cxx_std_20)

target_link_libraries (NoTracDSP
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_gui_extra
        juce::juce_audio_processors 
        juce::juce_events
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

juce_add_gui_app(PluginScannerWorkerMain
    # This prevents it from needing a console window and expects WinMain
    IS_BACKGROUND_PROCESS TRUE 
)

target_sources(PluginScannerWorkerMain PRIVATE
    Source/PluginScannerWorkerMain.cpp
    # Only include files the worker specifically needs to compile
)

target_compile_definitions(PluginScannerWorkerMain PUBLIC 
    JUCE_DEBUG=1
    JUCE_PLUGINHOST_VST3=1
)

target_link_libraries(PluginScannerWorkerMain
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_gui_extra
        juce::juce_audio_processors 
        juce::juce_events
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
