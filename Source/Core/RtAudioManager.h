/*
    Kousaten Mixer - RtAudio Manager
    Manages multiple audio output devices using RtAudio
*/

#pragma once

#include <RtAudio.h>
#include <JuceHeader.h>
#include <map>
#include <memory>
#include <mutex>
#include <vector>
#include <atomic>
#include <cstring>

namespace Kousaten {

// Information about an RtAudio device
struct RtDeviceInfo
{
    unsigned int id;
    juce::String name;
    unsigned int outputChannels;
    unsigned int inputChannels;
    std::vector<unsigned int> sampleRates;
    bool isDefault;
};

// A single output stream to a device
class RtOutputStream
{
public:
    RtOutputStream(unsigned int deviceId, unsigned int numChannels,
                   unsigned int sampleRate, unsigned int bufferSize);
    ~RtOutputStream();

    bool isOpen() const { return streamOpen; }
    bool start();
    void stop();

    // Write audio data to the output buffer
    void writeBuffer(const float* left, const float* right, int numSamples);

    unsigned int getDeviceId() const { return deviceId; }
    unsigned int getNumChannels() const { return numChannels; }
    unsigned int getChannelOffset() const { return channelOffset; }
    void setChannelOffset(unsigned int offset) { channelOffset = offset; }

private:
    RtAudio rtAudio;
    unsigned int deviceId;
    unsigned int numChannels;
    unsigned int channelOffset = 0;
    unsigned int sampleRate;
    unsigned int bufferSize;
    bool streamOpen = false;
    bool streamRunning = false;

    // Lock-free ring buffer for audio data (no mutex in audio callback!)
    std::vector<float> ringBuffer;
    std::atomic<size_t> writePos{0};
    std::atomic<size_t> readPos{0};
    size_t ringBufferSize;

    // Fade-in to prevent pop/click at stream start (count in callbacks, not samples)
    std::atomic<size_t> fadeInCallbacksRemaining{0};
    static constexpr size_t fadeInCallbacks = 4;  // 4 callbacks of silence (~40ms at 512 buffer/48kHz)

    static int audioCallback(void* outputBuffer, void* inputBuffer,
                            unsigned int nFrames, double streamTime,
                            RtAudioStreamStatus status, void* userData);
};

// Manages multiple RtAudio output streams
class RtAudioManager
{
public:
    RtAudioManager();
    ~RtAudioManager();

    // Initialize and scan for devices
    void initialize();

    // Get available output devices
    std::vector<RtDeviceInfo> getOutputDevices() const;
    juce::StringArray getOutputDeviceNames() const;

    // Get device info by name
    RtDeviceInfo getDeviceInfo(const juce::String& deviceName) const;

    // Create/destroy output streams
    int createOutputStream(const juce::String& deviceName,
                          unsigned int channelOffset = 0,
                          unsigned int numChannels = 2);
    void destroyOutputStream(int streamId);

    // Write to a stream
    void writeToStream(int streamId, const float* left, const float* right, int numSamples);

    // Start/stop all streams
    void startAll();
    void stopAll();

    // Set global sample rate and buffer size
    void setSampleRate(unsigned int rate) { sampleRate = rate; }
    void setBufferSize(unsigned int size) { bufferSize = size; }
    unsigned int getSampleRate() const { return sampleRate; }
    unsigned int getBufferSize() const { return bufferSize; }

private:
    std::vector<RtDeviceInfo> devices;
    std::map<int, std::unique_ptr<RtOutputStream>> streams;
    int nextStreamId = 0;

    unsigned int sampleRate = 48000;
    unsigned int bufferSize = 512;

    mutable std::mutex deviceMutex;
    mutable std::mutex streamMutex;

    void scanDevices();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RtAudioManager)
};

} // namespace Kousaten
