# Parameter Specification (Draft)

**Status:** Draft - Awaiting UI mockup for full specification
**Created:** 2026-02-27
**Source:** Quick capture during ideation

This is a lightweight specification to enable parallel DSP research.
Full specification will be generated from finalized UI mockup.

## Parameters

### topology
- **Type:** Float
- **Range:** 0 to 100
- **Default:** 50
- **DSP Purpose:** Controls the FDN feedback matrix structure — the fundamental configuration of delay paths and cross-coupling coefficients. No physical analog. Continuous morphing between impossible topological states.

### decay
- **Type:** Float
- **Range:** 0.1 to 60 seconds
- **Default:** 4.0
- **DSP Purpose:** Length of the reverb tail. Controls feedback gain coefficient in the FDN. At extreme values (30-60s) creates near-infinite sustain.

### preDelay
- **Type:** Float
- **Range:** 0 to 250 ms
- **Default:** 10
- **DSP Purpose:** Delay before the reverb onset. Simple delay line before the FDN input.

### density
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 60
- **DSP Purpose:** Echo density and diffusion in the tail. Low = sparse discrete reflections. High = infinite wash. Implemented via allpass diffuser chains before the FDN.

### spectralTilt
- **Type:** Float
- **Range:** -100 to +100
- **Default:** 0
- **DSP Purpose:** Per-band independent decay scaling. Center (0) = neutral. Negative = lows outlast highs (physical). Positive = highs outlast lows (physically impossible). Implemented as frequency-dependent feedback gain modifiers on FDN delay lines.

### resonance
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 0
- **DSP Purpose:** Injects resonant feedback peaks into the tail — certain frequencies accumulate and sustain impossibly long. Implemented as narrow bandpass feedback loops within the FDN. Requires careful stability limiting.

### modRate
- **Type:** Float
- **Range:** 0.01 to 10 Hz
- **Default:** 0.3
- **DSP Purpose:** Speed of LFO modulation applied to FDN delay line lengths. Creates pitch warping in the tail. Different from standard chorus shimmer — targets individual delay lines independently.

### modDepth
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 20
- **DSP Purpose:** Amount of delay line length modulation. At high values creates pronounced pitch instability in reflections — flanging, time-stretching artifacts in the tail.

### width
- **Type:** Float
- **Range:** 0 to 200 %
- **Default:** 100
- **DSP Purpose:** Stereo spread of wet signal. 0% = mono wet. 100% = normal stereo. 200% = exaggerated mid/side expansion.

### mix
- **Type:** Float
- **Range:** 0 to 100 %
- **Default:** 50
- **DSP Purpose:** Dry/wet balance between unprocessed input and FDN reverb output.

### mutationInterval
- **Type:** Float
- **Range:** 5 to 600 seconds
- **Default:** 30
- **DSP Purpose:** Timer interval between mutation events. Controls internal TimerThread that fires the global randomization event.

### crossfadeSpeed
- **Type:** Float
- **Range:** 0 to 500 ms
- **Default:** 100
- **DSP Purpose:** Duration of wet signal crossfade between current and new FDN state at mutation. 0ms = instant. Requires two parallel FDN instances with a crossfade gain ramp.

## Lock Parameters (Boolean)

Each audio parameter has a corresponding lock that excludes it from mutation events.

- **topologyLock**, **decayLock**, **preDelayLock**, **densityLock**
- **spectralTiltLock**, **resonanceLock**
- **modRateLock**, **modDepthLock**
- **widthLock**, **mixLock**

All: Bool, Default: false (unlocked)

## Technical Notes

- Dual FDN instances required for crossfade: FDN-A (current) and FDN-B (new), with a wet gain crossfader
- At mutation: FDN-B receives new random parameters, crossfader ramps A→B over crossfadeSpeed ms, then FDN-A becomes idle until next mutation
- True chaos: randomization uses uniform distribution across full parameter range (no musical weighting)
- spectralTilt at +100 requires high-shelf decay longer than low-shelf — physically impossible, implemented as independent per-band feedback scalars
- resonance requires hard stability limiting to prevent runaway feedback (max resonance gain clamp)
- Total APVTS parameter count: 12 floats + 10 booleans = 22 parameters

## Next Steps

- [ ] Complete UI mockup workflow (/dream Chaosverb → Create UI mockup)
- [ ] Finalize design and generate full parameter-spec.md
- [ ] Validate consistency between draft and final spec
