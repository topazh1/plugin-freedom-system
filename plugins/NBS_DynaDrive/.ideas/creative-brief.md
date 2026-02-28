# NBS DynaDrive — Creative Brief

## The Problem

Digital saturation has always forced a compromise: clean but lifeless, or colored but aliased. Oversampling is the standard fix, but it introduces latency, burns CPU, and still sounds "digital" at high drive. The mixing and mastering world deserves better.

## The Vision

NBS DynaDrive is a mastering-grade saturation plugin that refuses the compromise. It combines **transformer harmonic character** (Neve-style iron warmth) with **program-dependent dynamics** (API-style transient authority) — implemented via **ADAA anti-aliasing** (Antiderivative Anti-Aliasing), which eliminates aliasing mathematically at 1x sample rate without oversampling.

The result: saturation that colors, energizes, and glues — without digital artifacts, without CPU overhead, without latency.

---

## Core Identity

**Type:** Saturation + dynamics processor
**Target users:** Mixing engineers, mastering engineers, music producers
**Use cases:** Channel strips, mix bus, master bus, stem processing, parallel saturation
**Plugin format:** VST3, AU, AAX

---

## Technical Innovation: ADAA Anti-Aliasing

The defining technical feature. Traditional anti-aliasing approaches:
- **Oversampling (2x–16x):** Heavy CPU, added latency, phase issues
- **No anti-aliasing:** Aliasing artifacts, especially above 4kHz at moderate drive

ADAA uses the **antiderivatives of the waveshaping function** to compute the output analytically, eliminating aliasing without processing at a higher sample rate. This is achievable for sigmoid-based and polynomial waveshapers. Result:
- Zero aliasing at 1x sample rate (44.1k, 48k, 88.2k, 96k, 192k)
- No added latency
- Low CPU overhead
- Zero-phase behavior (no oversampling filter ringing)

This is the engineering cornerstone. It must be implemented correctly and should be clearly communicated to users as a differentiator.

---

## Saturation Engine

### Character
Primary character is **transformer saturation** — the magnetic hysteresis and iron core of a Neve-style transformer. This produces:
- A blend of even and odd harmonics with transformer-specific asymmetry
- Low-end weight and density
- Subtle high-frequency softening at moderate drive
- Musical break-up at high drive levels

### Harmonic Control (Key Differentiator)
Two independent knobs on the main panel:
- **Even harmonics knob** — Controls 2nd, 4th harmonic content. Tube-like warmth, fullness, vintage color
- **Odd harmonics knob** — Controls 3rd, 5th harmonic content. Presence, edge, transistor grit, aggression

This combination gives a precise harmonic palette from pure "Neve-style" to "API-aggressive" to anything between or beyond — without switching modes.

### Variable Saturation Shape
The waveshaping curve is **continuously variable** from soft to hard — not discrete mode switching:
- **Soft (0%):** Gentle, gradual onset of saturation. Wide linear region, slow roll-off into clipping. Transparent at low drive, warm at high drive. Best for mastering bus and subtle coloring.
- **Medium (50%):** Balanced clipping curve. Clear harmonic generation with musical compression. The default "sweet spot" for most mixing applications.
- **Hard (100%):** Aggressive clipping with a tight knee. Rapid transition from linear to saturated. Produces dense, compressed harmonics. For parallel distortion, drum bus aggression, and sound design.

The variable shape changes the mathematical transfer function continuously — the waveshaping polynomial coefficients morph smoothly. This interacts with ADAA: since ADAA requires the antiderivative of the waveshaper, the shape parameter must produce analytically differentiable curves across its full range.

---

## Dynamics Engine

### Type: Upward + Downward Compressor
- **Downward compression:** Controls peaks, adds glue and consistency (standard compression behavior)
- **Upward compression:** Raises the floor — quiet signals get lifted, adding energy, sustain, and density without touching peaks

Program-dependent attack and release: the plugin analyzes the signal's program material to set optimal time constants automatically. Users can override with manual controls in the Advanced panel.

### Pre/Post Saturation Toggle
**Critical feature.** Changes the entire character of processing:
- **Pre-saturation (Dynamics → Saturation):** Dynamics shapes what hits the saturation stage. Consistent input = consistent harmonic color. Great for mastering — the saturation character stays predictable.
- **Post-saturation (Saturation → Dynamics):** Saturation colors everything first, then dynamics controls the result. More musical for mixing — saturation follows the natural dynamics of the performance.

---

## Tilt Filters

Two independent tilt EQs positioned at critical points in the signal chain, providing spectral shaping that interacts with the saturation and dynamics stages.

### Post-Saturation Tilt
Placed immediately after the saturation stage. Shapes the harmonic content that the saturation has generated:
- Tilting bright after saturation emphasizes the upper harmonics the drive created
- Tilting dark after saturation tames harsh harmonics while preserving low-end weight
- This is where you sculpt the *tone* of the saturation

### Post-Dynamics Tilt
Placed immediately after the dynamics stage. Shapes the final output character:
- Tilting bright after dynamics adds air and presence to the compressed/expanded signal
- Tilting dark after dynamics creates a warmer, denser final output
- This is where you match the plugin's output to the mix context

### Parameters (per tilt filter)
- **Frequency** — Center/pivot frequency. Range: 100 Hz to 10 kHz, default 1000 Hz. This is the "seesaw" point — frequencies above are boosted while frequencies below are cut (or vice versa).
- **Slope** — Tilt amount in dB/octave. Range: -6 dB to +6 dB. At 0 dB the tilt is bypassed (flat). Positive values tilt bright (boost highs, cut lows). Negative values tilt dark (cut highs, boost lows).

### Implementation Notes
- Tilt filter should be a first-order shelving filter pair (low shelf + high shelf with complementary gains) centered at the specified frequency
- Must be phase-coherent and low-CPU
- The two tilt filters are independent — they can be set to different frequencies and slopes
- In the signal chain: Input → [Dynamics if Pre] → Saturation → **Post-Sat Tilt** → [Dynamics if Post] → **Post-Dyn Tilt** → Output

---

## M/S Processing

Full mid/side support for mastering applications:
- Independent saturation control for mid and side signals
- Independent dynamics control per channel (M/S)
- Allows adding harmonic density to the center without affecting stereo width
- Allows tightening the side channel independently
- Essential for mastering-grade stereo control

---

## UI Design Philosophy: Two-Panel (Compact, Zero Blank Space)

Window: 740×400 collapsed, 740×548 expanded. Dense hardware-style layout.

### Main Panel (fast workflow)
Always visible. Core controls for quick decisions:
- **Saturation section (left):** Drive (large knob), Even + Odd (medium knobs), drive curve SVG display, Shape (medium knob, below curve)
- **Center section:** Plugin title, Pre/Post toggle pill, Input + Mix + Output (small 360° fill knobs), Post-Sat Tilt freq + slope (small knobs, red accent)
- **Dynamics section (right):** Dynamics (large knob), Up + Down (small knobs), dynamics curve SVG display
- **Metering (far right):** Stereo IN + OUT level meters

### Advanced Panel (collapsible, 130px)
Revealed for deep control. All titles and knobs centered horizontally:
- **Dynamics detail:** Threshold, ratio, attack, release (manual override)
- **Post-Dyn Tilt:** Frequency + slope for spectral tilt after dynamics stage
- **M/S routing (centered):** Enable M/S mode, independent mid/side controls

---

## Reference Points

| Characteristic | Reference |
|---|---|
| Harmonic character | Neve 1073 transformer saturation |
| Dynamics behavior | API 2500 program-dependent compression |
| Technical approach | ADAA (anti-aliased at 1x, no oversampling) |
| Workflow philosophy | Mastering-grade precision with mixing-speed UX |

---

## Competitive Differentiation

| Plugin | What it does | What DynaDrive does differently |
|---|---|---|
| Decapitator | Multi-mode saturation | ADAA (no aliasing), integrated dynamics |
| Satin | Tape saturation | Transformer + dynamics, harmonic control |
| FabFilter Saturn | Multiband, modes | Focused character, ADAA, upward compression |
| Vertigo VSM-3 | Parallel saturation | Dynamics integration, ADAA, M/S |
| Shaper Box | Waveshaping | Musical character, mastering workflow |

None of the above combine: ADAA anti-aliasing + upward/downward dynamics + independent even/odd harmonic control + variable saturation shape + dual tilt filters + M/S + pre/post routing in one focused plugin.

---

## Emotional Target

When a mix engineer inserts NBS DynaDrive on a flat, lifeless drum bus, they should feel the mix come alive — more weight, more presence, more energy — and when they check the spectrum, no new aliases, no added brightness from digital artifacts, no phase issues. It should feel like they plugged into an expensive hardware chain, not an algorithmic shortcut.

---

## Success Criteria

- [ ] ADAA implemented correctly — no measurable aliasing at 1x sample rate
- [ ] Even/odd harmonic knobs produce clearly distinct, musical tonal changes
- [ ] Pre/post toggle produces noticeably different compression-saturation interactions
- [ ] Upward compression adds energy without audible pumping or artifacts
- [ ] M/S mode works correctly on stereo material (phase coherent)
- [ ] Plugin sounds as good at 44.1k as at 96k (ADAA handles this)
- [ ] Main panel workflow: from insert to dialed-in sound in under 30 seconds
- [ ] Variable saturation shape produces audibly different curves across full range
- [ ] Post-saturation tilt shapes harmonic character without phase artifacts
- [ ] Post-dynamics tilt provides final tonal sculpting
- [ ] Tilt filters are transparent at 0 dB slope (true bypass behavior)
- [ ] Advanced panel gives mastering engineers full parameter access

---

*Creative brief created: 2026-02-27*
*Status: Ideation — ready for planning*
