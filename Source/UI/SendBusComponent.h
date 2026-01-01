/*
    Kousaten Mixer - Send Bus Component
    Single send bus strip with return level and parameters
*/

#pragma once

#include <JuceHeader.h>
#include "../Mixer/MixBus.h"

namespace Kousaten {

class SendBusComponent : public juce::Component,
                          public juce::Slider::Listener,
                          public juce::Timer
{
public:
    SendBusComponent(MixBus* bus, const juce::String& name);
    ~SendBusComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void timerCallback() override;

private:
    // Colors
    const juce::Colour backgroundDark { 0xff0e0c0c };
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour backgroundLight { 0xff2a2424 };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour accentDim { 0xffc08090 };
    const juce::Colour textLight { 0xffffffff };
    const juce::Colour textDim { 0xffc8b8b8 };

    MixBus* bus;
    juce::String busName;

    juce::Slider returnSlider;

    // Effect-specific parameters
    juce::Slider param1Slider;
    juce::Slider param2Slider;
    juce::Slider param3Slider;

    float currentLevel = 0.0f;

    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendBusComponent)
};

class SendBusSectionComponent : public juce::Component
{
public:
    SendBusSectionComponent(MixBus* delayBus, MixBus* grainBus, MixBus* reverbBus);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour textLight { 0xffffffff };

    SendBusComponent delayComponent;
    SendBusComponent grainComponent;
    SendBusComponent reverbComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendBusSectionComponent)
};

} // namespace Kousaten
