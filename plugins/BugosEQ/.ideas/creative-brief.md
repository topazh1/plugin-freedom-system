# BugosEQ - Creative Brief

## Overview

**Type:** Audio Effect (Dynamic EQ)
**Core Concept:** Analog-character 4-band dynamic EQ with per-band dynamics modes (static, expander, upward/downward compression) and Neve/API-inspired transformer saturation
**Status:** 💡 Ideated
**Created:** 2026-02-27

## Vision

BugosEQ is an analog-flavored dynamic equalizer that puts full dynamic control in every band. Inspired by the punchy, transformer-saturated character of Neve and API console EQs, it combines the musicality of classic analog EQ curves with the surgical flexibility of a modern dynamic EQ. Each of the four bands can independently operate as a static EQ, expander, upward compressor, or downward compressor — letting producers and engineers shape both the frequency response and the transient/dynamic behavior of a signal band-by-band. A Drive knob unlocks the harmonic saturation circuit to dial in anything from a subtle transformer warmth to dense, punchy coloration. M/S processing mode makes it equally at home on mastering chains as on individual tracks and mix buses.

## Parameters

### Global

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Drive | 0–100% | 20% | Amount of transformer-style harmonic saturation applied to the signal |
| Output Gain | -18 to +18 dB | 0 dB | Makeup gain after EQ and dynamics processing |
| Processing Mode | Stereo / Mid-Side | Stereo | Switch between standard stereo and M-S processing |

### Per-Band (×4: LF Shelf, Low Mid Peak, High Mid Peak, HF Shelf)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Frequency | 20–20000 Hz | Band-dependent | Center/corner frequency of the EQ band |
| Gain | -18 to +18 dB | 0 dB | Static boost or cut amount |
| Q / Bandwidth | 0.1–10 | 0.7 (shelves) / 1.0 (peaks) | Bandwidth of the filter; lower Q = wider |
| Mode | Static / Expander / UpComp / DnComp | Static | Dynamics mode for this band |
| Threshold | -60 to 0 dBFS | -24 dBFS | Level at which dynamics processing engages |
| Ratio | 1:1 – 20:1 | 4:1 | Dynamics ratio (only active in dynamic modes) |
| Attack | 0.1 – 300 ms | 10 ms | How quickly the dynamics circuit responds to level changes |
| Release | 5 – 2000 ms | 100 ms | How quickly the dynamics circuit recovers |

**Mode Descriptions:**
- **Static** — Classic analog EQ, no dynamics processing
- **Expander** — Band gain reduces when signal falls below threshold (emphasizes loud content)
- **UpComp (Upward Compression)** — Band gain increases quiet signals toward threshold (lifts low-level content)
- **DnComp (Downward Compression)** — Band gain is attenuated above threshold (tames loud peaks)

## UI Concept

**Layout:** 820×620px. Four equal vertical band strips across the center. Header bar (title, M-S toggle, I/O meters). EQ curve display (160px, full width) above the strips. Global section (Drive, Output, meters) at the bottom.

**Visual Style:** Vintage hardware aesthetic — dark warm-brown background (#2a1a0a), scan lines, radial vignette. Each band is color-coded: Band 1 (LF Shelf) charcoal/black, Band 2 (Lo Mid) deep blue, Band 3 (Hi Mid) dark purple, Band 4 (HF Shelf) dark amber/yellow. Knob bodies use band-specific radial gradients. Brass/gold (#c49564) for global text and controls. Helvetica Neue, uppercase, wide letter-spacing.

**Knob Style:** 44px for EQ controls (FREQ, GAIN), 32px for dynamics controls (Q, THR, RATIO, ATCK, REL). Indicator: small dot at the outer rim, colored in the band accent color, rotates with knob value. Global Drive/Output knobs: 65px with brass bar indicator.

**Mode Buttons:** 4-state rounded button group per band [STATIC | EXP | UP | DN] with 2px gaps between segments. Inactive: dark with band-colored border at 30% opacity. Active: band color fill at 70% opacity.

**EQ Display:** Band curves drawn in band colors (Band 1: gray, Band 2: blue, Band 3: purple, Band 4: dark yellow) with subtle fill. Frequency grid, axis labels. GR meter strip per band also matches band color.

**Key Elements:**
- Per-band color identity (knobs, curves, GR meters, mode buttons all share band color)
- Dot indicator on knob rim — no bar/line
- Rounded mode selector segments per band
- Real-time EQ curve display with per-band colored curves
- GR/GE meter strip per band
- Input/Output animated level meters (header + global)
- M/S toggle in header

**Finalized:** v2 (2026-02-27)

## Use Cases

- Surgical frequency-dynamic control on mix buses (e.g., taming low-mid buildup only when it gets loud)
- Mastering chain dynamic EQ to transparently balance the spectrum under program-dependent conditions
- Drum bus processing: expand the high-mid attack frequencies while compressing low-end resonance
- Vocal dynamic EQ: downward compress harshness in the 3–5 kHz range above a threshold
- Upward compress the body of a thin acoustic guitar in the 200–400 Hz band to increase perceived fullness

## Inspirations

- **Neve 1073 / 1084** — Transformer saturation character, musical EQ curves
- **API 550B** — Punchy, forward mid-range coloration
- **Fabfilter Pro-Q 3** — Per-band dynamic mode toggle concept, EQ curve display
- **SPL Twin Tube** / **Weiss DS1-MK3** — High-end mastering dynamic EQ workflow
- **Tokyo Dawn TDR Nova** — Per-band dynamic EQ approach in a clean UI

## Technical Notes

- 4 filter types: Low Shelf (Band 1), Bell/Peak (Bands 2–3), High Shelf (Band 4)
- Per-band dynamics applied as a modulated gain on the filter's output gain parameter (gain-based dynamic EQ approach)
- Analog saturation circuit: soft-clip waveshaper with even/odd harmonic blend tuned to Neve/API character — Drive knob scales the saturation amount pre-output
- M-S matrix: encode to M/S before processing, decode after — processing mode applies independently to Mid and Side signals
- Consider oversampling (2×) for the saturation stage to minimize aliasing artifacts at high Drive settings
- Attack/release times: per-band envelope followers on the sidechain signal (internal sidechain from the band's filtered output)

## Next Steps

- [ ] Create UI mockup (`/dream BugosEQ` → option 3)
- [ ] Start implementation (`/implement BugosEQ`)
