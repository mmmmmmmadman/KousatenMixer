/*
    Kousaten Mixer - Grain Processor
    Granular synthesis processor
    Extracted from Ellen Ripley VCV Rack module
*/

#pragma once

#include <cmath>
#include <algorithm>
#include <array>
#include <random>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Kousaten {

class GrainProcessor
{
public:
    static constexpr int BUFFER_SIZE = 8192;
    static constexpr int MAX_GRAINS = 16;

    GrainProcessor()
    {
        reset();
    }

    void reset()
    {
        grainBuffer.fill(0.0f);
        writeIndex = 0;

        for (auto& grain : grains)
        {
            grain.active = false;
        }
        phase = 0.0f;
    }

    float process(float input, float grainSize, float density, float position,
                  bool chaosEnabled, float chaosOutput, float sampleRate)
    {
        // Write input to circular buffer
        grainBuffer[writeIndex] = input;
        writeIndex = (writeIndex + 1) % BUFFER_SIZE;

        // Calculate grain parameters
        float grainSizeMs = grainSize * 99.0f + 1.0f;
        float grainSamples = (grainSizeMs / 1000.0f) * sampleRate;

        float densityValue = density;
        if (chaosEnabled)
        {
            densityValue += chaosOutput * 0.3f;
        }
        densityValue = std::clamp(densityValue, 0.0f, 1.0f);

        // Trigger new grains based on density
        float triggerRate = densityValue * 50.0f + 1.0f;
        phase += triggerRate / sampleRate;

        if (phase >= 1.0f)
        {
            phase -= 1.0f;
            triggerNewGrain(grainSamples, position, chaosEnabled, chaosOutput, densityValue);
        }

        // Process all active grains
        float output = 0.0f;
        int activeGrains = 0;

        for (auto& grain : grains)
        {
            if (grain.active)
            {
                float envPhase = grain.envelope / grain.size;

                if (envPhase >= 1.0f)
                {
                    grain.active = false;
                    continue;
                }

                // Hann window envelope
                float env = 0.5f * (1.0f - std::cos(envPhase * 2.0f * M_PI));

                int readPos = static_cast<int>(grain.position);
                readPos = ((readPos % BUFFER_SIZE) + BUFFER_SIZE) % BUFFER_SIZE;

                float sample = grainBuffer[readPos];
                output += sample * env;

                // Update grain position
                grain.position += grain.direction * grain.pitch;

                while (grain.position >= BUFFER_SIZE)
                    grain.position -= BUFFER_SIZE;
                while (grain.position < 0)
                    grain.position += BUFFER_SIZE;

                grain.envelope += 1.0f;
                activeGrains++;
            }
        }

        // Normalize output
        if (activeGrains > 0)
        {
            output /= std::sqrt(static_cast<float>(activeGrains));
        }

        return output;
    }

private:
    struct Grain
    {
        bool active = false;
        float position = 0.0f;
        float size = 0.0f;
        float envelope = 0.0f;
        float direction = 1.0f;
        float pitch = 1.0f;
    };

    void triggerNewGrain(float grainSamples, float position,
                         bool chaosEnabled, float chaosOutput, float densityValue)
    {
        for (auto& grain : grains)
        {
            if (!grain.active)
            {
                grain.active = true;
                grain.size = grainSamples;
                grain.envelope = 0.0f;

                float pos = position;
                if (chaosEnabled)
                {
                    pos += chaosOutput * 20.0f;

                    if (randomFloat() < 0.3f)
                        grain.direction = -1.0f;
                    else
                        grain.direction = 1.0f;

                    if (densityValue > 0.7f && randomFloat() < 0.2f)
                        grain.pitch = randomFloat() < 0.5f ? 0.5f : 2.0f;
                    else
                        grain.pitch = 1.0f;
                }
                else
                {
                    grain.direction = 1.0f;
                    grain.pitch = 1.0f;
                }

                pos = std::clamp(pos, 0.0f, 1.0f);
                grain.position = pos * BUFFER_SIZE;
                break;
            }
        }
    }

    float randomFloat()
    {
        static std::mt19937 gen(std::random_device{}());
        static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(gen);
    }

    std::array<float, BUFFER_SIZE> grainBuffer{};
    std::array<Grain, MAX_GRAINS> grains{};
    int writeIndex = 0;
    float phase = 0.0f;
};

} // namespace Kousaten
