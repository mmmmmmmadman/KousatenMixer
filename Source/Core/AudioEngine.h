/*
    Kousaten Mixer - Audio Engine
    Main audio processing engine with mixer and effects
*/

#pragma once

#include <JuceHeader.h>
#include "../Mixer/Channel.h"
#include "../Mixer/MixBus.h"
#include "../Mixer/AuxBus.h"
#include "RtAudioManager.h"
#include <vector>
#include <memory>

namespace Kousaten {

class AudioEngine : public juce::AudioSource
{
public:
    static constexpr int MAX_CHANNELS = 32;

    AudioEngine();
    ~AudioEngine() override;

    // AudioSource interface
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    // Channel management
    int addChannel();
    void removeChannel(int channelId);
    Channel* getChannel(int channelId);
    int getChannelCount() const { return static_cast<int>(channels.size()); }

    // Get send buses (effects)
    MixBus* getDelayBus() { return &delayBus; }
    MixBus* getGrainBus() { return &grainBus; }
    MixBus* getReverbBus() { return &reverbBus; }

    // Aux bus management
    int addAuxBus();
    void removeAuxBus(int auxId);
    AuxBus* getAuxBus(int auxId);
    int getAuxBusCount() const { return static_cast<int>(auxBuses.size()); }
    const std::vector<std::unique_ptr<AuxBus>>& getAllAuxBuses() const { return auxBuses; }

    // RtAudio manager for multi-device output
    RtAudioManager* getRtAudioManager() { return &rtAudioManager; }

    // Master controls
    void setMasterVolume(float volume);
    float getMasterVolume() const { return masterVolume; }
    float getMasterLevelLeft() const { return masterLevelLeft; }
    float getMasterLevelRight() const { return masterLevelRight; }

    // Master output routing
    void setMasterOutputDevice(const juce::String& device) { masterOutputDevice = device; }
    juce::String getMasterOutputDevice() const { return masterOutputDevice; }
    void setMasterOutputChannelStart(int channel) { masterOutputChannelStart = channel; }
    int getMasterOutputChannelStart() const { return masterOutputChannelStart; }

    // Solo handling
    void updateSoloState();

    // Set input buffer for processing (called before getNextAudioBlock)
    void setInputBuffer(const juce::AudioBuffer<float>* buffer) { inputBuffer = buffer; }

private:
    const juce::AudioBuffer<float>* inputBuffer = nullptr;
    std::vector<std::unique_ptr<Channel>> channels;
    juce::SpinLock channelLock;  // Protects channels vector access

    MixBus delayBus { BusType::Delay };
    MixBus grainBus { BusType::Grain };
    MixBus reverbBus { BusType::Reverb };

    // Dynamic aux buses
    std::vector<std::unique_ptr<AuxBus>> auxBuses;
    int nextAuxId = 0;

    // RtAudio manager for multi-device output
    RtAudioManager rtAudioManager;

    float masterVolume = 1.0f;
    float masterLevelLeft = 0.0f;
    float masterLevelRight = 0.0f;

    juce::String masterOutputDevice;
    int masterOutputChannelStart = 0;

    bool soloActive = false;

    double currentSampleRate = 48000.0;
    int currentBlockSize = 512;

    // Temporary buffers for processing
    juce::AudioBuffer<float> tempBuffer;
    juce::AudioBuffer<float> delaySendBuffer;
    juce::AudioBuffer<float> grainSendBuffer;
    juce::AudioBuffer<float> reverbSendBuffer;
    juce::AudioBuffer<float> delayReturnBuffer;
    juce::AudioBuffer<float> grainReturnBuffer;
    juce::AudioBuffer<float> reverbReturnBuffer;

    // Per-channel send buffers (pre-allocated to avoid allocation in audio callback)
    juce::AudioBuffer<float> channelDelaySendBuffer;
    juce::AudioBuffer<float> channelGrainSendBuffer;
    juce::AudioBuffer<float> channelReverbSendBuffer;

    // Aux bus output buffer (pre-allocated)
    juce::AudioBuffer<float> auxOutputBuffer;

    juce::SmoothedValue<float> smoothedMasterVolume;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioEngine)
};

} // namespace Kousaten
