/*
    Kousaten Mixer - Channel Strip Component
    Single channel strip with fader, pan, sends, mute/solo
*/

#pragma once

#include <JuceHeader.h>
#include "../Mixer/Channel.h"
#include "../Core/AudioDeviceHandler.h"
#include "SendPannerComponent.h"
#include <map>

namespace Kousaten {

// Custom LookAndFeel for minimal slider thumb (2px white line)
class MinimalSliderLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float minSliderPos, float maxSliderPos,
                          const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto trackColour = slider.findColour(juce::Slider::trackColourId);
        auto backgroundColour = slider.findColour(juce::Slider::backgroundColourId);

        if (style == juce::Slider::LinearVertical)
        {
            // Draw track background
            g.setColour(backgroundColour);
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                                   static_cast<float>(width), static_cast<float>(height), 2.0f);

            // Draw filled portion
            float fillHeight = static_cast<float>(height) - sliderPos + static_cast<float>(y);
            if (fillHeight > 0)
            {
                g.setColour(trackColour);
                g.fillRoundedRectangle(static_cast<float>(x), sliderPos,
                                       static_cast<float>(width), fillHeight, 2.0f);
            }

            // Draw thumb as 2px horizontal white line
            g.setColour(juce::Colours::white);
            g.fillRect(static_cast<float>(x), sliderPos - 1.0f,
                       static_cast<float>(width), 2.0f);
        }
        else // Horizontal
        {
            // Draw track background
            g.setColour(backgroundColour);
            g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                                   static_cast<float>(width), static_cast<float>(height), 2.0f);

            // Draw filled portion
            float fillWidth = sliderPos - static_cast<float>(x);
            if (fillWidth > 0)
            {
                g.setColour(trackColour);
                g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                                       fillWidth, static_cast<float>(height), 2.0f);
            }

            // Draw thumb as 2px vertical white line
            g.setColour(juce::Colours::white);
            g.fillRect(sliderPos - 1.0f, static_cast<float>(y),
                       2.0f, static_cast<float>(height));
        }
    }
};

// Global instance for shared use
inline MinimalSliderLookAndFeel& getMinimalSliderLookAndFeel()
{
    static MinimalSliderLookAndFeel instance;
    return instance;
}

// Forward declaration
class AudioEngine;

class ChannelStripComponent : public juce::Component,
                               public juce::Slider::Listener,
                               public juce::Button::Listener,
                               public juce::ComboBox::Listener,
                               public juce::Timer
{
public:
    ChannelStripComponent(Channel* channel, AudioDeviceHandler* deviceHandler = nullptr, AudioEngine* engine = nullptr);
    ~ChannelStripComponent() override;

    void setDeviceHandler(AudioDeviceHandler* handler);
    void setAudioEngine(AudioEngine* engine);
    void updateDeviceLists();

    // Aux send management
    void addAuxSend(int auxId, const juce::String& auxName);
    void removeAuxSend(int auxId);
    void syncAuxSends();  // Sync with AudioEngine's aux buses

    void paint(juce::Graphics& g) override;
    void paintOverChildren(juce::Graphics& g) override;
    void resized() override;

    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;
    void timerCallback() override;

    Channel* getChannel() { return channel; }

    // Callbacks
    std::function<void(int)> onRemoveChannel;
    std::function<void(int)> onAddAuxRequested;  // Request to add new aux bus

private:
    // Colors
    const juce::Colour backgroundDark { 0xff0e0c0c };
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour backgroundLight { 0xff2a2424 };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour accentDim { 0xffc08090 };
    const juce::Colour textLight { 0xffffffff };
    const juce::Colour textDim { 0xffc8b8b8 };
    const juce::Colour muteColor { 0xffcc4444 };
    const juce::Colour soloColor { 0xffcccc44 };

    Channel* channel;
    AudioDeviceHandler* deviceHandler = nullptr;
    AudioEngine* audioEngine = nullptr;

    juce::TextEditor nameEditor;

    // Input I/O selection
    juce::ComboBox inputDeviceCombo;
    juce::ComboBox inputChannelCombo;

    // Fixed effect sends
    juce::Slider delaySendSlider;
    juce::Slider grainSendSlider;
    juce::Slider reverbSendSlider;

    // Dynamic aux sends with scrollable container
    struct AuxSendControl {
        int auxId;
        juce::String name;
        std::unique_ptr<juce::Slider> slider;
    };
    std::vector<AuxSendControl> auxSendControls;
    juce::Viewport auxViewport;
    juce::Component auxContainer;

    // Add send button
    juce::TextButton addSendButton { "+ Add Send" };

    // Send Panner
    std::unique_ptr<SendPannerComponent> sendPannerComponent;

    void updateAuxLayout();
    void updateSendPannerAuxPositions();

    // Pan
    juce::Slider panSlider;

    // Volume fader
    juce::Slider volumeSlider;

    // Mute / Solo
    juce::TextButton muteButton { "Mute" };
    juce::TextButton soloButton { "Solo" };

    // Remove
    juce::TextButton removeButton { "Remove" };

    // Level display
    float currentLevel = 0.0f;

    void setupSlider(juce::Slider& slider, double min, double max, double defaultValue);
    void setupComboBox(juce::ComboBox& combo);
    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level);
    void updateInputChannelOptions();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStripComponent)
};

} // namespace Kousaten
