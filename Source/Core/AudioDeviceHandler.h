/*
    Kousaten Mixer - Audio Device Handler
    Manages multiple audio devices for per-channel I/O
*/

#pragma once

#include <JuceHeader.h>
#include <map>
#include <memory>

namespace Kousaten {

// Information about an audio device
struct DeviceInfo
{
    juce::String name;
    juce::StringArray inputChannelNames;
    juce::StringArray outputChannelNames;
    int numInputChannels = 0;
    int numOutputChannels = 0;
    juce::Array<double> sampleRates;
};

// Manages audio device enumeration and I/O
class AudioDeviceHandler
{
public:
    AudioDeviceHandler();
    ~AudioDeviceHandler();

    // Initialize and scan for devices
    void initialize();

    // Get available devices
    juce::StringArray getInputDeviceNames() const;
    juce::StringArray getOutputDeviceNames() const;

    // Get device info
    DeviceInfo getInputDeviceInfo(const juce::String& deviceName) const;
    DeviceInfo getOutputDeviceInfo(const juce::String& deviceName) const;

    // Get channel names for a device
    juce::StringArray getInputChannelNames(const juce::String& deviceName) const;
    juce::StringArray getOutputChannelNames(const juce::String& deviceName) const;

    // Get channel count
    int getInputChannelCount(const juce::String& deviceName) const;
    int getOutputChannelCount(const juce::String& deviceName) const;

    // Build channel selection options (e.g., "1 (Mono)", "1-2 (Stereo)")
    juce::StringArray buildInputChannelOptions(const juce::String& deviceName) const;
    juce::StringArray buildOutputChannelOptions(const juce::String& deviceName) const;

    // Get the main device manager (for single-device mode fallback)
    juce::AudioDeviceManager& getDeviceManager() { return deviceManager; }

    // Rescan devices
    void rescanDevices();

private:
    juce::AudioDeviceManager deviceManager;

    std::map<juce::String, DeviceInfo> inputDevices;
    std::map<juce::String, DeviceInfo> outputDevices;

    void scanDevices();
    juce::StringArray buildChannelOptions(int numChannels) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioDeviceHandler)
};

} // namespace Kousaten
