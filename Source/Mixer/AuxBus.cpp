/*
    Kousaten Mixer - Aux Bus
    Implementation
*/

#include "AuxBus.h"

namespace Kousaten {

AuxBus::AuxBus(int busId)
    : id(busId)
    , name("Aux " + juce::String(busId + 1))
{
    buffer.setSize(2, 512);
    buffer.clear();
}

void AuxBus::setOutputDevice(const juce::String& deviceName)
{
    outputDeviceName = deviceName;
}

void AuxBus::setOutputChannelStart(int channel)
{
    outputChannelStart = std::max(0, channel);
}

void AuxBus::setStereo(bool stereo)
{
    stereoMode = stereo;
}

void AuxBus::setReturnLevel(float level)
{
    returnLevel = juce::jlimit(0.0f, 1.0f, level);
}

void AuxBus::prepareToPlay(int samplesPerBlock, double sampleRate)
{
    currentBlockSize = samplesPerBlock;
    currentSampleRate = sampleRate;
    buffer.setSize(2, samplesPerBlock);
    buffer.clear();
}

void AuxBus::clearBuffer()
{
    buffer.clear();
}

void AuxBus::addToBuffer(const float* leftChannel, const float* rightChannel, int numSamples, float sendLevel)
{
    if (sendLevel <= 0.0f) return;

    auto* bufferL = buffer.getWritePointer(0);
    auto* bufferR = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        bufferL[i] += leftChannel[i] * sendLevel;
        bufferR[i] += rightChannel[i] * sendLevel;
    }
}

void AuxBus::process(float* outputLeft, float* outputRight, int numSamples)
{
    auto* bufferL = buffer.getReadPointer(0);
    auto* bufferR = buffer.getReadPointer(1);

    float maxLevel = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float left = bufferL[i] * returnLevel;
        float right = bufferR[i] * returnLevel;

        outputLeft[i] = left;
        outputRight[i] = right;

        maxLevel = std::max(maxLevel, std::abs(left));
        maxLevel = std::max(maxLevel, std::abs(right));
    }

    outputLevel = maxLevel;
}

} // namespace Kousaten
