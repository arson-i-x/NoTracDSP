cmake_minimum_required (VERSION 3.22)

project (NoTracDSP VERSION 0.1.0 LANGUAGES C CXX)

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
        # this is the main entry point for the application
        Source/Main.cpp

        # core source files
        Source/core/AppController.cpp
        Source/core/IOEngine.cpp        
        Source/core/DirectoryManager.cpp
        Source/core/PresetManager.cpp
        Source/core/PluginWindow.cpp

        # command source files
        Source/commands/AddPluginCommand.cpp
        Source/commands/RemovePluginCommand.cpp
        Source/commands/BypassPluginCommand.cpp
        Source/commands/SetPluginOrderCommand.cpp
        Source/commands/ChangePresetCommand.cpp

        # custom processor source files
        Source/customProcessors/PolyphonicOctaver.cpp

        # ui source files
        Source/ui/main/PluginGraphViewComponent.cpp
        Source/ui/main/SettingsOverlayComponent.cpp
        Source/ui/main/MainComponent.cpp
        Source/ui/main/PresetManagerComponent.cpp

        Source/ui/pluginList/PluginListBoxComponent.cpp
        Source/ui/pluginList/PluginListWindow.cpp

        # scanner source files
        Source/scanner/PluginScannerCoordinator.cpp
        )

target_include_directories (NoTracDSP 
    PRIVATE 
        Source 
        ${CMAKE_CURRENT_BINARY_DIR}/juce_binarydata_NoTracDSPBinaryData)

juce_add_binary_data(NoTracDSPBinaryData
    SOURCES
        Assets/undo.png
        Assets/redo.png
        Assets/bypass.png
        Assets/settings.png
        Assets/save.png
)

target_compile_definitions (NoTracDSP
    PUBLIC
        JUCE_ASIO=1
    PRIVATE
        SCANNER_WORKER_PATH="$<TARGET_FILE:PluginScannerWorkerMain>"
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_MODAL_LOOPS_PERMITTED=1
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:NoTracDSP,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:NoTracDSP,JUCE_VERSION>"
        $<$<CONFIG:Debug>:JUCE_DEBUG=1>
        $<$<CONFIG:Debug>:JUCE_CHECK_MEMORY_LEAKS=1>
)

target_compile_features (NoTracDSP PRIVATE cxx_std_20)

target_link_libraries (NoTracDSP
    PRIVATE
        juce::juce_core
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_gui_extra
        juce::juce_audio_processors 
        juce::juce_events
        NoTracDSPBinaryData
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

juce_add_gui_app(PluginScannerWorkerMain
    # This prevents it from needing a console window and expects WinMain
    IS_BACKGROUND_PROCESS TRUE 
)

target_sources(PluginScannerWorkerMain PRIVATE
    Source/scanner/PluginScannerWorkerMain.cpp
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
