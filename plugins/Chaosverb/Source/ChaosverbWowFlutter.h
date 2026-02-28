#pragma once

#include <juce_dsp/juce_dsp.h>

namespace nbs {

class ChaosverbWowFlutter {
public:
  ChaosverbWowFlutter();
  ~ChaosverbWowFlutter() = default;

  void prepare(const juce::dsp::ProcessSpec &spec);
  void reset();

  // Processes an interleaved stereo block (in-place)
  void process(int numSamples, float *dataL, float *dataR, float wfAmount,
               bool wfEnabled);

private:
  double currentSampleRate = 48000.0;

  juce::dsp::DelayLine<float,
                       juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
      wfDelayLineL;
  juce::dsp::DelayLine<float,
                       juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
      wfDelayLineR;

  juce::SmoothedValue<float> wfAmountSmoother;

  float wfWowPhaseL = 0.0f;
  float wfWowPhaseR = 1.5707963f; // pi/2 offset for stereo decorrelation
  float wfFlutterPhaseL = 0.0f;
  float wfFlutterPhaseR = 1.5707963f;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChaosverbWowFlutter)
};

} // namespace nbs
