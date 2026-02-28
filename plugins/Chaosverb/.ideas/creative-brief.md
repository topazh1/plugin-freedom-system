# Chaosverb — Creative Brief

## The Problem

Every reverb plugin simulates a real or idealized acoustic environment — rooms, halls, plates, springs. Even the most experimental ones are anchored to physical reality. The mental model is always "what kind of space is this?" But sound design doesn't live in reality. Sound designers need spaces that don't exist and couldn't exist — and they need a process for discovering them that isn't just tweaking knobs one at a time.

## The Vision

Chaosverb is an impossible-space reverb that mutates itself. It uses an FDN (Feedback Delay Network) engine configured with physically unachievable topologies — feedback matrices no room could produce, spectral decay behaviors that violate acoustics, modulation that warps time in the tail. And every N seconds (user-defined), every unlocked parameter randomizes simultaneously to a completely new state.

The result is a reverb discovery engine. Not a reverb you design — a reverb you *negotiate with*.

---

## Core Identity

**Type:** Algorithmic reverb effect (FDN — impossible topology)
**Target users:** Sound designers, experimental producers, film/game audio
**Use cases:** Texture generation, evolving atmospheres, alien/sci-fi spaces, generative ambient, drone processing
**Plugin format:** VST3, AU, AAX

---

## DSP Architecture: FDN with Impossible Topology

The engine is a **Feedback Delay Network** — a network of delay lines with a feedback matrix governing how signals circulate between them. In a real room, the matrix is constrained by physics. Chaosverb removes those constraints entirely:

- **Feedback matrices** that no physical enclosure could produce
- **Spectral decay** that's fully independent per band — high frequencies can decay slower than lows, which is physically impossible
- **Modulation** on delay line lengths that warps pitch in the tail, not just adds shimmer
- **Non-linear feedback** — saturation and soft-clipping in the feedback path creates harmonic content that grows inside the reverb tail
- **Tail envelope shaping** — the reverb tail can swell (get louder before decaying) rather than always decaying immediately

The goal is not "what kind of room is this?" but "what kind of space has never existed?"

---

## Parameters

All parameters are subject to the mutation cycle unless locked. All have Lock buttons.

### Space Parameters
| Parameter | Description | Range |
|---|---|---|
| **Topology** | FDN feedback matrix structure — controls the fundamental shape of the impossible space | 0–100 (continuous, no presets) |
| **Decay** | Length of the reverb tail | 0.1s – 60s |
| **Pre-Delay** | Time before the reverb onset | 0ms – 250ms |
| **Density** | Diffusion and echo density in the tail — from sparse discrete reflections to infinite wash | 0–100 |

### Spectral Parameters
| Parameter | Description | Range |
|---|---|---|
| **Spectral Tilt** | Controls which frequencies decay faster. Center = neutral. Left = lows outlast highs. Right = highs outlast lows (impossible in reality) | -100 to +100 |
| **Resonance** | Injects feedback resonant peaks into the tail — certain frequencies bloom and sustain impossibly long | 0–100 |

### Motion Parameters
| Parameter | Description | Range |
|---|---|---|
| **Mod Rate** | Speed of pitch/time modulation applied to delay lines in the tail | 0.01Hz – 10Hz |
| **Mod Depth** | Amount of pitch and time warping in the reflections | 0–100 |

### Output Parameters
| Parameter | Description | Range |
|---|---|---|
| **Width** | Stereo spread of the wet signal | 0–200% |
| **Mix** | Dry/wet blend | 0–100% |

---

## The Mutation System

### Timer
- User-adjustable interval: **5 seconds – 10 minutes**
- Default: **30 seconds**
- Displayed as a countdown on the UI — the user always knows when the next event is coming

### What Happens at Mutation
1. New values for all **unlocked** parameters are generated simultaneously using **true chaos randomization** — any value anywhere in each parameter's range, no musical weighting, no bias
2. Both the current reverb state and new reverb state run in parallel
3. The wet signal crossfades from current → new over the **Crossfade Speed** duration
4. Locked parameters are untouched

### Crossfade Speed
- **Range:** 0ms – 500ms
- **0ms:** Instant cut — jarring, glitchy, harsh (a feature, not a flaw)
- **500ms:** Half-second dissolve between spaces
- Crossfade is implemented as two parallel reverb engine instances with wet signal blending

### Mutate Now
A dedicated button triggers the mutation cycle immediately, outside the timer. Crossfade Speed applies.

### The Lock System
Every parameter has a **Lock button** displayed alongside it in the UI. When locked:
- Parameter ignores the mutation cycle
- User retains manual control
- Lock state is visual (distinct locked/unlocked appearance)

This is the core workflow: run mutations, find something interesting, lock it, keep mutating the rest.

---

## UI Design Philosophy

The UI should reflect the nature of the plugin: something that exists outside normal acoustic reality. Not clinical, not warm, not vintage. **Dark, spatial, slightly unsettling.**

### Key UI Requirements
- **Countdown display** — prominent, always visible. The user needs to anticipate the next mutation
- **Mutate Now button** — accessible, distinct
- **Per-parameter Lock buttons** — visually integrated with each parameter, not an afterthought
- **Crossfade Speed** and **Interval** controls live together in a "Mutation" section, separate from the reverb parameters
- Visual indication during active crossfade (brief animation or color shift as mutation happens)

### Layout Concept
```
┌─────────────────────────────────────────────┐
│  CHAOSVERB                    [NEXT: 00:23]  │
│                               [MUTATE NOW]   │
├──────────────┬──────────────────────────────┤
│  SPACE       │  SPECTRAL      MOTION        │
│  Topology 🔒 │  Spectral Tilt 🔒  Mod Rate 🔒│
│  Decay    🔒 │  Resonance  🔒     Mod Depth🔒│
│  Pre-Delay🔒 │                              │
│  Density  🔒 │                              │
├──────────────┴──────────────────────────────┤
│  MUTATION                                    │
│  Interval [___________] Crossfade [________] │
├─────────────────────────────────────────────┤
│  Width 🔒            Mix 🔒                  │
└─────────────────────────────────────────────┘
```

---

## Emotional Target

When a sound designer inserts Chaosverb on a drone or texture, they should feel like they've handed control to something alien — something that will find combinations they never would have reached by hand. And when a mutation lands on something spectacular, the lock system lets them catch it mid-flight and hold it there while the rest of the space keeps evolving around it.

The experience should feel like discovering spaces rather than designing them.

---

## What This Is Not

- Not a room/hall/plate/spring simulation
- Not a convolution reverb
- Not a "creative reverb" that's just a plate with extra modulation
- Not a plugin that sounds polished or "professional" by default — chaos means some mutations will be ugly, and that's correct behavior

---

## Success Criteria

- [ ] FDN engine produces reverb textures with no physically plausible analog
- [ ] Mutation cycle randomizes all unlocked parameters simultaneously on the interval
- [ ] Crossfade Speed (0–500ms) correctly crossfades between two reverb engine states
- [ ] Lock buttons work correctly — locked params ignore mutation, retain user values
- [ ] Mutate Now triggers immediate mutation with Crossfade Speed applied
- [ ] Countdown display is accurate to within ±1 frame
- [ ] Spectral Tilt can produce decay where highs outlast lows (physically impossible)
- [ ] Resonance parameter creates sustained frequency peaks inside the tail
- [ ] True chaos: random values can land anywhere in each parameter's full range
- [ ] Two parallel reverb instances during crossfade produce no clicks or artifacts

---

*Creative brief created: 2026-02-27*
*Status: Ideation — ready for planning*
