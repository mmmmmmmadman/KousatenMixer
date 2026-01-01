/*
    Kousaten Mixer - Mix Bus
    Send/Return effect bus
*/

#pragma once

#include <JuceHeader.h>
#include "../Effects/DelayProcessor.h"
#include "../Effects/GrainProcessor.h"
#include "../Effects/ReverbProcessor.h"
#include "../Effects/ChaosGenerator.h"

namespace Kousaten {

enum class BusType
{
    Delay,
    Grain,
    Reverb
};

class MixBus
{
public:
    MixBus(BusType type);

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();

    void setReturnLevel(float level);
    float getReturnLevel() const { return returnLevel; }

    float getOutputLevel() const { return outputLevel; }

    // Process send input and return processed audio
    void process(const float* inputLeft, const float* inputRight,
                 float* outputLeft, float* outputRight,
                 int numSamples);

    // Effect-specific parameters
    void setDelayTime(float timeLeft, float timeRight);
    void setDelayFeedback(float feedback);

    void setGrainSize(float size);
    void setGrainDensity(float density);
    void setGrainPosition(float position);

    void setReverbRoomSize(float size);
    void setReverbDamping(float damping);
    void setReverbDecay(float decay);

    void setChaosEnabled(bool enabled);
    void setChaosRate(float rate);

    BusType getType() const { return type; }

private:
    BusType type;
    double sampleRate = 48000.0;

    float returnLevel = 1.0f;
    float outputLevel = 0.0f;

    // Effect processors
    DelayProcessor delayProcessor;
    GrainProcessor grainProcessorLeft;
    GrainProcessor grainProcessorRight;
    ReverbProcessor reverbProcessorLeft;
    ReverbProcessor reverbProcessorRight;
    ChaosGenerator chaosGenerator;

    // Effect parameters
    float delayTimeLeft = 0.25f;
    float delayTimeRight = 0.25f;
    float delayFeedback = 0.3f;

    float grainSize = 0.3f;
    float grainDensity = 0.4f;
    float grainPosition = 0.5f;

    float reverbRoomSize = 0.5f;
    float reverbDamping = 0.4f;
    float reverbDecay = 0.6f;

    bool chaosEnabled = false;
    float chaosRate = 0.01f;

    juce::SmoothedValue<float> smoothedReturnLevel;
};

} // namespace Kousaten
