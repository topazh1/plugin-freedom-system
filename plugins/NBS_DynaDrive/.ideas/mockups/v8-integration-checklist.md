# Stage 3 (GUI) Integration Checklist — NBS DynaDrive v8

**Plugin:** NBS_DynaDrive
**Mockup version:** v8
**Generated:** 2026-02-27
**Window size:** 740x400 (collapsed), 740x548 (expanded)
**Parameters:** 22 total (20 sliders + 2 toggles)

---

## Overview

This checklist guides gui-agent through integrating the finalized v8 WebView UI
into the JUCE plugin during Stage 3. All template files are in:
`plugins/NBS_DynaDrive/.ideas/mockups/`

---

## Phase 1 — Copy UI Files

- [ ] Create directory: `plugins/NBS_DynaDrive/Source/ui/public/js/juce/`
- [ ] Copy `v8-ui.html` to `Source/ui/public/index.html`
- [ ] Obtain JUCE frontend library and copy to `Source/ui/public/js/juce/index.js`
  - Source: `<JUCE_ROOT>/modules/juce_gui_extra/native/javascript/index.js`
  - Or copy from a working plugin (e.g., plugins/GainKnob/Source/ui/public/js/juce/index.js)
- [ ] Copy `check_native_interop.js` to `Source/ui/public/js/juce/check_native_interop.js`
  - Source: `<JUCE_ROOT>/modules/juce_gui_extra/native/javascript/check_native_interop.js`
  - Or copy from working plugin
- [ ] Verify no viewport units remain in index.html (grep for `100vh`, `100vw`)
- [ ] Verify `user-select: none` is present in index.html body CSS

---

## Phase 2 — Update PluginEditor Files

- [ ] Replace `Source/PluginEditor.h` with content from `v8-PluginEditor.h`
  - Update class name if different from `NBS_DynaDriveAudioProcessorEditor`
  - Update processor reference type if different from `NBS_DynaDriveAudioProcessor`
- [ ] Verify member declaration order in PluginEditor.h:
  - [ ] Section 1: All relay declarations (driveRelay...sideDriveRelay)
  - [ ] Section 2: webView declaration (ONE entry)
  - [ ] Section 3: All attachment declarations (driveAttachment...sideDriveAttachment)
  - [ ] Relay count (22) == Attachment count (22)
- [ ] Replace `Source/PluginEditor.cpp` with content from `v8-PluginEditor.cpp`
  - Update `#include "PluginProcessor.h"` if filename differs
  - Verify initializer list order matches declaration order in .h
  - Verify `audioProcessor.parameters.getParameter("X")` uses correct APVTS member name

---

## Phase 3 — Update CMakeLists.txt

- [ ] Open `plugins/NBS_DynaDrive/CMakeLists.txt`
- [ ] Add `NEEDS_WEB_BROWSER TRUE` to `juce_add_plugin()` call
  - Required for VST3 WebView support — without it VST3 builds but doesn't load in DAWs
- [ ] Add `juce_add_binary_data(NBS_DynaDrive_UIResources ...)` block (from v8-CMakeLists.txt)
  - Must include all 3 files: index.html, index.js, check_native_interop.js
- [ ] Add `NBS_DynaDrive_UIResources` to `target_link_libraries`
- [ ] Add `juce::juce_gui_extra` to `target_link_libraries`
- [ ] Ensure `juce_generate_juce_header(NBS_DynaDrive)` is present AFTER target_link_libraries
- [ ] Add compile definitions: `JUCE_WEB_BROWSER=1` and `JUCE_USE_CURL=0`

---

## Phase 4 — Verify Parameter IDs Match APVTS

The following 22 parameter IDs MUST exist in `PluginProcessor.cpp`'s
`AudioProcessorValueTreeState` layout. Each must match exactly (case-sensitive).

### Slider parameters (20)

| Parameter ID    | Section          | Range          | Default |
|-----------------|------------------|----------------|---------|
| drive           | Saturation       | 0–100 %        | 20      |
| even            | Saturation       | 0–100 %        | 0       |
| odd             | Saturation       | 0–100 %        | 0       |
| h_curve         | Saturation       | 0–100 %        | 50      |
| input           | Center           | -48–10 dB      | 0       |
| mix             | Center           | 0–100 %        | 100     |
| output          | Center           | -48–10 dB      | 0       |
| sat_tilt_freq   | Center           | 100–10000 Hz   | 1000    |
| sat_tilt_slope  | Center           | -6–6 dB/oct    | 0       |
| dynamics        | Dynamics         | 0–100 %        | 30      |
| up              | Dynamics         | 0–100 %        | 0       |
| down            | Dynamics         | 0–100 %        | 50      |
| threshold       | Adv: Dyn Detail  | -40–0 dB       | -18     |
| ratio           | Adv: Dyn Detail  | 1–10 :1        | 4       |
| attack_time     | Adv: Dyn Detail  | 0.1–100 ms     | 10      |
| release_time    | Adv: Dyn Detail  | 10–1000 ms     | 100     |
| dyn_tilt_freq   | Adv: Post-Dyn    | 100–10000 Hz   | 1000    |
| dyn_tilt_slope  | Adv: Post-Dyn    | -6–6 dB/oct    | 0       |
| mid_drive       | Adv: M/S         | 0–100 %        | 50      |
| side_drive      | Adv: M/S         | 0–100 %        | 50      |

### Toggle/Bool parameters (2)

| Parameter ID | Section | Values         | Default |
|--------------|---------|----------------|---------|
| pre_post     | Center  | false/true     | false   |
| ms_enable    | Adv: MS | false/true     | false   |

- [ ] All 22 parameter IDs verified present in APVTS layout
- [ ] `pre_post` defined as `AudioParameterBool` (not float)
- [ ] `ms_enable` defined as `AudioParameterBool` (not float)
- [ ] Relay types match parameter types (Bool params use WebToggleButtonRelay)

---

## Phase 5 — Build and Test (Debug)

- [ ] Configure: `cmake -B build -DCMAKE_BUILD_TYPE=Debug`
- [ ] Build: `cmake --build build --target NBS_DynaDrive_Standalone -- -j$(nproc)`
- [ ] Build succeeds without errors or warnings about missing headers
- [ ] Launch Standalone app
- [ ] WebView loads (not blank white/black screen)
- [ ] Right-click in WebView → "Inspect" opens DevTools
- [ ] DevTools Console shows no errors
- [ ] `window.__JUCE__` object exists in console
- [ ] `import { getSliderState }` resolves without error
- [ ] All knob SVGs render (not empty circles)
- [ ] Pre/Post toggle pill shows "DYN → SAT" (blue) by default
- [ ] M/S toggle shows "STEREO" (inactive) by default
- [ ] Advanced panel collapses/expands when clicking the toggle bar
- [ ] MID/SIDE knobs hidden when M/S is STEREO mode

---

## Phase 6 — Test Parameter Bindings (Debug)

- [ ] Drag DRIVE knob — arc updates, value display updates
- [ ] Drag EVEN knob — arc updates, drive curve SVG redraws
- [ ] Drag ODD knob — arc updates, drive curve SVG redraws
- [ ] Drag SHAPE knob — shows "Soft" / "Medium" / "Hard" in value display
- [ ] Drag INPUT knob — circle fill updates (360° indicator)
- [ ] Drag MIX knob — at 100% shows full ring; at 0% ring is empty
- [ ] Drag OUTPUT knob — circle fill updates (360° indicator)
- [ ] Click pre_post toggle — switches between DYN→SAT (blue) and SAT→DYN (red)
- [ ] Drag sat_tilt_freq — value shows kHz format above 1000 Hz
- [ ] Drag sat_tilt_slope — shows +/- sign in value display
- [ ] Drag DYNAMICS knob — dyn curve SVG redraws
- [ ] Expand Advanced panel — threshold/ratio/attack/release knobs visible
- [ ] Drag threshold — dyn curve threshold marker updates
- [ ] Drag ratio — dyn curve slope changes
- [ ] Click M/S toggle — "M/S" appears, MID/SIDE knobs become visible
- [ ] Drag MID/SIDE knobs (M/S active) — values update
- [ ] Test DAW automation: automate drive parameter — UI updates in real time
- [ ] Test preset recall: save preset, change values, recall — UI reflects recalled values
- [ ] Test plugin reload: close and reopen plugin — all values persist correctly

---

## Phase 7 — Build and Test (Release)

- [ ] Build Release: `cmake --build build --config Release --target NBS_DynaDrive_Standalone`
- [ ] Standalone launches without crash
- [ ] Run 10 plugin reload cycles (open/close Standalone) — no crashes
  - This specifically tests member destruction order correctness
  - Release builds expose order bugs that Debug builds hide
- [ ] Build VST3: `cmake --build build --config Release --target NBS_DynaDrive_VST3`
- [ ] Install VST3 to system and rescan in DAW
- [ ] Plugin appears in DAW effects browser (NOT instruments)
- [ ] Plugin loads in DAW without error
- [ ] WebView visible in DAW plugin window
- [ ] Parameter automation works in DAW

---

## Phase 8 — WebView-Specific Validation

- [ ] No `100vh`, `100vw`, `100dvh`, `100svh` in `Source/ui/public/index.html`
- [ ] `html, body { height: 100%; }` present (not viewport units)
- [ ] `user-select: none` present in body CSS
- [ ] `contextmenu` event prevented (no right-click browser menu)
- [ ] `type="module"` on all `<script>` tags using JUCE imports
- [ ] `import { getSliderState, getToggleState }` at top of module script
- [ ] No direct `window.__JUCE__.backend.getSliderState()` calls (use ES6 import)
- [ ] Resource provider returns 200 for `/`, `/index.html`, `/js/juce/index.js`, `/js/juce/check_native_interop.js`
- [ ] Resource provider returns `application/javascript` MIME type (not `text/javascript`) for .js files
- [ ] No 404 errors in DevTools Network tab

---

## Phase 9 — Meter Integration (if C++ sends meter data)

The UI listens for `meter_update` events from C++:
```javascript
window.__JUCE__.backend.addEventListener('meter_update', (data) => {
    // data.inL, data.inR, data.outL, data.outR  (0-100 percentage)
});
```

To send from C++ (in processBlock or a timer):
```cpp
// In PluginEditor.cpp — call after processBlock updates meter values
juce::String json = "{\"inL\":" + juce::String(inLPct) +
                    ",\"inR\":" + juce::String(inRPct) +
                    ",\"outL\":" + juce::String(outLPct) +
                    ",\"outR\":" + juce::String(outRPct) + "}";
webView->evaluateJavascript(
    "window.__JUCE__.backend.emitEvent('meter_update', " + json + ")",
    [](juce::WebBrowserComponent::EvaluationResult) {}
);
```

- [ ] C++ meter emission implemented (optional, UI shows static bars without it)
- [ ] Meter bars animate smoothly (ballistic attack/decay in requestAnimationFrame loop)
- [ ] Meter colors: green <70%, amber 70-85%, red >85%

---

## Common Integration Pitfalls

| Symptom | Likely Cause | Fix |
|---------|-------------|-----|
| Plugin loads but WebView is blank | Missing JUCE JS files, or resource provider returning wrong path | Check DevTools Network — look for 404 |
| Knobs display but don't respond to drag | Missing `type="module"` on script tag | Add `type="module"` to both script tags |
| Knobs visible but frozen after drag | `getSliderState` returning null | Verify parameter ID matches APVTS exactly |
| Crash on plugin reload (release only) | Member order wrong in PluginEditor.h | Verify: relays → webView → attachments |
| VST3 doesn't appear in DAW | Missing `NEEDS_WEB_BROWSER TRUE` in CMake | Add flag to `juce_add_plugin()` |
| "JuceHeader.h not found" build error | Missing `juce_generate_juce_header()` | Add after `target_link_libraries()` |
| toggle `pre_post` doesn't sync | Using `getSliderState` for bool param | Use `getToggleState` for Bool parameters |
| Knob jumps on click (absolute drag) | Using absolute drag from start point | Use relative delta (lastY, not startY) |
| Knob values undefined on init | Treating valueChangedEvent as value-passing | Call `getNormalisedValue()` inside callback |
