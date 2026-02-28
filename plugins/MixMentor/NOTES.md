# MixMentor Notes

## Status
- **Current Status:** 🚧 Stage 1 (Foundation)
- **Version:** -
- **Type:** Audio Effect (Master Bus Analyzer)

## Lifecycle Timeline

- **2026-02-26 (Stage 1):** Initial scaffold — processor, editor, and analysis class stubs created. No DSP algorithms yet.

## Plugin Vision

Sits on the master bus and analyses real-time audio across three dimensions:
1. **Dynamics** — RMS & LUFS (BS.1770-4)
2. **Tonality** — Spectral centroid / flatness
3. **Stereo Width** — L/R correlation, M/S analysis

Compares metrics to predefined genre profiles and optionally sends mix data to
a human mentor via SNIPAI.co web integration.

## Architecture

```
PluginProcessor
├── DynamicsAnalyzer   → rmsDb, lufsIntegrated, peakDb
├── TonalityAnalyzer   → spectralCentroidHz, spectralFlatness
├── StereoAnalyzer     → stereoCorrelation, stereoWidth
└── atomics            → thread-safe snapshot for Editor

PluginEditor (juce::Timer @ 30 Hz)
├── genreSelector      (juce::ComboBox — AudioParameterChoice "genre")
├── feedbackText       (juce::TextEditor — read-only, response from mentor)
└── sendButton         (juce::TextButton — POST to SNIPAI.co API)
```

## Planned Stages

| Stage | Work |
|-------|------|
| 1 ✅  | Foundation: scaffold, analyzer stubs, placeholder GUI |
| 2     | DSP: BS.1770 LUFS, FFT spectral centroid, M/S width |
| 3     | Genre profiles: define per-genre target ranges |
| 4     | Web integration: SNIPAI.co login + POST mix data |

## Known Issues

None — stub implementation only.

## Parameters

| ID    | Type   | Values | Default | Purpose |
|-------|--------|--------|---------|---------|
| genre | Choice | Pop / Hip-Hop / Electronic / Rock / Jazz / Classical / R&B / Country | Pop | Genre profile for comparison |
