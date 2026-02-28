#pragma once

// Self-contained: uses only standard C++ math.
// No JUCE headers needed — keeps ADAASaturator.h independently compilable
// and avoids include-order dependencies with juce_dsp.
#include <algorithm>
#include <cmath>

//==============================================================================
// DC1Blocker — inline 1-pole highpass for DC removal
//
// Bilinear-approximation highpass:  y[n] = x[n] - x[n-1] + R * y[n-1]
// where R = exp(-2π * fc / fs).
// At fc = 5 Hz and fs = 48000 Hz: R ≈ 0.999346
//
// Requires no JUCE headers and is real-time safe (no allocation).
//==============================================================================
struct DC1Blocker
{
    float xPrev = 0.0f;
    float yPrev = 0.0f;
    float R     = 0.9993f;  // default suitable for 48kHz / 5Hz

    void prepare (double sampleRate, float cutoffHz = 5.0f) noexcept
    {
        R = static_cast<float> (
            std::exp (-2.0 * 3.14159265358979323846 * cutoffHz / sampleRate));
        xPrev = 0.0f;
        yPrev = 0.0f;
    }

    void reset() noexcept
    {
        xPrev = 0.0f;
        yPrev = 0.0f;
    }

    float processSample (float x) noexcept
    {
        const float y = x - xPrev + R * yPrev;
        xPrev = x;
        yPrev = y;
        return y;
    }
};

//==============================================================================
// ADAASaturator
//
// First-order Antiderivative Anti-Aliasing (ADAA) waveshaper.
//
// Phase 4.2 additions over Phase 4.1:
//   - h_curve blend between soft (tanh) and hard (cubic polynomial)
//   - Even harmonic control via DC bias applied before the waveshaper
//   - Odd harmonic control via cubic pre-distortion applied before bias
//   - 5 Hz DC1Blocker post-waveshaper (removes bias-induced DC)
//
// Signal flow per sample:
//   1. odd pre-distortion:  x_odd  = x + oddGain * x^3
//   2. even bias:           x_full = x_odd + bias
//   3. drive scale:         x_drv  = x_full * driveGain
//   4. ADAA blend:          y = (F1_blend(x_drv) - F1_blend(xPrev)) / delta
//   5. output compensation: y /= tanh(driveGain) / driveGain
//   6. DC block:            y = HPF_5Hz(y)
//
// All ADAA math is double precision for numerical safety.
// Input/output are float (JUCE audio thread convention).
//
// CRITICAL: xPrev stores the DRIVEN, pre-processed value (not raw input).
// CRITICAL: F1_blend(xPrev) is recomputed each sample with the CURRENT alpha
//           so parameter changes remain artefact-free (see adaa-technical-reference.md §4).
//==============================================================================
class ADAASaturator
{
public:
    ADAASaturator() = default;

    //==========================================================================
    // prepare — call from PluginProcessor::prepareToPlay()
    //==========================================================================
    void prepare (double sampleRate) noexcept
    {
        xPrev[0] = 0.0;
        xPrev[1] = 0.0;

        for (int ch = 0; ch < 2; ++ch)
            dcBlocker[ch].prepare (sampleRate, 5.0f);
    }

    //==========================================================================
    // reset — call from prepareToPlay() and releaseResources()
    //==========================================================================
    void reset() noexcept
    {
        xPrev[0] = 0.0;
        xPrev[1] = 0.0;

        for (int ch = 0; ch < 2; ++ch)
            dcBlocker[ch].reset();
    }

    //==========================================================================
    // processSample
    //
    //   inputSample — audio sample (float)
    //   channel     — 0 = L/mid, 1 = R/side (clamped to 0–1)
    //   driveGain   — linear scale factor (1.0 to 16.0)
    //   alpha       — h_curve blend: 0 = soft (tanh), 1 = hard (cubic)
    //   bias        — DC bias for even harmonics (0.0 to 0.15)
    //   oddGain     — cubic pre-distortion for odd harmonics (0.0 to 0.05)
    //
    //   Returns anti-aliased, DC-blocked, gain-compensated output (float).
    //==========================================================================
    float processSample (float inputSample,
                         int   channel,
                         float driveGain,
                         float alpha,
                         float bias,
                         float oddGain) noexcept
    {
        const int ch = (channel >= 0 && channel < 2) ? channel : 0;

        // ------------------------------------------------------------------
        // Pre-processing (before ADAA waveshaper):
        //   1. Odd harmonic cubic pre-distortion  (applied to raw signal)
        //   2. Even harmonic DC bias              (shifts operating point)
        //   3. Drive scaling                      (into nonlinear region)
        // ------------------------------------------------------------------
        const double x      = static_cast<double> (inputSample);
        const double x_odd  = x + static_cast<double> (oddGain) * x * x * x;
        const double x_full = x_odd + static_cast<double> (bias);
        const double x_drv  = x_full * static_cast<double> (driveGain);

        // ------------------------------------------------------------------
        // ADAA first-order:
        //   y = (F1_blend(x_drv) - F1_blend(xPrev)) / (x_drv - xPrev)
        //
        // Both F1 values use the CURRENT alpha so mid-block parameter changes
        // don't create discontinuities (we store x_prev, not F1_prev).
        // ------------------------------------------------------------------
        const double dalpha  = static_cast<double> (alpha);
        const double F1_now  = blendedF1 (x_drv,     dalpha);
        const double F1_prev = blendedF1 (xPrev[ch], dalpha);
        const double delta   = x_drv - xPrev[ch];

        double y;
        if (std::abs (delta) < 1.0e-5)
        {
            // Near-singularity: L'Hopital limit  =>  y = f_blend(x_drv)
            y = blendedF (x_drv, dalpha);
        }
        else
        {
            y = (F1_now - F1_prev) / delta;
        }

        // Store driven value for next sample (NOT raw input)
        xPrev[ch] = x_drv;

        // ------------------------------------------------------------------
        // Output gain compensation:
        //   Divide by driveGain to undo the linear gain applied before the
        //   waveshaper.  For small signals: tanh(g*x)/g ≈ x (unity gain).
        //   For loud signals the waveshaper compresses naturally.
        // ------------------------------------------------------------------
        if (driveGain > 1.0f)
        {
            y /= static_cast<double> (driveGain);
        }

        // ------------------------------------------------------------------
        // Safety: clamp and reset state on NaN / Inf
        // ------------------------------------------------------------------
        if (! std::isfinite (y))
        {
            y = 0.0;
            xPrev[ch] = 0.0;
        }
        else
        {
            // Hard clamp at ±10: no physical signal reaches this after waveshaping
            if (y >  10.0) y =  10.0;
            if (y < -10.0) y = -10.0;
        }

        // ------------------------------------------------------------------
        // DC-blocking 5 Hz highpass
        //   Removes the DC component introduced by the even-harmonic bias.
        // ------------------------------------------------------------------
        return dcBlocker[ch].processSample (static_cast<float> (y));
    }

private:
    //==========================================================================
    // Waveshaper: soft (tanh)
    //==========================================================================
    static double f_soft (double x) noexcept
    {
        return std::tanh (x);
    }

    //==========================================================================
    // Waveshaper: hard (cubic soft-clipper, clamped at ±1)
    //   f_hard(x) = 1.5x - 0.5x^3   for |x| <= 1
    //   f_hard(x) = sign(x)           for |x| > 1
    //==========================================================================
    static double f_hard (double x) noexcept
    {
        if (x >  1.0) return  1.0;
        if (x < -1.0) return -1.0;
        return 1.5 * x - 0.5 * x * x * x;
    }

    //==========================================================================
    // Antiderivative: F1_soft(x) = ln(cosh(x))  — numerically stable
    //
    // Identity: ln(cosh(x)) = |x| + ln(1 + e^(-2|x|)) - ln(2)
    // For |x| > 15: e^(-2|x|) < 9e-14, log1p term negligible.
    //==========================================================================
    static double F1_soft (double x) noexcept
    {
        const double ax = std::abs (x);
        if (ax > 15.0)
            return ax - 0.6931471805599453;  // |x| - ln(2)
        return ax + std::log1p (std::exp (-2.0 * ax)) - 0.6931471805599453;
    }

    //==========================================================================
    // Antiderivative: F1_hard(x) — piecewise, C1-continuous at |x| = 1
    //
    // |x| <= 1:  F1_hard(x) = 0.75x^2 - 0.125x^4
    //  x >  1:   F1_hard(x) = x - 0.375
    //  x < -1:   F1_hard(x) = -x - 0.375
    //
    // Continuity: F1_hard(1) = 0.625 = F1_hard(1+)  ✓
    //==========================================================================
    static double F1_hard (double x) noexcept
    {
        if (x >  1.0) return  x - 0.375;
        if (x < -1.0) return -x - 0.375;
        return 0.75 * x * x - 0.125 * x * x * x * x;
    }

    //==========================================================================
    // Blended waveshaper:  alpha=0 → pure tanh,  alpha=1 → pure cubic
    //==========================================================================
    static double blendedF (double x, double alpha) noexcept
    {
        return (1.0 - alpha) * f_soft (x) + alpha * f_hard (x);
    }

    //==========================================================================
    // Blended antiderivative: linearity of integration makes this exact.
    // See adaa-technical-reference.md §4 for the proof.
    //==========================================================================
    static double blendedF1 (double x, double alpha) noexcept
    {
        return (1.0 - alpha) * F1_soft (x) + alpha * F1_hard (x);
    }

    //==========================================================================
    // State
    //==========================================================================

    // Per-channel ADAA state: stores the driven, pre-processed x value
    double xPrev[2] = { 0.0, 0.0 };

    // Per-channel DC-blocking 5 Hz 1-pole highpass (pure C++, no JUCE dependency)
    DC1Blocker dcBlocker[2];
};
