cmake_minimum_required(VERSION 3.22)

project(KousatenMixer VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(JUCE)

juce_add_gui_app(KousatenMixer
    PRODUCT_NAME "Kousaten Mixer"
    COMPANY_NAME "MADZINE"
    BUNDLE_ID "com.madzine.kousatenmixer"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "KOUSATEN Mixer needs microphone access for audio input."
)

juce_generate_juce_header(KousatenMixer)

target_sources(KousatenMixer
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
        Source/Core/AudioEngine.cpp
        Source/Core/AudioDeviceHandler.cpp
        Source/Core/RtAudioManager.cpp
        Source/Effects/ChaosGenerator.cpp
        Source/Effects/DelayProcessor.cpp
        Source/Effects/GrainProcessor.cpp
        Source/Effects/ReverbProcessor.cpp
        Source/Mixer/Channel.cpp
        Source/Mixer/MixBus.cpp
        Source/Mixer/AuxBus.cpp
        Source/Mixer/SendPanner.cpp
        Source/Sampler/AudioLayer.cpp
        Source/UI/ChannelStripComponent.cpp
        Source/UI/SendBusComponent.cpp
        Source/UI/AuxOutputComponent.cpp
        Source/UI/SendPannerComponent.cpp
        ThirdParty/RtAudio.cpp
)

# RtAudio include path
target_include_directories(KousatenMixer PRIVATE ThirdParty)

target_compile_definitions(KousatenMixer
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:KousatenMixer,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:KousatenMixer,JUCE_VERSION>"
        # RtAudio: use Core Audio on macOS
        __MACOSX_CORE__
)

target_link_libraries(KousatenMixer
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        # RtAudio dependencies for macOS
        "-framework CoreAudio"
        "-framework CoreFoundation"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
