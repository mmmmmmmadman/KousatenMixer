/*
    Kousaten Mixer - Send Panner Component
    Implementation
*/

#include "SendPannerComponent.h"
#include "ChannelStripComponent.h"

namespace Kousaten {

SendPannerComponent::SendPannerComponent(SendPanner* panner)
    : sendPanner(panner)
{
    // Mode buttons
    xyButton.setClickingTogglesState(true);
    xyButton.setRadioGroupId(1);
    xyButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    xyButton.setColour(juce::TextButton::buttonOnColourId, accent);
    xyButton.setColour(juce::TextButton::textColourOffId, textDim);
    xyButton.setColour(juce::TextButton::textColourOnId, backgroundDark);
    xyButton.addListener(this);
    addAndMakeVisible(xyButton);

    seqButton.setClickingTogglesState(true);
    seqButton.setRadioGroupId(1);
    seqButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    seqButton.setColour(juce::TextButton::buttonOnColourId, accent);
    seqButton.setColour(juce::TextButton::textColourOffId, textDim);
    seqButton.setColour(juce::TextButton::textColourOnId, backgroundDark);
    seqButton.addListener(this);
    addAndMakeVisible(seqButton);

    rndButton.setClickingTogglesState(true);
    rndButton.setRadioGroupId(1);
    rndButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    rndButton.setColour(juce::TextButton::buttonOnColourId, accent);
    rndButton.setColour(juce::TextButton::textColourOffId, textDim);
    rndButton.setColour(juce::TextButton::textColourOnId, backgroundDark);
    rndButton.addListener(this);
    addAndMakeVisible(rndButton);

    rotButton.setClickingTogglesState(true);
    rotButton.setRadioGroupId(1);
    rotButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    rotButton.setColour(juce::TextButton::buttonOnColourId, accent);
    rotButton.setColour(juce::TextButton::textColourOffId, textDim);
    rotButton.setColour(juce::TextButton::textColourOnId, backgroundDark);
    rotButton.addListener(this);
    addAndMakeVisible(rotButton);

    // Enable button
    enableButton.setClickingTogglesState(true);
    enableButton.setToggleState(true, juce::dontSendNotification);
    enableButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    enableButton.setColour(juce::TextButton::buttonOnColourId, accent);
    enableButton.setColour(juce::TextButton::textColourOffId, textDim);
    enableButton.setColour(juce::TextButton::textColourOnId, backgroundDark);
    enableButton.addListener(this);
    addAndMakeVisible(enableButton);

    // Speed slider
    speedSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    speedSlider.setRange(0.1, 10.0, 0.1);
    speedSlider.setValue(1.0);
    speedSlider.setDoubleClickReturnValue(true, 1.0);
    speedSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    speedSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    speedSlider.setColour(juce::Slider::trackColourId, accentDim);
    speedSlider.setColour(juce::Slider::thumbColourId, accent);
    speedSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    speedSlider.addListener(this);
    addAndMakeVisible(speedSlider);

    // Smooth slider
    smoothSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    smoothSlider.setRange(0.0, 1.0, 0.01);
    smoothSlider.setValue(0.5);
    smoothSlider.setDoubleClickReturnValue(true, 0.5);
    smoothSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    smoothSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    smoothSlider.setColour(juce::Slider::trackColourId, accentDim);
    smoothSlider.setColour(juce::Slider::thumbColourId, accent);
    smoothSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    smoothSlider.addListener(this);
    addAndMakeVisible(smoothSlider);

    // Amount slider
    amountSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    amountSlider.setRange(0.0, 1.0, 0.01);
    amountSlider.setValue(1.0);
    amountSlider.setDoubleClickReturnValue(true, 1.0);
    amountSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    amountSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    amountSlider.setColour(juce::Slider::trackColourId, accentDim);
    amountSlider.setColour(juce::Slider::thumbColourId, accent);
    amountSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    amountSlider.addListener(this);
    addAndMakeVisible(amountSlider);

    updateModeButtons();
    startTimerHz(30);
}

SendPannerComponent::~SendPannerComponent()
{
    stopTimer();
    speedSlider.setLookAndFeel(nullptr);
    smoothSlider.setLookAndFeel(nullptr);
    amountSlider.setLookAndFeel(nullptr);
}

void SendPannerComponent::updateModeButtons()
{
    if (!sendPanner) return;

    auto mode = sendPanner->getMode();
    xyButton.setToggleState(mode == SendPannerMode::XYPad, juce::dontSendNotification);
    seqButton.setToggleState(mode == SendPannerMode::Sequencer, juce::dontSendNotification);
    rndButton.setToggleState(mode == SendPannerMode::Random, juce::dontSendNotification);
    rotButton.setToggleState(mode == SendPannerMode::Rotate, juce::dontSendNotification);

    enableButton.setToggleState(sendPanner->isEnabled(), juce::dontSendNotification);
    speedSlider.setValue(sendPanner->getSpeed(), juce::dontSendNotification);
    smoothSlider.setValue(sendPanner->getSmooth(), juce::dontSendNotification);
    amountSlider.setValue(sendPanner->getAmount(), juce::dontSendNotification);
}

void SendPannerComponent::paint(juce::Graphics& g)
{
    // Background
    g.setColour(backgroundMid);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Title
    g.setColour(accentDim);
    g.setFont(14.0f);
    g.drawText("PANNER", 4, 2, getWidth() - 8, 16, juce::Justification::centred);

    // Draw XY Pad
    drawXYPad(g);

    // LFO parameters labels (below mode buttons)
    g.setFont(12.0f);
    int labelY = xyPadBounds.getBottom() + 56;
    int rowHeight = 32;

    // Speed label
    g.setColour(textDim);
    g.drawText("Speed", 4, labelY, 42, 14, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(speedSlider.getValue(), 1) + "Hz",
               getWidth() - 42, labelY, 38, 14, juce::Justification::right);
    labelY += rowHeight;

    // Smooth label
    g.setColour(textDim);
    g.drawText("Smooth", 4, labelY, 48, 14, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(static_cast<int>(smoothSlider.getValue() * 100)) + "%",
               getWidth() - 42, labelY, 38, 14, juce::Justification::right);
    labelY += rowHeight;

    // Amount label
    g.setColour(textDim);
    g.drawText("Amount", 4, labelY, 48, 14, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(static_cast<int>(amountSlider.getValue() * 100)) + "%",
               getWidth() - 42, labelY, 38, 14, juce::Justification::right);
}

void SendPannerComponent::drawXYPad(juce::Graphics& g)
{
    if (!sendPanner) return;

    // Pad background
    g.setColour(backgroundDark);
    g.fillRoundedRectangle(xyPadBounds.toFloat(), 4.0f);

    // Grid lines
    g.setColour(backgroundLight.withAlpha(0.5f));
    int cx = xyPadBounds.getCentreX();
    int cy = xyPadBounds.getCentreY();
    g.drawHorizontalLine(cy, static_cast<float>(xyPadBounds.getX()),
                         static_cast<float>(xyPadBounds.getRight()));
    g.drawVerticalLine(cx, static_cast<float>(xyPadBounds.getY()),
                       static_cast<float>(xyPadBounds.getBottom()));

    // Draw recorded path (if any)
    const auto& recordedPath = sendPanner->getRecordedPath();
    if (recordedPath.size() > 1)
    {
        g.setColour(accent.withAlpha(0.3f));
        juce::Path path;
        auto firstPt = positionToXY(recordedPath[0].first, recordedPath[0].second);
        path.startNewSubPath(firstPt);

        for (size_t i = 1; i < recordedPath.size(); ++i)
        {
            auto pt = positionToXY(recordedPath[i].first, recordedPath[i].second);
            path.lineTo(pt);
        }
        g.strokePath(path, juce::PathStrokeType(1.5f));
    }

    // Draw home position marker (small diamond)
    auto homePoint = positionToXY(sendPanner->getHomeX(), sendPanner->getHomeY());
    g.setColour(accentDim);
    juce::Path diamond;
    diamond.addStar(homePoint, 4, 3.0f, 6.0f, juce::MathConstants<float>::halfPi);
    g.fillPath(diamond);

    // Calculate current send levels
    auto levels = sendPanner->calculateSendLevels();

    // Draw aux positions
    for (const auto& [auxId, pos] : sendPanner->getAllAuxPositions())
    {
        float level = 0.0f;
        auto it = levels.find(auxId);
        if (it != levels.end())
            level = it->second;

        drawAuxPoint(g, auxId, pos.first, pos.second, level);
    }

    // Draw cursor
    drawCursor(g);

    // Border
    g.setColour(backgroundLight);
    g.drawRoundedRectangle(xyPadBounds.toFloat(), 4.0f, 1.0f);
}

void SendPannerComponent::drawAuxPoint(juce::Graphics& g, int auxId, float x, float y, float level)
{
    auto point = positionToXY(x, y);

    // Draw glow based on level (light pink), size scales with aux count
    if (level > 0.1f)
    {
        int numAux = static_cast<int>(auxNames.size());
        float scaleFactor = (numAux <= 2) ? 1.0f : 2.0f / std::sqrt(static_cast<float>(numAux));
        float glowRadius = (12.0f + level * 18.0f) * scaleFactor;
        g.setColour(accent.withAlpha(level * 0.5f));
        g.fillEllipse(point.x - glowRadius, point.y - glowRadius,
                      glowRadius * 2.0f, glowRadius * 2.0f);
    }

    // Draw point (white)
    float radius = 5.0f + level * 3.0f;
    g.setColour(textLight);
    g.fillEllipse(point.x - radius, point.y - radius, radius * 2.0f, radius * 2.0f);

    // Draw aux name/number (black text on white point)
    g.setColour(backgroundDark);
    g.setFont(9.0f);

    juce::String label;
    auto nameIt = auxNames.find(auxId);
    if (nameIt != auxNames.end() && nameIt->second.isNotEmpty())
    {
        // Use first letter of name
        label = nameIt->second.substring(0, 1).toUpperCase();
    }
    else
    {
        label = juce::String(auxId + 1);
    }

    g.drawText(label, static_cast<int>(point.x) - 5, static_cast<int>(point.y) - 5,
               10, 10, juce::Justification::centred);
}

void SendPannerComponent::drawCursor(juce::Graphics& g)
{
    if (!sendPanner) return;

    // Use animated position in auto modes, manual position in XYPad mode
    float x, y;
    if (sendPanner->getMode() == SendPannerMode::XYPad)
    {
        x = sendPanner->getPositionX();
        y = sendPanner->getPositionY();
    }
    else
    {
        x = sendPanner->getCurrentX();
        y = sendPanner->getCurrentY();
    }
    auto point = positionToXY(x, y);

    // Crosshair cursor
    g.setColour(textLight);

    // Horizontal line
    g.drawHorizontalLine(static_cast<int>(point.y),
                         point.x - 8.0f, point.x - 3.0f);
    g.drawHorizontalLine(static_cast<int>(point.y),
                         point.x + 3.0f, point.x + 8.0f);

    // Vertical line
    g.drawVerticalLine(static_cast<int>(point.x),
                       point.y - 8.0f, point.y - 3.0f);
    g.drawVerticalLine(static_cast<int>(point.x),
                       point.y + 3.0f, point.y + 8.0f);

    // Center dot
    g.fillEllipse(point.x - 2.0f, point.y - 2.0f, 4.0f, 4.0f);
}

juce::Point<float> SendPannerComponent::positionToXY(float x, float y) const
{
    return {
        static_cast<float>(xyPadBounds.getX()) + x * static_cast<float>(xyPadBounds.getWidth()),
        static_cast<float>(xyPadBounds.getY()) + (1.0f - y) * static_cast<float>(xyPadBounds.getHeight())
    };
}

std::pair<float, float> SendPannerComponent::xyToPosition(juce::Point<int> point) const
{
    float x = static_cast<float>(point.x - xyPadBounds.getX()) /
              static_cast<float>(xyPadBounds.getWidth());
    float y = 1.0f - static_cast<float>(point.y - xyPadBounds.getY()) /
              static_cast<float>(xyPadBounds.getHeight());

    return { juce::jlimit(0.0f, 1.0f, x), juce::jlimit(0.0f, 1.0f, y) };
}

juce::Colour SendPannerComponent::getAuxColor(int auxId) const
{
    return auxColors[static_cast<size_t>(auxId) % auxColors.size()];
}

void SendPannerComponent::resized()
{
    int margin = 4;
    int y = 20;

    // XY Pad (square, as large as width allows)
    int padSize = getWidth() - margin * 2;
    if (padSize > 120) padSize = 120;
    int padX = (getWidth() - padSize) / 2;
    xyPadBounds = juce::Rectangle<int>(padX, y, padSize, padSize);
    y += padSize + 6;

    // Mode buttons (2x2 grid)
    int buttonW = (getWidth() - margin * 3) / 2;
    int buttonH = 22;

    xyButton.setBounds(margin, y, buttonW, buttonH);
    seqButton.setBounds(margin * 2 + buttonW, y, buttonW, buttonH);
    y += buttonH + 2;

    rndButton.setBounds(margin, y, buttonW, buttonH);
    rotButton.setBounds(margin * 2 + buttonW, y, buttonW, buttonH);
    y += buttonH + 6;

    int sliderHeight = 16;
    int rowHeight = 32;

    // Speed slider
    y += 16; // label space
    speedSlider.setBounds(margin, y, getWidth() - margin * 2, sliderHeight);
    y += rowHeight;

    // Smooth slider
    smoothSlider.setBounds(margin, y, getWidth() - margin * 2, sliderHeight);
    y += rowHeight;

    // Amount slider
    amountSlider.setBounds(margin, y, getWidth() - margin * 2, sliderHeight);
    y += rowHeight;

    // Enable button
    enableButton.setBounds(margin, y, getWidth() - margin * 2, 22);
}

void SendPannerComponent::mouseDown(const juce::MouseEvent& e)
{
    if (xyPadBounds.contains(e.getPosition()) && sendPanner)
    {
        auto pos = xyToPosition(e.getPosition());

        // Double-click sets home position
        if (e.getNumberOfClicks() >= 2)
        {
            sendPanner->setHomePosition(pos.first, pos.second);
            repaint();
            return;
        }

        isDragging = true;
        sendPanner->startRecording();
        sendPanner->setPosition(pos.first, pos.second);
        repaint();
    }
}

void SendPannerComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (isDragging && sendPanner)
    {
        auto pos = xyToPosition(e.getPosition());
        sendPanner->setPosition(pos.first, pos.second);
        repaint();
    }
}

void SendPannerComponent::mouseUp(const juce::MouseEvent& /*e*/)
{
    if (isDragging && sendPanner)
    {
        sendPanner->stopRecording();
        updateModeButtons();  // Update UI since mode may have changed
    }
    isDragging = false;
}

void SendPannerComponent::sliderValueChanged(juce::Slider* slider)
{
    if (!sendPanner) return;

    if (slider == &speedSlider)
    {
        sendPanner->setSpeed(static_cast<float>(slider->getValue()));
    }
    else if (slider == &smoothSlider)
    {
        sendPanner->setSmooth(static_cast<float>(slider->getValue()));
    }
    else if (slider == &amountSlider)
    {
        sendPanner->setAmount(static_cast<float>(slider->getValue()));
    }
    repaint();
}

void SendPannerComponent::buttonClicked(juce::Button* button)
{
    if (!sendPanner) return;

    if (button == &xyButton)
    {
        sendPanner->setMode(SendPannerMode::XYPad);
    }
    else if (button == &seqButton)
    {
        sendPanner->setMode(SendPannerMode::Sequencer);
    }
    else if (button == &rndButton)
    {
        sendPanner->setMode(SendPannerMode::Random);
    }
    else if (button == &rotButton)
    {
        sendPanner->setMode(SendPannerMode::Rotate);
    }
    else if (button == &enableButton)
    {
        sendPanner->setEnabled(enableButton.getToggleState());
    }

    repaint();
}

void SendPannerComponent::timerCallback()
{
    // Repaint to show automation movement
    if (sendPanner && sendPanner->getMode() != SendPannerMode::XYPad)
    {
        repaint();
    }
}

void SendPannerComponent::updateAuxNames(const std::map<int, juce::String>& names)
{
    auxNames = names;
    repaint();
}

void SendPannerComponent::syncFromPanner()
{
    updateModeButtons();
    repaint();
}

} // namespace Kousaten
