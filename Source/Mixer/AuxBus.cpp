/*
    Kousaten Mixer - Aux Bus
    Implementation
*/

#include "AuxBus.h"
#include "../Core/RtAudioManager.h"

namespace Kousaten {

AuxBus::AuxBus(int busId)
    : id(busId)
    , name("Aux " + juce::String(busId + 1))
{
    buffer.setSize(2, 512);
    buffer.clear();
    processedBuffer.setSize(2, 512);
    processedBuffer.clear();
}

void AuxBus::setOutputDevice(const juce::String& deviceName)
{
    if (outputDeviceName != deviceName)
    {
        outputDeviceName = deviceName;
        updateRtStream();
    }
}

void AuxBus::setOutputChannelStart(int channel)
{
    // Allow -1 for "no output"
    outputChannelStart = std::max(-1, channel);
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
    processedBuffer.setSize(2, samplesPerBlock);
    processedBuffer.clear();

    // Update RtAudio stream with new parameters
    if (rtAudioManager != nullptr)
    {
        rtAudioManager->setSampleRate(static_cast<unsigned int>(sampleRate));
        rtAudioManager->setBufferSize(static_cast<unsigned int>(samplesPerBlock));
        updateRtStream();
    }
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

        // Store in processed buffer for RtAudio
        processedBuffer.getWritePointer(0)[i] = left;
        processedBuffer.getWritePointer(1)[i] = right;

        maxLevel = std::max(maxLevel, std::abs(left));
        maxLevel = std::max(maxLevel, std::abs(right));
    }

    outputLevel = maxLevel;
}

void AuxBus::sendToDevice(int numSamples)
{
    // Send to RtAudio device if stream is active
    if (rtAudioManager != nullptr && rtStreamId >= 0)
    {
        rtAudioManager->writeToStream(rtStreamId,
                                      processedBuffer.getReadPointer(0),
                                      processedBuffer.getReadPointer(1),
                                      numSamples);
    }
}

void AuxBus::updateRtStream()
{
    if (rtAudioManager == nullptr) return;

    // Capture current state for async operation
    juce::String deviceName = outputDeviceName;
    int channelStart = outputChannelStart;
    bool stereo = stereoMode;
    int busId = id;

    // Use async switching to prevent UI thread blocking
    rtAudioManager->switchDeviceAsync([this, deviceName, channelStart, stereo, busId]() {
        // Destroy existing stream
        if (rtStreamId >= 0)
        {
            rtAudioManager->destroyOutputStream(rtStreamId);
            rtStreamId = -1;
        }

        // Create new stream if device is valid
        if (deviceName.isNotEmpty() && deviceName != "None")
        {
            rtStreamId = rtAudioManager->createOutputStream(
                deviceName,
                static_cast<unsigned int>(std::max(0, channelStart)),
                stereo ? 2 : 1
            );

            if (rtStreamId >= 0)
            {
                DBG("AuxBus " + juce::String(busId) + ": Created RtAudio stream " +
                    juce::String(rtStreamId) + " for device: " + deviceName);
            }
        }
    });
}

} // namespace Kousaten
