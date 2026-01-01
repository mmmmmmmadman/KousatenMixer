/*
    Kousaten Mixer - Main Component
    Implementation
*/

#include "MainComponent.h"
#include "UI/ChannelStripComponent.h"  // For MinimalSliderLookAndFeel

MainComponent::MainComponent()
{
    // Initialize device handler
    deviceHandler.initialize();

    // Setup audio device manager - stereo input and output
    auto result = audioDeviceManager.initialiseWithDefaultDevices(2, 2);
    if (result.isNotEmpty())
    {
        DBG("Audio device error: " + result);
    }

    // Register as the single AudioIODeviceCallback
    audioDeviceManager.addAudioCallback(this);

    // Add channel button
    addChannelButton.setColour(juce::TextButton::buttonColourId, accent);
    addChannelButton.setColour(juce::TextButton::textColourOffId, backgroundDark);
    addChannelButton.onClick = [this] { addChannel(); };
    addAndMakeVisible(addChannelButton);

    // Channel viewport
    channelViewport.setViewedComponent(&channelContainer, false);
    channelViewport.setScrollBarsShown(false, true);
    addAndMakeVisible(channelViewport);

    // Unified Send Returns + Aux Outputs section
    auxOutputSection = std::make_unique<Kousaten::AuxOutputSectionComponent>(&audioEngine, &deviceHandler);
    auxOutputSection->onAuxAdded = [this]() { syncAllChannelAuxSends(); };
    auxOutputSection->onAuxRemoved = [this](int) { syncAllChannelAuxSends(); };
    auxOutputSection->onAuxNameChanged = [this]() { syncAllChannelAuxSends(); };
    addAndMakeVisible(*auxOutputSection);

    // Master volume slider - horizontal for bottom bar layout
    masterVolumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    masterVolumeSlider.setRange(0.0, 100.0, 0.1);
    masterVolumeSlider.setValue(100.0);
    masterVolumeSlider.setDoubleClickReturnValue(true, 100.0);
    masterVolumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    masterVolumeSlider.setColour(juce::Slider::backgroundColourId, backgroundMid);
    masterVolumeSlider.setColour(juce::Slider::trackColourId, accentDim);
    masterVolumeSlider.setColour(juce::Slider::thumbColourId, accent);
    masterVolumeSlider.setLookAndFeel(&Kousaten::getMinimalSliderLookAndFeel());
    masterVolumeSlider.onValueChange = [this] {
        audioEngine.setMasterVolume(static_cast<float>(masterVolumeSlider.getValue() / 100.0));
    };
    addAndMakeVisible(masterVolumeSlider);

    // Master device selection
    masterDeviceCombo.setColour(juce::ComboBox::backgroundColourId, backgroundLight);
    masterDeviceCombo.setColour(juce::ComboBox::textColourId, textLight);
    masterDeviceCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    masterDeviceCombo.onChange = [this] {
        juce::String selectedDevice = masterDeviceCombo.getText();
        audioEngine.setMasterOutputDevice(selectedDevice);

        // Try to change the audio output device
        auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
        if (currentDevice != nullptr && currentDevice->getName() != selectedDevice)
        {
            // Create new audio device setup with selected output
            juce::AudioDeviceManager::AudioDeviceSetup setup;
            audioDeviceManager.getAudioDeviceSetup(setup);
            setup.outputDeviceName = selectedDevice;

            // Apply the new setup
            juce::String error = audioDeviceManager.setAudioDeviceSetup(setup, true);
            if (error.isNotEmpty())
            {
                DBG("Error changing audio device: " + error);
            }
        }

        updateMasterChannelOptions();
    };
    addAndMakeVisible(masterDeviceCombo);

    // Master channel selection
    masterChannelCombo.setColour(juce::ComboBox::backgroundColourId, backgroundLight);
    masterChannelCombo.setColour(juce::ComboBox::textColourId, textLight);
    masterChannelCombo.setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    masterChannelCombo.onChange = [this] {
        juce::String text = masterChannelCombo.getText();
        int channelStart = text.getIntValue() - 1;
        audioEngine.setMasterOutputChannelStart(std::max(0, channelStart));
    };
    addAndMakeVisible(masterChannelCombo);

    // Populate master device list from audioDeviceManager
    {
        auto* currentDevice = audioDeviceManager.getCurrentAudioDevice();
        if (currentDevice != nullptr)
        {
            // Show current device name (read-only for now since changing device is complex)
            masterDeviceCombo.addItem(currentDevice->getName(), 1);
            masterDeviceCombo.setSelectedId(1);

            // Get available output devices for info
            auto& deviceTypes = audioDeviceManager.getAvailableDeviceTypes();
            for (auto* deviceType : deviceTypes)
            {
                auto deviceNames = deviceType->getDeviceNames(false);  // false = output devices
                for (int i = 0; i < deviceNames.size(); ++i)
                {
                    if (deviceNames[i] != currentDevice->getName())
                    {
                        masterDeviceCombo.addItem(deviceNames[i], masterDeviceCombo.getNumItems() + 1);
                    }
                }
            }
        }
        else
        {
            masterDeviceCombo.addItem("No Device", 1);
            masterDeviceCombo.setSelectedId(1);
        }
        updateMasterChannelOptions();
    }

    // Effect parameter sliders (horizontal, thin)
    auto setupEffectSlider = [this](juce::Slider& slider, double defaultVal) {
        slider.setSliderStyle(juce::Slider::LinearHorizontal);
        slider.setRange(0.0, 100.0, 0.1);
        slider.setValue(defaultVal);
        slider.setDoubleClickReturnValue(true, defaultVal);
        slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        slider.setColour(juce::Slider::backgroundColourId, backgroundLight);
        slider.setColour(juce::Slider::trackColourId, accentDim);
        slider.setColour(juce::Slider::thumbColourId, accent);
        slider.setLookAndFeel(&Kousaten::getMinimalSliderLookAndFeel());
        addAndMakeVisible(slider);
    };

    setupEffectSlider(delayTimeLSlider, 25.0);
    delayTimeLSlider.onValueChange = [this] {
        float timeL = static_cast<float>(delayTimeLSlider.getValue() / 100.0) * 2.0f;
        float timeR = static_cast<float>(delayTimeRSlider.getValue() / 100.0) * 2.0f;
        audioEngine.getDelayBus()->setDelayTime(timeL, timeR);
    };

    setupEffectSlider(delayTimeRSlider, 25.0);
    delayTimeRSlider.onValueChange = [this] {
        float timeL = static_cast<float>(delayTimeLSlider.getValue() / 100.0) * 2.0f;
        float timeR = static_cast<float>(delayTimeRSlider.getValue() / 100.0) * 2.0f;
        audioEngine.getDelayBus()->setDelayTime(timeL, timeR);
    };

    setupEffectSlider(delayFeedbackSlider, 30.0);
    delayFeedbackSlider.onValueChange = [this] {
        audioEngine.getDelayBus()->setDelayFeedback(static_cast<float>(delayFeedbackSlider.getValue() / 100.0) * 0.95f);
    };

    setupEffectSlider(grainSizeSlider, 30.0);
    grainSizeSlider.onValueChange = [this] {
        audioEngine.getGrainBus()->setGrainSize(static_cast<float>(grainSizeSlider.getValue() / 100.0));
    };

    setupEffectSlider(grainDensitySlider, 40.0);
    grainDensitySlider.onValueChange = [this] {
        audioEngine.getGrainBus()->setGrainDensity(static_cast<float>(grainDensitySlider.getValue() / 100.0));
    };

    setupEffectSlider(grainPositionSlider, 50.0);
    grainPositionSlider.onValueChange = [this] {
        audioEngine.getGrainBus()->setGrainPosition(static_cast<float>(grainPositionSlider.getValue() / 100.0));
    };

    setupEffectSlider(reverbRoomSlider, 50.0);
    reverbRoomSlider.onValueChange = [this] {
        audioEngine.getReverbBus()->setReverbRoomSize(static_cast<float>(reverbRoomSlider.getValue() / 100.0));
    };

    setupEffectSlider(reverbDampingSlider, 40.0);
    reverbDampingSlider.onValueChange = [this] {
        audioEngine.getReverbBus()->setReverbDamping(static_cast<float>(reverbDampingSlider.getValue() / 100.0));
    };

    setupEffectSlider(reverbDecaySlider, 60.0);
    reverbDecaySlider.onValueChange = [this] {
        audioEngine.getReverbBus()->setReverbDecay(static_cast<float>(reverbDecaySlider.getValue() / 100.0));
    };

    // Chaos controls
    chaosEnableButton.setColour(juce::ToggleButton::textColourId, textLight);
    chaosEnableButton.setColour(juce::ToggleButton::tickColourId, accent);
    chaosEnableButton.setColour(juce::ToggleButton::tickDisabledColourId, backgroundLight);
    chaosEnableButton.onStateChange = [this] {
        bool enabled = chaosEnableButton.getToggleState();
        audioEngine.getDelayBus()->setChaosEnabled(enabled);
        audioEngine.getGrainBus()->setChaosEnabled(enabled);
        audioEngine.getReverbBus()->setChaosEnabled(enabled);
    };
    addAndMakeVisible(chaosEnableButton);

    setupEffectSlider(chaosAmountSlider, 100.0);
    chaosAmountSlider.onValueChange = [this] {
        // Chaos amount is handled internally
    };

    setupEffectSlider(chaosRateSlider, 10.0);
    chaosRateSlider.onValueChange = [this] {
        float rate = static_cast<float>(chaosRateSlider.getValue() / 100.0);
        audioEngine.getDelayBus()->setChaosRate(rate);
        audioEngine.getGrainBus()->setChaosRate(rate);
        audioEngine.getReverbBus()->setChaosRate(rate);
    };

    chaosShapeButton.setColour(juce::ToggleButton::textColourId, textLight);
    chaosShapeButton.setColour(juce::ToggleButton::tickColourId, accent);
    chaosShapeButton.setColour(juce::ToggleButton::tickDisabledColourId, backgroundLight);
    addAndMakeVisible(chaosShapeButton);

    // Start UI refresh timer
    startTimerHz(30);

    // Set size LAST so resized() can position all components
    setSize(1600, 970);  // Increased height to accommodate 120px master bar
}

MainComponent::~MainComponent()
{
    stopTimer();
    audioDeviceManager.removeAudioCallback(this);
    audioDeviceManager.closeAudioDevice();
}

// AudioIODeviceCallback implementation - handles both input and output
void MainComponent::audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                                      int numInputChannels,
                                                      float* const* outputChannelData,
                                                      int numOutputChannels,
                                                      int numSamples,
                                                      const juce::AudioIODeviceCallbackContext& context)
{
    juce::ignoreUnused(context);

    // Copy input to pre-allocated buffer (no allocation in callback!)
    int inputChans = std::min(numInputChannels, inputBuffer.getNumChannels());
    int samplesToProcess = std::min(numSamples, inputBuffer.getNumSamples());

    for (int ch = 0; ch < inputChans; ++ch)
    {
        if (inputChannelData[ch] != nullptr)
        {
            juce::FloatVectorOperations::copy(inputBuffer.getWritePointer(ch),
                                               inputChannelData[ch],
                                               samplesToProcess);
        }
    }

    // Debug: check input level
    if (numInputChannels > 0 && inputChannelData[0] != nullptr)
    {
        float maxLevel = 0.0f;
        for (int i = 0; i < samplesToProcess; ++i)
        {
            maxLevel = std::max(maxLevel, std::abs(inputChannelData[0][i]));
        }
        inputLevel = maxLevel;
    }

    // Clear output buffer
    outputBuffer.clear(0, samplesToProcess);

    // Set input buffer for engine and process
    audioEngine.setInputBuffer(&inputBuffer);

    // Create AudioSourceChannelInfo for the engine
    juce::AudioSourceChannelInfo info(&outputBuffer, 0, samplesToProcess);
    audioEngine.getNextAudioBlock(info);

    // Copy output buffer to device output
    int outputChans = std::min(numOutputChannels, outputBuffer.getNumChannels());
    for (int ch = 0; ch < outputChans; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            juce::FloatVectorOperations::copy(outputChannelData[ch],
                                               outputBuffer.getReadPointer(ch),
                                               samplesToProcess);
        }
    }

    // Clear any extra output channels
    for (int ch = outputChans; ch < numOutputChannels; ++ch)
    {
        if (outputChannelData[ch] != nullptr)
        {
            juce::FloatVectorOperations::clear(outputChannelData[ch], numSamples);
        }
    }
}

void MainComponent::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    if (device != nullptr)
    {
        int numInputChannels = device->getActiveInputChannels().countNumberOfSetBits();
        int numOutputChannels = device->getActiveOutputChannels().countNumberOfSetBits();
        int blockSize = device->getCurrentBufferSizeSamples();
        double sampleRate = device->getCurrentSampleRate();

        inputBuffer.setSize(std::max(2, numInputChannels), blockSize);
        outputBuffer.setSize(std::max(2, numOutputChannels), blockSize);

        audioEngine.prepareToPlay(blockSize, sampleRate);
    }
}

void MainComponent::audioDeviceStopped()
{
    audioEngine.releaseResources();
    inputBuffer.setSize(0, 0);
    outputBuffer.setSize(0, 0);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(backgroundDark);

    // Title
    g.setColour(accent);
    g.setFont(28.0f);
    g.drawText("KOUSATEN Mixer", 20, 12, 300, 36, juce::Justification::left);

    // Subtitle - pink color, 20pt
    g.setColour(accent);
    g.setFont(20.0f);
    g.drawText("MADZINE", 20, 48, 120, 24, juce::Justification::left);

    // Channel count
    g.setColour(textDim);
    g.setFont(14.0f);
    g.drawText("Channels: " + juce::String(audioEngine.getChannelCount()),
               280, 52, 120, 20, juce::Justification::left);

    // Debug: Input level indicator
    g.setColour(textDim);
    g.drawText("Input: " + juce::String(inputLevel.load(), 3),
               150, 52, 100, 20, juce::Justification::left);

    // Draw master section
    drawMasterSection(g);
}

void MainComponent::drawMasterSection(juce::Graphics& g)
{
    int margin = 20;
    int masterHeight = 120;
    int masterY = getHeight() - masterHeight - 10;

    // Background - full width horizontal bar at bottom
    g.setColour(backgroundMid);
    g.fillRoundedRectangle(static_cast<float>(margin), static_cast<float>(masterY),
                           static_cast<float>(getWidth() - margin * 2), static_cast<float>(masterHeight), 8.0f);

    // === LEFT COLUMN: Master label + Volume slider ===
    // Master label at top
    g.setColour(accent);
    g.setFont(18.0f);
    g.drawText("Master", margin + 10, masterY + 12, 70, 22, juce::Justification::left);

    // Volume value (next to slider)
    g.setColour(accent);
    g.setFont(18.0f);
    g.drawText(juce::String(static_cast<int>(masterVolumeSlider.getValue())), margin + 190, masterY + 45, 40, 22, juce::Justification::left);

    // === RIGHT COLUMN: Device, Channel, L meter, R meter ===
    int rightColX = margin + 260;
    int rowH = 26;
    int row1Y = masterY + 10;
    int row2Y = row1Y + rowH;
    int row3Y = row2Y + rowH;
    int row4Y = row3Y + rowH;

    // Labels for meters
    g.setColour(textDim);
    g.setFont(14.0f);
    g.drawText("L", rightColX - 18, row3Y, 16, 20, juce::Justification::right);
    g.drawText("R", rightColX - 18, row4Y, 16, 20, juce::Justification::right);

    // Master meters (horizontal, in right column - extend to near effects section)
    float levelL = audioEngine.getMasterLevelLeft();
    float levelR = audioEngine.getMasterLevelRight();

    int fxStartX = margin + 480;  // Where effects section starts
    int meterWidth = fxStartX - rightColX - 20;  // Extend to near effects, with 20px gap
    int meterHeight = 10;

    // Left meter
    g.setColour(backgroundLight);
    g.fillRoundedRectangle(static_cast<float>(rightColX), static_cast<float>(row3Y + 5),
                           static_cast<float>(meterWidth), static_cast<float>(meterHeight), 2.0f);
    int fillWidth = static_cast<int>(levelL * meterWidth);
    if (fillWidth > 0)
    {
        g.setColour(accent);
        g.fillRoundedRectangle(static_cast<float>(rightColX + 1), static_cast<float>(row3Y + 6),
                               static_cast<float>(fillWidth - 2), static_cast<float>(meterHeight - 2), 1.0f);
    }

    // Right meter
    g.setColour(backgroundLight);
    g.fillRoundedRectangle(static_cast<float>(rightColX), static_cast<float>(row4Y + 5),
                           static_cast<float>(meterWidth), static_cast<float>(meterHeight), 2.0f);
    fillWidth = static_cast<int>(levelR * meterWidth);
    if (fillWidth > 0)
    {
        g.setColour(accent);
        g.fillRoundedRectangle(static_cast<float>(rightColX + 1), static_cast<float>(row4Y + 6),
                               static_cast<float>(fillWidth - 2), static_cast<float>(meterHeight - 2), 1.0f);
    }

    // Effect parameter labels - 18pt minimum, text left + slider right same row
    // Row layout: 4 rows * 24px = 96px content, with 12px top padding = 108px used within 120px bar
    g.setFont(18.0f);

    int fxX = margin + 480;
    int fxRowH = 24;
    int fxRow1Y = masterY + 12;  // 12px top padding
    int fxRow2Y = fxRow1Y + fxRowH;
    int fxRow3Y = fxRow2Y + fxRowH;
    int fxRow4Y = fxRow3Y + fxRowH;
    int labelW = 75;

    // Delay section (Time L, Time R, Feedback)
    g.setColour(accent);
    g.drawText("Delay", fxX, fxRow1Y, 50, 22, juce::Justification::left);
    g.setColour(textDim);
    g.drawText("Time L", fxX + 55, fxRow1Y, labelW, 22, juce::Justification::left);
    g.drawText("Time R", fxX + 55, fxRow2Y, labelW, 22, juce::Justification::left);
    g.drawText("Feedback", fxX + 55, fxRow3Y, labelW, 22, juce::Justification::left);

    // Grain section (Size, Density, Position)
    int grainX = fxX + 200;
    g.setColour(accent);
    g.drawText("Grain", grainX, fxRow1Y, 50, 22, juce::Justification::left);
    g.setColour(textDim);
    g.drawText("Size", grainX + 55, fxRow1Y, labelW, 22, juce::Justification::left);
    g.drawText("Density", grainX + 55, fxRow2Y, labelW, 22, juce::Justification::left);
    g.drawText("Position", grainX + 55, fxRow3Y, labelW, 22, juce::Justification::left);

    // Reverb section (Room, Damping, Decay)
    int reverbX = fxX + 400;
    g.setColour(accent);
    g.drawText("Reverb", reverbX, fxRow1Y, 60, 22, juce::Justification::left);
    g.setColour(textDim);
    g.drawText("Room", reverbX + 65, fxRow1Y, labelW, 22, juce::Justification::left);
    g.drawText("Damping", reverbX + 65, fxRow2Y, labelW, 22, juce::Justification::left);
    g.drawText("Decay", reverbX + 65, fxRow3Y, labelW, 22, juce::Justification::left);

    // Chaos section (Enable, Amount, Rate, Shape)
    int chaosX = fxX + 620;
    g.setColour(juce::Colour(0xffff8888));
    g.drawText("Chaos", chaosX, fxRow1Y, 55, 22, juce::Justification::left);
    g.setColour(textDim);
    g.drawText("Enable", chaosX + 60, fxRow1Y, labelW, 22, juce::Justification::left);
    g.drawText("Amount", chaosX + 60, fxRow2Y, labelW, 22, juce::Justification::left);
    g.drawText("Rate", chaosX + 60, fxRow3Y, labelW, 22, juce::Justification::left);
    g.drawText("Shape", chaosX + 60, fxRow4Y, labelW, 22, juce::Justification::left);
}

void MainComponent::resized()
{
    int margin = 20;
    int topBarHeight = 80;
    int rightPanelWidth = 300;
    int masterBarHeight = 120;  // Increased to fit 4 rows properly
    int bottomMargin = masterBarHeight + 20;

    // Add channel button - positioned to the right of subtitle
    addChannelButton.setBounds(400, 42, 140, 28);

    // Channel viewport - left side (most of the width)
    int channelAreaWidth = getWidth() - margin * 2 - rightPanelWidth - 10;
    int channelAreaHeight = getHeight() - topBarHeight - bottomMargin;
    channelViewport.setBounds(margin, topBarHeight, channelAreaWidth, channelAreaHeight);

    // Right panel - full height for outputs (minus master bar at bottom)
    int rightPanelX = getWidth() - rightPanelWidth - margin;
    int rightPanelHeight = getHeight() - topBarHeight - bottomMargin;

    // Unified Send Returns + Aux Outputs section - takes full right panel
    if (auxOutputSection)
    {
        auxOutputSection->setBounds(rightPanelX, topBarHeight, rightPanelWidth, rightPanelHeight);
    }

    // Master section - horizontal bar at bottom
    int masterY = getHeight() - masterBarHeight - 10;

    // === LEFT COLUMN: Volume slider below "Master" label ===
    // Volume slider - horizontal, below Master label
    masterVolumeSlider.setBounds(margin + 10, masterY + 42, 170, 8);

    // === RIGHT COLUMN: Device, Channel, L meter, R meter ===
    int rightColX = margin + 260;
    int rowH = 26;
    int row1Y = masterY + 10;
    int row2Y = row1Y + rowH;

    // Device and Channel combos in right column
    masterDeviceCombo.setBounds(rightColX, row1Y, 100, 22);
    masterChannelCombo.setBounds(rightColX, row2Y, 100, 22);

    // Effect parameter sliders - text left, slider right same row
    // Must match Y positions in drawMasterSection() for proper alignment
    int fxX = margin + 480;
    int fxRowH = 24;
    int fxRow1Y = masterY + 12;  // Match drawMasterSection row1Y
    int fxRow2Y = fxRow1Y + fxRowH;
    int fxRow3Y = fxRow2Y + fxRowH;
    int fxRow4Y = fxRow3Y + fxRowH;
    int sliderW = 50;
    int sliderH = 8;
    int sliderOffsetY = 7;  // Vertically center 8px slider in 22px text row: (22-8)/2 = 7

    // Delay: Time L, Time R, Feedback - slider after label
    delayTimeLSlider.setBounds(fxX + 130, fxRow1Y + sliderOffsetY, sliderW, sliderH);
    delayTimeRSlider.setBounds(fxX + 130, fxRow2Y + sliderOffsetY, sliderW, sliderH);
    delayFeedbackSlider.setBounds(fxX + 130, fxRow3Y + sliderOffsetY, sliderW, sliderH);

    // Grain: Size, Density, Position
    int grainX = fxX + 200;
    grainSizeSlider.setBounds(grainX + 130, fxRow1Y + sliderOffsetY, sliderW, sliderH);
    grainDensitySlider.setBounds(grainX + 130, fxRow2Y + sliderOffsetY, sliderW, sliderH);
    grainPositionSlider.setBounds(grainX + 130, fxRow3Y + sliderOffsetY, sliderW, sliderH);

    // Reverb: Room, Damping, Decay
    int reverbX = fxX + 400;
    reverbRoomSlider.setBounds(reverbX + 145, fxRow1Y + sliderOffsetY, sliderW, sliderH);
    reverbDampingSlider.setBounds(reverbX + 145, fxRow2Y + sliderOffsetY, sliderW, sliderH);
    reverbDecaySlider.setBounds(reverbX + 145, fxRow3Y + sliderOffsetY, sliderW, sliderH);

    // Chaos: Enable, Amount, Rate, Shape - checkbox size 24x24 for visibility
    int chaosX = fxX + 620;
    int checkboxSize = 24;
    int checkboxOffsetY = (22 - checkboxSize) / 2;  // Center in row (-1)
    chaosEnableButton.setBounds(chaosX + 135, fxRow1Y + checkboxOffsetY, checkboxSize, checkboxSize);
    chaosAmountSlider.setBounds(chaosX + 135, fxRow2Y + sliderOffsetY, sliderW, sliderH);
    chaosRateSlider.setBounds(chaosX + 135, fxRow3Y + sliderOffsetY, sliderW, sliderH);
    chaosShapeButton.setBounds(chaosX + 135, fxRow4Y + checkboxOffsetY, checkboxSize, checkboxSize);

    updateLayout();
}

void MainComponent::timerCallback()
{
    repaint();
}

void MainComponent::addChannel()
{
    int id = audioEngine.addChannel();
    if (id >= 0)
    {
        auto* channel = audioEngine.getChannel(id);
        if (channel)
        {
            auto strip = std::make_unique<Kousaten::ChannelStripComponent>(channel, &deviceHandler, &audioEngine);
            strip->onRemoveChannel = [this](int channelId) {
                removeChannel(channelId);
            };
            strip->onAddAuxRequested = [this](int) {
                auxOutputSection->addAuxOutput();
            };
            strip->syncAuxSends();
            channelContainer.addAndMakeVisible(*strip);
            channelStrips.push_back(std::move(strip));
            updateLayout();
        }
    }
}

void MainComponent::removeChannel(int channelId)
{
    // Find and remove the strip
    for (auto it = channelStrips.begin(); it != channelStrips.end(); ++it)
    {
        if ((*it)->getChannel()->getId() == channelId)
        {
            channelContainer.removeChildComponent(it->get());
            channelStrips.erase(it);
            break;
        }
    }

    // Remove from audio engine
    audioEngine.removeChannel(channelId);
    audioEngine.updateSoloState();

    updateLayout();
}

void MainComponent::updateLayout()
{
    int stripWidth = 240;
    int stripHeight = channelViewport.getHeight() - 10;
    int spacing = 6;

    int totalWidth = static_cast<int>(channelStrips.size()) * (stripWidth + spacing);
    channelContainer.setSize(std::max(totalWidth, channelViewport.getWidth()), stripHeight);

    int x = 0;
    for (auto& strip : channelStrips)
    {
        strip->setBounds(x, 0, stripWidth, stripHeight);
        x += stripWidth + spacing;
    }

    repaint();
}

void MainComponent::syncAllChannelAuxSends()
{
    for (auto& strip : channelStrips)
    {
        strip->syncAuxSends();
    }
}

void MainComponent::updateMasterChannelOptions()
{
    masterChannelCombo.clear();

    auto* device = audioDeviceManager.getCurrentAudioDevice();
    if (device == nullptr)
    {
        masterChannelCombo.addItem("No Output", 1);
        masterChannelCombo.setSelectedId(1);
        return;
    }

    // Get active output channels
    auto outputChannels = device->getActiveOutputChannels();
    int numChannels = outputChannels.countNumberOfSetBits();

    if (numChannels == 0)
    {
        masterChannelCombo.addItem("No Output", 1);
        masterChannelCombo.setSelectedId(1);
        return;
    }

    // Build channel options (stereo pairs)
    int itemId = 1;
    for (int i = 0; i < numChannels - 1; i += 2)
    {
        masterChannelCombo.addItem(juce::String(i + 1) + "-" + juce::String(i + 2) + " (Stereo)", itemId++);
    }

    // Add mono options if odd number of channels
    if (numChannels % 2 == 1)
    {
        masterChannelCombo.addItem(juce::String(numChannels) + " (Mono)", itemId++);
    }

    if (masterChannelCombo.getNumItems() > 0)
        masterChannelCombo.setSelectedId(1);
}
