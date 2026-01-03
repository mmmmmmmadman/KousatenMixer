/*
    Kousaten Mixer - Send Panner Component
    XY Pad UI for controlling send distribution across aux buses
*/

#pragma once

#include <JuceHeader.h>
#include "../Mixer/SendPanner.h"

namespace Kousaten {

class SendPannerComponent : public juce::Component,
                             public juce::Slider::Listener,
                             public juce::Button::Listener,
                             public juce::Timer
{
public:
    SendPannerComponent(SendPanner* panner);
    ~SendPannerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void sliderValueChanged(juce::Slider* slider) override;
    void buttonClicked(juce::Button* button) override;
    void timerCallback() override;

    // Update aux bus names for display
    void updateAuxNames(const std::map<int, juce::String>& names);

    // Sync positions from panner
    void syncFromPanner();

private:
    // Colors
    const juce::Colour backgroundDark { 0xff0e0c0c };
    const juce::Colour backgroundMid { 0xff201a1a };
    const juce::Colour backgroundLight { 0xff2a2424 };
    const juce::Colour accent { 0xffff9eb0 };
    const juce::Colour accentDim { 0xffc08090 };
    const juce::Colour textLight { 0xffffffff };
    const juce::Colour textDim { 0xffc8b8b8 };

    // Aux colors for visual distinction
    const std::array<juce::Colour, 8> auxColors = {
        juce::Colour(0xffff6b6b),  // Red
        juce::Colour(0xff4ecdc4),  // Teal
        juce::Colour(0xffffe66d),  // Yellow
        juce::Colour(0xff95e1d3),  // Mint
        juce::Colour(0xfff38181),  // Coral
        juce::Colour(0xffaa96da),  // Lavender
        juce::Colour(0xfffcbad3),  // Pink
        juce::Colour(0xffa8d8ea)   // Light Blue
    };

    SendPanner* sendPanner;

    // XY Pad area
    juce::Rectangle<int> xyPadBounds;
    bool isDragging = false;

    // Mode buttons
    juce::TextButton xyButton { "Manual" };
    juce::TextButton seqButton { "Sequence" };
    juce::TextButton rndButton { "Random" };
    juce::TextButton rotButton { "Rotate" };

    // LFO controls
    juce::Slider speedSlider;
    juce::Slider smoothSlider;
    juce::Slider amountSlider;

    // Enable toggle
    juce::TextButton enableButton { "On" };

    // Aux names for display
    std::map<int, juce::String> auxNames;

    // Helper functions
    void updateModeButtons();
    juce::Point<float> positionToXY(float x, float y) const;
    std::pair<float, float> xyToPosition(juce::Point<int> point) const;
    void drawXYPad(juce::Graphics& g);
    void drawAuxPoint(juce::Graphics& g, int auxId, float x, float y, float level);
    void drawCursor(juce::Graphics& g);

    juce::Colour getAuxColor(int auxId) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendPannerComponent)
};

} // namespace Kousaten
