#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

/**
 * StereoAnalyzer
 *
 * Measures stereo correlation (phase relationship between L and R) and a
 * normalised stereo width derived from Mid/Side energy.
 *
 * STUB — placeholder returns defaults until Stage 2 DSP is wired in.
 */
class StereoAnalyzer {
public:
  StereoAnalyzer() = default;
  void prepare(double /*sampleRate*/, int /*blockSize*/) {
    // TODO Stage 2: initialise running sums for correlation measurement
    //               and a smoothing filter (e.g. 300 ms time constant)
  }

  void process(const juce::AudioBuffer<float> &buffer) {
    if (buffer.getNumChannels() < 2) {
      // Mono input — fully correlated, no width
      correlation_atomic.store(1.0f);
      width_atomic.store(0.0f);
      return;
    }

    // TODO Stage 2:
    //   Correlation:  corr = sum(L*R) / sqrt(sum(L^2) * sum(R^2))
    //   Width via M/S energy:
    //     M = (L + R) / 2,  S = (L - R) / 2
    //     width = rms(S) / (rms(M) + rms(S))   clamped 0-1

    // Values remain at defaults until DSP is implemented
  }

  float getStereoCorrelation() const { return correlation_atomic.load(); }
  float getStereoWidth() const { return width_atomic.load(); }

private:
  std::atomic<float> correlation_atomic{1.0f};
  std::atomic<float> width_atomic{0.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoAnalyzer)
};
