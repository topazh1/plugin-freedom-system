#pragma once
#include <JuceHeader.h>
#include <atomic>

/**
 * TonalityAnalyzer
 *
 * Computes spectral centroid (perceptual brightness in Hz) and spectral
 * flatness (tonal vs. noise-like character) via FFT.
 *
 * STUB — values remain at sensible defaults until Stage 2 DSP is wired in.
 */
class TonalityAnalyzer {
public:
  TonalityAnalyzer() = default;
  void prepare(double sampleRate, int /*blockSize*/) {
    sr = sampleRate;
    // TODO Stage 2: initialise juce::dsp::FFT (order 10 → 1024-point)
    //               and pre-allocate windowed input + magnitude buffers
  }

  void process(const juce::AudioBuffer<float> & /*buffer*/) {
    // TODO Stage 2:
    //   1. Copy L channel into FFT input buffer, apply Hann window
    //   2. Perform forward FFT
    //   3. Compute spectral centroid:
    //        centroid = sum(f[k] * mag[k]) / sum(mag[k])
    //   4. Compute spectral flatness:
    //        flatness = geometricMean(mag) / arithmeticMean(mag)
    //   5. Store results into atomics below

    // Values remain at defaults (1000 Hz / 0.5) until DSP is implemented
  }

  float getSpectralCentroidHz() const { return centroidHz_atomic.load(); }
  float getSpectralFlatness() const { return flatness_atomic.load(); }

private:
  double sr = 44100.0;

  std::atomic<float> centroidHz_atomic{1000.0f};
  std::atomic<float> flatness_atomic{0.5f};

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TonalityAnalyzer)
};
