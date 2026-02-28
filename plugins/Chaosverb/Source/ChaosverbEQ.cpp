#include "ChaosverbEQ.h"

namespace nbs {

ChaosverbEQ::ChaosverbEQ() = default;

void ChaosverbEQ::prepare(const juce::dsp::ProcessSpec &spec) {
  currentSampleRate = spec.sampleRate;

  juce::dsp::ProcessSpec monoSpec{spec.sampleRate, spec.maximumBlockSize, 1};

  lowCutL.prepare(monoSpec);
  lowCutL.setType(juce::dsp::StateVariableTPTFilterType::highpass);
  lowCutR.prepare(monoSpec);
  lowCutR.setType(juce::dsp::StateVariableTPTFilterType::highpass);

  highCutL.prepare(monoSpec);
  highCutL.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
  highCutR.prepare(monoSpec);
  highCutR.setType(juce::dsp::StateVariableTPTFilterType::lowpass);

  tiltLowL.prepare(monoSpec);
  tiltLowR.prepare(monoSpec);
  tiltHighL.prepare(monoSpec);
  tiltHighR.prepare(monoSpec);

  lowCutSmoother.reset(currentSampleRate, 0.05);  // 50ms smoothing
  highCutSmoother.reset(currentSampleRate, 0.15); // 150ms — longer ramp for zipper-free sweeps

  cachedLowCut = -1.0f;
  cachedHighCut = -1.0f;
  cachedTilt = -999.0f;
}

void ChaosverbEQ::reset() {
  lowCutL.reset();
  lowCutR.reset();
  highCutL.reset();
  highCutR.reset();
  tiltLowL.reset();
  tiltLowR.reset();
  tiltHighL.reset();
  tiltHighR.reset();
}

void ChaosverbEQ::update(float lowCutHz, float highCutHz, float tiltVal) {
  if (std::abs(lowCutHz - cachedLowCut) > 0.5f) {
    cachedLowCut = lowCutHz;
    lowCutSmoother.setTargetValue(lowCutHz);
  }

  if (std::abs(highCutHz - cachedHighCut) > 1.0f) {
    cachedHighCut = highCutHz;
    highCutSmoother.setTargetValue(highCutHz);
  }

  if (std::abs(tiltVal - cachedTilt) > 0.1f) {
    cachedTilt = tiltVal;
    const float t = tiltVal / 100.0f;
    const float tiltGainDb = t * 6.0f;

    auto lowShelf = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        currentSampleRate, 600.0f, 0.707f,
        juce::Decibels::decibelsToGain(-tiltGainDb));
    auto highShelf = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, 3000.0f, 0.707f,
        juce::Decibels::decibelsToGain(tiltGainDb));

    *tiltLowL.coefficients = *lowShelf;
    *tiltLowR.coefficients = *lowShelf;
    *tiltHighL.coefficients = *highShelf;
    *tiltHighR.coefficients = *highShelf;
  }
}

void ChaosverbEQ::process(int numSamples, float *dataL, float *dataR) {
  if (dataL == nullptr || dataR == nullptr)
    return;

  for (int n = 0; n < numSamples; ++n) {
    const float currentLowCut = lowCutSmoother.getNextValue();
    lowCutL.setCutoffFrequency(currentLowCut);
    lowCutR.setCutoffFrequency(currentLowCut);

    const float currentHighCut = highCutSmoother.getNextValue();
    highCutL.setCutoffFrequency(currentHighCut);
    highCutR.setCutoffFrequency(currentHighCut);

    dataL[n] = lowCutL.processSample(0, dataL[n]);
    dataR[n] = lowCutR.processSample(0, dataR[n]);

    dataL[n] = highCutL.processSample(0, dataL[n]);
    dataR[n] = highCutR.processSample(0, dataR[n]);

    dataL[n] = tiltLowL.processSample(dataL[n]);
    dataL[n] = tiltHighL.processSample(dataL[n]);

    dataR[n] = tiltLowR.processSample(dataR[n]);
    dataR[n] = tiltHighR.processSample(dataR[n]);
  }
}

} // namespace nbs
