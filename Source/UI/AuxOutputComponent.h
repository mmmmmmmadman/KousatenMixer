/*
    Kousaten Mixer - Aux Output Component
    Unified horizontal row format for Send Returns and Aux Outputs
*/

#pragma once

#include <JuceHeader.h>
#include "../Mixer/AuxBus.h"
#include "../Mixer/MixBus.h"
#include "../Core/AudioDeviceHandler.h"

namespace Kousaten {

// =============================================================================
// SendReturnRowComponent - Horizontal row for built-in effects (Delay/Grain/Reverb)
// No device selection, just name + meter + level
// =============================================================================
class SendReturnRowComponent : public juce::Component,
                                public juce::Slider::Listener,
                                public juce::Timer
{
public:
    SendReturnRowComponent(MixBus* bus, const juce::String& name);
    ~SendReturnRowComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;

    MixBus* getMixBus() { return mixBus; }

private:
    const juce::Colour backgroundDark { 0xff0e0c0c };
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour backgroundLight { 0xff2a2424 };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour accentDim { 0xffc08090 };
    const juce::Colour textLight { 0xffffffff };
    const juce::Colour textDim { 0xffc8b8b8 };

    MixBus* mixBus;
    juce::String busName;

    juce::Slider levelSlider;

    float currentLevel = 0.0f;

    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendReturnRowComponent)
};

// =============================================================================
// AuxOutputComponent - Horizontal row for dynamic aux outputs with device selection
// =============================================================================
class AuxOutputComponent : public juce::Component,
                            public juce::Slider::Listener,
                            public juce::ComboBox::Listener,
                            public juce::Button::Listener,
                            public juce::Timer
{
public:
    AuxOutputComponent(AuxBus* bus, AudioDeviceHandler* deviceHandler);
    ~AuxOutputComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    AuxBus* getAuxBus() { return auxBus; }

    std::function<void(int)> onRemoveAux;
    std::function<void()> onNameChanged;

private:
    const juce::Colour backgroundDark { 0xff0e0c0c };
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour backgroundLight { 0xff2a2424 };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour accentDim { 0xffc08090 };
    const juce::Colour textLight { 0xffffffff };
    const juce::Colour textDim { 0xffc8b8b8 };

    AuxBus* auxBus;
    AudioDeviceHandler* deviceHandler;

    juce::TextEditor nameEditor;
    juce::ComboBox deviceCombo;
    juce::ComboBox channelCombo;
    juce::Slider levelSlider;
    juce::TextButton removeButton { "X" };

    float currentLevel = 0.0f;

    void updateChannelOptions();
    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxOutputComponent)
};

// =============================================================================
// UnifiedOutputSectionComponent - Combined Send Returns + Aux Outputs
// =============================================================================
class UnifiedOutputSectionComponent : public juce::Component
{
public:
    UnifiedOutputSectionComponent(class AudioEngine* engine, AudioDeviceHandler* deviceHandler);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void addAuxOutput();
    void removeAuxOutput(int auxId);
    void updateLayout();

    std::function<void()> onAuxAdded;
    std::function<void(int)> onAuxRemoved;
    std::function<void()> onAuxNameChanged;

private:
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour textLight { 0xffffffff };

    AudioEngine* audioEngine;
    AudioDeviceHandler* deviceHandler;

    // Fixed Send Returns (Delay, Grain, Reverb)
    std::unique_ptr<SendReturnRowComponent> delayReturn;
    std::unique_ptr<SendReturnRowComponent> grainReturn;
    std::unique_ptr<SendReturnRowComponent> reverbReturn;

    // Dynamic Aux Outputs
    std::vector<std::unique_ptr<AuxOutputComponent>> auxComponents;
    juce::TextButton addButton { "+ Add Aux Out" };
    juce::Viewport auxViewport;
    juce::Component auxContainer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UnifiedOutputSectionComponent)
};

// Backwards compatibility alias
using AuxOutputSectionComponent = UnifiedOutputSectionComponent;

} // namespace Kousaten
