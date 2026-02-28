#pragma once

// Self-contained: uses only standard C++ math.
// No JUCE headers needed — keeps DynamicsEngine.h independently compilable.
#include <algorithm>
#include <cmath>

//==============================================================================
// DynamicsEngine
//
// Phase 4.3: Custom dual upward+downward compressor.
//
// Architecture:
//   - RMS level detector with configurable window (~10ms)
//   - Program-dependent ballistics (crest factor analysis over 50ms, 200ms smoothed)
//   - Downward compression with 6dB soft knee (standard compressor logic)
//   - Upward compression: raises signals below threshold toward threshold
//   - Linked stereo detection: uses max(L, R) RMS for gain computation
//   - Gains computed in dB, summed, converted to linear once
//   - Gain smoothing via 1-pole ballistic IIR (smoothed gain computer output)
//
// Usage:
//   engine.prepare(sampleRate);
//   engine.reset();
//
//   In processBlock, per sample:
//     // Compute linked detection level first (max of both channels)
//     engine.detectLevel(inputL, inputR);
//
//     // Apply compression to both channels with the shared gain
//     float gainLinear = engine.computeGain(thresholdDb, ratio,
//                                           attackCoeff, releaseCoeff,
//                                           downAmount, upAmount, dynamicsMacro);
//     outputL = inputL * gainLinear;
//     outputR = inputR * gainLinear;
//
// M/S mode: use two independent DynamicsEngine instances (one per channel),
//   each detects its own mono level and applies its own gain independently.
//==============================================================================
class DynamicsEngine
{
public:
    DynamicsEngine() = default;

    //==========================================================================
    // prepare — call from PluginProcessor::prepareToPlay()
    //==========================================================================
    void prepare (double sampleRate) noexcept
    {
        sampleRateVal = sampleRate;

        // RMS window: ~10ms
        const int rmsWindowSamples = static_cast<int> (sampleRate * 0.010);
        rmsWindowSize = std::max (1, rmsWindowSamples);

        // Crest factor analysis window: 50ms
        const int crestWindowSamples = static_cast<int> (sampleRate * 0.050);
        crestWindowSize = std::max (1, crestWindowSamples);

        // Crest factor smoothing: 200ms one-pole
        const double crestSmoothSeconds = 0.200;
        crestSmoothCoeff = static_cast<float> (
            std::exp (-1.0 / (crestSmoothSeconds * sampleRate)));

        reset();
    }

    //==========================================================================
    // reset — call from prepareToPlay() and releaseResources()
    //==========================================================================
    void reset() noexcept
    {
        rmsAccum        = 0.0f;
        rmsCount        = 0;
        rmsCurrent      = 0.0f;

        peakAccum       = 0.0f;
        peakCount       = 0;
        peakCurrent     = 0.0f;

        crestFactor     = 1.0f;
        crestSmoothed   = 1.0f;

        gainDb          = 0.0f;
        gainSmoothed    = 0.0f;
    }

    //==========================================================================
    // detectLevel (linked stereo version)
    //
    //   Accumulates RMS and peak for a single sample (using max of L/R).
    //   Updates rmsCurrent and peakCurrent once a full window is filled.
    //   Call this with both channels to get linked stereo detection.
    //==========================================================================
    void detectLevel (float sampleL, float sampleR) noexcept
    {
        // Linked stereo: use the louder channel
        const float s = std::max (std::abs (sampleL), std::abs (sampleR));

        // RMS accumulation (sum of squares)
        rmsAccum += s * s;
        ++rmsCount;

        if (rmsCount >= rmsWindowSize)
        {
            rmsCurrent  = std::sqrt (std::max (rmsAccum / static_cast<float> (rmsCount), 1.0e-9f));
            rmsAccum    = 0.0f;
            rmsCount    = 0;
        }

        // Peak accumulation (max abs)
        if (s > peakAccum)
            peakAccum = s;
        ++peakCount;

        if (peakCount >= crestWindowSize)
        {
            peakCurrent = peakAccum;
            peakAccum   = 0.0f;
            peakCount   = 0;

            // Crest factor = peak / RMS (clamped to [1, 10])
            const float rawCrest = (rmsCurrent > 1.0e-6f)
                                   ? (peakCurrent / rmsCurrent)
                                   : 1.0f;
            const float clampedCrest = std::min (rawCrest, 10.0f);

            // 200ms smoothed crest factor
            crestSmoothed = crestSmoothCoeff * crestSmoothed
                            + (1.0f - crestSmoothCoeff) * clampedCrest;
            crestFactor   = crestSmoothed;
        }
    }

    //==========================================================================
    // detectLevelMono (M/S mode — single channel)
    //
    //   Same logic as detectLevel but for a mono signal.
    //==========================================================================
    void detectLevelMono (float sample) noexcept
    {
        detectLevel (sample, sample);
    }

    //==========================================================================
    // computeGain
    //
    //   Runs the compressor gain computer from current level state.
    //   Returns the linear gain factor to apply to audio.
    //
    //   Parameters:
    //     thresholdDb  — threshold in dB (-40 to 0)
    //     ratio        — compression ratio (1 to 10)
    //     attackCoeff  — pre-computed 1-pole attack coeff
    //     releaseCoeff — pre-computed 1-pole release coeff
    //     downAmount   — downward compression depth (0–1)
    //     upAmount     — upward compression depth (0–1)
    //     dynamicsMacro — global dynamics macro (0–1), scales both
    //==========================================================================
    float computeGain (float thresholdDb,
                       float ratio,
                       float attackCoeff,
                       float releaseCoeff,
                       float downAmount,
                       float upAmount,
                       float dynamicsMacro) noexcept
    {
        // Convert current RMS level to dB
        const float levelDb = (rmsCurrent > 1.0e-6f)
                              ? (20.0f * std::log10 (rmsCurrent))
                              : -120.0f;

        // ------------------------------------------------------------------
        // Program-dependent ballistics: high crest factor → faster times
        // crestFactor: 1.0 (sustained) to ~10 (highly transient)
        // effectiveFactor: 0.0 (sustained) to 1.0 (highly transient)
        //
        // At full transient richness: attack shortened 50%, release shortened 30%
        // ------------------------------------------------------------------
        const float crestFac = std::min ((crestFactor - 1.0f) / 9.0f, 1.0f);

        // Modulate coefficients toward faster values:
        // Higher crestFac → higher (closer to 1.0) coefficient → slower IIR convergence
        // BUT we want FASTER attack/release at high crestFac:
        // attack: shorter time → higher coeff → actually we want lower coeff for faster
        // coeff = exp(-1/tau), lower tau = faster = lower coeff
        // For attack: multiply by (1 - 0.5 * crestFac) to shorten time constant
        // For release: multiply by (1 - 0.3 * crestFac)
        // Recompute coefficients from modified time constants is expensive per-sample;
        // instead blend between manual and faster fixed coefficients:
        //   A faster version of a coefficient c is c^k where k > 1 (raises to power)
        //   For attack 50% faster: equivalent to squaring the time constant factor
        //     c_fast = pow(c, 2.0) if coeff represents exp(-1/tau) style
        //   We use a simpler blend: c_eff = lerp(c, c * c, crestFac * 0.5)
        //   Note: c_eff = c + crestFac * 0.5 * (c^2 - c) = c * (1 + crestFac*0.5*(c-1))
        const float attFast    = attackCoeff  * attackCoeff;   // twice as fast attack
        const float relFast    = releaseCoeff * releaseCoeff;  // twice as fast release
        const float effAttack  = attackCoeff  + crestFac * 0.5f * (attFast - attackCoeff);
        const float effRelease = releaseCoeff + crestFac * 0.3f * (relFast - releaseCoeff);

        // ------------------------------------------------------------------
        // Downward compression: soft-knee compressor
        //   Knee width: 6dB centered on threshold
        //   Below (threshold - 3dB): no gain reduction (unity)
        //   Between (threshold - 3dB) and (threshold + 3dB): soft knee blend
        //   Above (threshold + 3dB): full ratio
        // ------------------------------------------------------------------
        const float halfKnee    = 3.0f;
        const float kneeBottom  = thresholdDb - halfKnee;
        const float kneeTop     = thresholdDb + halfKnee;

        float downGainDb = 0.0f;

        if (downAmount > 0.001f && dynamicsMacro > 0.001f)
        {
            if (levelDb > kneeTop)
            {
                // Full downward compression above knee
                // gainReduction = (threshold + (level - threshold) / ratio) - level
                downGainDb = (thresholdDb + (levelDb - thresholdDb) / ratio) - levelDb;
            }
            else if (levelDb > kneeBottom)
            {
                // Soft knee region: blend from 0 → full compression
                // Using quadratic blend: t = (levelDb - kneeBottom) / (kneeTop - kneeBottom)
                //   at t=0: ratio=1 (no compression), at t=1: full ratio
                const float t = (levelDb - kneeBottom) / (kneeTop - kneeBottom);
                // Blended ratio for this sample
                const float blendedRatio = 1.0f + t * (ratio - 1.0f);
                downGainDb = (thresholdDb + (levelDb - thresholdDb) / blendedRatio) - levelDb;
            }
            // else: below knee, no downward gain reduction

            // Scale by down amount and dynamics macro
            downGainDb *= downAmount * dynamicsMacro;
        }

        // ------------------------------------------------------------------
        // Upward compression: raises signals below threshold
        //   upwardGain = (threshold - levelDb) * upRatio * upAmount * dynamicsMacro
        //   upRatio fixed at 0.3 (gentle floor lifting)
        //   Only applied when level is below threshold
        // ------------------------------------------------------------------
        const float upRatio = 0.3f;
        float upGainDb = 0.0f;

        if (upAmount > 0.001f && dynamicsMacro > 0.001f && levelDb < thresholdDb)
        {
            const float gap = thresholdDb - levelDb;  // Positive: how far below threshold
            upGainDb = gap * upRatio * upAmount * dynamicsMacro;
        }

        // ------------------------------------------------------------------
        // Combined gain in dB (downward is negative, upward is positive)
        // ------------------------------------------------------------------
        const float targetGainDb = downGainDb + upGainDb;

        // ------------------------------------------------------------------
        // Gain smoothing: 1-pole IIR with ballistic coefficients
        //   Attack (gain going negative = compression engaging): use effAttack
        //   Release (gain returning toward 0): use effRelease
        //
        //   Convention: gainSmoothed tracks targetGainDb
        //   If targetGainDb < gainSmoothed: compressor is attacking (gain going down)
        //   If targetGainDb > gainSmoothed: compressor is releasing (gain going up)
        // ------------------------------------------------------------------
        if (targetGainDb < gainSmoothed)
            gainSmoothed = effAttack * gainSmoothed + (1.0f - effAttack) * targetGainDb;
        else
            gainSmoothed = effRelease * gainSmoothed + (1.0f - effRelease) * targetGainDb;

        // Denormal protection: clamp very small values to zero
        if (std::abs (gainSmoothed) < 1.0e-9f)
            gainSmoothed = 0.0f;

        // Convert smoothed dB gain to linear
        return juce_dsp_decibelsToGain (gainSmoothed);
    }

    //==========================================================================
    // Accessors — for metering / GUI (audio thread only)
    //==========================================================================
    float getGainReductionDb()  const noexcept { return gainSmoothed; }
    float getCrestFactor()      const noexcept { return crestFactor; }

private:
    //==========================================================================
    // Inline dB → linear conversion (no JUCE dependency)
    //==========================================================================
    static float juce_dsp_decibelsToGain (float db) noexcept
    {
        return std::pow (10.0f, db / 20.0f);
    }

    //==========================================================================
    // State
    //==========================================================================

    double sampleRateVal  = 44100.0;

    // RMS detector state (10ms window)
    float rmsAccum  = 0.0f;
    int   rmsCount  = 0;
    int   rmsWindowSize = 441;    // updated in prepare()
    float rmsCurrent = 0.0f;

    // Peak detector state (50ms crest window)
    float peakAccum  = 0.0f;
    int   peakCount  = 0;
    int   crestWindowSize = 2205;  // updated in prepare()
    float peakCurrent = 0.0f;

    // Crest factor state
    float crestFactor   = 1.0f;
    float crestSmoothed = 1.0f;
    float crestSmoothCoeff = 0.9f;  // updated in prepare()

    // Gain computer state
    float gainDb       = 0.0f;   // instantaneous (unused — kept for clarity)
    float gainSmoothed = 0.0f;   // smoothed gain in dB (applied to audio)
};
