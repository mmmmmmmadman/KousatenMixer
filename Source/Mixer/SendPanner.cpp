/*
    Kousaten Mixer - Send Panner
    Implementation
*/

#include "SendPanner.h"

namespace Kousaten {

SendPanner::SendPanner()
{
    smoothedX.setCurrentAndTargetValue(posX);
    smoothedY.setCurrentAndTargetValue(posY);

    // Initialize random generator with random seed
    std::random_device rd;
    rng.seed(rd());
}

void SendPanner::setMode(SendPannerMode newMode)
{
    if (mode != newMode)
    {
        mode = newMode;
        phase = 0.0f;
        transitionProgress = 1.0f;

        // Reset to current position for smooth transition
        if (mode == SendPannerMode::XYPad)
        {
            smoothedX.setCurrentAndTargetValue(posX);
            smoothedY.setCurrentAndTargetValue(posY);
        }
    }
}

void SendPanner::setPosition(float x, float y)
{
    posX = juce::jlimit(0.0f, 1.0f, x);
    posY = juce::jlimit(0.0f, 1.0f, y);

    // Record position if recording is active
    if (isRecording)
    {
        recordedPath.push_back({ posX, posY });
    }

    if (mode == SendPannerMode::XYPad || mode == SendPannerMode::Sequencer)
    {
        smoothedX.setTargetValue(posX);
        smoothedY.setTargetValue(posY);
    }
}

void SendPanner::setSpeed(float hz)
{
    speed = juce::jlimit(0.01f, 20.0f, hz);
}

void SendPanner::setSmooth(float value)
{
    smooth = juce::jlimit(0.0f, 1.0f, value);
    // Map smooth (0-1) to ramp time in samples (10ms to 2000ms)
    float rampTimeMs = 10.0f + smooth * 1990.0f;
    int rampSamples = static_cast<int>(rampTimeMs * currentSampleRate / 1000.0);
    smoothedX.reset(rampSamples);
    smoothedY.reset(rampSamples);
}

void SendPanner::setAmount(float value)
{
    amount = juce::jlimit(0.0f, 1.0f, value);
}

void SendPanner::startRecording()
{
    clearRecordedPath();
    isRecording = true;
}

void SendPanner::stopRecording()
{
    isRecording = false;
    pathPlaybackPos = 0.0f;

    // If we recorded a path, switch to Sequencer mode to play it
    if (!recordedPath.empty())
    {
        mode = SendPannerMode::Sequencer;
    }
}

void SendPanner::clearRecordedPath()
{
    recordedPath.clear();
    pathPlaybackPos = 0.0f;
}

void SendPanner::setHomePosition(float x, float y)
{
    homeX = juce::jlimit(0.0f, 1.0f, x);
    homeY = juce::jlimit(0.0f, 1.0f, y);
}

void SendPanner::setAuxPosition(int auxId, float x, float y)
{
    auxPositions[auxId] = { juce::jlimit(0.0f, 1.0f, x),
                            juce::jlimit(0.0f, 1.0f, y) };
}

void SendPanner::removeAuxPosition(int auxId)
{
    auxPositions.erase(auxId);
}

std::pair<float, float> SendPanner::getAuxPosition(int auxId) const
{
    auto it = auxPositions.find(auxId);
    if (it != auxPositions.end())
        return it->second;
    return { 0.5f, 0.5f };
}

void SendPanner::arrangeAuxPositionsCircle(const std::vector<int>& auxIds)
{
    if (auxIds.empty()) return;

    float angleStep = juce::MathConstants<float>::twoPi / static_cast<float>(auxIds.size());
    float radius = 0.4f;  // Keep within 0-1 range

    for (size_t i = 0; i < auxIds.size(); ++i)
    {
        float angle = static_cast<float>(i) * angleStep - juce::MathConstants<float>::halfPi;
        float x = 0.5f + radius * std::cos(angle);
        float y = 0.5f + radius * std::sin(angle);
        setAuxPosition(auxIds[i], x, y);
    }
}

float SendPanner::calculateWeight(float auxX, float auxY) const
{
    float currentX = smoothedX.getCurrentValue();
    float currentY = smoothedY.getCurrentValue();

    float dx = currentX - auxX;
    float dy = currentY - auxY;
    float distance = std::sqrt(dx * dx + dy * dy);

    // Inverse distance weighting with minimum distance to avoid division by zero
    // Using power of 2 for sharper falloff
    float weight = 1.0f / (distance * distance + 0.01f);

    return weight;
}

float SendPanner::calculateManualWeight(float auxX, float auxY) const
{
    // Calculate weight based on manual position (posX/posY)
    float dx = posX - auxX;
    float dy = posY - auxY;
    float distance = std::sqrt(dx * dx + dy * dy);

    float weight = 1.0f / (distance * distance + 0.01f);
    return weight;
}

std::map<int, float> SendPanner::calculateSendLevels() const
{
    std::map<int, float> levels;

    if (auxPositions.empty())
        return levels;

    float uniformLevel = 1.0f / static_cast<float>(auxPositions.size());

    if (!pannerEnabled)
    {
        // Uniform distribution when disabled
        for (const auto& [auxId, pos] : auxPositions)
        {
            levels[auxId] = uniformLevel;
        }
        return levels;
    }

    // Calculate weights for all aux buses (from both auto and manual positions)
    float totalWeight = 0.0f;
    std::map<int, float> weights;

    // Manual position influence (30% manual, 70% auto in non-XYPad modes)
    float manualInfluence = (mode == SendPannerMode::XYPad) ? 1.0f : 0.3f;
    float autoInfluence = 1.0f - manualInfluence;

    for (const auto& [auxId, pos] : auxPositions)
    {
        float autoWeight = calculateWeight(pos.first, pos.second);
        float manualWeight = calculateManualWeight(pos.first, pos.second);

        // Blend auto and manual weights
        float weight = autoWeight * autoInfluence + manualWeight * manualInfluence;
        weights[auxId] = weight;
        totalWeight += weight;
    }

    // Normalize weights and blend with uniform distribution based on amount
    if (totalWeight > 0.0f)
    {
        for (const auto& [auxId, weight] : weights)
        {
            float pannedLevel = weight / totalWeight;
            // Blend between uniform and panned based on amount
            levels[auxId] = uniformLevel + (pannedLevel - uniformLevel) * amount;
        }
    }

    return levels;
}

void SendPanner::process(int numSamples, double sampleRate)
{
    // Update sample rate if changed
    if (sampleRate != currentSampleRate)
    {
        currentSampleRate = sampleRate;
        setSmooth(smooth);  // Recalculate ramp samples
    }

    // Update smoothed values
    smoothedX.skip(numSamples);
    smoothedY.skip(numSamples);

    // Update automation if not in XY Pad mode
    if (mode != SendPannerMode::XYPad && pannerEnabled)
    {
        updateAutomation(numSamples, sampleRate);
    }
}

void SendPanner::updateAutomation(int numSamples, double sampleRate)
{
    if (auxPositions.empty()) return;

    float phaseIncrement = (speed * static_cast<float>(numSamples)) / static_cast<float>(sampleRate);
    phase += phaseIncrement;

    // Build sorted list of aux IDs for consistent ordering
    std::vector<int> auxIds;
    for (const auto& [auxId, pos] : auxPositions)
    {
        auxIds.push_back(auxId);
    }
    std::sort(auxIds.begin(), auxIds.end());

    int numAux = static_cast<int>(auxIds.size());
    if (numAux == 0) return;

    float targetX = homeX;
    float targetY = homeY;

    switch (mode)
    {
        case SendPannerMode::Rotate:
        {
            // Circular movement around home position
            while (phase >= 1.0f) phase -= 1.0f;

            float floatIndex = phase * static_cast<float>(numAux);
            int index1 = static_cast<int>(floatIndex) % numAux;
            int index2 = (index1 + 1) % numAux;
            float blend = floatIndex - std::floor(floatIndex);

            auto& pos1 = auxPositions[auxIds[static_cast<size_t>(index1)]];
            auto& pos2 = auxPositions[auxIds[static_cast<size_t>(index2)]];

            // Interpolate between aux positions, biased toward home
            float auxX = pos1.first + (pos2.first - pos1.first) * blend;
            float auxY = pos1.second + (pos2.second - pos1.second) * blend;

            // Blend with home position (70% aux movement, 30% home bias)
            targetX = auxX * 0.7f + homeX * 0.3f;
            targetY = auxY * 0.7f + homeY * 0.3f;
            break;
        }

        case SendPannerMode::Sequencer:
        {
            // If we have a recorded path, play it back
            if (!recordedPath.empty())
            {
                // Update playback position
                float pathLength = static_cast<float>(recordedPath.size());
                pathPlaybackPos += (speed * static_cast<float>(numSamples)) / static_cast<float>(sampleRate) * pathLength;

                // Loop the path
                while (pathPlaybackPos >= pathLength)
                {
                    pathPlaybackPos -= pathLength;
                }

                // Interpolate between path points
                int index1 = static_cast<int>(pathPlaybackPos) % static_cast<int>(pathLength);
                int index2 = (index1 + 1) % static_cast<int>(pathLength);
                float blend = pathPlaybackPos - std::floor(pathPlaybackPos);

                auto& p1 = recordedPath[static_cast<size_t>(index1)];
                auto& p2 = recordedPath[static_cast<size_t>(index2)];

                targetX = p1.first + (p2.first - p1.first) * blend;
                targetY = p1.second + (p2.second - p1.second) * blend;
            }
            else
            {
                // No recorded path - step through aux buses
                while (phase >= 1.0f)
                {
                    phase -= 1.0f;
                    currentAuxIndex = (currentAuxIndex + 1) % numAux;
                }

                auto& pos = auxPositions[auxIds[static_cast<size_t>(currentAuxIndex)]];
                targetX = pos.first * 0.7f + homeX * 0.3f;
                targetY = pos.second * 0.7f + homeY * 0.3f;
            }
            break;
        }

        case SendPannerMode::Random:
        {
            // Random jumps biased toward home
            while (phase >= 1.0f)
            {
                phase -= 1.0f;

                // Pick new random target
                std::uniform_int_distribution<int> dist(0, numAux - 1);
                int newTarget = dist(rng);

                // Avoid picking same as current
                if (numAux > 1 && newTarget == currentAuxIndex)
                {
                    newTarget = (newTarget + 1) % numAux;
                }
                currentAuxIndex = newTarget;
            }

            auto& pos = auxPositions[auxIds[static_cast<size_t>(currentAuxIndex)]];
            // Blend with home position
            targetX = pos.first * 0.7f + homeX * 0.3f;
            targetY = pos.second * 0.7f + homeY * 0.3f;
            break;
        }

        default:
            break;
    }

    smoothedX.setTargetValue(targetX);
    smoothedY.setTargetValue(targetY);
}

void SendPanner::setEnabled(bool enabled)
{
    pannerEnabled = enabled;
}

} // namespace Kousaten
