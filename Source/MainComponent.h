/*
    Kousaten Mixer - Main Component
    Main GUI component with mixer interface
*/

#pragma once

#include <JuceHeader.h>
#include "Core/AudioEngine.h"
#include "Core/AudioDeviceHandler.h"
#include "UI/ChannelStripComponent.h"
#include "UI/AuxOutputComponent.h"
#include <vector>
#include <memory>

class MainComponent : public juce::Component,
                      public juce::Timer,
                      public juce::AudioIODeviceCallback
{
public:
    MainComponent();
    ~MainComponent() override;

    // AudioIODeviceCallback - single callback for both input and output
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceStopped() override;

    // Component
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Timer
    void timerCallback() override;

private:
    // Colors (Techno Machine style)
    const juce::Colour backgroundDark { 0xff0e0c0c };
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour backgroundLight { 0xff2a2424 };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour accentDim { 0xffc08090 };
    const juce::Colour textLight { 0xffffffff };
    const juce::Colour textDim { 0xffc8b8b8 };

    juce::AudioDeviceManager audioDeviceManager;
    Kousaten::AudioEngine audioEngine;
    Kousaten::AudioDeviceHandler deviceHandler;

    // UI Components
    juce::TextButton addChannelButton { "+ Add Channel" };
    juce::Viewport channelViewport;
    juce::Component channelContainer;

    std::vector<std::unique_ptr<Kousaten::ChannelStripComponent>> channelStrips;
    std::unique_ptr<Kousaten::AuxOutputSectionComponent> auxOutputSection;  // Unified Send Returns + Aux Outputs

    // Master section
    juce::Slider masterVolumeSlider;
    juce::ComboBox masterDeviceCombo;
    juce::ComboBox masterChannelCombo;

    // Effect parameters (in master bar) - 3 params each
    juce::Slider delayTimeLSlider;       // Time L
    juce::Slider delayTimeRSlider;       // Time R
    juce::Slider delayFeedbackSlider;
    juce::Slider grainSizeSlider;
    juce::Slider grainDensitySlider;
    juce::Slider grainPositionSlider;    // Grain position
    juce::Slider reverbRoomSlider;
    juce::Slider reverbDampingSlider;    // Damping
    juce::Slider reverbDecaySlider;

    // Chaos controls
    juce::ToggleButton chaosEnableButton { "" };
    juce::Slider chaosAmountSlider;
    juce::Slider chaosRateSlider;
    juce::ToggleButton chaosShapeButton { "" };

    void addChannel();
    void removeChannel(int channelId);
    void updateLayout();
    void drawMasterSection(juce::Graphics& g);
    void syncAllChannelAuxSends();
    void updateMasterChannelOptions();
    void updateChaosAmount();

    // Audio buffers
    juce::AudioBuffer<float> inputBuffer;
    juce::AudioBuffer<float> outputBuffer;
    std::atomic<float> inputLevel { 0.0f };  // Debug: input level

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
