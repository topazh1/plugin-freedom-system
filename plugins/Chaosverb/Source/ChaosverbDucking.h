#pragma once

#include <juce_dsp/juce_dsp.h>

namespace nbs {

class ChaosverbDucking
{
public:
    ChaosverbDucking();
    ~ChaosverbDucking() = default;

    void prepare(const juce::dsp::ProcessSpec& spec);

    // Feed the envelope follower with the un-wet dry audio signal peak
    void trackInput(float rawL, float rawR);

    // Applies the calculated gain reduction to a stereo sample based on ducking amount
    void processSample(float& valL, float& valR, float duckingAmountPercent);

private:
    float duckEnvelope      = 0.0f;
    float duckAttackCoeff   = 0.0f;
    float duckReleaseCoeff  = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChaosverbDucking)
};

} // namespace nbs
