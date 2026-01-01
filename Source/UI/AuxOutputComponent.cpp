/*
    Kousaten Mixer - Aux Output Component
    Implementation - Unified horizontal row format for Send Returns and Aux Outputs
*/

#include "AuxOutputComponent.h"
#include "ChannelStripComponent.h"
#include "../Core/AudioEngine.h"

namespace Kousaten {

// =============================================================================
// SendReturnRowComponent - Horizontal row for built-in effects
// =============================================================================

SendReturnRowComponent::SendReturnRowComponent(MixBus* bus, const juce::String& name)
    : mixBus(bus), busName(name)
{
    setSize(400, 36);

    // Level slider - horizontal, thin (half height)
    levelSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    levelSlider.setRange(0.0, 100.0, 0.1);
    levelSlider.setValue(100.0);
    levelSlider.setDoubleClickReturnValue(true, 100.0);
    levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    levelSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    levelSlider.setColour(juce::Slider::trackColourId, accentDim);
    levelSlider.setColour(juce::Slider::thumbColourId, accent);
    levelSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    levelSlider.addListener(this);
    addAndMakeVisible(levelSlider);

    startTimerHz(30);
}

SendReturnRowComponent::~SendReturnRowComponent()
{
    stopTimer();
}

void SendReturnRowComponent::paint(juce::Graphics& g)
{
    // Row background
    g.setColour(backgroundLight);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Meter on left side (vertical, thin)
    drawMeter(g, 4, 4, 6, getHeight() - 8, currentLevel);

    // Bus name
    g.setColour(accent);
    g.setFont(18.0f);
    g.drawText(busName, 14, 0, 60, getHeight(), juce::Justification::centredLeft);

    // Level value on right
    g.setColour(accent);
    g.setFont(18.0f);
    g.drawText(juce::String(static_cast<int>(levelSlider.getValue())),
               getWidth() - 35, 0, 30, getHeight(), juce::Justification::centred);
}

void SendReturnRowComponent::drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level)
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

void SendReturnRowComponent::resized()
{
    int h = getHeight();
    // Level slider - spans from name to value (thin: 8px height)
    levelSlider.setBounds(78, (h - 8) / 2, getWidth() - 78 - 40, 8);
}

void SendReturnRowComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &levelSlider)
    {
        mixBus->setReturnLevel(static_cast<float>(slider->getValue() / 100.0));
    }
    repaint();
}

void SendReturnRowComponent::timerCallback()
{
    float newLevel = mixBus->getOutputLevel();
    if (std::abs(newLevel - currentLevel) > 0.01f)
    {
        currentLevel = newLevel;
        repaint();
    }
}

// =============================================================================
// AuxOutputComponent - Horizontal row layout with device selection
// =============================================================================

AuxOutputComponent::AuxOutputComponent(AuxBus* bus, AudioDeviceHandler* handler)
    : auxBus(bus), deviceHandler(handler)
{
    setSize(400, 52);

    // Editable name
    nameEditor.setText(auxBus->getName());
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.setColour(juce::TextEditor::backgroundColourId, backgroundLight);
    nameEditor.setColour(juce::TextEditor::textColourId, textLight);
    nameEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    nameEditor.setFont(18.0f);
    nameEditor.onTextChange = [this] {
        auxBus->setName(nameEditor.getText());
        if (onNameChanged)
            onNameChanged();
    };
    addAndMakeVisible(nameEditor);

    // Device ComboBox
    deviceCombo.setColour(juce::ComboBox::backgroundColourId, backgroundLight);
    deviceCombo.setColour(juce::ComboBox::textColourId, textLight);
    deviceCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    deviceCombo.addListener(this);
    addAndMakeVisible(deviceCombo);

    // Channel ComboBox
    channelCombo.setColour(juce::ComboBox::backgroundColourId, backgroundLight);
    channelCombo.setColour(juce::ComboBox::textColourId, textLight);
    channelCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    channelCombo.addListener(this);
    addAndMakeVisible(channelCombo);

    // Level slider - horizontal, thin (half height: 8px)
    levelSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    levelSlider.setRange(0.0, 100.0, 0.1);
    levelSlider.setValue(100.0);
    levelSlider.setDoubleClickReturnValue(true, 100.0);
    levelSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    levelSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    levelSlider.setColour(juce::Slider::trackColourId, accentDim);
    levelSlider.setColour(juce::Slider::thumbColourId, accent);
    levelSlider.setLookAndFeel(&getMinimalSliderLookAndFeel());
    levelSlider.addListener(this);
    addAndMakeVisible(levelSlider);

    // Remove button
    removeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    removeButton.setColour(juce::TextButton::textColourOffId, textDim);
    removeButton.addListener(this);
    addAndMakeVisible(removeButton);

    // Populate device list
    if (deviceHandler)
    {
        deviceCombo.clear();
        auto devices = deviceHandler->getOutputDeviceNames();
        int itemId = 1;
        for (const auto& name : devices)
        {
            deviceCombo.addItem(name, itemId++);
        }
        deviceCombo.setSelectedId(1);
        updateChannelOptions();
    }

    startTimerHz(30);
}

AuxOutputComponent::~AuxOutputComponent()
{
    stopTimer();
}

void AuxOutputComponent::paint(juce::Graphics& g)
{
    // Row background
    g.setColour(backgroundLight);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

    // Meter on left side (vertical, thin)
    drawMeter(g, 4, 4, 6, getHeight() - 8, currentLevel);

    // Level value
    g.setColour(accent);
    g.setFont(18.0f);
    g.drawText(juce::String(static_cast<int>(levelSlider.getValue())),
               getWidth() - 60, getHeight() / 2 - 10, 30, 20, juce::Justification::centred);
}

void AuxOutputComponent::drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level)
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

void AuxOutputComponent::resized()
{
    int x = 14;  // After meter
    int h = getHeight();
    int rowH = 22;
    int y1 = 4;
    int y2 = h - 14;  // Adjusted for thinner slider

    // Name editor - top left
    nameEditor.setBounds(x, y1, 70, rowH);

    // Device combo - top middle
    deviceCombo.setBounds(x + 75, y1, 90, rowH);

    // Channel combo - top right area
    channelCombo.setBounds(x + 170, y1, 70, rowH);

    // Level slider - bottom, thin (8px height)
    levelSlider.setBounds(x, y2, getWidth() - x - 65, 8);

    // Remove button - right side
    removeButton.setBounds(getWidth() - 28, h / 2 - 12, 24, 24);
}

void AuxOutputComponent::sliderValueChanged(juce::Slider* slider)
{
    if (slider == &levelSlider)
    {
        auxBus->setReturnLevel(static_cast<float>(slider->getValue() / 100.0));
    }
    repaint();
}

void AuxOutputComponent::comboBoxChanged(juce::ComboBox* comboBox)
{
    if (comboBox == &deviceCombo)
    {
        auxBus->setOutputDevice(comboBox->getText());
        updateChannelOptions();
    }
    else if (comboBox == &channelCombo)
    {
        juce::String text = comboBox->getText();
        int channelStart = text.getIntValue() - 1;
        auxBus->setOutputChannelStart(std::max(0, channelStart));
        auxBus->setStereo(text.contains("Stereo"));
    }
}

void AuxOutputComponent::buttonClicked(juce::Button* button)
{
    if (button == &removeButton)
    {
        if (onRemoveAux)
            onRemoveAux(auxBus->getId());
    }
}

void AuxOutputComponent::timerCallback()
{
    float newLevel = auxBus->getOutputLevel();
    if (std::abs(newLevel - currentLevel) > 0.01f)
    {
        currentLevel = newLevel;
        repaint();
    }
}

void AuxOutputComponent::updateChannelOptions()
{
    channelCombo.clear();

    if (!deviceHandler) return;

    juce::String deviceName = deviceCombo.getText();
    if (deviceName == "None" || deviceName.isEmpty())
    {
        channelCombo.addItem("--", 1);
        channelCombo.setSelectedId(1);
        return;
    }

    auto options = deviceHandler->buildOutputChannelOptions(deviceName);
    int itemId = 1;
    for (const auto& option : options)
    {
        channelCombo.addItem(option, itemId++);
    }
    if (channelCombo.getNumItems() > 0)
        channelCombo.setSelectedId(1);
}

// =============================================================================
// UnifiedOutputSectionComponent - Combined Send Returns + Aux Outputs
// =============================================================================

UnifiedOutputSectionComponent::UnifiedOutputSectionComponent(AudioEngine* engine, AudioDeviceHandler* handler)
    : audioEngine(engine), deviceHandler(handler)
{
    // Create fixed Send Returns
    delayReturn = std::make_unique<SendReturnRowComponent>(engine->getDelayBus(), "Delay");
    grainReturn = std::make_unique<SendReturnRowComponent>(engine->getGrainBus(), "Grain");
    reverbReturn = std::make_unique<SendReturnRowComponent>(engine->getReverbBus(), "Reverb");

    addAndMakeVisible(*delayReturn);
    addAndMakeVisible(*grainReturn);
    addAndMakeVisible(*reverbReturn);

    // Viewport for dynamic aux outputs
    auxViewport.setViewedComponent(&auxContainer, false);
    auxViewport.setScrollBarsShown(true, false);  // Vertical scroll only
    addAndMakeVisible(auxViewport);

    addButton.setColour(juce::TextButton::buttonColourId, accent);
    addButton.setColour(juce::TextButton::textColourOffId, backgroundMid);
    addButton.onClick = [this] { addAuxOutput(); };
    addAndMakeVisible(addButton);

    setSize(280, 480);
}

void UnifiedOutputSectionComponent::paint(juce::Graphics& g)
{
    g.setColour(backgroundMid);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 8.0f);

    // Section title
    g.setColour(textLight);
    g.setFont(18.0f);
    g.drawText("Send Returns", 0, 10, getWidth(), 22, juce::Justification::centred);

    // Aux Outputs label (below send returns)
    int sendReturnHeight = 36 * 3 + 4 * 2;  // 3 rows + 2 spacings
    int auxLabelY = 36 + sendReturnHeight + 8;
    g.setColour(textLight);
    g.setFont(18.0f);
    g.drawText("Aux Outputs", 0, auxLabelY, getWidth(), 20, juce::Justification::centred);
}

void UnifiedOutputSectionComponent::resized()
{
    int margin = 8;
    int buttonHeight = 28;
    int sendRowHeight = 36;
    int spacing = 4;

    // Send Returns (3 rows)
    int y = 36;
    int rowWidth = getWidth() - margin * 2;

    delayReturn->setBounds(margin, y, rowWidth, sendRowHeight);
    y += sendRowHeight + spacing;

    grainReturn->setBounds(margin, y, rowWidth, sendRowHeight);
    y += sendRowHeight + spacing;

    reverbReturn->setBounds(margin, y, rowWidth, sendRowHeight);
    y += sendRowHeight + spacing;

    // Aux Outputs label space
    y += 24;

    // Viewport for dynamic aux outputs
    int viewportHeight = getHeight() - y - buttonHeight - margin * 2;
    auxViewport.setBounds(margin, y, rowWidth, viewportHeight);

    // Add button at bottom
    addButton.setBounds(margin, getHeight() - buttonHeight - margin, rowWidth, buttonHeight);

    updateLayout();
}

void UnifiedOutputSectionComponent::addAuxOutput()
{
    int auxId = audioEngine->addAuxBus();
    auto* auxBus = audioEngine->getAuxBus(auxId);

    if (auxBus)
    {
        auto component = std::make_unique<AuxOutputComponent>(auxBus, deviceHandler);
        component->onRemoveAux = [this](int id) { removeAuxOutput(id); };
        component->onNameChanged = [this]() {
            if (onAuxNameChanged)
                onAuxNameChanged();
        };
        auxContainer.addAndMakeVisible(*component);
        auxComponents.push_back(std::move(component));
        updateLayout();

        if (onAuxAdded)
            onAuxAdded();
    }
}

void UnifiedOutputSectionComponent::removeAuxOutput(int auxId)
{
    for (auto it = auxComponents.begin(); it != auxComponents.end(); ++it)
    {
        if ((*it)->getAuxBus()->getId() == auxId)
        {
            auxContainer.removeChildComponent(it->get());
            auxComponents.erase(it);
            break;
        }
    }

    audioEngine->removeAuxBus(auxId);
    updateLayout();

    if (onAuxRemoved)
        onAuxRemoved(auxId);
}

void UnifiedOutputSectionComponent::updateLayout()
{
    int rowHeight = 52;  // AuxOutput row height
    int spacing = 4;
    int containerWidth = auxViewport.getWidth() - 8;
    int totalHeight = static_cast<int>(auxComponents.size()) * (rowHeight + spacing);

    auxContainer.setSize(containerWidth, std::max(totalHeight, auxViewport.getHeight()));

    int y = 0;
    for (auto& comp : auxComponents)
    {
        comp->setBounds(0, y, containerWidth, rowHeight);
        y += rowHeight + spacing;
    }
}

} // namespace Kousaten
