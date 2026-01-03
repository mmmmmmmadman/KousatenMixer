/*
    Kousaten Mixer - Channel
    Implementation
*/

#include "Channel.h"

namespace Kousaten {

Channel::Channel(int channelId)
    : id(channelId)
    , name("Channel " + juce::String(channelId + 1))
{
    smoothedVolume.setCurrentAndTargetValue(volume);
    smoothedPan.setCurrentAndTargetValue(pan);
}

void Channel::setVolume(float newVolume)
{
    volume = juce::jlimit(0.0f, 1.0f, newVolume);
    smoothedVolume.setTargetValue(volume);
}

void Channel::setPan(float newPan)
{
    pan = juce::jlimit(-1.0f, 1.0f, newPan);
    smoothedPan.setTargetValue(pan);
}

void Channel::setMute(bool mute)
{
    muted = mute;
}

void Channel::setSolo(bool solo)
{
    soloed = solo;
}

void Channel::setDelaySend(float amount)
{
    delaySend = juce::jlimit(0.0f, 1.0f, amount);
}

void Channel::setGrainSend(float amount)
{
    grainSend = juce::jlimit(0.0f, 1.0f, amount);
}

void Channel::setReverbSend(float amount)
{
    reverbSend = juce::jlimit(0.0f, 1.0f, amount);
}

void Channel::setAuxSend(int auxId, float amount)
{
    auxSends[auxId] = juce::jlimit(0.0f, 1.0f, amount);
}

float Channel::getAuxSend(int auxId) const
{
    auto it = auxSends.find(auxId);
    if (it != auxSends.end())
        return it->second;
    return 0.0f;
}

void Channel::removeAuxSend(int auxId)
{
    auxSends.erase(auxId);
    sendPanner.removeAuxPosition(auxId);
}

std::map<int, float> Channel::getPannedAuxSendLevels() const
{
    std::map<int, float> result;

    if (!sendPanner.isEnabled())
    {
        // Return static aux send levels when panner is disabled
        return auxSends;
    }

    // Get panner distribution weights
    auto pannerLevels = sendPanner.calculateSendLevels();

    // Combine static levels with panner modulation
    for (const auto& [auxId, staticLevel] : auxSends)
    {
        float pannerWeight = 1.0f;
        auto it = pannerLevels.find(auxId);
        if (it != pannerLevels.end())
        {
            pannerWeight = it->second;
        }

        // Multiply static level by panner weight
        result[auxId] = staticLevel * pannerWeight;
    }

    return result;
}

void Channel::setInputDevice(const juce::String& deviceName)
{
    inputDeviceName = deviceName;
}

void Channel::setInputChannelStart(int channel)
{
    // Allow -1 for "no input selected"
    inputChannelStart = std::max(-1, channel);
}

void Channel::setStereo(bool stereo)
{
    stereoMode = stereo;
}

void Channel::process(const float* inputLeft, const float* inputRight,
                      float* outputLeft, float* outputRight,
                      float* delaySendLeft, float* delaySendRight,
                      float* grainSendLeft, float* grainSendRight,
                      float* reverbSendLeft, float* reverbSendRight,
                      int numSamples)
{
    // Update input level
    float maxInput = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        maxInput = std::max(maxInput, std::abs(inputLeft[i]));
        maxInput = std::max(maxInput, std::abs(inputRight[i]));
    }
    inputLevel = maxInput;

    // If muted, output silence
    if (muted)
    {
        std::fill(outputLeft, outputLeft + numSamples, 0.0f);
        std::fill(outputRight, outputRight + numSamples, 0.0f);
        std::fill(delaySendLeft, delaySendLeft + numSamples, 0.0f);
        std::fill(delaySendRight, delaySendRight + numSamples, 0.0f);
        std::fill(grainSendLeft, grainSendLeft + numSamples, 0.0f);
        std::fill(grainSendRight, grainSendRight + numSamples, 0.0f);
        std::fill(reverbSendLeft, reverbSendLeft + numSamples, 0.0f);
        std::fill(reverbSendRight, reverbSendRight + numSamples, 0.0f);
        outputLevel = 0.0f;
        return;
    }

    float maxOutput = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float vol = smoothedVolume.getNextValue();
        float p = smoothedPan.getNextValue();

        // Apply volume
        float left = inputLeft[i] * vol;
        float right = inputRight[i] * vol;

        // Apply pan (constant power panning)
        float leftGain = std::cos((p + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        float rightGain = std::sin((p + 1.0f) * 0.25f * juce::MathConstants<float>::pi);

        left *= leftGain;
        right *= rightGain;

        // Direct output
        outputLeft[i] = left;
        outputRight[i] = right;

        // Send outputs (post-fader, post-pan)
        delaySendLeft[i] = left * delaySend;
        delaySendRight[i] = right * delaySend;
        grainSendLeft[i] = left * grainSend;
        grainSendRight[i] = right * grainSend;
        reverbSendLeft[i] = left * reverbSend;
        reverbSendRight[i] = right * reverbSend;

        maxOutput = std::max(maxOutput, std::abs(left));
        maxOutput = std::max(maxOutput, std::abs(right));
    }

    outputLevel = maxOutput;

    // Update send panner automation (for non-XYPad modes)
    sendPanner.process(numSamples, 48000.0);  // TODO: pass actual sample rate
}

} // namespace Kousaten
