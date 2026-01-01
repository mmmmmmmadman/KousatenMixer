/*
    Kousaten Mixer - Audio Engine
    Implementation
*/

#include "AudioEngine.h"

namespace Kousaten {

AudioEngine::AudioEngine()
{
    smoothedMasterVolume.setCurrentAndTargetValue(masterVolume);
}

AudioEngine::~AudioEngine()
{
    releaseResources();
}

void AudioEngine::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlockExpected;

    smoothedMasterVolume.reset(sampleRate, 0.02);

    // Prepare send buses
    delayBus.prepare(sampleRate, samplesPerBlockExpected);
    grainBus.prepare(sampleRate, samplesPerBlockExpected);
    reverbBus.prepare(sampleRate, samplesPerBlockExpected);

    // Prepare aux buses
    for (auto& auxBus : auxBuses)
    {
        auxBus->prepareToPlay(samplesPerBlockExpected, sampleRate);
    }

    // Allocate temporary buffers
    tempBuffer.setSize(2, samplesPerBlockExpected);
    delaySendBuffer.setSize(2, samplesPerBlockExpected);
    grainSendBuffer.setSize(2, samplesPerBlockExpected);
    reverbSendBuffer.setSize(2, samplesPerBlockExpected);
    delayReturnBuffer.setSize(2, samplesPerBlockExpected);
    grainReturnBuffer.setSize(2, samplesPerBlockExpected);
    reverbReturnBuffer.setSize(2, samplesPerBlockExpected);

    // Per-channel send buffers (pre-allocated to avoid allocation in audio callback)
    channelDelaySendBuffer.setSize(2, samplesPerBlockExpected);
    channelGrainSendBuffer.setSize(2, samplesPerBlockExpected);
    channelReverbSendBuffer.setSize(2, samplesPerBlockExpected);
}

void AudioEngine::releaseResources()
{
    tempBuffer.setSize(0, 0);
    delaySendBuffer.setSize(0, 0);
    grainSendBuffer.setSize(0, 0);
    reverbSendBuffer.setSize(0, 0);
    delayReturnBuffer.setSize(0, 0);
    grainReturnBuffer.setSize(0, 0);
    reverbReturnBuffer.setSize(0, 0);
    channelDelaySendBuffer.setSize(0, 0);
    channelGrainSendBuffer.setSize(0, 0);
    channelReverbSendBuffer.setSize(0, 0);
}

void AudioEngine::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    auto* outputBuffer = bufferToFill.buffer;
    auto numSamples = bufferToFill.numSamples;
    auto startSample = bufferToFill.startSample;

    // Clear output and send buffers
    outputBuffer->clear(startSample, numSamples);
    delaySendBuffer.clear();
    grainSendBuffer.clear();
    reverbSendBuffer.clear();

    // Clear aux bus buffers
    for (auto& auxBus : auxBuses)
    {
        auxBus->clearBuffer();
    }

    // Process each channel
    for (auto& channel : channels)
    {
        // Skip muted channels (unless solo is active and this channel is soloed)
        if (soloActive && !channel->isSoloed())
            continue;

        // Copy input audio to temp buffer based on channel's input settings
        int inputStart = channel->getInputChannelStart();
        bool stereo = channel->isStereo();

        tempBuffer.clear();

        // Only process if input is selected (inputStart >= 0)
        if (inputStart >= 0 && inputBuffer != nullptr && inputBuffer->getNumChannels() > 0)
        {
            // Copy from input buffer to temp buffer
            if (inputStart < inputBuffer->getNumChannels())
            {
                // Left channel (or mono)
                for (int i = 0; i < numSamples; ++i)
                {
                    tempBuffer.getWritePointer(0)[i] = inputBuffer->getReadPointer(inputStart)[i];
                }

                // Right channel (if stereo and available)
                if (stereo && inputStart + 1 < inputBuffer->getNumChannels())
                {
                    for (int i = 0; i < numSamples; ++i)
                    {
                        tempBuffer.getWritePointer(1)[i] = inputBuffer->getReadPointer(inputStart + 1)[i];
                    }
                }
                else
                {
                    // Mono: copy left to right
                    for (int i = 0; i < numSamples; ++i)
                    {
                        tempBuffer.getWritePointer(1)[i] = tempBuffer.getReadPointer(0)[i];
                    }
                }
            }
        }
        // If inputStart < 0, tempBuffer remains silent (cleared above)

        // Process channel
        float* channelOutL = tempBuffer.getWritePointer(0);
        float* channelOutR = tempBuffer.getWritePointer(1);
        float* delaySendL = delaySendBuffer.getWritePointer(0);
        float* delaySendR = delaySendBuffer.getWritePointer(1);
        float* grainSendL = grainSendBuffer.getWritePointer(0);
        float* grainSendR = grainSendBuffer.getWritePointer(1);
        float* reverbSendL = reverbSendBuffer.getWritePointer(0);
        float* reverbSendR = reverbSendBuffer.getWritePointer(1);

        // Clear pre-allocated per-channel send buffers (no allocation!)
        channelDelaySendBuffer.clear(0, numSamples);
        channelGrainSendBuffer.clear(0, numSamples);
        channelReverbSendBuffer.clear(0, numSamples);

        channel->process(tempBuffer.getReadPointer(0), tempBuffer.getReadPointer(1),
                        channelOutL, channelOutR,
                        channelDelaySendBuffer.getWritePointer(0), channelDelaySendBuffer.getWritePointer(1),
                        channelGrainSendBuffer.getWritePointer(0), channelGrainSendBuffer.getWritePointer(1),
                        channelReverbSendBuffer.getWritePointer(0), channelReverbSendBuffer.getWritePointer(1),
                        numSamples);

        // Sum to output
        for (int i = 0; i < numSamples; ++i)
        {
            outputBuffer->getWritePointer(0)[startSample + i] += channelOutL[i];
            outputBuffer->getWritePointer(1)[startSample + i] += channelOutR[i];
        }

        // Sum to send buffers (using pre-allocated buffers)
        for (int i = 0; i < numSamples; ++i)
        {
            delaySendL[i] += channelDelaySendBuffer.getReadPointer(0)[i];
            delaySendR[i] += channelDelaySendBuffer.getReadPointer(1)[i];
            grainSendL[i] += channelGrainSendBuffer.getReadPointer(0)[i];
            grainSendR[i] += channelGrainSendBuffer.getReadPointer(1)[i];
            reverbSendL[i] += channelReverbSendBuffer.getReadPointer(0)[i];
            reverbSendR[i] += channelReverbSendBuffer.getReadPointer(1)[i];
        }

        // Send to aux buses
        for (auto& auxBus : auxBuses)
        {
            float auxSendLevel = channel->getAuxSend(auxBus->getId());
            if (auxSendLevel > 0.0f)
            {
                auxBus->addToBuffer(channelOutL, channelOutR, numSamples, auxSendLevel);
            }
        }
    }

    // Process send buses
    delayBus.process(delaySendBuffer.getReadPointer(0), delaySendBuffer.getReadPointer(1),
                     delayReturnBuffer.getWritePointer(0), delayReturnBuffer.getWritePointer(1),
                     numSamples);

    grainBus.process(grainSendBuffer.getReadPointer(0), grainSendBuffer.getReadPointer(1),
                     grainReturnBuffer.getWritePointer(0), grainReturnBuffer.getWritePointer(1),
                     numSamples);

    reverbBus.process(reverbSendBuffer.getReadPointer(0), reverbSendBuffer.getReadPointer(1),
                      reverbReturnBuffer.getWritePointer(0), reverbReturnBuffer.getWritePointer(1),
                      numSamples);

    // Sum returns to output
    float maxLeft = 0.0f;
    float maxRight = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float left = outputBuffer->getReadPointer(0)[startSample + i];
        float right = outputBuffer->getReadPointer(1)[startSample + i];

        // Add effect returns
        left += delayReturnBuffer.getReadPointer(0)[i];
        left += grainReturnBuffer.getReadPointer(0)[i];
        left += reverbReturnBuffer.getReadPointer(0)[i];

        right += delayReturnBuffer.getReadPointer(1)[i];
        right += grainReturnBuffer.getReadPointer(1)[i];
        right += reverbReturnBuffer.getReadPointer(1)[i];

        // Apply master volume
        float vol = smoothedMasterVolume.getNextValue();
        left *= vol;
        right *= vol;

        // Soft clip
        left = std::tanh(left);
        right = std::tanh(right);

        outputBuffer->getWritePointer(0)[startSample + i] = left;
        outputBuffer->getWritePointer(1)[startSample + i] = right;

        maxLeft = std::max(maxLeft, std::abs(left));
        maxRight = std::max(maxRight, std::abs(right));
    }

    masterLevelLeft = maxLeft;
    masterLevelRight = maxRight;
}

int AudioEngine::addChannel()
{
    if (channels.size() >= MAX_CHANNELS)
        return -1;

    // Find the lowest available ID (reuse IDs from removed channels)
    int id = 0;
    while (true)
    {
        bool idUsed = false;
        for (const auto& ch : channels)
        {
            if (ch->getId() == id)
            {
                idUsed = true;
                break;
            }
        }
        if (!idUsed)
            break;
        id++;
    }

    channels.push_back(std::make_unique<Channel>(id));
    return id;
}

void AudioEngine::removeChannel(int channelId)
{
    channels.erase(
        std::remove_if(channels.begin(), channels.end(),
                       [channelId](const std::unique_ptr<Channel>& ch) {
                           return ch->getId() == channelId;
                       }),
        channels.end());

    updateSoloState();
}

Channel* AudioEngine::getChannel(int channelId)
{
    for (auto& channel : channels)
    {
        if (channel->getId() == channelId)
            return channel.get();
    }
    return nullptr;
}

void AudioEngine::setMasterVolume(float volume)
{
    masterVolume = juce::jlimit(0.0f, 1.0f, volume);
    smoothedMasterVolume.setTargetValue(masterVolume);
}

void AudioEngine::updateSoloState()
{
    soloActive = false;
    for (const auto& channel : channels)
    {
        if (channel->isSoloed())
        {
            soloActive = true;
            break;
        }
    }
}

int AudioEngine::addAuxBus()
{
    int id = nextAuxId++;
    auto auxBus = std::make_unique<AuxBus>(id);
    auxBus->prepareToPlay(currentBlockSize, currentSampleRate);
    auxBuses.push_back(std::move(auxBus));
    return id;
}

void AudioEngine::removeAuxBus(int auxId)
{
    // Remove aux send from all channels
    for (auto& channel : channels)
    {
        channel->removeAuxSend(auxId);
    }

    // Remove the aux bus
    auxBuses.erase(
        std::remove_if(auxBuses.begin(), auxBuses.end(),
                       [auxId](const std::unique_ptr<AuxBus>& bus) {
                           return bus->getId() == auxId;
                       }),
        auxBuses.end());
}

AuxBus* AudioEngine::getAuxBus(int auxId)
{
    for (auto& auxBus : auxBuses)
    {
        if (auxBus->getId() == auxId)
            return auxBus.get();
    }
    return nullptr;
}

} // namespace Kousaten
