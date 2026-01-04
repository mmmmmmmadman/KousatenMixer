/*
    Kousaten Mixer - RtAudio Manager
    Implementation
*/

#include "RtAudioManager.h"

namespace Kousaten {

// =============================================================================
// RtOutputStream
// =============================================================================

RtOutputStream::RtOutputStream(unsigned int deviceId, unsigned int numChannels,
                               unsigned int sampleRate, unsigned int bufferSize)
    : deviceId(deviceId)
    , numChannels(numChannels)
    , sampleRate(sampleRate)
    , bufferSize(bufferSize)
{
    // Ring buffer: 8x buffer size for safety (more headroom)
    ringBufferSize = bufferSize * numChannels * 8;
    ringBuffer.resize(ringBufferSize, 0.0f);

    // Initialize with zeros and set write position ahead
    std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);
    writePos.store(bufferSize * numChannels * 2);  // Pre-fill 2 buffers of silence
    readPos.store(0);

    try
    {
        RtAudio::StreamParameters outputParams;
        outputParams.deviceId = deviceId;
        outputParams.nChannels = numChannels;
        outputParams.firstChannel = 0;

        RtAudio::StreamOptions options;
        // Use RTAUDIO_SCHEDULE_REALTIME like VCV Rack, not MINIMIZE_LATENCY
        options.flags = RTAUDIO_SCHEDULE_REALTIME;
        options.numberOfBuffers = 2;  // VCV Rack uses 2 buffers

        unsigned int frames = bufferSize;

        rtAudio.openStream(&outputParams, nullptr, RTAUDIO_FLOAT32,
                          sampleRate, &frames, &audioCallback, this, &options);

        streamOpen = true;
        DBG("RtOutputStream opened: device " + juce::String(deviceId) +
            ", channels " + juce::String(numChannels));
    }
    catch (RtAudioErrorType& e)
    {
        DBG("RtAudio error opening stream: " + juce::String(static_cast<int>(e)));
        streamOpen = false;
    }
}

RtOutputStream::~RtOutputStream()
{
    stop();
    if (streamOpen)
    {
        try
        {
            rtAudio.closeStream();
        }
        catch (...)
        {
            // Ignore errors on close
        }
    }
}

bool RtOutputStream::start()
{
    if (!streamOpen) return false;

    try
    {
        // Reset ring buffer positions (lock-free)
        writePos.store(0, std::memory_order_release);
        readPos.store(0, std::memory_order_release);

        // Clear ring buffer (safe before stream starts)
        std::fill(ringBuffer.begin(), ringBuffer.end(), 0.0f);

        // Enable fade-in to let hardware settle
        fadeInCallbacksRemaining.store(fadeInCallbacks, std::memory_order_release);

        rtAudio.startStream();
        streamRunning = true;
        return true;
    }
    catch (RtAudioErrorType& e)
    {
        DBG("RtAudio error starting stream: " + juce::String(static_cast<int>(e)));
        return false;
    }
}

void RtOutputStream::stop()
{
    if (streamRunning)
    {
        try
        {
            rtAudio.stopStream();
            streamRunning = false;
        }
        catch (...)
        {
            // Ignore errors on stop
        }
    }
}

void RtOutputStream::writeBuffer(const float* left, const float* right, int numSamples)
{
    // Lock-free write to ring buffer
    size_t wp = writePos.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        size_t pos = wp % ringBufferSize;

        // Interleaved stereo
        ringBuffer[pos] = left[i];
        ringBuffer[(pos + 1) % ringBufferSize] = right[i];

        wp = (wp + 2) % ringBufferSize;
    }

    writePos.store(wp, std::memory_order_release);
}

int RtOutputStream::audioCallback(void* outputBuffer, void* /*inputBuffer*/,
                                  unsigned int nFrames, double /*streamTime*/,
                                  RtAudioStreamStatus /*status*/, void* userData)
{
    auto* stream = static_cast<RtOutputStream*>(userData);
    auto* out = static_cast<float*>(outputBuffer);

    // Check if we're in fade-in period (output silence to let hardware settle)
    size_t fadeRemaining = stream->fadeInCallbacksRemaining.load(std::memory_order_acquire);

    if (fadeRemaining > 0)
    {
        // Output silence during initial callbacks
        std::memset(out, 0, nFrames * stream->numChannels * sizeof(float));
        stream->fadeInCallbacksRemaining.store(fadeRemaining - 1, std::memory_order_release);
        return 0;
    }

    // Lock-free read from ring buffer
    size_t rp = stream->readPos.load(std::memory_order_acquire);
    const size_t totalSamples = nFrames * stream->numChannels;

    for (unsigned int i = 0; i < totalSamples; ++i)
    {
        size_t pos = rp % stream->ringBufferSize;
        out[i] = stream->ringBuffer[pos];
        stream->ringBuffer[pos] = 0.0f;  // Clear after reading
        rp = (rp + 1) % stream->ringBufferSize;
    }

    stream->readPos.store(rp, std::memory_order_release);

    return 0;
}

// =============================================================================
// RtAudioManager
// =============================================================================

RtAudioManager::RtAudioManager()
{
}

RtAudioManager::~RtAudioManager()
{
    stopAll();
    streams.clear();
}

void RtAudioManager::initialize()
{
    scanDevices();
}

void RtAudioManager::scanDevices()
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    devices.clear();

    try
    {
        RtAudio rtAudio;

        // RtAudio 6.x: use getDeviceIds() instead of iterating by index
        std::vector<unsigned int> deviceIds = rtAudio.getDeviceIds();

        for (unsigned int deviceId : deviceIds)
        {
            try
            {
                RtAudio::DeviceInfo info = rtAudio.getDeviceInfo(deviceId);

                if (info.outputChannels > 0)
                {
                    RtDeviceInfo deviceInfo;
                    deviceInfo.id = deviceId;
                    deviceInfo.name = juce::String(info.name);
                    deviceInfo.outputChannels = info.outputChannels;
                    deviceInfo.inputChannels = info.inputChannels;
                    deviceInfo.isDefault = info.isDefaultOutput;

                    for (unsigned int rate : info.sampleRates)
                    {
                        deviceInfo.sampleRates.push_back(rate);
                    }

                    devices.push_back(deviceInfo);

                    DBG("RtAudio device found: " + deviceInfo.name +
                        " (" + juce::String(deviceInfo.outputChannels) + " out)");
                }
            }
            catch (...)
            {
                // Skip devices that can't be queried
            }
        }
    }
    catch (RtAudioErrorType& e)
    {
        DBG("RtAudio error scanning devices: " + juce::String(static_cast<int>(e)));
    }
}

std::vector<RtDeviceInfo> RtAudioManager::getOutputDevices() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    return devices;
}

juce::StringArray RtAudioManager::getOutputDeviceNames() const
{
    std::lock_guard<std::mutex> lock(deviceMutex);
    juce::StringArray names;

    for (const auto& device : devices)
    {
        names.add(device.name);
    }

    return names;
}

RtDeviceInfo RtAudioManager::getDeviceInfo(const juce::String& deviceName) const
{
    std::lock_guard<std::mutex> lock(deviceMutex);

    for (const auto& device : devices)
    {
        if (device.name == deviceName)
            return device;
    }

    return {};
}

int RtAudioManager::createOutputStream(const juce::String& deviceName,
                                       unsigned int channelOffset,
                                       unsigned int numChannels)
{
    std::lock_guard<std::mutex> lock(streamMutex);

    // Find device by name
    unsigned int deviceId = 0;
    bool found = false;

    {
        std::lock_guard<std::mutex> devLock(deviceMutex);
        for (const auto& device : devices)
        {
            if (device.name == deviceName)
            {
                deviceId = device.id;
                found = true;
                break;
            }
        }
    }

    if (!found)
    {
        DBG("RtAudioManager: Device not found: " + deviceName);
        return -1;
    }

    auto stream = std::make_unique<RtOutputStream>(deviceId, numChannels,
                                                    sampleRate, bufferSize);

    if (!stream->isOpen())
    {
        DBG("RtAudioManager: Failed to open stream for: " + deviceName);
        return -1;
    }

    stream->setChannelOffset(channelOffset);

    int streamId = nextStreamId++;
    streams[streamId] = std::move(stream);

    DBG("RtAudioManager: Created stream " + juce::String(streamId) +
        " for device: " + deviceName);

    return streamId;
}

void RtAudioManager::destroyOutputStream(int streamId)
{
    std::lock_guard<std::mutex> lock(streamMutex);

    auto it = streams.find(streamId);
    if (it != streams.end())
    {
        it->second->stop();
        streams.erase(it);
        DBG("RtAudioManager: Destroyed stream " + juce::String(streamId));
    }
}

void RtAudioManager::writeToStream(int streamId, const float* left,
                                   const float* right, int numSamples)
{
    // Lock-free check: only write if streams are active
    if (!streamsActive.load(std::memory_order_acquire))
        return;

    // Fast path: try lock, if fails, skip this write (audio continues without glitch)
    std::unique_lock<std::mutex> lock(streamMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return;

    auto it = streams.find(streamId);
    if (it != streams.end())
    {
        it->second->writeBuffer(left, right, numSamples);
    }
}

void RtAudioManager::startAll()
{
    std::lock_guard<std::mutex> lock(streamMutex);

    for (auto& pair : streams)
    {
        pair.second->start();
    }

    streamsActive.store(true, std::memory_order_release);
    DBG("RtAudioManager: Started all streams");
}

void RtAudioManager::stopAll()
{
    // First, signal that streams are no longer active (lock-free)
    streamsActive.store(false, std::memory_order_release);

    // Small delay to let audio callbacks finish
    juce::Thread::sleep(10);

    // Now safe to acquire lock and stop streams
    std::lock_guard<std::mutex> lock(streamMutex);

    for (auto& pair : streams)
    {
        pair.second->stop();
    }

    DBG("RtAudioManager: Stopped all streams");
}

void RtAudioManager::switchDeviceAsync(std::function<void()> switchOperation)
{
    // Execute on message thread to avoid blocking UI
    juce::MessageManager::callAsync([this, switchOperation]() {
        // Stop streams first (non-blocking for audio thread)
        stopAll();

        // Additional delay for hardware to settle
        juce::Thread::sleep(50);

        // Perform the switch operation
        switchOperation();

        // Start streams again
        startAll();
    });
}

} // namespace Kousaten
