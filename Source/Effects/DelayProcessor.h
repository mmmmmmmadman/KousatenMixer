/*
    Kousaten Mixer - Delay Processor
    Stereo delay with feedback and chaos modulation
    Extracted from Ellen Ripley VCV Rack module
*/

#pragma once

#include <cmath>
#include <algorithm>
#include <array>

namespace Kousaten {

class DelayProcessor
{
public:
    static constexpr int BUFFER_SIZE = 96000;  // 2 seconds at 48kHz

    DelayProcessor()
    {
        reset();
    }

    void reset()
    {
        leftBuffer.fill(0.0f);
        rightBuffer.fill(0.0f);
        writeIndex = 0;
    }

    void setParameters(float timeLeft, float timeRight, float feedback, float sampleRate)
    {
        delaySamplesLeft = static_cast<int>(std::clamp(timeLeft, 0.001f, 2.0f) * sampleRate);
        delaySamplesRight = static_cast<int>(std::clamp(timeRight, 0.001f, 2.0f) * sampleRate);
        delaySamplesLeft = std::clamp(delaySamplesLeft, 1, BUFFER_SIZE - 1);
        delaySamplesRight = std::clamp(delaySamplesRight, 1, BUFFER_SIZE - 1);
        this->feedback = std::clamp(feedback, 0.0f, 0.95f);
    }

    void process(float inputLeft, float inputRight, float& outputLeft, float& outputRight)
    {
        // Read from delay buffer
        int readIndexLeft = (writeIndex - delaySamplesLeft + BUFFER_SIZE) % BUFFER_SIZE;
        int readIndexRight = (writeIndex - delaySamplesRight + BUFFER_SIZE) % BUFFER_SIZE;

        outputLeft = leftBuffer[readIndexLeft];
        outputRight = rightBuffer[readIndexRight];

        // Write to delay buffer with feedback
        leftBuffer[writeIndex] = inputLeft + outputLeft * feedback;
        rightBuffer[writeIndex] = inputRight + outputRight * feedback;

        // Advance write index
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;
    }

    // Process with wet/dry mix
    void processWithMix(float inputLeft, float inputRight,
                        float& outputLeft, float& outputRight,
                        float wetDry)
    {
        float delayedLeft, delayedRight;
        process(inputLeft, inputRight, delayedLeft, delayedRight);

        outputLeft = inputLeft * (1.0f - wetDry) + delayedLeft * wetDry;
        outputRight = inputRight * (1.0f - wetDry) + delayedRight * wetDry;
    }

private:
    std::array<float, BUFFER_SIZE> leftBuffer{};
    std::array<float, BUFFER_SIZE> rightBuffer{};
    int writeIndex = 0;
    int delaySamplesLeft = 12000;
    int delaySamplesRight = 12000;
    float feedback = 0.3f;
};

} // namespace Kousaten
