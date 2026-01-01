/*
    Kousaten Mixer - Audio Layer
    Implementation
*/

#include "AudioLayer.h"

namespace Kousaten {

AudioLayer::AudioLayer(int maxLengthSeconds, double sr)
    : sampleRate(sr)
{
    maxLength = static_cast<int>(maxLengthSeconds * sampleRate);
    bufferLeft.resize(maxLength, 0.0f);
    bufferRight.resize(maxLength, 0.0f);
    loopEnd = maxLength;
}

void AudioLayer::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    maxLength = static_cast<int>(60.0 * sampleRate);
    bufferLeft.resize(maxLength, 0.0f);
    bufferRight.resize(maxLength, 0.0f);
    clear();
}

void AudioLayer::clear()
{
    std::fill(bufferLeft.begin(), bufferLeft.end(), 0.0f);
    std::fill(bufferRight.begin(), bufferRight.end(), 0.0f);
    recordedLength = 0;
    recordPosition = 0;
    playbackPhase = 0.0f;
    loopStart = 0;
    loopEnd = maxLength;
    recording = false;
    playing = false;
}

void AudioLayer::startRecording()
{
    recording = true;
    recordPosition = 0;
    recordedLength = 0;
}

void AudioLayer::stopRecording()
{
    recording = false;
    recordedLength = recordPosition;
    loopEnd = recordedLength;
}

void AudioLayer::recordSample(float left, float right)
{
    if (!recording || recordPosition >= maxLength)
        return;

    bufferLeft[recordPosition] = left;
    bufferRight[recordPosition] = right;
    recordPosition++;
}

void AudioLayer::startPlayback()
{
    playing = true;
    playbackPhase = static_cast<float>(loopStart);
}

void AudioLayer::stopPlayback()
{
    playing = false;
}

void AudioLayer::setSpeed(float newSpeed)
{
    speed = juce::jlimit(-8.0f, 8.0f, newSpeed);
}

void AudioLayer::setLoopStart(float normalized)
{
    loopStart = static_cast<int>(juce::jlimit(0.0f, 1.0f, normalized) * recordedLength);
    if (loopStart >= loopEnd && loopEnd > 0)
        loopStart = loopEnd - 1;
}

void AudioLayer::setLoopEnd(float normalized)
{
    loopEnd = static_cast<int>(juce::jlimit(0.0f, 1.0f, normalized) * recordedLength);
    if (loopEnd <= loopStart)
        loopEnd = loopStart + 1;
    if (loopEnd > recordedLength)
        loopEnd = recordedLength;
}

void AudioLayer::getPlaybackSamples(float& left, float& right)
{
    if (!playing || recordedLength == 0)
    {
        left = 0.0f;
        right = 0.0f;
        return;
    }

    // Linear interpolation for smooth playback
    int pos0 = static_cast<int>(playbackPhase);
    int pos1 = pos0 + 1;
    float frac = playbackPhase - pos0;

    // Clamp positions
    pos0 = juce::jlimit(0, recordedLength - 1, pos0);
    pos1 = juce::jlimit(0, recordedLength - 1, pos1);

    // Interpolate
    left = bufferLeft[pos0] * (1.0f - frac) + bufferLeft[pos1] * frac;
    right = bufferRight[pos0] * (1.0f - frac) + bufferRight[pos1] * frac;

    // Advance playback position
    playbackPhase += speed;

    // Handle loop boundaries
    int loopLength = loopEnd - loopStart;
    if (loopLength <= 0)
    {
        playing = false;
        return;
    }

    if (speed > 0)
    {
        while (playbackPhase >= loopEnd)
            playbackPhase -= loopLength;
    }
    else if (speed < 0)
    {
        while (playbackPhase < loopStart)
            playbackPhase += loopLength;
    }
}

float AudioLayer::getPlaybackPosition() const
{
    if (recordedLength == 0)
        return 0.0f;
    return playbackPhase / static_cast<float>(recordedLength);
}

} // namespace Kousaten
