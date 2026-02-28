# NBS_DynaDrive - Implementation Plan

**Date:** 2026-02-27
**Complexity Score:** 5.0 (Complex — capped at maximum)
**Strategy:** Phase-based implementation

---

## Complexity Factors

- **Parameters:** 22 parameters (22/5 = 4.4, capped at 2.0) = **2.0**
- **Algorithms:** 7 DSP components = **7**
  - Input Gain (`juce::dsp::Gain`)
  - ADAA Waveshaping Engine (custom)
  - Post-Saturation Tilt Filter (`juce::dsp::IIR`)
  - Dynamics Engine (custom — dual upward+downward)
  - Post-Dynamics Tilt Filter (`juce::dsp::IIR`)
  - M/S Matrix Encoder/Decoder (custom)
  - DryWetMixer (`juce::dsp::DryWetMixer`)
- **Features:** 4 complexity points
  - Program-dependent dynamics (auto attack/release, crest factor analysis) (+1)
  - Conditional pre/post signal routing (dynamic processing order toggle) (+1)
  - M/S processing matrix (mid/side independent processing) (+1)
  - Upward + downward dual-path compression (non-standard compressor architecture) (+1)
- **Raw Total:** 2.0 + 7 + 4 = 13.0
- **Capped Total:** **5.0** (maximum complexity score)

---

## Stages

- Stage 0: Research & Planning ✓ (this document)
- Stage 1: Foundation (project scaffold + CMakeLists.txt)
- Stage 1: Shell (APVTS parameters + pass-through audio)
- Stage 3: DSP — 4 phases
- Stage 3: GUI — 3 phases
- Stage 3: Validation (presets, pluginval, changelog)

---

## Complex Implementation (Score 5.0)

### Stage 3: DSP Phases

#### Phase 4.1: Core Saturation (ADAA Foundation)

**Goal:** Implement the ADAA waveshaping engine with drive control only. Validate anti-aliasing correctness before adding harmonic controls or curve morphing. This is the highest-risk component and must be proven stable before building on it.

**Components:**
- Custom ADAA first-order implementation:
  - `y[n] = (F1(x[n]) - F1(x[n-1])) / (x[n] - x[n-1])` with near-singularity fallback
  - Tanh soft clipper with antiderivative: `F1(x) = log(cosh(k*x)) / k`
  - Per-channel `xPrev` state management
  - `juce::ScopedNoDenormals` in processBlock
- Input Gain stage (`juce::dsp::Gain<float>`)
- Output Gain stage (`juce::dsp::Gain<float>`)
- Basic stereo pass-through bus configuration (effect plugin: stereo in → stereo out)
- Dry/Wet Mixer (`juce::dsp::DryWetMixer<float>`)
- Drive parameter connected to ADAA input gain

**Test Criteria:**
- [ ] Plugin loads in DAW without crashes
- [ ] Audio passes through at drive=0 (unity, no distortion)
- [ ] Drive at 20% (default): mild saturation audible
- [ ] Drive at 100%: heavy saturation, no digital artifacts/aliasing spikes when sweeping frequency
- [ ] No NaN or Inf values in output (test with silence and full-scale input)
- [ ] No clicks or discontinuities when changing drive parameter in real time
- [ ] xPrev state correctly reset in prepareToPlay (no startup transients)
- [ ] CPU usage within estimated range (< 10% at 48kHz stereo)

---

#### Phase 4.2: Harmonic Controls + Curve Morph + Tilt Filters

**Goal:** Add even/odd harmonic controls (DC bias + cubic pre-distortion), h_curve morphing (tanh ↔ polynomial blend), and both tilt EQ stages. Complete the saturation section.

**Components:**
- Even harmonic control (DC bias technique):
  - `x_in = x + bias` where `bias = (even/100) * 0.15`
  - Post-waveshaper 5Hz highpass (`juce::dsp::IIR::Filter<float>` with highpass coefficients)
- Odd harmonic control (cubic pre-distortion):
  - `x_in = x + (odd/100) * 0.05 * x^3` (applied before bias)
- h_curve polynomial blend:
  - Soft: tanh waveshaper with `F1_soft(x) = log(cosh(k*x))/k`
  - Hard: cubic polynomial `f_hard(x) = 1.5x - 0.5x^3` with `F1_hard(x) = 0.75x^2 - 0.125x^4`
  - Blend: `f_blend = (1-alpha)*f_soft + alpha*f_hard`, `F1_blend = (1-alpha)*F1_soft + alpha*F1_hard`
- Post-Saturation Tilt Filter:
  - Low shelf + high shelf pair (`juce::dsp::IIR::Coefficients<float>::makeLowShelf/makeHighShelf`)
  - `juce::dsp::ProcessorDuplicator` for stereo processing
  - Bypass when `sat_tilt_slope == 0` (atomic flag)
- Post-Dynamics Tilt Filter (same architecture, different parameters)

**Test Criteria:**
- [ ] even knob at 100%: audible warmth/fullness, even harmonics increase (measurable 2nd harmonic)
- [ ] odd knob at 100%: audible presence/grit, odd harmonics increase (measurable 3rd harmonic)
- [ ] h_curve at 0%: smooth/gentle saturation onset
- [ ] h_curve at 100%: aggressive clipping character, harder knee audible
- [ ] h_curve morphing is continuous (no discontinuities when sweeping 0-100%)
- [ ] sat_tilt_slope at +6: audibly brighter, highpass tilt
- [ ] sat_tilt_slope at -6: audibly darker, lowpass tilt
- [ ] sat_tilt_slope at 0: no EQ effect (bypass active)
- [ ] sat_tilt_freq sweeps from 100Hz to 10kHz — tilt pivot audibly shifts
- [ ] No DC buildup in output at high even values (DC highpass working)
- [ ] All saturation parameters smooth during real-time adjustment (no zipper noise)

---

#### Phase 4.3: Dynamics Engine + Pre/Post Routing + M/S Processing

**Goal:** Implement the complete dynamics engine (downward + upward compression with program-dependent ballistics), conditional pre/post routing, and M/S matrix processing. Final DSP feature completion.

**Components:**
- Dynamics Engine:
  - RMS level detector with 10ms window
  - Program-dependent ballistics (crest factor analysis over 50ms window, 200ms smoothed)
  - Downward compression with soft-knee (threshold, ratio, down amount, dynamics macro)
  - Upward compression (raises signals below threshold by up to 30% of gap × up amount × dynamics macro)
  - Gain smoothing via ballistic one-pole IIR
  - Linked stereo detection (max of L/R for gain computation)
- Pre/Post routing (pre_post bool parameter):
  - DYN→SAT: Dynamics → ADAA → Post-Sat Tilt → Post-Dyn Tilt
  - SAT→DYN: ADAA → Post-Sat Tilt → Dynamics → Post-Dyn Tilt
  - Implemented as conditional buffer routing in processBlock
- M/S Matrix:
  - Encoder: Mid = (L+R)*0.5, Side = (L-R)*0.5
  - Decoder: L = Mid+Side, R = Mid-Side
  - mid_drive replaces drive for mid channel in M/S mode
  - side_drive replaces drive for side channel in M/S mode
  - Independent ADAA state (xPrevMid, xPrevSide) in M/S mode

**Test Criteria:**
- [ ] Dynamics at 30% (default): subtle compression character audible on peaks
- [ ] down=100%, dynamics=100%: significant downward compression, gain reduction audible
- [ ] up=100%, dynamics=100%, threshold=-18dB: quiet signals lifted, dense sustain
- [ ] threshold parameter audibly changes onset of compression
- [ ] ratio parameter changes compression severity above threshold
- [ ] attack_time fast (0.1ms): transients pass through unaffected initially
- [ ] attack_time slow (100ms): transients more compressed
- [ ] release_time fast (10ms): gain returns quickly after loud passages
- [ ] release_time slow (1000ms): gain holds long after loud passages
- [ ] pre_post=false (DYN→SAT): dynamics controls what hits saturation (different character)
- [ ] pre_post=true (SAT→DYN): saturation colors signal, dynamics then controls result
- [ ] Toggling pre_post in real time: clean transition (no click or level jump)
- [ ] ms_enable=true: M/S mode active, mid/side processed independently
- [ ] mid_drive higher than side_drive: center image more saturated than sides
- [ ] side_drive=0, mid_drive=50: sides clean, center saturated
- [ ] M/S mode: stereo image preserved after encode/decode round-trip (phase correlation check)
- [ ] No pumping artifacts from upward compression on sustained tones
- [ ] Program-dependent ballistics: transient signals (drums) respond faster than sustained (pads)

---

### Stage 3: GUI Phases

#### Phase 5.1: Layout and Basic Controls

**Goal:** Integrate the finalized v8 WebView UI into the plugin. Wire up all 20 slider parameters and 2 boolean parameters. The UI template files are already generated in the mockups folder.

**Components:**
- Copy `v8-ui.html` → `Source/ui/public/index.html`
- Copy JUCE JS frontend library → `Source/ui/public/js/juce/index.js`
- Copy `check_native_interop.js` → `Source/ui/public/js/juce/check_native_interop.js`
- Replace `PluginEditor.h` with `v8-PluginEditor.h` template
- Replace `PluginEditor.cpp` with `v8-PluginEditor.cpp` template
- Update `CMakeLists.txt` with WebView configuration from `v8-CMakeLists.txt`:
  - `NEEDS_WEB_BROWSER TRUE`
  - `juce_add_binary_data(NBS_DynaDrive_UIResources ...)`
  - `juce::juce_gui_extra` in `target_link_libraries`
  - `JUCE_WEB_BROWSER=1` and `JUCE_USE_CURL=0` in compile definitions

**Test Criteria:**
- [ ] WebView window opens at correct size (740x400 collapsed, 740x548 expanded)
- [ ] All 8 sections render: Saturation, Center, Dynamics, Advanced
- [ ] Rotary knobs visible and styled correctly (red for saturation, blue for dynamics, gray for center)
- [ ] Pre/Post toggle pill renders as "DYN→SAT" blue (default)
- [ ] M/S toggle renders as "STEREO" (inactive)
- [ ] Advanced panel collapsed by default
- [ ] Advanced panel expands when clicking the toggle bar
- [ ] MID/SIDE knobs hidden when M/S is STEREO
- [ ] Background and color scheme matches v8 design
- [ ] No JavaScript console errors

---

#### Phase 5.2: Parameter Binding and Interaction

**Goal:** Two-way parameter communication between WebView UI and DSP engine. All 22 parameters must bind correctly.

**Components:**
- Verify all 22 relay constructors match parameter IDs exactly (case-sensitive)
- Verify 20 `WebSliderParameterAttachment` instances use 3-parameter constructor (JUCE 8 requirement: `..., nullptr`)
- Verify 2 `WebToggleButtonParameterAttachment` instances for `pre_post` and `ms_enable`
- Test slider drag → DSP parameter change (each of 20 float params)
- Test toggle click → DSP bool parameter change (pre_post, ms_enable)
- Test host automation: DAW sends param value → UI updates
- Test preset recall: change values, save preset, recall → UI reflects new values
- Test plugin reload: close/reopen → all parameter values persist

**Test Criteria:**
- [ ] DRIVE knob drag → drive parameter changes, ADAA saturation audibly changes
- [ ] EVEN knob drag → even harmonics change in real time
- [ ] ODD knob drag → odd harmonics change in real time
- [ ] SHAPE knob → shows "Soft" / "Medium" / "Hard" text label correctly at thresholds
- [ ] INPUT/OUTPUT knobs use 360° fill ring — at 0dB shows mid fill, at +10dB fully lit
- [ ] MIX knob at 0% → full dry signal, at 100% → full wet signal
- [ ] Pre/Post toggle → switches between DYN→SAT (blue) and SAT→DYN (red)
- [ ] sat_tilt_freq value displays "1.0kHz" format above 1000Hz
- [ ] sat_tilt_slope shows +/- sign in value display
- [ ] M/S toggle → "M/S" label active, MID/SIDE knobs become visible
- [ ] Dragging MID knob when M/S active → mid_drive DSP parameter changes
- [ ] All parameter names in `valueChangedEvent` callbacks use `getNormalisedValue()` (not callback param)
- [ ] Knob drag is relative (delta-based, not absolute) — knobs don't jump to cursor
- [ ] Host automation: automate DRIVE in DAW → UI DRIVE knob updates in real time
- [ ] Preset save/recall: all 22 parameter values correctly restore

---

#### Phase 5.3: Advanced UI Elements (Meters + Curve Displays)

**Goal:** Implement real-time meter animation and DSP curve visualizations. The v8 UI includes input/output meters and saturation/dynamics curve SVG displays.

**Components:**
- Level meter data emission from C++ (PluginProcessor tracks peak levels):
  - Track `inL`, `inR`, `outL`, `outR` peak levels in processBlock (atomic floats)
  - PluginEditor timer (30Hz) reads peaks, sends `meter_update` event to WebView
  - WebView `requestAnimationFrame` loop animates meter bars with ballistic motion
- Saturation curve SVG display:
  - Redraws when drive, even, odd, or h_curve change
  - Drawn from JS: plots `f_blend(x)` with approximate math for visual display (not ADAA math)
- Dynamics curve SVG display:
  - Redraws when threshold, ratio, dynamics, up, down change
  - Shows compressor knee, threshold marker, upward/downward regions

**Test Criteria:**
- [ ] Meter bars animate with audio input (attack fast, decay slow — ballistic motion)
- [ ] Meter colors: green <70%, amber 70-85%, red >85%
- [ ] Meter updates at ~30Hz (not every processBlock — use timer)
- [ ] Saturation curve SVG redraws on drive/shape/harmonic parameter changes
- [ ] Saturation curve shape visually matches soft/medium/hard expectation
- [ ] Dynamics curve shows threshold marker at correct position
- [ ] Dynamics curve slope changes with ratio parameter
- [ ] Dynamics curve upward region visible when up > 0
- [ ] All visualizations update smoothly (no dropped frames or jerky updates)
- [ ] No UI thread stalls from meter update frequency

---

### Implementation Flow

- Stage 1: Foundation (CMakeLists.txt, project structure, source files scaffold)
- Stage 1: Shell (APVTS 22-parameter layout, pass-through audio, bus config as stereo effect)
- Stage 3: DSP — 3 phases
  - Phase 4.1: Core ADAA saturation + drive + dry/wet + gain stages
  - Phase 4.2: Harmonic controls (even/odd) + h_curve morph + tilt filters (sat + dyn)
  - Phase 4.3: Dynamics engine + pre/post routing + M/S matrix
- Stage 3: GUI — 3 phases
  - Phase 5.1: Layout and basic controls (copy v8 template, WebView setup, CMake)
  - Phase 5.2: Parameter binding (all 22 params, relay verification, host automation)
  - Phase 5.3: Advanced UI (meters, curve displays, animations)
- Stage 3: Validation (presets, pluginval, changelog v1.0.0)

---

## Implementation Notes

### Thread Safety

- ADAA state (`xPrev` per channel): Audio thread only, no locking needed
- Dynamics state (level detector, gain smoother): Audio thread only, no locking needed
- Tilt coefficient update flags: `std::atomic<bool>` — set on message thread, read/clear on audio thread
- All parameter reads: `getRawParameterValue("X")->load()` (lock-free atomic)
- Level meter peaks for UI: `std::atomic<float>` per channel — write in processBlock, read in editor timer

### Performance

- ADAA: Uses `std::log`, `std::cosh` per sample — consider fast math approximations if CPU exceeds 15%
  - `log(cosh(x))` for large |x| can be approximated as `|x| - log(2)` to avoid overflow
  - For |x| < 5: standard `log(cosh(x))` is numerically stable
- Tilt coefficients: recomputed only on parameter change (not per-sample or per-block)
- Dynamics crest factor: computed per 50ms block, not per-sample
- Budget: aim for < 25% CPU at 48kHz stereo, 512-sample buffer

### Latency

- Zero latency design — no oversampling, no lookahead
- `getLatencySamples()` returns 0
- Host does NOT need to compensate latency
- DryWetMixer configured with 0 delay: `setWetLatency(0)` or ensure no latency source

### Denormal Protection

- `juce::ScopedNoDenormals noDenormals` at top of processBlock
- Dynamics level detector: add `levelSquared = std::max(levelSquared, 1e-9f)` floor
- ADAA: `xPrev` initialized to 0 — near-zero inputs handled by near-singularity fallback

### Known Challenges

1. **ADAA near-singularity case:** When consecutive samples are very close (|x[n] - x[n-1]| < 1e-5), division by near-zero must be detected and fallback applied. Test specifically with constant DC input and with very slow sine sweeps.

2. **log(cosh(x)) overflow:** For large |x| (> ~85), `cosh(x)` overflows float. Solution: Use the identity `log(cosh(x)) = |x| + log(1 + exp(-2|x|)) - log(2)` for |x| > 15 (safe threshold).

3. **M/S decode placement:** The M/S decoder must be placed AFTER both saturation and dynamics, regardless of pre_post mode. This means in SAT→DYN mode: Encode → SAT → Post-Sat Tilt → DYN → Decode → Post-Dyn Tilt.

4. **Dynamics + M/S interaction:** When ms_enable=true, the dynamics engine needs to process mid and side as separate mono signals, maintaining independent gain computers. The simplest approach is two instances of the dynamics state variables (level, gain) for mid and side channels.

5. **Tilt filter coefficient thread safety:** IIR filter objects in JUCE have a `coefficients` pointer. In `ProcessorDuplicator`, updating the shared coefficient pointer requires care. The recommended pattern: compute new coefficients on audio thread (gated by atomic flag), then assign to `filter.coefficients` — this is atomic in JUCE 8.

6. **upward + downward gain combining:** Both gain computers run simultaneously. Gains are computed in dB and added before converting to linear. Ensure that upward gain (positive dB) and downward gain reduction (negative dB) are correctly signed before summation.

### Pre-built UI Templates (Stage 3 GUI)

The following files in `plugins/NBS_DynaDrive/.ideas/mockups/` are ready to use in Stage 3:

- `v8-ui.html` → copy to `Source/ui/public/index.html`
- `v8-PluginEditor.h` → replace `Source/PluginEditor.h`
- `v8-PluginEditor.cpp` → replace `Source/PluginEditor.cpp`
- `v8-CMakeLists.txt` → reference for CMakeLists.txt updates
- `v8-integration-checklist.md` → step-by-step integration verification

**Important:** The v8 template files assume class names `NBS_DynaDriveAudioProcessor` and `NBS_DynaDriveAudioProcessorEditor`. Verify these match the generated foundation-shell code.

---

## References

- Creative brief: `plugins/NBS_DynaDrive/.ideas/creative-brief.md`
- Parameter spec: `plugins/NBS_DynaDrive/.ideas/parameter-spec.md`
- DSP architecture: `plugins/NBS_DynaDrive/.ideas/architecture.md`
- UI mockup (final): `plugins/NBS_DynaDrive/.ideas/mockups/v8-ui.html`
- Integration checklist: `plugins/NBS_DynaDrive/.ideas/mockups/v8-integration-checklist.md`

Reference plugins for implementation patterns:
- `plugins/AutoClip/` — Hard clipper, APVTS pattern, ScopedNoDenormals, processBlock structure
- `plugins/Chaosverb/` — Complex multi-component DSP, IIR filter usage, signal routing patterns
- `plugins/FlutterVerb/` — DryWetMixer usage, WebView integration with many parameters
