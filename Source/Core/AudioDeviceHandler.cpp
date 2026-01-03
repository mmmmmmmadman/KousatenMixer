/*
    Kousaten Mixer - Audio Device Handler
    Implementation
*/

#include "AudioDeviceHandler.h"

namespace Kousaten {

AudioDeviceHandler::AudioDeviceHandler()
{
}

AudioDeviceHandler::~AudioDeviceHandler()
{
    deviceManager.closeAudioDevice();
}

void AudioDeviceHandler::initialize()
{
    // Initialize device manager with default settings
    auto result = deviceManager.initialiseWithDefaultDevices(2, 2);
    if (result.isNotEmpty())
    {
        DBG("Audio device init error: " + result);
    }

    scanDevices();
}

void AudioDeviceHandler::scanDevices()
{
    inputDevices.clear();
    outputDevices.clear();

    auto& deviceTypes = deviceManager.getAvailableDeviceTypes();

    for (auto* deviceType : deviceTypes)
    {
        deviceType->scanForDevices();

        // Scan input devices
        auto inputNames = deviceType->getDeviceNames(true);
        for (const auto& name : inputNames)
        {
            DeviceInfo info;
            info.name = name;

            // Get channel info by temporarily creating device
            if (auto* device = deviceType->createDevice(name, juce::String()))
            {
                auto inputChannels = device->getInputChannelNames();
                info.inputChannelNames = inputChannels;
                info.numInputChannels = inputChannels.size();
                info.sampleRates = device->getAvailableSampleRates();
                delete device;
            }

            inputDevices[name] = info;
        }

        // Scan output devices
        auto outputNames = deviceType->getDeviceNames(false);
        for (const auto& name : outputNames)
        {
            DeviceInfo info;
            info.name = name;

            // Get channel info
            if (auto* device = deviceType->createDevice(juce::String(), name))
            {
                auto outputChannels = device->getOutputChannelNames();
                info.outputChannelNames = outputChannels;
                info.numOutputChannels = outputChannels.size();
                info.sampleRates = device->getAvailableSampleRates();
                delete device;
            }

            outputDevices[name] = info;
        }
    }
}

void AudioDeviceHandler::rescanDevices()
{
    scanDevices();
}

juce::StringArray AudioDeviceHandler::getInputDeviceNames() const
{
    juce::StringArray names;
    names.add("None");  // Option for no input

    for (const auto& [name, info] : inputDevices)
    {
        names.add(name);
    }

    return names;
}

juce::StringArray AudioDeviceHandler::getOutputDeviceNames() const
{
    juce::StringArray names;
    names.add("None");  // Option for no output (use master)

    for (const auto& [name, info] : outputDevices)
    {
        names.add(name);
    }

    return names;
}

DeviceInfo AudioDeviceHandler::getInputDeviceInfo(const juce::String& deviceName) const
{
    auto it = inputDevices.find(deviceName);
    if (it != inputDevices.end())
        return it->second;
    return DeviceInfo();
}

DeviceInfo AudioDeviceHandler::getOutputDeviceInfo(const juce::String& deviceName) const
{
    auto it = outputDevices.find(deviceName);
    if (it != outputDevices.end())
        return it->second;
    return DeviceInfo();
}

juce::StringArray AudioDeviceHandler::getInputChannelNames(const juce::String& deviceName) const
{
    auto info = getInputDeviceInfo(deviceName);
    return info.inputChannelNames;
}

juce::StringArray AudioDeviceHandler::getOutputChannelNames(const juce::String& deviceName) const
{
    auto info = getOutputDeviceInfo(deviceName);
    return info.outputChannelNames;
}

int AudioDeviceHandler::getInputChannelCount(const juce::String& deviceName) const
{
    auto info = getInputDeviceInfo(deviceName);
    return info.numInputChannels;
}

int AudioDeviceHandler::getOutputChannelCount(const juce::String& deviceName) const
{
    auto info = getOutputDeviceInfo(deviceName);
    return info.numOutputChannels;
}

juce::StringArray AudioDeviceHandler::buildChannelOptions(int numChannels) const
{
    juce::StringArray options;

    // First option: No Input
    options.add("No Input");

    // Mono options
    for (int i = 0; i < numChannels; ++i)
    {
        options.add(juce::String(i + 1) + " (Mono)");
    }

    // Stereo options (pairs)
    for (int i = 0; i < numChannels - 1; i += 2)
    {
        options.add(juce::String(i + 1) + "-" + juce::String(i + 2) + " (Stereo)");
    }

    // Also allow odd stereo pairs if needed
    if (numChannels > 2)
    {
        for (int i = 1; i < numChannels - 1; i += 2)
        {
            options.add(juce::String(i + 1) + "-" + juce::String(i + 2) + " (Stereo)");
        }
    }

    return options;
}

juce::StringArray AudioDeviceHandler::buildInputChannelOptions(const juce::String& deviceName) const
{
    int numChannels = getInputChannelCount(deviceName);

    // If scan didn't detect channels, try to get from current device or provide defaults
    if (numChannels == 0)
    {
        // Try to get from the actual device via device manager
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        if (currentDevice != nullptr)
        {
            auto inputChannels = currentDevice->getInputChannelNames();
            numChannels = static_cast<int>(inputChannels.size());
        }
    }

    // If still 0, provide generous fallback (8 channels covers most interfaces)
    if (numChannels == 0)
    {
        numChannels = 8;
    }

    return buildChannelOptions(numChannels);
}

juce::StringArray AudioDeviceHandler::buildOutputChannelOptions(const juce::String& deviceName) const
{
    int numChannels = getOutputChannelCount(deviceName);

    // If scan didn't detect channels, try to get from current device or provide defaults
    if (numChannels == 0)
    {
        auto* currentDevice = deviceManager.getCurrentAudioDevice();
        if (currentDevice != nullptr)
        {
            auto outputChannels = currentDevice->getOutputChannelNames();
            numChannels = static_cast<int>(outputChannels.size());
        }
    }

    // Multi-channel interfaces often report fewer channels than available
    // Use minimum of 8 for known multi-channel devices or as fallback
    if (numChannels < 8)
    {
        // Check for known multi-channel device names
        if (deviceName.containsIgnoreCase("ES-") ||
            deviceName.containsIgnoreCase("Focusrite") ||
            deviceName.containsIgnoreCase("MOTU") ||
            deviceName.containsIgnoreCase("RME") ||
            deviceName.containsIgnoreCase("Aggregate") ||
            deviceName.containsIgnoreCase("Multi-Output"))
        {
            numChannels = 16;  // Generous default for pro interfaces
        }
        else if (numChannels == 0)
        {
            numChannels = 8;  // General fallback
        }
    }

    return buildChannelOptions(numChannels);
}

juce::String AudioDeviceHandler::getCurrentOutputDeviceName() const
{
    auto* device = deviceManager.getCurrentAudioDevice();
    if (device != nullptr)
        return device->getName();
    return {};
}

} // namespace Kousaten
