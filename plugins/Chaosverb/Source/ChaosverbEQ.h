#pragma once

#include <juce_dsp/juce_dsp.h>

namespace nbs {

class ChaosverbEQ {
public:
  ChaosverbEQ();
  ~ChaosverbEQ() = default;

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  // Updates coefficients based on parameters if they changed.
  void update(float lowCutHz, float highCutHz, float tiltVal);

  // Processes an interleaved stereo block (in-place)
  void process(int numSamples, float *dataL, float *dataR);

private:
  double currentSampleRate = 48000.0;

  juce::dsp::StateVariableTPTFilter<float> lowCutL, lowCutR;
  juce::dsp::StateVariableTPTFilter<float> highCutL, highCutR;
  juce::dsp::IIR::Filter<float> tiltLowL, tiltLowR;
  juce::dsp::IIR::Filter<float> tiltHighL, tiltHighR;

  juce::SmoothedValue<float> lowCutSmoother;
  juce::SmoothedValue<float> highCutSmoother;

  float cachedLowCut = -1.0f;
  float cachedHighCut = -1.0f;
  float cachedTilt = -999.0f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChaosverbEQ)
};

} // namespace nbs
