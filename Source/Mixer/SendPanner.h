/*
    Kousaten Mixer - Send Panner
    Dynamic send distribution across aux buses using XY pad or automation
*/

#pragma once

#include <JuceHeader.h>
#include <map>
#include <random>

namespace Kousaten {

enum class SendPannerMode {
    XYPad,      // Manual XY control
    Sequencer,  // Step through aux buses with tempo
    Random,     // Random jumps between aux buses
    Rotate      // Rotate through aux buses with LFO
};

class SendPanner
{
public:
    SendPanner();

    // Mode control
    void setMode(SendPannerMode newMode);
    SendPannerMode getMode() const { return mode; }

    // XY Position (0.0 to 1.0)
    void setPosition(float x, float y);
    float getPositionX() const { return posX; }
    float getPositionY() const { return posY; }

    // Get current animated position (smoothed, used for display)
    float getCurrentX() const { return smoothedX.getCurrentValue(); }
    float getCurrentY() const { return smoothedY.getCurrentValue(); }

    // Speed for automation modes (Hz)
    void setSpeed(float hz);
    float getSpeed() const { return speed; }

    // Smooth - transition time (0.0 = instant, 1.0 = very slow)
    void setSmooth(float value);
    float getSmooth() const { return smooth; }

    // Amount - panning depth (0.0 = no effect/uniform, 1.0 = full panning)
    void setAmount(float value);
    float getAmount() const { return amount; }

    // Path recording
    void startRecording();
    void stopRecording();
    bool isRecordingPath() const { return isRecording; }
    void clearRecordedPath();
    bool hasRecordedPath() const { return !recordedPath.empty(); }
    const std::vector<std::pair<float, float>>& getRecordedPath() const { return recordedPath; }

    // Home position (LFO center, set by double-click)
    void setHomePosition(float x, float y);
    float getHomeX() const { return homeX; }
    float getHomeY() const { return homeY; }

    // Aux bus positions in XY space
    void setAuxPosition(int auxId, float x, float y);
    void removeAuxPosition(int auxId);
    std::pair<float, float> getAuxPosition(int auxId) const;
    const std::map<int, std::pair<float, float>>& getAllAuxPositions() const { return auxPositions; }

    // Auto-arrange aux positions in a circle
    void arrangeAuxPositionsCircle(const std::vector<int>& auxIds);

    // Calculate current send levels for all aux buses
    // Returns map of auxId -> send level (0.0 to 1.0)
    std::map<int, float> calculateSendLevels() const;

    // Process automation (call once per audio block)
    void process(int numSamples, double sampleRate);

    // Enable/disable panner (when disabled, returns uniform distribution)
    void setEnabled(bool enabled);
    bool isEnabled() const { return pannerEnabled; }

private:
    SendPannerMode mode = SendPannerMode::XYPad;
    bool pannerEnabled = true;

    // XY position
    float posX = 0.5f;
    float posY = 0.5f;

    // Smoothed position for click-free movement
    juce::SmoothedValue<float> smoothedX;
    juce::SmoothedValue<float> smoothedY;

    // Automation parameters
    float speed = 1.0f;   // Hz
    float smooth = 0.5f;  // Transition smoothness (0-1)
    float amount = 1.0f;  // Panning depth (0-1)
    float phase = 0.0f;   // 0.0 to 1.0
    double currentSampleRate = 48000.0;  // Actual sample rate

    // Path recording
    std::vector<std::pair<float, float>> recordedPath;
    bool isRecording = false;
    float pathPlaybackPos = 0.0f;

    // Home position (LFO center)
    float homeX = 0.5f;
    float homeY = 0.5f;

    // Aux bus positions
    std::map<int, std::pair<float, float>> auxPositions;

    // For sequencer/random modes
    int currentAuxIndex = 0;
    int targetAuxIndex = 0;
    float transitionProgress = 1.0f;

    // Random generator
    std::mt19937 rng;

    // Calculate distance-based weight from automated position
    float calculateWeight(float auxX, float auxY) const;

    // Calculate distance-based weight from manual position
    float calculateManualWeight(float auxX, float auxY) const;

    // Update position based on automation mode
    void updateAutomation(int numSamples, double sampleRate);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendPanner)
};

} // namespace Kousaten
