/*
    Kousaten Mixer - Audio Layer
    Audio buffer for sampler with variable speed playback
    Extracted from WeiiiDocumenta VCV Rack module
*/

#pragma once

#include <JuceHeader.h>
#include <vector>

namespace Kousaten {

class AudioLayer
{
public:
    AudioLayer(int maxLengthSeconds = 60, double sampleRate = 48000.0);

    void prepare(double sampleRate);
    void clear();

    // Recording
    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording; }
    void recordSample(float left, float right);

    // Playback
    void startPlayback();
    void stopPlayback();
    bool isPlaying() const { return playing; }

    void setSpeed(float speed);  // -8.0 to 8.0
    void setLoopStart(float normalized);  // 0.0 to 1.0
    void setLoopEnd(float normalized);    // 0.0 to 1.0

    // Get playback samples
    void getPlaybackSamples(float& left, float& right);

    // Info
    int getRecordedLength() const { return recordedLength; }
    float getPlaybackPosition() const;  // Normalized 0.0 to 1.0

private:
    std::vector<float> bufferLeft;
    std::vector<float> bufferRight;

    double sampleRate = 48000.0;
    int maxLength = 0;
    int recordedLength = 0;
    int recordPosition = 0;

    float playbackPhase = 0.0f;
    float speed = 1.0f;

    int loopStart = 0;
    int loopEnd = 0;

    bool recording = false;
    bool playing = false;
};

} // namespace Kousaten
