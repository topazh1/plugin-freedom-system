#include "ChaosverbWowFlutter.h"

namespace nbs {

ChaosverbWowFlutter::ChaosverbWowFlutter() = default;

void ChaosverbWowFlutter::prepare(const juce::dsp::ProcessSpec &spec) {
  currentSampleRate = spec.sampleRate;

  juce::dsp::ProcessSpec monoSpec{spec.sampleRate, spec.maximumBlockSize, 1};

  // Max 6ms (3ms center + 3ms modulation) — reduced for subtlety
  const int maxWfDelaySamples =
      static_cast<int>(std::ceil(0.006 * spec.sampleRate)) + 1;

  wfDelayLineL.prepare(monoSpec);
  wfDelayLineL.setMaximumDelayInSamples(maxWfDelaySamples);
  wfDelayLineL.setDelay(0.0f);
  wfDelayLineL.reset();

  wfDelayLineR.prepare(monoSpec);
  wfDelayLineR.setMaximumDelayInSamples(maxWfDelaySamples);
  wfDelayLineR.setDelay(0.0f);
  wfDelayLineR.reset();

  wfAmountSmoother.reset(currentSampleRate, 0.10); // 100ms smoothing for zipper-free sweeps

  // Reset wow/flutter LFO phases (R channels offset by pi/2 for stereo)
  wfWowPhaseL = 0.0f;
  wfWowPhaseR = juce::MathConstants<float>::halfPi;
  wfFlutterPhaseL = 0.0f;
  wfFlutterPhaseR = juce::MathConstants<float>::halfPi;
}

void ChaosverbWowFlutter::reset() {
  wfDelayLineL.reset();
  wfDelayLineR.reset();
}

void ChaosverbWowFlutter::process(int numSamples, float *dataL, float *dataR,
                                  float wfAmount, bool wfEnabled) {
  if (dataL == nullptr || dataR == nullptr)
    return;

  // Always update smoother target — even when disabled/zero — so transitions
  // ramp smoothly instead of snapping when re-enabled.
  const float targetAmount = (wfEnabled && wfAmount >= 0.001f) ? wfAmount / 100.0f : 0.0f;
  wfAmountSmoother.setTargetValue(targetAmount);

  // Skip processing only when smoother has fully settled at zero
  if (!wfAmountSmoother.isSmoothing() && targetAmount < 0.0001f)
    return;

  const float sr = static_cast<float>(currentSampleRate);
  const float maxDepthMs = 2.0f; // reduced from 5ms for subtlety

  const float twoPi = juce::MathConstants<float>::twoPi;
  // Center delay: always at max depth so modulation stays within buffer
  const float centerDelay = maxDepthMs * 0.001f * sr;

  for (int n = 0; n < numSamples; ++n) {
    const float wfNorm = wfAmountSmoother.getNextValue();
    const float depthSamples = wfNorm * maxDepthMs * 0.001f * sr;

    // Wow: slow, deep pitch drift (tape transport instability)
    // Flutter: fast, shallow pitch wobble (head vibration)
    const float wowRate = 0.3f + wfNorm * 0.8f;     // 0.3–1.1 Hz
    const float flutterRate = 5.0f + wfNorm * 6.0f; // 5–11 Hz

    const float wowInc = twoPi * wowRate / sr;
    const float flutterInc = twoPi * flutterRate / sr;

    const float modL = depthSamples * (0.7f * std::sin(wfWowPhaseL) +
                                       0.3f * std::sin(wfFlutterPhaseL));
    const float modR = depthSamples * (0.7f * std::sin(wfWowPhaseR) +
                                       0.3f * std::sin(wfFlutterPhaseR));

    // Separate L/R delay lines — guarantees full stereo independence
    wfDelayLineL.pushSample(0, dataL[n]);
    wfDelayLineR.pushSample(0, dataR[n]);
    dataL[n] = wfDelayLineL.popSample(0, juce::jmax(1.0f, centerDelay + modL));
    dataR[n] = wfDelayLineR.popSample(0, juce::jmax(1.0f, centerDelay + modR));

    wfWowPhaseL += wowInc;
    wfWowPhaseR += wowInc;
    wfFlutterPhaseL += flutterInc;
    wfFlutterPhaseR += flutterInc;

    if (wfWowPhaseL >= twoPi)
      wfWowPhaseL -= twoPi;
    if (wfWowPhaseR >= twoPi)
      wfWowPhaseR -= twoPi;
    if (wfFlutterPhaseL >= twoPi)
      wfFlutterPhaseL -= twoPi;
    if (wfFlutterPhaseR >= twoPi)
      wfFlutterPhaseR -= twoPi;
  }
}

} // namespace nbs
