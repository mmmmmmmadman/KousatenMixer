/*
    Kousaten Mixer - Aux Bus
    Auxiliary output bus for routing to physical outputs
*/

#pragma once

#include <JuceHeader.h>

namespace Kousaten {

// Forward declaration
class RtAudioManager;

class AuxBus
{
public:
    AuxBus(int busId);

    int getId() const { return id; }

    const juce::String& getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }

    // RtAudio manager for multi-device output
    void setRtAudioManager(RtAudioManager* manager) { rtAudioManager = manager; }

    // Output routing
    void setOutputDevice(const juce::String& deviceName);
    void setOutputChannelStart(int channel);
    void setStereo(bool stereo);

    const juce::String& getOutputDevice() const { return outputDeviceName; }
    int getOutputChannelStart() const { return outputChannelStart; }
    bool isStereo() const { return stereoMode; }

    // Return level (master fader for this aux)
    void setReturnLevel(float level);
    float getReturnLevel() const { return returnLevel; }

    // Metering
    float getOutputLevel() const { return outputLevel; }

    // Audio processing
    void prepareToPlay(int samplesPerBlock, double sampleRate);
    void clearBuffer();
    void addToBuffer(const float* leftChannel, const float* rightChannel, int numSamples, float sendLevel);
    void process(float* outputLeft, float* outputRight, int numSamples);

    // Send processed audio to RtAudio device
    void sendToDevice(int numSamples);

private:
    int id;
    juce::String name;

    // RtAudio manager
    RtAudioManager* rtAudioManager = nullptr;
    int rtStreamId = -1;  // -1 = no stream

    // Output routing
    juce::String outputDeviceName = "None";
    int outputChannelStart = -1;  // -1 = no output
    bool stereoMode = true;

    // Levels
    float returnLevel = 1.0f;
    float outputLevel = 0.0f;

    // Audio buffer
    juce::AudioBuffer<float> buffer;
    juce::AudioBuffer<float> processedBuffer;  // For sending to RtAudio
    int currentBlockSize = 512;
    double currentSampleRate = 44100.0;

    void updateRtStream();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxBus)
};

} // namespace Kousaten
