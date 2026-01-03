/*
    Kousaten Mixer - Channel
    Single mixer channel with volume, pan, mute, solo, and sends
*/

#pragma once

#include <JuceHeader.h>
#include "SendPanner.h"
#include <map>
#include <memory>

namespace Kousaten {

class Channel
{
public:
    Channel(int channelId = 0);

    void setVolume(float volume);
    void setPan(float pan);  // -1.0 (L) to 1.0 (R)
    void setMute(bool mute);
    void setSolo(bool solo);

    void setDelaySend(float amount);
    void setGrainSend(float amount);
    void setReverbSend(float amount);

    float getVolume() const { return volume; }
    float getPan() const { return pan; }
    bool isMuted() const { return muted; }
    bool isSoloed() const { return soloed; }

    float getDelaySend() const { return delaySend; }
    float getGrainSend() const { return grainSend; }
    float getReverbSend() const { return reverbSend; }

    // Dynamic aux sends
    void setAuxSend(int auxId, float amount);
    float getAuxSend(int auxId) const;
    void removeAuxSend(int auxId);
    const std::map<int, float>& getAllAuxSends() const { return auxSends; }

    // Send Panner for dynamic aux send distribution
    SendPanner* getSendPanner() { return &sendPanner; }
    const SendPanner* getSendPanner() const { return &sendPanner; }

    // Get panned aux send levels (combines static levels with panner modulation)
    std::map<int, float> getPannedAuxSendLevels() const;

    float getInputLevel() const { return inputLevel; }
    float getOutputLevel() const { return outputLevel; }

    int getId() const { return id; }
    const juce::String& getName() const { return name; }
    void setName(const juce::String& newName) { name = newName; }

    // Audio input settings
    void setInputDevice(const juce::String& deviceName);
    void setInputChannelStart(int channel);
    void setStereo(bool stereo);

    const juce::String& getInputDevice() const { return inputDeviceName; }
    int getInputChannelStart() const { return inputChannelStart; }
    bool isStereo() const { return stereoMode; }

    // Process audio and return outputs
    // Returns: direct output, delay send, grain send, reverb send
    void process(const float* inputLeft, const float* inputRight,
                 float* outputLeft, float* outputRight,
                 float* delaySendLeft, float* delaySendRight,
                 float* grainSendLeft, float* grainSendRight,
                 float* reverbSendLeft, float* reverbSendRight,
                 int numSamples);

private:
    int id;
    juce::String name;

    float volume = 0.8f;
    float pan = 0.0f;
    bool muted = false;
    bool soloed = false;

    float delaySend = 0.0f;
    float grainSend = 0.0f;
    float reverbSend = 0.0f;

    // Dynamic aux sends (auxId -> level)
    std::map<int, float> auxSends;

    // Send Panner for dynamic distribution
    SendPanner sendPanner;

    float inputLevel = 0.0f;
    float outputLevel = 0.0f;

    // Audio input settings
    juce::String inputDeviceName = "None";
    int inputChannelStart = -1;  // -1 = no input selected
    bool stereoMode = true;

    // Smoothed parameters to avoid clicks
    juce::SmoothedValue<float> smoothedVolume;
    juce::SmoothedValue<float> smoothedPan;
};

} // namespace Kousaten
