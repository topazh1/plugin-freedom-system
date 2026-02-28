cmake_minimum_required(VERSION 3.22)

juce_add_plugin(NBS_DynaDrive
    COMPANY_NAME "NBS"
    PLUGIN_MANUFACTURER_CODE Nbss
    PLUGIN_CODE DyDr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "NBS DynaDrive"
    BUNDLE_ID "com.nbs.dynadrive"
    PLUGIN_VERSION "1.3.0"
    NEEDS_WEB_BROWSER TRUE
)

target_sources(NBS_DynaDrive
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(NBS_DynaDrive
    PRIVATE
        Source
)

target_link_libraries(NBS_DynaDrive
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# CRITICAL: Generate JuceHeader.h — must come after target_link_libraries (JUCE 8 requirement)
juce_generate_juce_header(NBS_DynaDrive)

# WebView UI Resources — bundles HTML/JS into plugin binary
juce_add_binary_data(NBS_DynaDrive_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

target_link_libraries(NBS_DynaDrive
    PRIVATE
        NBS_DynaDrive_UIResources
)

target_compile_definitions(NBS_DynaDrive
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
)
