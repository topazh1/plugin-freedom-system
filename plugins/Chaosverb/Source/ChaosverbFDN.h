#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cmath>
#include <cstring>

//==============================================================================
/**
 * ShortAllpass — Lightweight allpass for in-loop diffusion.
 *
 * Uses a circular buffer (no heap allocation). Placed inside the FDN
 * feedback path to break up comb resonances on every recirculation.
 * Four of these in series per delay line (with subtle modulation)
 * eliminates metallic ringing and creates dense, lush tails.
 *
 * Supports both fixed-delay and modulated-delay (linear interpolation)
 * modes. The modulated mode breaks up static resonance modes when
 * driven by slow LFO offsets.
 */
struct ShortAllpass {
  static constexpr int kMaxBuf = 2048; // supports 4-stage delays up to 192kHz
  float buf[kMaxBuf] = {};
  int writeIdx = 0;
  int delayLen = 1;
  float g = 0.65f;

  void init(int delay, float coeff) {
    delayLen = juce::jlimit(1, kMaxBuf - 2, delay);
    g = coeff;
  }

  void reset() {
    std::memset(buf, 0, sizeof(buf));
    writeIdx = 0;
  }

  float process(float input) {
    int readIdx = writeIdx - delayLen;
    if (readIdx < 0)
      readIdx += kMaxBuf;
    const float delayed = buf[readIdx];
    const float v = input - g * delayed;
    buf[writeIdx] = v;
    writeIdx = (writeIdx + 1) % kMaxBuf;
    return delayed + g * v;
  }

  /**
   * Modulated allpass with linear interpolation.
   * modOffset shifts the read position by fractional samples,
   * breaking up static resonance modes when driven by slow LFO.
   */
  float processModulated(float input, float modOffset) {
    float effDelay = static_cast<float>(delayLen) + modOffset;
    effDelay = juce::jlimit(1.0f, static_cast<float>(kMaxBuf - 2), effDelay);

    const int intDel = static_cast<int>(effDelay);
    const float frac = effDelay - static_cast<float>(intDel);

    int r0 = writeIdx - intDel;
    if (r0 < 0)
      r0 += kMaxBuf;
    int r1 = r0 - 1;
    if (r1 < 0)
      r1 += kMaxBuf;

    const float delayed = buf[r0] + frac * (buf[r1] - buf[r0]);
    const float v = input - g * delayed;
    buf[writeIdx] = v;
    writeIdx = (writeIdx + 1) % kMaxBuf;
    return delayed + g * v;
  }
};

//==============================================================================
/**
 * ChaosverbFDN
 *
 * 8-line Feedback Delay Network with Hadamard feedback matrix,
 * true L/R decorrelation, and dense 4-stage allpass diffusion.
 *
 * Key design: L and R channels use different prime-number delay lengths
 * in the FDN, producing naturally decorrelated broadband stereo.
 * Combined with 4-stage modulated allpass diffusion per line, this
 * creates lush, non-metallic reverb tails with massive stereo width.
 *
 * Signal flow per sample (per channel) inside this class:
 *   diffused input
 *     -> Hadamard topology matrix
 *     -> 4-stage modulated allpass diffusion (per-line)
 *     -> HF Damping (one-pole LPF per line)
 *     -> Spectral Tilt filter bank  (per-line low+high shelf)
 *     -> Resonance Injector         (per-line 3-band bandpass)
 *     -> global feedback gain
 *     -> tanh saturation
 *     -> write to delay lines (LFO-modulated, channel-specific lengths)
 *   -> FDN wet output (single float per channel)
 *
 * Caller (PluginProcessor) owns:
 *   - Pre-delay line
 *   - Allpass diffuser chain
 *   - DryWetMixer
 *   - Stereo width M/S matrix
 *   - Crossfade state machine (selects between two ChaosverbFDN instances)
 */
struct ChaosverbFDN {
  //==========================================================================
  static constexpr int kNumLines = 8;
  static constexpr float kAllpassCoeff =
      0.7f; // kept for PluginProcessor diffuser

  // L channel FDN delay lengths at 48kHz — all verified prime.
  static constexpr int kDelayLengths48k_L[kNumLines] = {1447, 1621, 1873, 2143,
                                                        2311, 2677, 2963, 3191};

  // R channel FDN delay lengths at 48kHz — different primes, similar range.
  // Using distinct primes per channel creates broadband decorrelation:
  // L and R reverb tails are physically different signals across all
  // frequencies, not just filtered versions of each other.
  static constexpr int kDelayLengths48k_R[kNumLines] = {1453, 1637, 1889, 2161,
                                                        2339, 2693, 2999, 3209};

  // In-loop allpass diffusers: 4 stages per line (doubled from original 2).
  // More stages = denser smearing of transients = smoother, less metallic tail.
  // Unique prime delays at 48kHz. Shared between L/R (FDN delays handle
  // decorrelation).
  static constexpr int kAPStages = 4;
  static constexpr int kAPDelays48k[kNumLines][kAPStages] = {
      {113, 191, 271, 353}, {127, 197, 277, 359}, {137, 211, 281, 367},
      {149, 223, 283, 373}, {157, 229, 293, 379}, {163, 233, 307, 383},
      {167, 239, 311, 389}, {179, 251, 313, 397}};
  static constexpr float kAPCoeff = 0.65f;

  // Allpass modulation: fraction of main LFO depth applied to allpass delays.
  // Subtle modulation (~+-2 samples max) breaks up static resonance modes
  // that cause metallic ringing, without audible pitch wobble.
  static constexpr float kAPModScale = 0.08f;

  // Per-stage modulation multipliers: alternating signs prevent coherent
  // pitch shift, diminishing depth adds timbral variety across stages.
  static constexpr float kAPModMult[kAPStages] = {1.0f, -0.7f, 0.5f, -0.3f};

  // Resonance injector frequencies (Hz)
  static constexpr float kResoFreqs[3] = {330.0f, 880.0f, 2200.0f};
  static constexpr float kResoQ = 50.0f;

  // Shelf crossover frequency for spectral tilt (Hz)
  static constexpr float kShelfFreq = 800.0f;

  // Maximum LFO depth at 48kHz (+-24 samples ~500us), scaled to SR in prepare()
  static constexpr float kMaxLFODepth48k = 24.0f;

  // LFO update stride — update LFO phase every N samples (saves CPU)
  static constexpr int kLFOStride = 4;

  //==========================================================================
  // FDN delay lines — Lagrange3rd for smooth LFO-modulated reads.
  // Each line handles both L and R channels internally (with different delays).
  juce::dsp::DelayLine<float,
                       juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
      fdnLines[kNumLines];

  //==========================================================================
  // Spectral Tilt Filter Bank
  // Per-line independent biquad filters: [line][ch] for low and high shelf.
  juce::dsp::IIR::Filter<float> shelfLow[kNumLines][2];
  juce::dsp::IIR::Filter<float> shelfHigh[kNumLines][2];

  // Cached shelf coefficients — recalculated only when spectralTilt changes
  float cachedSpectralTilt = -999.0f; // invalid sentinel to force first update

  //==========================================================================
  // Resonance Injector
  // Per-line, per-band, per-channel bandpass filters: [line][band][ch].
  juce::dsp::IIR::Filter<float> resoFilter[kNumLines][3][2]; // [line][band][ch]

  // Cached resonance parameter — recalculated when value changes
  float cachedResonance = -1.0f; // invalid sentinel

  // Resonance gain smoothing (~50ms to prevent clicks on mutation)
  float smoothedResoGain = 0.0f; // current interpolated value
  float targetResoGain = 0.0f;   // target value from parameter

  //==========================================================================
  // In-loop allpass diffuser instances: [line][stage][channel]
  ShortAllpass lineAP[kNumLines][kAPStages][2];

  //==========================================================================
  // LFO Modulation Engine
  // 8 independent sine LFOs, one per FDN delay line.
  float lfoPhase[kNumLines] = {}; // Current phase in radians [0, 2pi)
  float lfoPhaseInc[kNumLines] =
      {}; // Phase increment per sample (differs +-15%)

  float maxLFODepthSamples =
      kMaxLFODepth48k; // Scaled to actual SR in prepare()

  // LFO update stride tracking
  int lfoUpdateCounter = 0;
  float lfoSinCache[kNumLines] =
      {}; // Last computed sin values (stride-rate updates)

  //==========================================================================
  // HF Damping — one-pole lowpass per delay line per channel in feedback path.
  // Removes metallic character by absorbing highs each recirculation (like real
  // spaces).
  float dampState[2][kNumLines] = {};
  float dampCoeff = 0.0f; // one-pole coefficient, computed in prepare()

  //==========================================================================
  // Scaled delay lengths per channel (set in prepare())
  int delayLengthsL[kNumLines] = {};
  int delayLengthsR[kNumLines] = {};

  // FDN feedback state: last output read from each delay line, per channel.
  // [channel][line] — only channels 0 and 1 used (stereo).
  float fdnState[2][kNumLines] = {};

  double currentSampleRate = 48000.0;

  //==========================================================================
  void prepare(const juce::dsp::ProcessSpec &spec) {
    currentSampleRate = spec.sampleRate;
    const double srRatio = spec.sampleRate / 48000.0;

    // Scale L and R delay lengths proportionally to actual sample rate
    for (int i = 0; i < kNumLines; ++i) {
      delayLengthsL[i] =
          static_cast<int>(std::ceil(kDelayLengths48k_L[i] * srRatio));
      delayLengthsR[i] =
          static_cast<int>(std::ceil(kDelayLengths48k_R[i] * srRatio));
    }

    // Scale max LFO depth to current sample rate
    maxLFODepthSamples = kMaxLFODepth48k * static_cast<float>(srRatio);

    // FDN delay lines — max delay accommodates both channels + LFO headroom
    const int lfoHeadroom = static_cast<int>(std::ceil(maxLFODepthSamples)) + 4;
    for (int i = 0; i < kNumLines; ++i) {
      auto &line = fdnLines[i];
      const int maxDel = juce::jmax(delayLengthsL[i], delayLengthsR[i]);
      line.prepare(spec);
      line.setMaximumDelayInSamples(maxDel + lfoHeadroom);
      line.setDelay(static_cast<float>(maxDel));
      line.reset();
    }

    // Prepare per-line IIR shelf filters (mono spec — we call processSample
    // manually)
    juce::dsp::ProcessSpec monoSpec{spec.sampleRate, spec.maximumBlockSize, 1};
    for (int line = 0; line < kNumLines; ++line) {
      for (int ch = 0; ch < 2; ++ch) {
        shelfLow[line][ch].prepare(monoSpec);
        shelfHigh[line][ch].prepare(monoSpec);
      }
    }

    // Prepare per-line resonance filters
    for (int line = 0; line < kNumLines; ++line)
      for (int band = 0; band < 3; ++band)
        for (int ch = 0; ch < 2; ++ch)
          resoFilter[line][band][ch].prepare(monoSpec);

    // Initialize in-loop allpass diffusers (4 stages, scale delays to actual
    // SR)
    for (int line = 0; line < kNumLines; ++line)
      for (int stage = 0; stage < kAPStages; ++stage)
        for (int ch = 0; ch < 2; ++ch) {
          const int delay =
              static_cast<int>(std::ceil(kAPDelays48k[line][stage] * srRatio));
          lineAP[line][stage][ch].init(delay, kAPCoeff);
          lineAP[line][stage][ch].reset();
        }

    // Compute HF damping coefficient: one-pole LPF at ~8kHz
    // Absorbs highs each recirculation (like real acoustic spaces)
    const float dampCutoff = 8000.0f;
    dampCoeff =
        1.0f - std::exp(-juce::MathConstants<float>::twoPi * dampCutoff /
                        static_cast<float>(spec.sampleRate));

    // Clear damping state
    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < kNumLines; ++i)
        dampState[ch][i] = 0.0f;

    // Initialize shelf coefficients to unity (spectralTilt = 0)
    updateShelfCoefficients(0.0f);

    // Initialize resonance filter coefficients
    updateResonanceCoefficients(0.0f);

    // Reset smoothed resonance
    smoothedResoGain = 0.0f;
    targetResoGain = 0.0f;

    // Initialize LFO phases with evenly-spaced offsets: i * pi/4
    for (int i = 0; i < kNumLines; ++i) {
      lfoPhase[i] = i * (juce::MathConstants<float>::pi / 4.0f);
      lfoSinCache[i] = std::sin(lfoPhase[i]);
      lfoPhaseInc[i] = 0.0f; // updated each processBlock call
    }
    lfoUpdateCounter = 0;

    // Clear feedback state
    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < kNumLines; ++i)
        fdnState[ch][i] = 0.0f;
  }

  //--------------------------------------------------------------------------
  void reset() {
    for (int i = 0; i < kNumLines; ++i)
      fdnLines[i].reset();

    for (int line = 0; line < kNumLines; ++line)
      for (int ch = 0; ch < 2; ++ch) {
        shelfLow[line][ch].reset();
        shelfHigh[line][ch].reset();
      }

    for (int line = 0; line < kNumLines; ++line)
      for (int band = 0; band < 3; ++band)
        for (int ch = 0; ch < 2; ++ch)
          resoFilter[line][band][ch].reset();

    smoothedResoGain = 0.0f;
    targetResoGain = 0.0f;

    for (int line = 0; line < kNumLines; ++line)
      for (int stage = 0; stage < kAPStages; ++stage)
        for (int ch = 0; ch < 2; ++ch)
          lineAP[line][stage][ch].reset();

    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < kNumLines; ++i)
        dampState[ch][i] = 0.0f;

    for (int i = 0; i < kNumLines; ++i)
      lfoSinCache[i] = std::sin(lfoPhase[i]);
    lfoUpdateCounter = 0;

    for (int ch = 0; ch < 2; ++ch)
      for (int i = 0; i < kNumLines; ++i)
        fdnState[ch][i] = 0.0f;
  }

  //==========================================================================
  /**
   * Update in-loop allpass diffusion coefficients based on density.
   * Low density (0.0) -> sparse, comb-like reflections (g = 0.15)
   * High density (1.0) -> dense, lush wash (g = 0.85)
   * Called at block rate from processBlock.
   */
  void updateDensity(float densityNorm) {
    const float minG = 0.15f;
    const float maxG = 0.85f;
    const float g = minG + densityNorm * (maxG - minG);
    for (int line = 0; line < kNumLines; ++line)
      for (int stage = 0; stage < kAPStages; ++stage)
        for (int ch = 0; ch < 2; ++ch)
          lineAP[line][stage][ch].g = g;
  }

  //==========================================================================
  /**
   * Update spectral tilt shelf filter coefficients.
   *
   * spectralTilt = 0    -> both shelves unity gain (flat response)
   * spectralTilt = +100 -> highShelfGain = 1.3, lowShelfGain = 0.7
   * spectralTilt = -100 -> lowShelfGain = 1.3, highShelfGain = 0.7
   *
   * Hard stability limit: shelf gain clamped to 0.9999.
   */
  void updateShelfCoefficients(float spectralTiltValue) {
    cachedSpectralTilt = spectralTiltValue;

    const float t = spectralTiltValue / 100.0f; // normalised -1..+1

    float highGain = 1.0f + t * 0.3f;
    float lowGain = 1.0f - t * 0.3f;

    highGain = juce::jlimit(0.001f, 0.9999f, highGain);
    lowGain = juce::jlimit(0.001f, 0.9999f, lowGain);

    auto lowShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        currentSampleRate, kShelfFreq, 1.0f / std::sqrt(2.0f), lowGain);
    auto highShelfCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        currentSampleRate, kShelfFreq, 1.0f / std::sqrt(2.0f), highGain);

    for (int line = 0; line < kNumLines; ++line)
      for (int ch = 0; ch < 2; ++ch) {
        *shelfLow[line][ch].coefficients = *lowShelfCoeffs;
        *shelfHigh[line][ch].coefficients = *highShelfCoeffs;
      }
  }

  //==========================================================================
  /**
   * Update resonance bandpass filter coefficients.
   *
   * resonanceValue = 0   -> targetResoGain = 0
   * resonanceValue = 100 -> targetResoGain = 0.02 (hard capped at 1.0/Q)
   */
  void updateResonanceCoefficients(float resonanceValue) {
    cachedResonance = resonanceValue;

    const float rawGain = (resonanceValue / 100.0f) * 0.9f;
    const float maxSafeGain = 1.0f / kResoQ;
    targetResoGain = juce::jmin(rawGain, maxSafeGain);

    for (int band = 0; band < 3; ++band) {
      auto bpCoeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
          currentSampleRate, kResoFreqs[band], kResoQ);

      for (int line = 0; line < kNumLines; ++line)
        for (int ch = 0; ch < 2; ++ch)
          *resoFilter[line][band][ch].coefficients = *bpCoeffs;
    }
  }

  //==========================================================================
  /**
   * Fast Hadamard Transform (in-place, butterfly, O(N log N)).
   * N must be a power of 2. Result is normalised by 1/sqrt(N).
   */
  static void hadamardInPlace(float *v, int N) {
    int h = 1;
    while (h < N) {
      for (int i = 0; i < N; i += h * 2) {
        for (int j = i; j < i + h; ++j) {
          const float a = v[j];
          const float b = v[j + h];
          v[j] = a + b;
          v[j + h] = a - b;
        }
      }
      h *= 2;
    }
    const float scale = 1.0f / std::sqrt(static_cast<float>(N));
    for (int i = 0; i < N; ++i)
      v[i] *= scale;
  }

  //==========================================================================
  /**
   * Update LFO phase increments. Call once per block before sample loop.
   *
   * modRateHz       — LFO frequency in Hz
   * modDepthPercent — LFO depth 0..100
   */
  void prepareLFO(float modRateHz, float modDepthPercent) {
    const float sr = static_cast<float>(currentSampleRate);
    const float twoPi = juce::MathConstants<float>::twoPi;
    for (int i = 0; i < kNumLines; ++i) {
      const float stagger = 0.70f + 0.60f * (static_cast<float>(i) /
                                             static_cast<float>(kNumLines - 1));
      lfoPhaseInc[i] = (modRateHz * stagger / sr) * twoPi;
    }
    juce::ignoreUnused(
        modDepthPercent); // depth is passed per-sample via processSample
  }

  //==========================================================================
  /**
   * Process one stereo sample pair through the FDN core.
   *
   * Called per-sample from PluginProcessor::processBlock(). The caller provides
   * the already-pre-delayed and diffused input for each channel.
   *
   * @param inputL            Left channel diffused input sample
   * @param inputR            Right channel diffused input sample
   * @param feedbackGain      Per-loop gain from T60 formula (precomputed per
   * block)
   * @param topologyBlend     0=diagonal, 1=full Hadamard, 2=exaggerated
   * (precomputed)
   * @param modDepthSamples   LFO depth in samples (precomputed per block)
   * @param resoSmoothCoeff   One-pole smoothing coefficient for resonance gain
   * @param outL              [output] FDN wet sample for left channel
   * @param outR              [output] FDN wet sample for right channel
   */
  void processSample(float inputL, float inputR, float feedbackGain,
                     float topologyBlend, float modDepthSamples,
                     float resoSmoothCoeff, float &outL, float &outR) {
    // -- Smooth resonance gain toward target (one-pole IIR) --
    smoothedResoGain += (targetResoGain - smoothedResoGain) * resoSmoothCoeff;

    // -- Update LFO phases at stride rate --
    const float twoPi = juce::MathConstants<float>::twoPi;
    if (lfoUpdateCounter == 0) {
      for (int i = 0; i < kNumLines; ++i) {
        lfoPhase[i] += lfoPhaseInc[i] * static_cast<float>(kLFOStride);
        if (lfoPhase[i] >= twoPi)
          lfoPhase[i] -= twoPi;
        lfoSinCache[i] = std::sin(lfoPhase[i]);
      }
    }
    lfoUpdateCounter = (lfoUpdateCounter + 1) % kLFOStride;

    // -- Compute per-line LFO delay offsets --
    float lfoOffsets[kNumLines];
    for (int i = 0; i < kNumLines; ++i)
      lfoOffsets[i] = lfoSinCache[i] * modDepthSamples;

    // -- Process each channel through FDN core --
    outL =
        processFDNChannel(inputL, 0, feedbackGain, topologyBlend, lfoOffsets);
    outR =
        processFDNChannel(inputR, 1, feedbackGain, topologyBlend, lfoOffsets);
  }

private:
  //==========================================================================
  /**
   * Process one sample through the FDN core for a single channel.
   *
   * Contains the full feedback path: read -> topology -> 4-stage modulated
   * allpass -> HF damping -> spectral tilt -> resonance -> feedback gain
   * -> tanh -> write.
   *
   * L and R channels use different prime-number delay lengths for the main
   * FDN delay lines, producing naturally decorrelated broadband stereo.
   */
  float processFDNChannel(float input, int channel, float feedbackGain,
                          float topologyBlend,
                          const float lfoDelayOffsets[kNumLines]) {
    float *state = fdnState[channel];

    // Select channel-specific delay lengths for L/R decorrelation
    const int *dl = (channel == 0) ? delayLengthsL : delayLengthsR;

    // --- Read current outputs from each delay line (LFO-modulated,
    // channel-specific lengths) ---
    float outputs[kNumLines];
    for (int i = 0; i < kNumLines; ++i)
      outputs[i] = fdnLines[i].popSample(channel, static_cast<float>(dl[i]) +
                                                      lfoDelayOffsets[i]);

    // --- Compute feedback from previous state via topology matrix ---
    float fb[kNumLines];
    for (int i = 0; i < kNumLines; ++i)
      fb[i] = state[i];

    float mixed[kNumLines];

    if (topologyBlend <= 0.001f) {
      for (int i = 0; i < kNumLines; ++i)
        mixed[i] = fb[i];
    } else {
      float hadOut[kNumLines];
      for (int i = 0; i < kNumLines; ++i)
        hadOut[i] = fb[i];
      hadamardInPlace(hadOut, kNumLines);

      if (topologyBlend <= 1.0f) {
        const float t = topologyBlend;
        for (int i = 0; i < kNumLines; ++i)
          mixed[i] = fb[i] * (1.0f - t) + hadOut[i] * t;
      } else {
        const float exaggeration = juce::jmin(topologyBlend, 2.0f);
        for (int i = 0; i < kNumLines; ++i)
          mixed[i] = hadOut[i] * exaggeration;
      }
    }

    // --- 4-stage modulated allpass diffusion ---
    // Each line's allpass stages receive a fraction of the main LFO offset,
    // with alternating signs and diminishing depth per stage.
    // This breaks up static resonance modes that cause metallic ringing
    // while being subtle enough to avoid audible pitch wobble (~+-2 samples
    // max).
    for (int i = 0; i < kNumLines; ++i) {
      const float apMod = lfoDelayOffsets[i] * kAPModScale;
      for (int s = 0; s < kAPStages; ++s)
        mixed[i] = lineAP[i][s][channel].processModulated(
            mixed[i], apMod * kAPModMult[s]);
    }

    // --- HF Damping: one-pole lowpass per line (absorbs highs each loop) ---
    for (int i = 0; i < kNumLines; ++i) {
      dampState[channel][i] += dampCoeff * (mixed[i] - dampState[channel][i]);
      mixed[i] = dampState[channel][i];
    }

    // --- Apply Spectral Tilt: per-line low shelf + high shelf in series ---
    for (int i = 0; i < kNumLines; ++i) {
      float s = shelfLow[i][channel].processSample(mixed[i]);
      s = shelfHigh[i][channel].processSample(s);
      mixed[i] = juce::jlimit(-0.9999f, 0.9999f, s);
    }

    // --- Resonance Injector: add narrow bandpass-filtered feedback ---
    if (smoothedResoGain > 0.0001f) {
      for (int i = 0; i < kNumLines; ++i) {
        float resoSum = 0.0f;
        for (int band = 0; band < 3; ++band)
          resoSum += resoFilter[i][band][channel].processSample(mixed[i]);

        mixed[i] += juce::jlimit(-0.9999f, 0.9999f, resoSum * smoothedResoGain);
      }
    }

    // --- Apply global feedback gain and tanh saturation ---
    for (int i = 0; i < kNumLines; ++i) {
      mixed[i] *= feedbackGain;
      mixed[i] = std::tanh(mixed[i]);
    }

    // --- Write new inputs to each delay line: input fan-in + feedback ---
    const float inputScale = 1.0f / static_cast<float>(kNumLines);
    const float dcOffset = 1.0e-9f; // Prevent denormal accumulation

    for (int i = 0; i < kNumLines; ++i)
      fdnLines[i].pushSample(channel, input * inputScale + mixed[i] + dcOffset);

    // --- Update state with freshly read outputs ---
    float fdnOut = 0.0f;
    for (int i = 0; i < kNumLines; ++i) {
      state[i] = outputs[i];
      fdnOut += outputs[i];
    }

    // --- Sum all delay line outputs to produce FDN wet sample ---
    fdnOut *= (1.0f / static_cast<float>(kNumLines));

    return fdnOut;
  }
};
