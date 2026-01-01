/*
    Kousaten Mixer - Aux Bus
    Auxiliary output bus for routing to physical outputs
*/

#pragma once

#include <JuceHeader.h>

namespace Kousaten {

class AuxBus
{
public:
    AuxBus(int busId);

    int getId() const { return id; }

    const juce::String& getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }

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

private:
    int id;
    juce::String name;

    // Output routing
    juce::String outputDeviceName = "None";
    int outputChannelStart = 0;
    bool stereoMode = true;

    // Levels
    float returnLevel = 1.0f;
    float outputLevel = 0.0f;

    // Audio buffer
    juce::AudioBuffer<float> buffer;
    int currentBlockSize = 512;
    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AuxBus)
};

} // namespace Kousaten
