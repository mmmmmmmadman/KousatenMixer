/*
    Kousaten Mixer - Send Bus Component
    Implementation
*/

#include "SendBusComponent.h"
#include "ChannelStripComponent.h"  // For MinimalSliderLookAndFeel

namespace Kousaten {

// =============================================================================
// SendBusComponent
// =============================================================================

SendBusComponent::SendBusComponent(MixBus* b, const juce::String& name)
    : bus(b), busName(name)
{
    setSize(60, 280);  // Width for full name display

    // Return level slider
    returnSlider.setSliderStyle(juce::Slider::LinearVertical);
    returnSlider.setRange(0.0, 100.0, 0.1);
    returnSlider.setValue(100.0);
    returnSlider.setDoubleClickReturnValue(true, 100.0);  // Double-click to reset
    returnSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    returnSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    returnSlider.setColour(juce::Slider::trackColourId, accentDim);
    returnSlider.setColour(juce::Slider::thumbColourId, accent);
    returnSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    returnSlider.addListener(this);
    addAndMakeVisible(returnSlider);

    // Parameter sliders
    auto setupParamSlider = [this](juce::Slider& slider) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setRange(0.0, 100.0, 0.1);
        slider.setValue(50.0);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour(juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, backgroundLight);
        slider.setColour(juce::Slider::thumbColourId, accent);
        slider.addListener(this);
        addAndMakeVisible(slider);
    };

    setupParamSlider(param1Slider);
    setupParamSlider(param2Slider);
    setupParamSlider(param3Slider);

    startTimerHz(30);
}

SendBusComponent::~SendBusComponent()
{
    stopTimer();
}

void SendBusComponent::paint(juce::Graphics& g)
{
    // Background
    g.setColour(backgroundLight);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

    // Bus name - horizontal, full name
    g.setColour(accent);
    g.setFont(14.0f);
    g.drawText(busName, 0, 6, getWidth(), 20, juce::Justification::centred);

    // Return value at bottom
    g.setColour(accent);
    g.setFont(14.0f);
    g.drawText(juce::String(static_cast<int>(returnSlider.getValue())),
               0, getHeight() - 24, getWidth(), 20, juce::Justification::centred);

    // Draw meter (thin, on left side)
    drawMeter(g, 6, 28, 10, getHeight() - 55, currentLevel);
}

void SendBusComponent::drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level)
{
    g.setColour(backgroundMid);
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(width), static_cast<float>(height), 2.0f);

    int fillHeight = static_cast<int>(level * height);
    if (fillHeight > 0)
    {
        juce::ColourGradient gradient(accent, static_cast<float>(x), static_cast<float>(y + height),
                                       juce::Colour(0xffff4444), static_cast<float>(x), static_cast<float>(y),
                                       false);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(static_cast<float>(x + 1),
                               static_cast<float>(y + height - fillHeight),
                               static_cast<float>(width - 2),
                               static_cast<float>(fillHeight), 1.0f);
    }
}

void SendBusComponent::resized()
{
    // Return fader - positioned after name label
    returnSlider.setBounds(20, 28, 28, getHeight() - 55);

    // Hide parameter knobs in compact mode
    param1Slider.setVisible(false);
    param2Slider.setVisible(false);
    param3Slider.setVisible(false);
}

void SendBusComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &returnSlider)
    {
        bus->setReturnLevel(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == &param1Slider)
    {
        float value = static_cast<float>(slider->getValue() / 100.0);
        switch (bus->getType())
        {
            case BusType::Delay:
                bus->setDelayTime(value * 2.0f, value * 2.0f);
                break;
            case BusType::Grain:
                bus->setGrainSize(value);
                break;
            case BusType::Reverb:
                bus->setReverbRoomSize(value);
                break;
        }
    }
    else if (slider == &param2Slider)
    {
        float value = static_cast<float>(slider->getValue() / 100.0);
        switch (bus->getType())
        {
            case BusType::Delay:
                bus->setDelayFeedback(value * 0.95f);
                break;
            case BusType::Grain:
                bus->setGrainDensity(value);
                break;
            case BusType::Reverb:
                bus->setReverbDecay(value);
                break;
        }
    }

    repaint();
}

void SendBusComponent::timerCallback()
{
    float newLevel = bus->getOutputLevel();
    if (std::abs(newLevel - currentLevel) > 0.01f)
    {
        currentLevel = newLevel;
        repaint();
    }
}

// =============================================================================
// SendBusSectionComponent
// =============================================================================

SendBusSectionComponent::SendBusSectionComponent(MixBus* delayBus, MixBus* grainBus, MixBus* reverbBus)
    : delayComponent(delayBus, "Delay")
    , grainComponent(grainBus, "Grain")
    , reverbComponent(reverbBus, "Reverb")
{
    addAndMakeVisible(delayComponent);
    addAndMakeVisible(grainComponent);
    addAndMakeVisible(reverbComponent);

    setSize(200, 320);  // Wider for full names
}

void SendBusSectionComponent::paint(juce::Graphics& g)
{
    g.setColour(backgroundMid);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    g.setColour(textLight);
    g.setFont(16.0f);
    g.drawText("Send Returns", 0, 10, getWidth(), 22, juce::Justification::centred);
}

void SendBusSectionComponent::resized()
{
    int busWidth = 60;
    int spacing = 5;
    int startX = 8;
    int y = 32;
    int busHeight = getHeight() - y - 10;

    delayComponent.setBounds(startX, y, busWidth, busHeight);
    grainComponent.setBounds(startX + busWidth + spacing, y, busWidth, busHeight);
    reverbComponent.setBounds(startX + (busWidth + spacing) * 2, y, busWidth, busHeight);
}

} // namespace Kousaten
