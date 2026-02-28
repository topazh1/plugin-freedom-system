#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>

namespace nbs {

class ChaosverbDiffuser {
public:
  ChaosverbDiffuser();
  ~ChaosverbDiffuser() = default;

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  // Updates the number of active stages based on the density parameter (0.0 to
  // 100.0)
  void updateDensity(float densityPercent);

  // Processes an individual stereo sample (returns the processed Left and
  // Right)
  void processSample(float &valL, float &valR);

private:
  static constexpr int kNumDiffuserStages = 4;
  static constexpr int kAllpassLengths48k[kNumDiffuserStages] = {347, 557, 743,
                                                                 1013};
  static constexpr float kAllpassCoeff = 0.7f;

  int allpassDelayLengths[kNumDiffuserStages] = {};
  int numActiveStages = 0;

  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      diffLine0;
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      diffLine1;
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      diffLine2;
  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      diffLine3;

  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> &
  getDiffLine(int i);

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChaosverbDiffuser)
};

} // namespace nbs
