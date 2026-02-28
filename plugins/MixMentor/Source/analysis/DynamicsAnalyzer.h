#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <cmath>

/**
 * DynamicsAnalyzer
 *
 * Measures short-term RMS and integrated loudness of the incoming stereo
 * signal.  All write operations happen on the audio thread; reads are safe
 * from any thread via std::atomic.
 *
 * STUB — replace the placeholder RMS logic with a proper BS.1770-4
 * K-weighted measurement in Stage 2.
 */
class DynamicsAnalyzer {
public:
  DynamicsAnalyzer() = default;
  void prepare(double sampleRate, int /*blockSize*/) {
    sr = sampleRate;
    // TODO Stage 2: initialise K-weighting biquad filters (two stages)
    //               and a 400 ms gating window for BS.1770 compliance
  }

  void process(const juce::AudioBuffer<float> &buffer) {
    // ---- Placeholder: short-term RMS of L channel ----------------
    // TODO Stage 2: replace with dual-channel K-weighted gated LUFS
    const int n = buffer.getNumSamples();
    if (n == 0)
      return;

    float sum = 0.0f;
    const float *L = buffer.getReadPointer(0);
    for (int i = 0; i < n; ++i)
      sum += L[i] * L[i];

    const float rms = std::sqrt(sum / static_cast<float>(n));
    const float db = (rms > 1e-7f) ? 20.0f * std::log10(rms) : -60.0f;

    rmsDb_atomic.store(juce::jlimit(-60.0f, 0.0f, db));
    lufsIntegrated_atomic.store(rmsDb_atomic.load()); // stub approximation
    peakDb_atomic.store(rmsDb_atomic.load());         // stub approximation
  }

  float getRmsDb() const { return rmsDb_atomic.load(); }
  float getLufsIntegrated() const { return lufsIntegrated_atomic.load(); }
  float getPeakDb() const { return peakDb_atomic.load(); }

private:
  double sr = 44100.0;

  std::atomic<float> rmsDb_atomic{-60.0f};
  std::atomic<float> lufsIntegrated_atomic{-60.0f};
  std::atomic<float> peakDb_atomic{-60.0f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DynamicsAnalyzer)
};
