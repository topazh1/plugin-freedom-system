# Chaosverb - Implementation Plan

**Date:** 2026-02-27
**Complexity Score:** 5.0 (Complex — capped)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 22 parameters (12 floats + 10 booleans) → min(22/5, 2.0) = min(4.4, 2.0) = **2.0**
- **Algorithms:** 7 DSP components = **7**
  - FDN Core Engine (Custom 8-line FDN with Hadamard matrix)
  - Allpass Diffuser Chain (4-stage Schroeder)
  - Spectral Tilt Filter Bank (per-band shelf IIR)
  - Resonance Injector (3x narrow bandpass IIR)
  - LFO Modulation Engine (8 independent sine LFOs)
  - Non-Linear Saturation (tanh waveshaper in feedback path)
  - Stereo Width Processor (M/S matrix)
- **Features:** 4 points
  - Feedback loops (+1) — FDN is a feedback network by definition
  - Modulation systems (+1) — 8 independent LFOs modulating delay line lengths
  - Non-standard state management (+1) — dual FDN instances + mutation timer system
  - External timer-driven automation (+1) — mutation system fires random parameter changes on interval
- **Total:** 2.0 + 7 + 4 = 13.0 → **capped at 5.0**

---

## Stages

- Stage 0: Research & Planning ✓
- Stage 1: Foundation (project structure + CMakeLists.txt)
- Stage 2: Shell (APVTS parameters + PluginProcessor skeleton)
- Stage 3: DSP — 4 phases
- Stage 3: GUI — 3 phases
- Stage 3: Validation (presets, pluginval, changelog)

---

## Complex Implementation (Score 5.0)

### Stage 3: DSP Phases

#### Phase 4.1: Core Single FDN

**Goal:** Implement a working single 8-line FDN reverb with basic Hadamard matrix. Verify audio passes through and basic reverb behavior is correct. No dual-instance yet.

**Components:**
- Pre-delay line (`juce::dsp::DelayLine`, no interpolation) — preDelay parameter
- Allpass Diffuser Chain (4 stages, `juce::dsp::DelayLine` per stage) — density parameter
- Single FDN instance: 8 delay lines with mutually-prime lengths, Hadamard 8x8 feedback matrix
- Global feedback gain controlled by decay parameter (T60 formula)
- Dry/Wet Mixer (`juce::dsp::DryWetMixer`) — mix parameter
- topology parameter: implement as simple matrix scaling (0=diagonal only, 50=full Hadamard, 100=Hadamard)
- Basic tanh saturation in feedback path (topology drives gain)

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through (both wet and dry audible)
- [ ] preDelay audibly delays reverb onset (0ms vs 250ms clearly different)
- [ ] decay controls reverb tail length (0.1s vs 10s clearly different)
- [ ] density changes from sparse echoes to dense wash
- [ ] topology morphs reverb character (sparse at 0, dense at 100)
- [ ] No runaway feedback or clipping at any parameter combination
- [ ] mix blend works correctly (0% = dry only, 100% = wet only)

---

#### Phase 4.2: Spectral Tilt, Resonance, and LFO Modulation

**Goal:** Add the three "impossible" features to the single FDN: per-band independent decay (spectralTilt), resonant feedback peaks (resonance), and pitch-warping LFO modulation (modRate, modDepth).

**Components:**
- Spectral Tilt Filter Bank: two biquad shelf filters (low+high shelf) in FDN feedback path
  - Low shelf at 800Hz, high shelf at 800Hz, gains driven by spectralTilt
  - Hard stability limit: effective per-band feedback gain never exceeds 0.9998
- Resonance Injector: 3 narrow bandpass filters (Q=50) at 330Hz, 880Hz, 2200Hz
  - Stability limit: resonance contribution hard-clipped below 1.0/Q per filter
  - Parameter smoothing: smooth resonance parameter changes over 50ms to prevent clicks
- LFO Modulation Engine: 8 independent sine LFOs with ±15% frequency stagger and per-line phase offset
  - Switch DelayLine interpolation to Lagrange3rd for smooth modulated reads
  - modRate and modDepth parameters wired to LFO frequency and amplitude
- Stereo Width Processor: M/S matrix applied to FDN wet output

**Test Criteria:**
- [ ] spectralTilt = +100: reverb tail gets brighter over time (highs outlast lows)
- [ ] spectralTilt = -100: reverb tail gets darker over time (lows outlast highs)
- [ ] spectralTilt = 0: neutral decay (no brightening or darkening)
- [ ] resonance audibly creates pitched sustained peaks in tail
- [ ] resonance = 0: no resonant peaks
- [ ] resonance = 100: strong sustained peaks without runaway oscillation
- [ ] modDepth = 0: no pitch warping
- [ ] modDepth = 100: pronounced pitch instability in reflections
- [ ] width = 0%: wet signal is mono
- [ ] width = 200%: wet signal is very wide (clearly beyond normal stereo)
- [ ] No stability issues at any combination of extreme parameters

---

#### Phase 4.3: Dual FDN and Crossfade System

**Goal:** Add second FDN instance (FDN-B) and implement the wet signal crossfader. Verify seamless crossfade between two completely different FDN states.

**Components:**
- FDN-B: second complete FDN instance identical to FDN-A (all same components, separate state)
- Both FDN-A and FDN-B receive audio input continuously (both always processing)
- Equal-power crossfader: `gainA = cos(phase * π/2); gainB = sin(phase * π/2)` gain ramp
- crossfadeSpeed parameter: controls ramp duration in samples
- Crossfade state machine: idle / ramping / complete
- After crossfade completes: swap A/B roles (B becomes A for next mutation)

**Test Criteria:**
- [ ] Both FDN instances process audio simultaneously without crashes
- [ ] crossfadeSpeed = 0ms: instant switch between FDN states (no pop/click)
- [ ] crossfadeSpeed = 500ms: smooth 500ms blend between FDN states
- [ ] Crossfade triggered manually produces no audible clicks or artifacts
- [ ] After crossfade, new FDN state remains active as expected
- [ ] CPU usage within acceptable bounds (both instances running)
- [ ] No accumulation of denormals in long-running idle FDN instance

---

#### Phase 4.4: Mutation Timer and Lock System

**Goal:** Implement the full mutation cycle: timer fires every mutationInterval seconds, reads lock states, generates random values for unlocked parameters, applies to FDN-B, and triggers crossfade.

**Components:**
- `juce::Timer` subclass (`MutationTimer`) owned by PluginProcessor
- `startTimer()` called in `prepareToPlay()` with mutationInterval in ms
- Timer callback: read all lock booleans → generate uniform random values for unlocked params → write to APVTS → set `std::atomic<bool> mutationPending = true`
- Audio thread: reads `mutationPending` at start of processBlock, starts crossfade ramp if true
- mutationInterval parameter change: restarts timer with new interval
- `getRemainingTimeMs()` method for UI countdown display
- Guard: if crossfade is in progress, ignore new mutation trigger until previous completes
- Lock boolean parameters (10): topologyLock, decayLock, preDelayLock, densityLock, spectralTiltLock, resonanceLock, modRateLock, modDepthLock, widthLock, mixLock

**Test Criteria:**
- [ ] Timer fires on mutationInterval schedule (verify with debug log)
- [ ] All unlocked parameters receive new random values on mutation
- [ ] Locked parameters are unchanged by mutation cycle
- [ ] Each individual lock parameter can be toggled independently
- [ ] mutationInterval change takes effect on next mutation cycle
- [ ] Mutation with all parameters locked: crossfade still fires (FDN-B gets same values as FDN-A)
- [ ] Crossfade overlapping guard: rapid mutations (interval = 5s, crossfade = 500ms) work correctly
- [ ] Parameter randomization: values land across full range (verify with debug log, not just near center)
- [ ] Timer countdown decrements and resets correctly

---

### Stage 3: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate UI mockup HTML/CSS, establish WebView, bind primary space/output parameter controls.

**Components:**
- Copy final mockup HTML to `Source/ui/public/index.html`
- WebView setup in PluginEditor.h/cpp
- Configure CMakeLists.txt for WebView resources (NEEDS_WEB_BROWSER TRUE — see juce8-critical-patterns.md Pattern 9)
- `check_native_interop.js` included (see juce8-critical-patterns.md Pattern 13)
- Relay system for primary parameter knobs: topology, decay, preDelay, density, mix, width
- `WebSliderParameterAttachment` with 3-argument constructor (see juce8-critical-patterns.md Pattern 12)
- Relative drag knob implementation (see juce8-critical-patterns.md Pattern 16)
- ES6 module loading: `type="module"` on script tags (see juce8-critical-patterns.md Pattern 21)

**Test Criteria:**
- [ ] WebView window opens at correct plugin size
- [ ] Layout matches mockup design
- [ ] Primary knobs (topology, decay, preDelay, density, mix, width) are visible and styled
- [ ] No JavaScript errors in console
- [ ] Plugin loads without crashes in DAW (VST3 + AU)

---

#### Phase 5.2: Full Parameter Binding and Mutation Controls

**Goal:** Complete two-way parameter binding for all controls including lock buttons, spectral/motion parameters, mutation timer display, and crossfade.

**Components:**
- JavaScript → C++ relay calls for all float parameters (spectralTilt, resonance, modRate, modDepth, mutationInterval, crossfadeSpeed)
- C++ → JavaScript updates for host automation and preset changes
- Lock button binding: `getToggleState()` for all 10 lock boolean parameters (NOT `getSliderState()` — see juce8-critical-patterns.md Pattern 19)
- valueChangedEvent listeners using `getNormalisedValue()` inside callback (see juce8-critical-patterns.md Pattern 15)
- Mutation interval countdown display wired to `getRemainingTimeMs()` via Timer poll

**Test Criteria:**
- [ ] All float parameter knobs respond to mouse drag and update DSP
- [ ] Host automation updates all knob visuals
- [ ] Preset changes update all controls including lock buttons
- [ ] Lock buttons toggle correctly (locked = won't change on mutation)
- [ ] Mutation countdown timer displays and counts down correctly
- [ ] Mutation countdown resets after each mutation event
- [ ] No frozen or non-responsive knobs

---

#### Phase 5.3: Mutation System UI and Polish

**Goal:** Implement the mutation trigger display, timer countdown visualization, and any advanced UI elements specific to Chaosverb's mutation system.

**Components:**
- Mutation timer countdown display (text or radial progress)
- Visual indication when mutation fires (flash, pulse, or animation)
- Manual trigger button (bypass timer, fire mutation immediately)
- Visual indication of which parameters are locked vs unlocked
- Parameter value display labels
- Any Chaosverb-specific visual identity elements

**Test Criteria:**
- [ ] Mutation countdown visible and accurate
- [ ] Visual feedback when mutation fires
- [ ] Manual trigger button works (fires mutation without waiting for timer)
- [ ] Lock state visually clear (locked vs unlocked styling)
- [ ] Full visual polish consistent with design intent
- [ ] No performance issues or UI lag during active mutation cycles

---

### Implementation Flow

- Stage 1: Foundation — project structure, CMakeLists.txt (VST3 + AU + Standalone, NEEDS_WEB_BROWSER TRUE, IS_SYNTH FALSE)
- Stage 2: Shell — APVTS setup with 12 float + 10 boolean parameters, PluginProcessor/Editor skeleton
- Stage 3: DSP — 4 phases
  - Phase 4.1: Core single FDN (pre-delay, diffuser, FDN, dry/wet)
  - Phase 4.2: Spectral tilt, resonance, LFO modulation, stereo width
  - Phase 4.3: Dual FDN + crossfade system
  - Phase 4.4: Mutation timer + lock system
- Stage 3: GUI — 3 phases
  - Phase 5.1: WebView setup + primary knob binding
  - Phase 5.2: Full parameter binding + lock buttons
  - Phase 5.3: Mutation display + polish
- Stage 3: Validation — presets, pluginval, changelog

---

## Implementation Notes

### Thread Safety

- All parameter reads in processBlock use `getRawParameterValue()->load()` (atomic)
- Mutation timer fires on message thread — APVTS writes are message-thread operations (thread-safe)
- Crossfade start communicated via `std::atomic<bool> mutationPending` — set by message thread, read+cleared by audio thread
- Lock state reads happen on message thread during timer callback — same thread as APVTS writes
- No mutex or `std::lock_guard` in audio thread path — zero blocking operations
- FDN-A and FDN-B state is exclusively owned by audio thread — no cross-thread access to delay line data

### Performance

- Both FDN instances run continuously: ~30-50% CPU each at 48kHz with full parameter activation
- Use Fast Hadamard Transform (butterfly algorithm) to reduce matrix multiply from O(N²) to O(N log N)
- LFO can be updated every 4-8 samples (block-rate update) instead of every sample — ~4-8x LFO CPU reduction
- Biquad filter coefficient recalculation: only on parameter change, not every sample
- Consider SIMD if CPU targets are not met (8 delay lines can be processed in parallel)
- Both FDN instances must run even during normal operation (not just during crossfade) — this is the main CPU cost driver

### Latency

- Base latency: preDelay value in samples (0 to `sampleRate * 0.250`)
- FDN: no added algorithmic latency (causal feedback network)
- Report via `getLatencySamples()`: `static_cast<int>(preDelaySeconds * sampleRate)`
- DryWetMixer handles dry path latency compensation automatically
- With preDelay = 0ms: report 0 latency

### Denormal Protection

- `juce::ScopedNoDenormals` at top of `processBlock()` — essential for long FDN feedback paths
- DC offset injection: add 1e-9f to delay line inputs to prevent denormals at long decay times
- Both FDN-A and FDN-B need denormal protection even when idle (crossfade gain = 0)
- tanh() naturally bounds output to (-1, +1), indirectly prevents denormal accumulation

### Known Challenges

1. **FDN stability with topology=100:** Off-diagonal matrix coefficients can exceed 1.0, only safe because tanh() bounds all outputs. Test with decay=60s + topology=100 + resonance=100 to verify no runaway.
2. **Dual FDN CPU budget:** Two full 8-line FDNs at high sample rates may exceed 60% CPU. Profile early in Phase 4.3 and implement Fast Hadamard Transform if needed.
3. **Resonance parameter smoothing:** Jump from resonance=0 to resonance=100 on mutation must be smoothed over ~50ms to prevent clicks. Use APVTS parameter smoothing or manual linear ramp in processBlock.
4. **Crossfade state machine complexity:** Handling mutation during active crossfade (ignore or queue) must be decided early. Recommendation: ignore mutations while crossfade is active (simpler, prevents overlapping states).
5. **Delay line sizes at 192kHz:** All delay lines sized for maximum expected sample rate. Pre-delay buffer: `ceil(0.250 * 192000)` = 48000 samples. Allocate in `prepareToPlay()` with `juce::dsp::ProcessSpec`.

### CMakeLists.txt Requirements

From juce8-critical-patterns.md:
- `juce_generate_juce_header()` AFTER `target_link_libraries()` — Pattern 1
- `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin` — Pattern 9
- `IS_SYNTH FALSE` — Chaosverb is an effect, not an instrument — Pattern 22 (by contrast)
- Bus config: stereo input + stereo output in constructor: Pattern 4
- `juce::juce_dsp` in target_link_libraries — required for all juce::dsp classes
- `juce::juce_events` in target_link_libraries — required for juce::Timer

### APVTS Parameter List

Float parameters (12):
- topology (0-100, default 50)
- decay (0.1-60, default 4.0)
- preDelay (0-250ms, default 10)
- density (0-100, default 60)
- spectralTilt (-100 to +100, default 0)
- resonance (0-100, default 0)
- modRate (0.01-10Hz, default 0.3)
- modDepth (0-100, default 20)
- width (0-200, default 100)
- mix (0-100, default 50)
- mutationInterval (5-600s, default 30)
- crossfadeSpeed (0-500ms, default 100)

Boolean parameters (10):
- topologyLock, decayLock, preDelayLock, densityLock (default: false)
- spectralTiltLock, resonanceLock (default: false)
- modRateLock, modDepthLock (default: false)
- widthLock, mixLock (default: false)

Total: 22 parameters

---

## References

- Creative brief: `plugins/Chaosverb/.ideas/creative-brief.md`
- Parameter spec: `plugins/Chaosverb/.ideas/parameter-spec-draft.md`
- DSP architecture: `plugins/Chaosverb/.ideas/architecture.md`
- JUCE 8 critical patterns: `troubleshooting/patterns/juce8-critical-patterns.md`

Reference plugins for implementation patterns:
- FlutterVerb — DelayLine modulation with Lagrange3rd interpolation, LFO implementation, DryWetMixer usage: `plugins/FlutterVerb/Source/PluginProcessor.cpp`
- GainKnob — IIR filter pattern, biquad coefficient calculation, filter state reset: `plugins/GainKnob/Source/PluginProcessor.cpp`
- DrumRoulette — Thread safety patterns with atomic flags, background operations, message thread coordination: `plugins/DrumRoulette/Source/PluginProcessor.cpp`

External references:
- jatinchowdhury18/Feedback-Delay-Networks (GitHub) — Nonlinear FDN research implementation
- ninuxi/MAI_verb (GitHub) — Professional 8x8 FDN in JUCE (direct implementation reference)
- CCRMA FDN Reverberation: https://ccrma.stanford.edu/~jos/pasp/FDN_Reverberation.html
