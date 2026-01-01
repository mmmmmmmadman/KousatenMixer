/*
    Kousaten Mixer - Chaos Generator
    Lorenz Attractor based chaos modulation
    Extracted from Ellen Ripley VCV Rack module
*/

#pragma once

#include <cmath>
#include <algorithm>

namespace Kousaten {

class ChaosGenerator
{
public:
    ChaosGenerator() { reset(); }

    void reset()
    {
        x = 0.1f;
        y = 0.1f;
        z = 0.1f;
    }

    float process(float rate)
    {
        float dt = rate * 0.001f;

        float dx = 7.5f * (y - x);
        float dy = x * (30.9f - z) - y;
        float dz = x * y - 1.02f * z;

        x += dx * dt;
        y += dy * dt;
        z += dz * dt;

        // Prevent numerical explosion
        if (std::isnan(x) || std::isnan(y) || std::isnan(z) ||
            std::abs(x) > 100.0f || std::abs(y) > 100.0f || std::abs(z) > 100.0f)
        {
            reset();
        }

        return std::clamp(x * 0.1f, -1.0f, 1.0f);
    }

private:
    float x = 0.1f;
    float y = 0.1f;
    float z = 0.1f;
};

} // namespace Kousaten
