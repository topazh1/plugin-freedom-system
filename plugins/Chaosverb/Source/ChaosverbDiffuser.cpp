#include "ChaosverbDiffuser.h"

namespace nbs {

ChaosverbDiffuser::ChaosverbDiffuser() = default;

void ChaosverbDiffuser::prepare(const juce::dsp::ProcessSpec &spec) {
  const double srRatio = spec.sampleRate / 48000.0;

  for (int i = 0; i < kNumDiffuserStages; ++i)
    allpassDelayLengths[i] =
        static_cast<int>(std::ceil(kAllpassLengths48k[i] * srRatio));

  diffLine0.prepare(spec);
  diffLine0.setMaximumDelayInSamples(allpassDelayLengths[0] + 1);
  diffLine0.setDelay(static_cast<float>(allpassDelayLengths[0]));
  diffLine0.reset();

  diffLine1.prepare(spec);
  diffLine1.setMaximumDelayInSamples(allpassDelayLengths[1] + 1);
  diffLine1.setDelay(static_cast<float>(allpassDelayLengths[1]));
  diffLine1.reset();

  diffLine2.prepare(spec);
  diffLine2.setMaximumDelayInSamples(allpassDelayLengths[2] + 1);
  diffLine2.setDelay(static_cast<float>(allpassDelayLengths[2]));
  diffLine2.reset();

  diffLine3.prepare(spec);
  diffLine3.setMaximumDelayInSamples(allpassDelayLengths[3] + 1);
  diffLine3.setDelay(static_cast<float>(allpassDelayLengths[3]));
  diffLine3.reset();
}

void ChaosverbDiffuser::reset() {
  diffLine0.reset();
  diffLine1.reset();
  diffLine2.reset();
  diffLine3.reset();
}

void ChaosverbDiffuser::updateDensity(float densityPercent) {
  if (densityPercent < 20.0f)
    numActiveStages = 0;
  else if (densityPercent < 40.0f)
    numActiveStages = 1;
  else if (densityPercent < 55.0f)
    numActiveStages = 2;
  else if (densityPercent < 75.0f)
    numActiveStages = 3;
  else
    numActiveStages = 4;
}

juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> &
ChaosverbDiffuser::getDiffLine(int i) {
  switch (i) {
  case 0:
    return diffLine0;
  case 1:
    return diffLine1;
  case 2:
    return diffLine2;
  default:
    return diffLine3;
  }
}

void ChaosverbDiffuser::processSample(float &valL, float &valR) {
  if (numActiveStages <= 0)
    return;

  for (int stage = 0; stage < numActiveStages; ++stage) {
    auto &dl = getDiffLine(stage);

    // Left Channel
    const float delayedL = dl.popSample(0);
    const float vL = valL - kAllpassCoeff * delayedL;
    dl.pushSample(0, vL);
    valL = delayedL + kAllpassCoeff * vL;

    // Right Channel
    const float delayedR = dl.popSample(1);
    const float vR = valR - kAllpassCoeff * delayedR;
    dl.pushSample(1, vR);
    valR = delayedR + kAllpassCoeff * vR;
  }
}

} // namespace nbs
