/*
    Kousaten Mixer - Channel Strip Component
    Implementation
*/

#include "ChannelStripComponent.h"
#include "../Core/AudioEngine.h"

namespace Kousaten {

ChannelStripComponent::ChannelStripComponent(Channel* ch, AudioDeviceHandler* handler, AudioEngine* engine)
    : channel(ch), deviceHandler(handler), audioEngine(engine)
{
    setSize(250, 700);  // Two-column layout: Left (Device/Level) | Right (Panner + Aux)

    // Name editor
    nameEditor.setText(channel->getName());
    nameEditor.setJustification(juce::Justification::centred);
    nameEditor.setColour(juce::TextEditor::backgroundColourId, backgroundMid);
    nameEditor.setColour(juce::TextEditor::textColourId, textLight);
    nameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    nameEditor.onTextChange = [this] {
        channel->setName(nameEditor.getText());
    };
    addAndMakeVisible(nameEditor);

    // Input I/O ComboBoxes
    setupComboBox(inputDeviceCombo);
    setupComboBox(inputChannelCombo);

    // Initialize device lists if handler provided
    if (deviceHandler)
        updateDeviceLists();

    // Setup fixed effect sliders
    setupSlider(delaySendSlider, 0.0, 100.0, 0.0);
    setupSlider(grainSendSlider, 0.0, 100.0, 0.0);
    setupSlider(reverbSendSlider, 0.0, 100.0, 0.0);
    setupSlider(panSlider, -100.0, 100.0, 0.0);

    // Aux viewport for scrollable aux sends
    auxViewport.setViewedComponent(&auxContainer, false);
    auxViewport.setScrollBarsShown(true, false);
    auxViewport.getVerticalScrollBar().setColour(juce::ScrollBar::backgroundColourId, backgroundDark);
    auxViewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId, backgroundMid);
    auxViewport.getVerticalScrollBar().setColour(juce::ScrollBar::trackColourId, backgroundDark);
    addAndMakeVisible(auxViewport);

    // Add send button
    addSendButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    addSendButton.setColour(juce::TextButton::textColourOffId, accent);
    addSendButton.onClick = [this] {
        if (onAddAuxRequested)
            onAddAuxRequested(channel->getId());
    };
    addAndMakeVisible(addSendButton);

    // Volume fader - vertical
    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setRange(0.0, 100.0, 0.1);
    volumeSlider.setValue(80.0);
    volumeSlider.setDoubleClickReturnValue(true, 80.0);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    volumeSlider.setColour(juce::Slider::trackColourId, accentDim);
    volumeSlider.setColour(juce::Slider::thumbColourId, accent);
    volumeSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    volumeSlider.addListener(this);
    addAndMakeVisible(volumeSlider);

    // Mute button
    muteButton.setClickingTogglesState(true);
    muteButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    muteButton.setColour(juce::TextButton::buttonOnColourId, muteColor);
    muteButton.setColour(juce::TextButton::textColourOffId, textDim);
    muteButton.setColour(juce::TextButton::textColourOnId, textLight);
    muteButton.addListener(this);
    addAndMakeVisible(muteButton);

    // Solo button
    soloButton.setClickingTogglesState(true);
    soloButton.setColour(juce::TextButton::buttonColourId, backgroundLight);
    soloButton.setColour(juce::TextButton::buttonOnColourId, soloColor);
    soloButton.setColour(juce::TextButton::textColourOffId, textDim);
    soloButton.setColour(juce::TextButton::textColourOnId, backgroundDark);
    soloButton.addListener(this);
    addAndMakeVisible(soloButton);

    // Remove button
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    removeButton.setColour(juce::TextButton::textColourOffId, textDim);
    removeButton.addListener(this);
    addAndMakeVisible(removeButton);

    // Sync aux sends from engine
    if (audioEngine)
        syncAuxSends();

    // Create Send Panner component
    sendPannerComponent = std::make_unique<SendPannerComponent>(channel->getSendPanner());
    addAndMakeVisible(*sendPannerComponent);
    updateSendPannerAuxPositions();

    startTimerHz(30);
}

ChannelStripComponent::~ChannelStripComponent()
{
    stopTimer();
}

void ChannelStripComponent::setupSlider(juce::Slider& slider, double min, double max, double defaultValue)
{
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setRange(min, max, 0.1);
    slider.setValue(defaultValue);
    slider.setDoubleClickReturnValue(true, defaultValue);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    slider.setColour(juce::Slider::trackColourId, accentDim);
    slider.setColour(juce::Slider::thumbColourId, accent);
    slider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    slider.addListener(this);
    addAndMakeVisible(slider);
}

void ChannelStripComponent::setupComboBox(juce::ComboBox& combo)
{
    combo.setColour(juce::ComboBox::backgroundColourId, backgroundLight);
    combo.setColour(juce::ComboBox::textColourId, textLight);
    combo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    combo.setColour(juce::ComboBox::arrowColourId, accent);
    combo.addListener(this);
    addAndMakeVisible(combo);
}

void ChannelStripComponent::setDeviceHandler(AudioDeviceHandler* handler)
{
    deviceHandler = handler;
    if (deviceHandler)
        updateDeviceLists();
}

void ChannelStripComponent::setAudioEngine(AudioEngine* engine)
{
    audioEngine = engine;
    if (audioEngine)
        syncAuxSends();
}

void ChannelStripComponent::updateDeviceLists()
{
    if (!deviceHandler) return;

    inputDeviceCombo.clear();
    auto inputDevices = deviceHandler->getInputDeviceNames();
    int itemId = 1;
    for (const auto& name : inputDevices)
    {
        inputDeviceCombo.addItem(name, itemId++);
    }
    inputDeviceCombo.setSelectedId(1);
    updateInputChannelOptions();
}

void ChannelStripComponent::updateInputChannelOptions()
{
    inputChannelCombo.clear();

    if (!deviceHandler) return;

    juce::String deviceName = inputDeviceCombo.getText();
    if (deviceName == "None" || deviceName.isEmpty())
    {
        inputChannelCombo.addItem("No Input", 1);
        inputChannelCombo.setSelectedId(1);
        return;
    }

    auto options = deviceHandler->buildInputChannelOptions(deviceName);
    int itemId = 1;
    for (const auto& option : options)
    {
        inputChannelCombo.addItem(option, itemId++);
    }
    if (inputChannelCombo.getNumItems() > 0)
        inputChannelCombo.setSelectedId(1);
}

void ChannelStripComponent::addAuxSend(int auxId, const juce::String& auxName)
{
    // Check if already exists
    for (const auto& ctrl : auxSendControls)
    {
        if (ctrl.auxId == auxId)
            return;
    }

    AuxSendControl ctrl;
    ctrl.auxId = auxId;
    ctrl.name = auxName;
    ctrl.slider = std::make_unique<juce::Slider>();

    ctrl.slider->setSliderStyle(juce::Slider::LinearHorizontal);
    ctrl.slider->setRange(0.0, 100.0, 0.1);
    ctrl.slider->setValue(0.0);
    ctrl.slider->setDoubleClickReturnValue(true, 0.0);
    ctrl.slider->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    ctrl.slider->setColour(juce::Slider::backgroundColourId, backgroundMid);
    ctrl.slider->setColour(juce::Slider::trackColourId, accentDim);
    ctrl.slider->setColour(juce::Slider::thumbColourId, accent);
    ctrl.slider->setLookAndFeel(&getMinimalSliderLookAndFeel());
    ctrl.slider->addListener(this);
    auxContainer.addAndMakeVisible(*ctrl.slider);

    auxSendControls.push_back(std::move(ctrl));
    updateAuxLayout();
    repaint();
}

void ChannelStripComponent::removeAuxSend(int auxId)
{
    for (auto it = auxSendControls.begin(); it != auxSendControls.end(); ++it)
    {
        if (it->auxId == auxId)
        {
            auxContainer.removeChildComponent(it->slider.get());
            auxSendControls.erase(it);
            break;
        }
    }
    updateAuxLayout();
    repaint();
}

void ChannelStripComponent::syncAuxSends()
{
    if (!audioEngine) return;

    const auto& auxBuses = audioEngine->getAllAuxBuses();

    // Remove sends that no longer exist
    std::vector<int> toRemove;
    for (const auto& ctrl : auxSendControls)
    {
        bool found = false;
        for (const auto& bus : auxBuses)
        {
            if (bus->getId() == ctrl.auxId)
            {
                found = true;
                break;
            }
        }
        if (!found)
            toRemove.push_back(ctrl.auxId);
    }
    for (int auxId : toRemove)
    {
        removeAuxSend(auxId);
        channel->removeAuxSend(auxId);
    }

    // Add new sends or update existing names
    for (const auto& bus : auxBuses)
    {
        bool exists = false;
        for (auto& ctrl : auxSendControls)
        {
            if (ctrl.auxId == bus->getId())
            {
                // Update name if changed
                if (ctrl.name != bus->getName())
                {
                    ctrl.name = bus->getName();
                    repaint();
                }
                exists = true;
                break;
            }
        }
        if (!exists)
        {
            addAuxSend(bus->getId(), bus->getName());
        }
    }

    // Update Send Panner with aux positions
    updateSendPannerAuxPositions();
}

void ChannelStripComponent::paint(juce::Graphics& g)
{
    g.setColour(backgroundMid);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    // Two-column layout: Left (Device/Level) | Right (Panner + Aux)
    int leftWidth = 100;
    int rightX = leftWidth + 4;
    int margin = 8;
    int labelHeight = 18;
    int rowHeight = 34;

    // === LEFT SIDE (Device/Level) ===
    int y = 8;

    // INPUT label
    y += 30 + 8; // name height + gap
    g.setColour(accentDim);
    g.setFont(18.0f);
    g.drawText("INPUT", margin, y, 60, labelHeight, juce::Justification::left);
    y += labelHeight + 24 + 4 + 24 + 12; // label + combo + gap + combo + spacing

    // SEND label (effects on left)
    g.setColour(accentDim);
    g.setFont(18.0f);
    g.drawText("SEND", margin, y, 60, labelHeight, juce::Justification::left);
    y += labelHeight + 6;

    // Fixed effect sends labels and values
    g.setFont(18.0f);
    g.setColour(textDim);
    g.drawText("Delay", margin, y, 60, labelHeight, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(static_cast<int>(delaySendSlider.getValue())), leftWidth - 40, y, 35, labelHeight, juce::Justification::right);
    y += rowHeight;

    g.setColour(textDim);
    g.drawText("Grain", margin, y, 60, labelHeight, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(static_cast<int>(grainSendSlider.getValue())), leftWidth - 40, y, 35, labelHeight, juce::Justification::right);
    y += rowHeight;

    g.setColour(textDim);
    g.drawText("Reverb", margin, y, 60, labelHeight, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(static_cast<int>(reverbSendSlider.getValue())), leftWidth - 40, y, 35, labelHeight, juce::Justification::right);
    y += rowHeight + 10;

    // Pan label and value
    g.setColour(textDim);
    g.drawText("Pan", margin, y, 50, labelHeight, juce::Justification::left);
    int panValue = static_cast<int>(panSlider.getValue());
    juce::String panText = (panValue == 0) ? "C" : (panValue < 0) ? "L" + juce::String(-panValue) : "R" + juce::String(panValue);
    g.setColour(accent);
    g.drawText(panText, leftWidth - 40, y, 35, labelHeight, juce::Justification::right);
    y += rowHeight;

    // Volume label and value
    g.setColour(textDim);
    g.drawText("Volume", margin, y, 60, labelHeight, juce::Justification::left);
    g.setColour(accent);
    g.drawText(juce::String(static_cast<int>(volumeSlider.getValue())), leftWidth - 40, y, 35, labelHeight, juce::Justification::right);

    // Meter (next to volume fader)
    int meterY = y + 28;
    int meterHeight = getHeight() - meterY - 70;
    if (meterHeight > 30)
        drawMeter(g, margin, meterY, 14, meterHeight, currentLevel);

    // === RIGHT SIDE (Panner + Aux) ===
    // Vertical divider
    g.setColour(backgroundLight);
    g.fillRect(rightX - 2, 8, 2, getHeight() - 16);
}

void ChannelStripComponent::paintOverChildren(juce::Graphics& g)
{
    // Use viewport's actual position for aux labels
    int auxStartY = auxViewport.getY();
    int auxX = auxViewport.getX();

    // Draw aux names on left side of each aux row
    g.setFont(14.0f);
    int scrollY = auxViewport.getViewPositionY();
    int rowHeight = 24;
    int visibleStart = scrollY / rowHeight;
    int visibleEnd = (scrollY + auxViewport.getHeight()) / rowHeight + 1;

    for (int i = visibleStart; i < static_cast<int>(auxSendControls.size()) && i <= visibleEnd; ++i)
    {
        const auto& ctrl = auxSendControls[static_cast<size_t>(i)];
        int drawY = auxStartY + (i * rowHeight) - scrollY;

        if (drawY >= auxStartY - rowHeight && drawY < auxStartY + auxViewport.getHeight())
        {
            g.setColour(textDim);
            // Show name if available, otherwise show number
            juce::String label = ctrl.name.isNotEmpty() ? ctrl.name : juce::String(ctrl.auxId + 1);
            g.drawText(label, auxX, drawY, 60, 18, juce::Justification::left);
        }
    }
}

void ChannelStripComponent::drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level)
{
    g.setColour(backgroundLight);
    g.fillRoundedRectangle(static_cast<float>(x), static_cast<float>(y),
                           static_cast<float>(width), static_cast<float>(height), 3.0f);

    int fillHeight = static_cast<int>(level * height);
    if (fillHeight > 0)
    {
        juce::ColourGradient gradient(accent, static_cast<float>(x), static_cast<float>(y + height),
                                       juce::Colour(0xffff4444), static_cast<float>(x), static_cast<float>(y),
                                       false);
        gradient.addColour(0.7, accentDim);
        g.setGradientFill(gradient);
        g.fillRoundedRectangle(static_cast<float>(x + 1),
                               static_cast<float>(y + height - fillHeight),
                               static_cast<float>(width - 2),
                               static_cast<float>(fillHeight), 2.0f);
    }
}

void ChannelStripComponent::resized()
{
    // Two-column layout: Left (Device/Level) | Right (Panner + Aux)
    int leftWidth = 100;
    int rightX = leftWidth + 4;
    int rightWidth = getWidth() - rightX - 4;
    int margin = 8;
    int nameHeight = 30;
    int comboHeight = 24;
    int rowHeight = 34;
    int sliderHeight = 16;  // 2x height for thicker faders

    // === LEFT SIDE (Device/Level) ===
    int y = 8;

    // Name editor - spans full width at top
    nameEditor.setBounds(margin, y, getWidth() - margin * 2, nameHeight);
    y += nameHeight + 8;

    // INPUT section
    y += 18; // label space
    inputDeviceCombo.setBounds(margin, y, leftWidth - margin * 2, comboHeight);
    y += comboHeight + 4;
    inputChannelCombo.setBounds(margin, y, leftWidth - margin * 2, comboHeight);
    y += comboHeight + 12;

    // SEND section (effects)
    y += 24; // label space

    // Fixed effect sliders (18px below label = 16 + 2px gap)
    delaySendSlider.setBounds(margin, y + 18, leftWidth - margin * 2, sliderHeight);
    y += rowHeight;

    grainSendSlider.setBounds(margin, y + 18, leftWidth - margin * 2, sliderHeight);
    y += rowHeight;

    reverbSendSlider.setBounds(margin, y + 18, leftWidth - margin * 2, sliderHeight);
    y += rowHeight + 10;

    // Pan slider
    panSlider.setBounds(margin, y + 18, leftWidth - margin * 2, sliderHeight);
    y += rowHeight;

    // Volume fader - fills remaining space
    y += 14;
    int volumeFaderHeight = getHeight() - y - 75;
    if (volumeFaderHeight < 60) volumeFaderHeight = 60;
    volumeSlider.setBounds(margin + 35, y + 14, 15, volumeFaderHeight);

    // Mute / Solo buttons
    int buttonY = getHeight() - 68;
    int buttonWidth = (leftWidth - margin * 3) / 2;
    muteButton.setBounds(margin, buttonY, buttonWidth, 28);
    soloButton.setBounds(margin * 2 + buttonWidth, buttonY, buttonWidth, 28);

    // Remove button
    removeButton.setBounds(margin, buttonY + 32, leftWidth - margin * 2, 26);

    // === RIGHT SIDE (Panner on top, Aux below) ===
    int pannerHeight = 350;  // Increased for Speed/Smooth/Amount sliders

    // Send Panner at top of right column
    if (sendPannerComponent)
    {
        sendPannerComponent->setBounds(rightX, 8, rightWidth, pannerHeight);
    }

    // Aux sends below panner
    int auxStartY = pannerHeight + 16;
    int auxViewportHeight = getHeight() - auxStartY - 40;
    if (auxViewportHeight < 60) auxViewportHeight = 60;

    auxViewport.setBounds(rightX + 4, auxStartY, rightWidth - 8, auxViewportHeight);

    // Add send button at bottom of right column
    addSendButton.setBounds(rightX + 4, getHeight() - 32, rightWidth - 8, 26);

    updateAuxLayout();
}

void ChannelStripComponent::updateAuxLayout()
{
    int rowHeight = 24;
    int sliderHeight = 8;
    int containerW = auxViewport.getWidth() - 8;
    int sliderW = containerW / 2;  // Half width for fader on right
    int totalHeight = static_cast<int>(auxSendControls.size()) * rowHeight;

    auxContainer.setSize(containerW, std::max(totalHeight, auxViewport.getHeight()));

    int y = 0;
    for (auto& ctrl : auxSendControls)
    {
        // Slider on right half, text area on left (drawn in paintOverChildren)
        ctrl.slider->setBounds(containerW - sliderW, y + 8, sliderW, sliderHeight);
        y += rowHeight;
    }
}

void ChannelStripComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &volumeSlider)
    {
        channel->setVolume(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == &panSlider)
    {
        channel->setPan(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == &delaySendSlider)
    {
        channel->setDelaySend(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == &grainSendSlider)
    {
        channel->setGrainSend(static_cast<float>(slider->getValue() / 100.0));
    }
    else if (slider == &reverbSendSlider)
    {
        channel->setReverbSend(static_cast<float>(slider->getValue() / 100.0));
    }
    else
    {
        // Check aux sends
        for (const auto& ctrl : auxSendControls)
        {
            if (slider == ctrl.slider.get())
            {
                channel->setAuxSend(ctrl.auxId, static_cast<float>(slider->getValue() / 100.0));
                break;
            }
        }
    }

    repaint();
}

void ChannelStripComponent::buttonClicked(juce::Button* button)
{
    if (button == &muteButton)
    {
        channel->setMute(muteButton.getToggleState());
    }
    else if (button == &soloButton)
    {
        channel->setSolo(soloButton.getToggleState());
        if (audioEngine)
            audioEngine->updateSoloState();
    }
    else if (button == &removeButton)
    {
        if (onRemoveChannel)
            onRemoveChannel(channel->getId());
    }
}

void ChannelStripComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &inputDeviceCombo)
    {
        channel->setInputDevice(comboBox->getText());
        updateInputChannelOptions();
    }
    else if (comboBox == &inputChannelCombo)
    {
        juce::String text = comboBox->getText();

        // Check for "No Input" option
        if (text == "No Input" || text.isEmpty())
        {
            channel->setInputChannelStart(-1);
            channel->setStereo(true);
        }
        else
        {
            int channelStart = text.getIntValue() - 1;
            channel->setInputChannelStart(channelStart);
            channel->setStereo(text.contains("Stereo"));
        }
    }
}

void ChannelStripComponent::timerCallback()
{
    float newLevel = channel->getOutputLevel();
    if (std::abs(newLevel - currentLevel) > 0.01f)
    {
        currentLevel = newLevel;
        repaint();
    }

    // Sync aux send sliders with panner levels
    auto* panner = channel->getSendPanner();
    if (panner && panner->isEnabled())
    {
        auto pannerLevels = panner->calculateSendLevels();
        for (auto& ctrl : auxSendControls)
        {
            auto it = pannerLevels.find(ctrl.auxId);
            if (it != pannerLevels.end())
            {
                // Scale panner level (0-1) to slider value (0-100)
                double newValue = static_cast<double>(it->second) * 100.0;
                if (std::abs(ctrl.slider->getValue() - newValue) > 0.5)
                {
                    ctrl.slider->setValue(newValue, juce::dontSendNotification);
                }
            }
        }
    }
}

void ChannelStripComponent::updateSendPannerAuxPositions()
{
    if (!sendPannerComponent || !channel) return;

    auto* panner = channel->getSendPanner();
    if (!panner) return;

    // Collect all aux IDs and names
    std::vector<int> auxIds;
    std::map<int, juce::String> auxNames;

    for (const auto& ctrl : auxSendControls)
    {
        auxIds.push_back(ctrl.auxId);
        auxNames[ctrl.auxId] = ctrl.name;
    }

    // Arrange aux positions in a circle
    panner->arrangeAuxPositionsCircle(auxIds);

    // Update UI with aux names
    sendPannerComponent->updateAuxNames(auxNames);
    sendPannerComponent->syncFromPanner();
}

} // namespace Kousaten
