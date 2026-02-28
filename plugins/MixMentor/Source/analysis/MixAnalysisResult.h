#pragma once

/**
 * MixAnalysisResult
 *
 * Plain data snapshot produced on the audio thread and consumed on the
 * message thread (via atomic fields in the processor).  All values are
 * in display-friendly units.
 */
struct MixAnalysisResult
{
    // --- Dynamics (DynamicsAnalyzer) ---
    float rmsDb          = -60.0f;  // Short-term RMS level, dB FS
    float lufsIntegrated = -60.0f;  // Integrated loudness, LUFS (BS.1770-4)
    float peakDb         = -60.0f;  // True-peak, dB FS

    // --- Tonality (TonalityAnalyzer) ---
    float spectralCentroidHz = 1000.0f; // Perceptual "brightness" in Hz
    float spectralFlatness   = 0.5f;    // 0 = tonal, 1 = noise-like

    // --- Stereo (StereoAnalyzer) ---
    float stereoCorrelation = 1.0f; // -1 (out-of-phase) … +1 (mono)
    float stereoWidth       = 0.0f; // 0 (mono) … 1 (fully wide)
};
