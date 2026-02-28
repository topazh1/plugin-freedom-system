# NBS DynaDrive Changelog

## [1.3.0] - 2026-02-28

### Added

- **Compressor On/Off toggle** — New `comp_enable` boolean parameter with ON/OFF button in the dynamics section header. When OFF, the dynamics stage is bypassed entirely (saturation and other processing remain active). Dynamics knobs dim when disabled.
- **Drive section output volume** — New `drive_out` parameter (-24 to +12 dB) with VOL OUT knob at the bottom of the saturation section. Controls the output level of the saturation stage independently.
- **Compressor section output volume** — New `comp_out` parameter (-24 to +12 dB) with VOL OUT knob at the bottom of the dynamics section. Controls the output level of the dynamics stage independently.
- **Numerical I/O dB readouts** — Exact dB level text readouts below the IN and OUT meters, updated in real-time from smoothed peak values. Shows peak of L/R channels.
- **Gain reduction visualizer** — Real-time horizontal GR bar below the compressor curve display. Fills proportionally to gain reduction (0 to -24 dB range) with exact dB text readout. Reacts dynamically to compression activity.

### Fixed

- **Bypass button overlapping meters** — `#bypass-btn` was positioned `absolute` relative to `#main-panel` (which has `position: relative`) instead of `#center-title-block`. This caused the 62x22px button to land on top of the meter section, covering the IN/OUT labels and controls. Added `position: relative` to `#center-title-block` to contain the button within the title area.
- **Meter title visibility** — IN/OUT meter labels were `color: #444` (nearly invisible on dark background). Brightened to `#666` for readability.

## [1.2.0] - 2026-02-28

### Fixed

- **Aliasing reduced further** — Switched oversampling filter from polyphase IIR to FIR equiripple (`filterHalfBandFIREquiripple`, `isMaximumQuality = true`). FIR equiripple provides much steeper rolloff and better stopband rejection than IIR, significantly reducing aliased harmonics.
- **Volume compensation (auto-gain)** — The previous formula `y /= tanh(driveGain)/driveGain` was mathematically wrong — it amplified quiet signals by up to 64x at high drive. Replaced with `y /= driveGain` which correctly undoes the linear gain before the waveshaper, maintaining approximate unity level for small signals while providing natural compression for loud signals.
- **Bypass button stuck** — CSS `pointer-events: none` on `#main-panel` when bypassed blocked the bypass button itself from being clicked again. Added `pointer-events: auto` override on `#bypass-btn` in bypassed state.
- **M/S toggle click/pop** — Toggling M/S instantly switched signal routing (L/R ↔ Mid/Side), creating an audible discontinuity. Added a 10ms per-sample crossfade using `LinearSmoothedValue<float>` for both M/S encode and decode. Pre-computed blend buffer ensures identical values in both encode and decode passes.

### Added

- **Soft Clipping toggle** — New `sat_enable` boolean parameter with ON/OFF button in the saturation section header. When OFF, the saturation stage is bypassed entirely (dynamics and other processing remain active). Saturation knobs dim when disabled.

## [1.1.1] - 2026-02-27

### Fixed

- **Advanced panel toggle bar invisible** — Colors were near-invisible (`#3a3a3a` on `#111`). Brightened label to `#777`, arrow to `#666`, background to `#1a1a1a` with `#333` border. Hover states now reach `#aaa`.
- **GUI not resizable** — JUCE `ResizableCornerComponent` was hidden behind the native WebView (native OS views always sit on top of JUCE components). Removed the JUCE corner resizer (`setResizable(true, false)`) and added an HTML drag-to-resize grip at the bottom-right corner that calls a new `requestResize` native function. Host-provided resize handles also work.
- **Audible aliasing on saturation** — Increased oversampling from 2x standard to 4x maximum quality (`filterHalfBandPolyphaseIIR`, `isMaximumQuality = true`). Updated ADAA saturator prepare to 4x sample rate. Significantly reduces aliased harmonics from the waveshaper and pre-distortion stages.

## [1.1.0] - 2026-02-27

### Fixed

- Advanced panel collapsed height 400 -> 418px so toggle bar is visible
- Drive range reduced from 16x to 8x max gain, gain compensation clamped at 0.15 floor

### Added

- Bypass button with red UI toggle, dims plugin when active
- 2x oversampling wrapping ADAA saturation
- Resizable GUI with fixed aspect ratio scaling (0.75x-1.5x)

### Changed

- Shape knob display from "Soft/Medium/Hard" to continuous percentage

## [1.0.0] - 2026-02-27

- Initial release: ADAA saturation with harmonic controls, dynamics engine, tilt filters, M/S processing, WebView GUI
