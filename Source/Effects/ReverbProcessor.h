/*
    Kousaten Mixer - Reverb Processor
    Freeverb-style parallel comb filters + series allpass
    Extracted from Ellen Ripley VCV Rack module
*/

#pragma once

#include <cmath>
#include <algorithm>
#include <array>

namespace Kousaten {

class ReverbProcessor
{
public:
    // Comb filter sizes (prime-ish numbers for diffusion)
    static constexpr int COMB_1_SIZE = 1557;
    static constexpr int COMB_2_SIZE = 1617;
    static constexpr int COMB_3_SIZE = 1491;
    static constexpr int COMB_4_SIZE = 1422;
    static constexpr int COMB_5_SIZE = 1277;
    static constexpr int COMB_6_SIZE = 1356;
    static constexpr int COMB_7_SIZE = 1188;
    static constexpr int COMB_8_SIZE = 1116;

    // Allpass filter sizes
    static constexpr int ALLPASS_1_SIZE = 556;
    static constexpr int ALLPASS_2_SIZE = 441;
    static constexpr int ALLPASS_3_SIZE = 341;
    static constexpr int ALLPASS_4_SIZE = 225;

    ReverbProcessor() { reset(); }

    void reset()
    {
        combBuffer1.fill(0.0f);
        combBuffer2.fill(0.0f);
        combBuffer3.fill(0.0f);
        combBuffer4.fill(0.0f);
        combBuffer5.fill(0.0f);
        combBuffer6.fill(0.0f);
        combBuffer7.fill(0.0f);
        combBuffer8.fill(0.0f);

        allpassBuffer1.fill(0.0f);
        allpassBuffer2.fill(0.0f);
        allpassBuffer3.fill(0.0f);
        allpassBuffer4.fill(0.0f);

        combIndex1 = combIndex2 = combIndex3 = combIndex4 = 0;
        combIndex5 = combIndex6 = combIndex7 = combIndex8 = 0;
        allpassIndex1 = allpassIndex2 = allpassIndex3 = allpassIndex4 = 0;

        combLp1 = combLp2 = combLp3 = combLp4 = 0.0f;
        combLp5 = combLp6 = combLp7 = combLp8 = 0.0f;
        hpState = 0.0f;
    }

    float process(float inputL, float inputR, float grainDensity,
                  float roomSize, float damping, float decay,
                  bool isLeftChannel, bool chaosEnabled, float chaosOutput,
                  float sampleRate)
    {
        float input = isLeftChannel ? inputL : inputR;

        // Calculate feedback based on decay
        float feedback = 0.5f + decay * 0.485f;
        if (chaosEnabled)
        {
            feedback += chaosOutput * 0.5f;
            feedback = std::clamp(feedback, 0.0f, 0.995f);
        }

        // Damping coefficient
        float dampingCoeff = 0.05f + damping * 0.9f;

        // Room size scaling
        float roomScale = 0.3f + roomSize * 1.4f;
        float roomInput = input * roomScale;

        float combOut = 0.0f;

        if (isLeftChannel)
        {
            combOut += processComb(roomInput, combBuffer1.data(), COMB_1_SIZE, combIndex1, feedback, combLp1, dampingCoeff);
            combOut += processComb(roomInput, combBuffer2.data(), COMB_2_SIZE, combIndex2, feedback, combLp2, dampingCoeff);
            combOut += processComb(roomInput, combBuffer3.data(), COMB_3_SIZE, combIndex3, feedback, combLp3, dampingCoeff);
            combOut += processComb(roomInput, combBuffer4.data(), COMB_4_SIZE, combIndex4, feedback, combLp4, dampingCoeff);
        }
        else
        {
            combOut += processComb(roomInput, combBuffer5.data(), COMB_5_SIZE, combIndex5, feedback, combLp5, dampingCoeff);
            combOut += processComb(roomInput, combBuffer6.data(), COMB_6_SIZE, combIndex6, feedback, combLp6, dampingCoeff);
            combOut += processComb(roomInput, combBuffer7.data(), COMB_7_SIZE, combIndex7, feedback, combLp7, dampingCoeff);
            combOut += processComb(roomInput, combBuffer8.data(), COMB_8_SIZE, combIndex8, feedback, combLp8, dampingCoeff);
        }

        combOut *= 0.25f;

        // Series allpass diffusion
        float diffused = combOut;
        diffused = processAllpass(diffused, allpassBuffer1.data(), ALLPASS_1_SIZE, allpassIndex1, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer2.data(), ALLPASS_2_SIZE, allpassIndex2, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer3.data(), ALLPASS_3_SIZE, allpassIndex3, 0.5f);
        diffused = processAllpass(diffused, allpassBuffer4.data(), ALLPASS_4_SIZE, allpassIndex4, 0.5f);

        // Highpass filter to remove sub-100Hz
        float hpCutoff = 100.0f / (sampleRate * 0.5f);
        hpCutoff = std::clamp(hpCutoff, 0.001f, 0.1f);
        hpState += (diffused - hpState) * hpCutoff;
        float hpOutput = diffused - hpState;

        return hpOutput;
    }

private:
    float processComb(float input, float* buffer, int size, int& index,
                      float feedback, float& lp, float damping)
    {
        float output = buffer[index];
        lp = lp + (output - lp) * damping;
        buffer[index] = input + lp * feedback;
        index = (index + 1) % size;
        return output;
    }

    float processAllpass(float input, float* buffer, int size, int& index, float gain)
    {
        float delayed = buffer[index];
        float output = -input * gain + delayed;
        buffer[index] = input + delayed * gain;
        index = (index + 1) % size;
        return output;
    }

    // Comb filter buffers
    std::array<float, COMB_1_SIZE> combBuffer1{};
    std::array<float, COMB_2_SIZE> combBuffer2{};
    std::array<float, COMB_3_SIZE> combBuffer3{};
    std::array<float, COMB_4_SIZE> combBuffer4{};
    std::array<float, COMB_5_SIZE> combBuffer5{};
    std::array<float, COMB_6_SIZE> combBuffer6{};
    std::array<float, COMB_7_SIZE> combBuffer7{};
    std::array<float, COMB_8_SIZE> combBuffer8{};

    int combIndex1 = 0, combIndex2 = 0, combIndex3 = 0, combIndex4 = 0;
    int combIndex5 = 0, combIndex6 = 0, combIndex7 = 0, combIndex8 = 0;

    float combLp1 = 0.0f, combLp2 = 0.0f, combLp3 = 0.0f, combLp4 = 0.0f;
    float combLp5 = 0.0f, combLp6 = 0.0f, combLp7 = 0.0f, combLp8 = 0.0f;

    // Allpass filter buffers
    std::array<float, ALLPASS_1_SIZE> allpassBuffer1{};
    std::array<float, ALLPASS_2_SIZE> allpassBuffer2{};
    std::array<float, ALLPASS_3_SIZE> allpassBuffer3{};
    std::array<float, ALLPASS_4_SIZE> allpassBuffer4{};

    int allpassIndex1 = 0, allpassIndex2 = 0, allpassIndex3 = 0, allpassIndex4 = 0;

    float hpState = 0.0f;
};

} // namespace Kousaten
