/*
    Kousaten Mixer - Mix Bus
    Implementation
*/

#include "MixBus.h"

namespace Kousaten {

MixBus::MixBus(BusType busType)
    : type(busType)
{
    smoothedReturnLevel.setCurrentAndTargetValue(returnLevel);
}

void MixBus::prepare(double newSampleRate, int samplesPerBlock)
{
    sampleRate = newSampleRate;
    smoothedReturnLevel.reset(sampleRate, 0.02);  // 20ms smoothing

    delayProcessor.reset();
    grainProcessorLeft.reset();
    grainProcessorRight.reset();
    reverbProcessorLeft.reset();
    reverbProcessorRight.reset();
    chaosGenerator.reset();
}

void MixBus::reset()
{
    delayProcessor.reset();
    grainProcessorLeft.reset();
    grainProcessorRight.reset();
    reverbProcessorLeft.reset();
    reverbProcessorRight.reset();
    chaosGenerator.reset();
}

void MixBus::setReturnLevel(float level)
{
    returnLevel = juce::jlimit(0.0f, 1.0f, level);
    smoothedReturnLevel.setTargetValue(returnLevel);
}

void MixBus::setDelayTime(float timeLeft, float timeRight)
{
    delayTimeLeft = juce::jlimit(0.001f, 2.0f, timeLeft);
    delayTimeRight = juce::jlimit(0.001f, 2.0f, timeRight);
}

void MixBus::setDelayFeedback(float feedback)
{
    delayFeedback = juce::jlimit(0.0f, 0.95f, feedback);
}

void MixBus::setGrainSize(float size)
{
    grainSize = juce::jlimit(0.0f, 1.0f, size);
}

void MixBus::setGrainDensity(float density)
{
    grainDensity = juce::jlimit(0.0f, 1.0f, density);
}

void MixBus::setGrainPosition(float position)
{
    grainPosition = juce::jlimit(0.0f, 1.0f, position);
}

void MixBus::setReverbRoomSize(float size)
{
    reverbRoomSize = juce::jlimit(0.0f, 1.0f, size);
}

void MixBus::setReverbDamping(float damping)
{
    reverbDamping = juce::jlimit(0.0f, 1.0f, damping);
}

void MixBus::setReverbDecay(float decay)
{
    reverbDecay = juce::jlimit(0.0f, 1.0f, decay);
}

void MixBus::setChaosEnabled(bool enabled)
{
    chaosEnabled = enabled;
}

void MixBus::setChaosRate(float rate)
{
    chaosRate = juce::jlimit(0.01f, 10.0f, rate);
}

void MixBus::process(const float* inputLeft, const float* inputRight,
                     float* outputLeft, float* outputRight,
                     int numSamples)
{
    float maxOutput = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float chaosOutput = 0.0f;
        if (chaosEnabled)
        {
            chaosOutput = chaosGenerator.process(chaosRate);
        }

        float left = inputLeft[i];
        float right = inputRight[i];
        float outLeft = 0.0f;
        float outRight = 0.0f;

        switch (type)
        {
            case BusType::Delay:
            {
                delayProcessor.setParameters(delayTimeLeft, delayTimeRight,
                                             delayFeedback, static_cast<float>(sampleRate));
                delayProcessor.process(left, right, outLeft, outRight);
                break;
            }

            case BusType::Grain:
            {
                outLeft = grainProcessorLeft.process(left, grainSize, grainDensity,
                                                     grainPosition, chaosEnabled,
                                                     chaosOutput, static_cast<float>(sampleRate));
                outRight = grainProcessorRight.process(right, grainSize, grainDensity,
                                                       grainPosition, chaosEnabled,
                                                       -chaosOutput, static_cast<float>(sampleRate));
                break;
            }

            case BusType::Reverb:
            {
                outLeft = reverbProcessorLeft.process(left, right, grainDensity,
                                                      reverbRoomSize, reverbDamping,
                                                      reverbDecay, true, chaosEnabled,
                                                      chaosOutput, static_cast<float>(sampleRate));
                outRight = reverbProcessorRight.process(left, right, grainDensity,
                                                        reverbRoomSize, reverbDamping,
                                                        reverbDecay, false, chaosEnabled,
                                                        chaosOutput, static_cast<float>(sampleRate));
                break;
            }
        }

        // Apply return level
        float level = smoothedReturnLevel.getNextValue();
        outputLeft[i] = outLeft * level;
        outputRight[i] = outRight * level;

        maxOutput = std::max(maxOutput, std::abs(outputLeft[i]));
        maxOutput = std::max(maxOutput, std::abs(outputRight[i]));
    }

    outputLevel = maxOutput;
}

} // namespace Kousaten
