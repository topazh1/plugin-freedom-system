# Stage 3 (GUI) Integration Checklist — v1

**Plugin:** Chaosverb
**Mockup Version:** v1
**Generated:** 2026-02-27
**Window:** 780 x 360 (fixed size)
**Total Parameters:** 22 (12 Float + 10 Bool)
**Relay Count:** 22 (12 WebSliderRelay + 10 WebToggleButtonRelay)
**Attachment Count:** 22 (12 WebSliderParameterAttachment + 10 WebToggleButtonParameterAttachment)

---

## Pre-Flight: Verify Stage 2 Complete

Before starting GUI integration, verify Stage 2 (Shell) is done:

- [ ] PluginProcessor.cpp has all 22 APVTS parameters declared (see parameter-spec.md)
- [ ] All 12 Float parameter IDs match exactly: `topology`, `decay`, `preDelay`, `density`,
  `spectralTilt`, `resonance`, `modRate`, `modDepth`, `mutationInterval`, `crossfadeSpeed`,
  `width`, `mix`
- [ ] All 10 Bool parameter IDs match exactly: `topologyLock`, `decayLock`, `preDelayLock`,
  `densityLock`, `spectralTiltLock`, `resonanceLock`, `modRateLock`, `modDepthLock`,
  `widthLock`, `mixLock`
- [ ] Plugin builds cleanly (debug) before touching the editor

---

## 1. Create UI File Structure

- [ ] Create directory: `plugins/Chaosverb/Source/ui/public/js/juce/`
- [ ] Copy `v1-ui.html` to `Source/ui/public/index.html`
- [ ] Copy JUCE frontend library to `Source/ui/public/js/juce/index.js`
  - Source: `[JUCE_ROOT]/modules/juce_gui_extra/native/javascript/index.js`
  - Or copy from a working plugin (e.g., GainKnob example)
- [ ] Copy `Source/ui/public/js/juce/check_native_interop.js`
  - Source: `[JUCE_ROOT]/modules/juce_gui_extra/native/javascript/check_native_interop.js`
  - Pattern 13: This file is REQUIRED — UI will silently fail without it

---

## 2. Update CMakeLists.txt

Reference: `v1-CMakeLists-SNIPPET.txt`

- [ ] Add `NEEDS_WEB_BROWSER TRUE` to existing `juce_add_plugin()` call (Pattern 9)
- [ ] Add `juce_add_binary_data(Chaosverb_UIResources ...)` block after `juce_add_plugin()`
  - Verify `Source/ui/public/index.html` is included
  - Verify `Source/ui/public/js/juce/index.js` is included
  - Verify `Source/ui/public/js/juce/check_native_interop.js` is included
- [ ] Add `juce::juce_gui_extra` to `target_link_libraries()` (Pattern 3)
- [ ] Add `Chaosverb_UIResources` to `target_link_libraries()`
- [ ] Add `JUCE_WEB_BROWSER=1` to `target_compile_definitions()` (Pattern 3)
- [ ] Add `JUCE_USE_CURL=0` to `target_compile_definitions()`
- [ ] Verify `juce_generate_juce_header(Chaosverb)` comes AFTER `target_link_libraries()` (Pattern 1)
- [ ] Run CMake configure — verify no errors before building

---

## 3. Update PluginEditor.h

Reference: `v1-PluginEditor-TEMPLATE.h`

- [ ] Replace PluginEditor.h with template content
- [ ] Verify includes: `<juce_audio_processors/juce_audio_processors.h>` and `<juce_gui_extra/juce_gui_extra.h>`
- [ ] Verify member DECLARATION ORDER (top to bottom in private section):
  - [ ] Section 1: 12 `std::unique_ptr<juce::WebSliderRelay>` members
  - [ ] Section 2: 10 `std::unique_ptr<juce::WebToggleButtonRelay>` members
  - [ ] Section 3: `std::unique_ptr<juce::WebBrowserComponent> webView`
  - [ ] Section 4: 12 `std::unique_ptr<juce::WebSliderParameterAttachment>` members
  - [ ] Section 5: 10 `std::unique_ptr<juce::WebToggleButtonParameterAttachment>` members
  - ORDER MATTERS: Relays -> WebView -> Attachments (destruction is reverse order)
- [ ] Verify relay count: 22 total (12 sliders + 10 toggles)
- [ ] Verify attachment count: 22 total (matches relay count)
- [ ] Verify `getResource()` private method declared

---

## 4. Update PluginEditor.cpp

Reference: `v1-PluginEditor-TEMPLATE.cpp`

- [ ] Replace PluginEditor.cpp with template content
- [ ] Verify APVTS member name — template uses `audioProcessor.parameters`, update if different
- [ ] Verify CONSTRUCTION ORDER in constructor body (must match declaration order):
  - [ ] All 22 relays created first (before webView)
  - [ ] webView created second (after all relays, before attachments)
  - [ ] All 22 attachments created last (after webView)
- [ ] Verify WebSliderRelay IDs match JUCE parameter IDs exactly (case-sensitive):
  - `"topology"`, `"decay"`, `"preDelay"`, `"density"`, `"spectralTilt"`, `"resonance"`
  - `"modRate"`, `"modDepth"`, `"mutationInterval"`, `"crossfadeSpeed"`, `"width"`, `"mix"`
- [ ] Verify WebToggleButtonRelay IDs match:
  - `"topologyLock"`, `"decayLock"`, `"preDelayLock"`, `"densityLock"`, `"spectralTiltLock"`
  - `"resonanceLock"`, `"modRateLock"`, `"modDepthLock"`, `"widthLock"`, `"mixLock"`
- [ ] Verify all 22 relays registered with `.withOptionsFrom(*relay)` in WebBrowserComponent::Options{}
- [ ] Verify WebSliderParameterAttachment uses 3-argument constructor: `(param, relay, nullptr)` (Pattern 12)
- [ ] Verify WebToggleButtonParameterAttachment uses 3-argument constructor: `(param, relay, nullptr)` (Pattern 12)
- [ ] Verify `setSize(780, 360)` in constructor body (from v1-ui.yaml dimensions)
- [ ] Wire up "mutateNow" native function (see NOTE in template):
  - [ ] Register native function that calls `audioProcessor.triggerMutationNow()`
  - [ ] Verify triggerMutationNow() is public on the processor
- [ ] Verify `getResource()` implementation:
  - [ ] Returns index.html for "/" and "/index.html"
  - [ ] Returns index.js for "/js/juce/index.js" with MIME "application/javascript"
  - [ ] Returns check_native_interop.js for "/js/juce/check_native_interop.js" (Pattern 13)
  - [ ] Returns std::nullopt for unknown URLs

---

## 5. Verify Production HTML

The `Source/ui/public/index.html` (copied from `v1-ui.html`) must satisfy:

- [ ] No viewport units in CSS — grep for `vh`, `vw`, `dvh`, `svh` (must find zero)
- [ ] `html, body { height: 100%; }` present
- [ ] `user-select: none` and `-webkit-user-select: none` present
- [ ] Context menu disabled: `document.addEventListener("contextmenu", (e) => e.preventDefault())`
- [ ] JUCE imports use `type="module"`: `<script type="module" src="js/juce/index.js"></script>`
- [ ] Inline script also has `type="module"`
- [ ] `import { getSliderState, getToggleState } from './js/juce/index.js'` at top of script (Pattern 21)
- [ ] All 12 float parameters use `getSliderState("paramId")`
- [ ] All 10 bool parameters use `getToggleState("paramId")` (Pattern 19)
- [ ] `valueChangedEvent.addListener(() => {...})` callbacks have NO parameters (Pattern 15)
- [ ] Knob drag uses relative delta (Pattern 16): `lastY = e.clientY` per frame, not `startY`

---

## 6. Build and Verify (Debug)

- [ ] `cmake --build build --target Chaosverb_Standalone` — clean build, no warnings
- [ ] Launch standalone — WebView loads (not blank, not "frame load interrupted")
- [ ] Open DevTools (right-click -> Inspect) — verify available
- [ ] Console shows no JavaScript errors on load
- [ ] `window.__JUCE__` object exists in console
- [ ] `import { getSliderState } from './js/juce/index.js'` works in console (no 404)
- [ ] All 12 knobs visible and positioned correctly
- [ ] Both horizontal sliders visible
- [ ] Countdown display showing correctly
- [ ] Mutate Now button visible in header

---

## 7. Test Parameter Binding

- [ ] Drag topology knob — verify parameter changes in DAW automation view
- [ ] Drag decay knob — knob rotates and value display updates
- [ ] Drag pre-delay knob — correct range 0–250ms
- [ ] Drag density knob — correct range 0–100%
- [ ] Drag spectralTilt knob — verify bipolar: center = 0, displays +/- values
- [ ] Drag resonance knob — correct range 0–100%
- [ ] Drag modRate knob — correct range 0.01–10Hz with logarithmic feel
- [ ] Drag modDepth knob — correct range 0–100%
- [ ] Drag mutationInterval slider — countdown display updates in sync
- [ ] Drag crossfadeSpeed slider — displays correct 0–500ms
- [ ] Drag width knob — correct range 0–200%
- [ ] Drag mix knob — correct range 0–100%
- [ ] Double-click or right-click any lockable knob — lock ring appears (solid 360deg halo)
- [ ] Locked knob ignores drag
- [ ] Unlock knob — returns to normal arc display
- [ ] All 10 lock states survive preset recall
- [ ] Mutate Now button fires mutation flash overlay
- [ ] Automation replay: record knob movement, play back — UI follows correctly

---

## 8. Build and Verify (Release)

- [ ] `cmake --build build --config Release --target Chaosverb_Standalone`
- [ ] Launch Standalone in Release — no crash on open
- [ ] Reload plugin 10 times — no crash on each open/close
  - This validates member destruction order (Pattern 11)
  - Crashes ONLY in release builds indicate member order violation in .h file
- [ ] Build VST3: `cmake --build build --config Release --target Chaosverb_VST3`
- [ ] Install and load in DAW — plugin appears in scanner (NEEDS_WEB_BROWSER TRUE validates Pattern 9)
- [ ] Build AU: `cmake --build build --config Release --target Chaosverb_AU`
- [ ] Load AU in Logic — WebView renders correctly

---

## 9. Chaosverb-Specific Validation

These checks are unique to Chaosverb's mutation system:

- [ ] Mutation timer fires at correct interval (verify with 5s minimum setting)
- [ ] All unlocked parameters randomize simultaneously on mutation event
- [ ] Locked parameters are skipped during mutation (test with 1 param locked)
- [ ] Crossfade produces no clicks at 0ms setting (instant cut)
- [ ] Crossfade at 500ms: smooth 0.5-second dissolve (verify with long decay)
- [ ] Mutate Now triggers mutation immediately (ignores countdown)
- [ ] Countdown display resets after mutation event
- [ ] Countdown switches to red/pulse mode at <= 5 seconds
- [ ] UI mutation flash (white overlay) appears on every mutation event
- [ ] Lock states saved and restored correctly in preset recall
- [ ] Two parallel FDN instances during crossfade: verify no CPU spike above budget

---

## 10. Spectral Tilt Bipolar Knob Validation

The spectralTilt parameter is bipolar (-100 to +100, default 0). The WebView UI
must handle this correctly:

- [ ] At default (0.5 normalized): knob shows center position
- [ ] At 0.0 normalized (min -100): knob at full-left, displays "-100"
- [ ] At 1.0 normalized (max +100): knob at full-right, displays "+100"
- [ ] Value display shows "+" prefix for positive values, "-" prefix for negative
- [ ] 0 value displays as "0" (no + prefix)
- [ ] Verify APVTS stores as normalized 0–1 float (JUCE handles this automatically)

---

## Parameter Quick Reference

| JUCE ID | Type | Range | Default | Relay Type | Lockable |
|---|---|---|---|---|---|
| topology | Float | 0.0–100.0 | 50.0 | WebSliderRelay | Yes |
| decay | Float | 0.1–60.0 s | 4.0 | WebSliderRelay | Yes |
| preDelay | Float | 0.0–250.0 ms | 10.0 | WebSliderRelay | Yes |
| density | Float | 0.0–100.0 % | 60.0 | WebSliderRelay | Yes |
| spectralTilt | Float | -100.0–100.0 | 0.0 | WebSliderRelay | Yes |
| resonance | Float | 0.0–100.0 % | 0.0 | WebSliderRelay | Yes |
| modRate | Float | 0.01–10.0 Hz | 0.3 | WebSliderRelay | Yes |
| modDepth | Float | 0.0–100.0 % | 20.0 | WebSliderRelay | Yes |
| mutationInterval | Float | 5.0–600.0 s | 30.0 | WebSliderRelay | No |
| crossfadeSpeed | Float | 0.0–500.0 ms | 100.0 | WebSliderRelay | No |
| width | Float | 0.0–200.0 % | 100.0 | WebSliderRelay | Yes |
| mix | Float | 0.0–100.0 % | 50.0 | WebSliderRelay | Yes |
| topologyLock | Bool | false/true | false | WebToggleButtonRelay | — |
| decayLock | Bool | false/true | false | WebToggleButtonRelay | — |
| preDelayLock | Bool | false/true | false | WebToggleButtonRelay | — |
| densityLock | Bool | false/true | false | WebToggleButtonRelay | — |
| spectralTiltLock | Bool | false/true | false | WebToggleButtonRelay | — |
| resonanceLock | Bool | false/true | false | WebToggleButtonRelay | — |
| modRateLock | Bool | false/true | false | WebToggleButtonRelay | — |
| modDepthLock | Bool | false/true | false | WebToggleButtonRelay | — |
| widthLock | Bool | false/true | false | WebToggleButtonRelay | — |
| mixLock | Bool | false/true | false | WebToggleButtonRelay | — |

---

## Critical Patterns Applied

| Pattern | Description | Where Applied |
|---|---|---|
| Pattern 1 | `juce_generate_juce_header()` after `target_link_libraries()` | CMakeLists.txt |
| Pattern 3 | `juce::juce_gui_extra` + `JUCE_WEB_BROWSER=1` | CMakeLists.txt |
| Pattern 8 | Explicit URL mapping in resource provider | PluginEditor.cpp `getResource()` |
| Pattern 9 | `NEEDS_WEB_BROWSER TRUE` in `juce_add_plugin()` | CMakeLists.txt |
| Pattern 11 | `std::unique_ptr` members, order: Relays->WebView->Attachments | PluginEditor.h |
| Pattern 12 | 3-argument WebSliderParameterAttachment: `(param, relay, nullptr)` | PluginEditor.cpp |
| Pattern 13 | `check_native_interop.js` in binary data + resource provider | CMakeLists.txt + PluginEditor.cpp |
| Pattern 15 | `valueChangedEvent.addListener(() => {...})` — no callback params | v1-ui.html |
| Pattern 16 | Relative drag for knobs: `lastY` per frame | v1-ui.html |
| Pattern 19 | `getToggleState()` for Bool parameters | v1-ui.html |
| Pattern 21 | `type="module"` on script tags + ES6 import | v1-ui.html |
