#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter layout — 22 parameters: 12 Float + 10 Bool
// JUCE 8 requires juce::ParameterID { "id", 1 } format (not bare strings)
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ChaosverbAudioProcessor::createParameterLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;

  // -------------------------------------------------------------------------
  // Float parameters (12) — Audio controls, all lockable via mutation system
  // (except mutationInterval and crossfadeSpeed which are never randomized)
  // -------------------------------------------------------------------------

  // topology — FDN feedback matrix structure (0–100, linear, default 50)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"topology", 1}, "Topology",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 50.0f));

  // decay — Reverb tail length (0.1–60s, logarithmic skew 0.35, default 4.0)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"decay", 1}, "Decay",
      juce::NormalisableRange<float>(0.1f, 60.0f, 0.0f, 0.35f), 4.0f, "s"));

  // preDelay — Bipolar: left=BPM-synced divisions, center=0ms, right=free time
  // Range: -500 to +500 (symmetric = 12 o'clock at 0ms). Linear skew.
  // Negative = BPM sync mode, Positive = milliseconds.
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"preDelay", 1}, "Pre-Delay",
      juce::NormalisableRange<float>(-500.0f, 500.0f, 0.0f, 1.0f), 0.0f));

  // density — Echo density / diffusion (0–100%, linear, default 60)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"density", 1}, "Density",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 60.0f, "%"));

  // spectralTilt — Per-band independent decay scaling (-100 to +100, linear,
  // default 0)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"spectralTilt", 1}, "Spectral Tilt",
      juce::NormalisableRange<float>(-100.0f, 100.0f, 0.0f, 1.0f), 0.0f));

  // resonance — Narrow resonant feedback peaks in tail (0–100%, linear, default
  // 0)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"resonance", 1}, "Resonance",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 0.0f, "%"));

  // modRate — LFO speed for delay line modulation (0.01–10Hz, logarithmic skew
  // 0.35, default 0.3)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"modRate", 1}, "Mod Rate",
      juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.35f), 0.3f, "Hz"));

  // modDepth — Amount of delay line length modulation (0–100%, linear, default
  // 20)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"modDepth", 1}, "Mod Depth",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 40.0f, "%"));

  // mutationInterval — Bipolar: left=BPM-synced divisions, center=0, right=free
  // time Range: -1000 to +1000 (symmetric = 12 o'clock at 0). Linear mapping.
  // Negative = BPM sync mode (snaps to note divisions), Positive = absolute ms
  // (smooth). Snap points on left side: -100(1/32), -200(1/16), -300(d1/16),
  // -400(1/8),
  //   -500(d1/8), -600(1/4), -700(d1/4), -800(1/2), -900(1/1),
  //   -925(2bar), -950(3bar), -975(4bar).
  {
    juce::NormalisableRange<float> intervalRange(
        -1000.0f, 1000.0f,
        // convertFrom0to1 (linear)
        [](float rangeStart, float rangeEnd, float normalised) -> float {
          return rangeStart + normalised * (rangeEnd - rangeStart);
        },
        // convertTo0to1 (linear)
        [](float rangeStart, float rangeEnd, float value) -> float {
          return (value - rangeStart) / (rangeEnd - rangeStart);
        },
        // snapToLegalValue — left side snaps to 100-step divisions, right side
        // smooth
        [](float /*rangeStart*/, float /*rangeEnd*/, float value) -> float {
          if (value < -50.0f) {
            if (value <= -850.0f) {
              // Fine 25-step snapping for bar divisions (-900, -925, -950,
              // -975, -1000)
              return juce::jlimit(-1000.0f, -900.0f,
                                  std::round(value / 25.0f) * 25.0f);
            }
            // Snap to nearest 100-increment for musical note divisions
            return std::round(value / 100.0f) * 100.0f;
          }
          if (value < 0.0f) {
            // Dead zone near center snaps to 0 (off)
            return 0.0f;
          }
          return value; // Right side: smooth continuous ms
        });
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"mutationInterval", 1}, "Mutation Interval",
        intervalRange, 0.0f));
  }

  // crossfadeSpeed — Wet signal crossfade duration at mutation (0–500ms,
  // linear, default 100)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"crossfadeSpeed", 1}, "Crossfade Speed",
      juce::NormalisableRange<float>(0.0f, 500.0f, 0.0f, 1.0f), 100.0f, "ms"));

  // width — Stereo spread of wet signal (0–400%, linear, default 200)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"width", 1}, "Width",
      juce::NormalisableRange<float>(0.0f, 400.0f, 0.0f, 1.0f), 200.0f, "%"));

  // mix — Dry/wet balance (0–100%, linear, default 50)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"mix", 1}, "Mix",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 50.0f, "%"));

  // outputLevel — Output gain trim (-12 to +12 dB, linear, default +6)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"outputLevel", 1}, "Output Level",
      juce::NormalisableRange<float>(-12.0f, 12.0f, 0.0f, 1.0f), 6.0f, "dB"));

  // -------------------------------------------------------------------------
  // Output EQ parameters (3) — post-FDN wet signal shaping
  // -------------------------------------------------------------------------

  // lowCut — Highpass frequency on wet output (20–500Hz, log skew, default 80)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"lowCut", 1}, "Low Cut",
      juce::NormalisableRange<float>(20.0f, 500.0f, 0.0f, 0.35f), 80.0f, "Hz"));

  // highCut — Lowpass frequency on wet output (1k–20kHz, log skew, default 10k)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"highCut", 1}, "High Cut",
      juce::NormalisableRange<float>(1000.0f, 20000.0f, 0.0f, 0.35f), 10000.0f,
      "Hz"));

  // tilt — Output tilt EQ (-100 to +100, negative=warm/dark, positive=bright,
  // default -10)
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"tilt", 1}, "Tilt",
      juce::NormalisableRange<float>(-100.0f, 100.0f, 0.0f, 1.0f), -10.0f));

  // -------------------------------------------------------------------------
  // Wow & Flutter parameter (1) — combined depth+speed control
  // -------------------------------------------------------------------------

  // wowFlutterAmount — Combined wow/flutter intensity (0–100%, default 0)
  // Scales both modulation depth and rate together for a single-knob control.
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"wowFlutterAmount", 1}, "Wow & Flutter",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 0.0f, "%"));

  // duckingAmount — Wet signal ducking based on input envelope (0–100%, default
  // 0) When input is loud, the wet reverb signal is reduced proportionally. 0%
  // = no ducking, 100% = full ducking (wet goes silent during loud input).
  layout.add(std::make_unique<juce::AudioParameterFloat>(
      juce::ParameterID{"duckingAmount", 1}, "Ducking",
      juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f), 0.0f, "%"));

  // -------------------------------------------------------------------------
  // Bool parameters (16) — Mutation lock controls + toggles
  // -------------------------------------------------------------------------

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"topologyLock", 1}, "Topology Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"decayLock", 1}, "Decay Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"preDelayLock", 1}, "Pre-Delay Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"densityLock", 1}, "Density Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"spectralTiltLock", 1}, "Spectral Tilt Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"resonanceLock", 1}, "Resonance Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"modRateLock", 1}, "Mod Rate Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"modDepthLock", 1}, "Mod Depth Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"widthLock", 1}, "Width Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"mixLock", 1}, "Mix Lock", false));

  // Tone lock controls (3) — protect tone params from mutation
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"lowCutLock", 1}, "Low Cut Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"highCutLock", 1}, "High Cut Lock", false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"tiltLock", 1}, "Tilt Lock", false));

  // Wow & Flutter lock + enable toggle
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"wowFlutterAmountLock", 1}, "Wow & Flutter Amount Lock",
      false));

  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"wowFlutterEnabled", 1}, "Wow & Flutter Enabled",
      false));

  // Output level lock
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"outputLevelLock", 1}, "Output Level Lock", false));

  // Ducking amount lock
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"duckingAmountLock", 1}, "Ducking Lock", false));

  // bypass — Global effect bypass (default: off = effect active)
  layout.add(std::make_unique<juce::AudioParameterBool>(
      juce::ParameterID{"bypass", 1}, "Bypass", false));

  return layout;
}

//==============================================================================
ChaosverbAudioProcessor::ChaosverbAudioProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "Parameters", createParameterLayout()) {

  topologyParam = parameters.getRawParameterValue("topology");
  decayParam = parameters.getRawParameterValue("decay");
  preDelayParam = parameters.getRawParameterValue("preDelay");
  densityParam = parameters.getRawParameterValue("density");
  spectralTiltParam = parameters.getRawParameterValue("spectralTilt");
  resonanceParam = parameters.getRawParameterValue("resonance");
  modRateParam = parameters.getRawParameterValue("modRate");
  modDepthParam = parameters.getRawParameterValue("modDepth");
  widthParam = parameters.getRawParameterValue("width");
  mixParam = parameters.getRawParameterValue("mix");
  crossfadeSpeedParam = parameters.getRawParameterValue("crossfadeSpeed");
  lowCutParam = parameters.getRawParameterValue("lowCut");
  highCutParam = parameters.getRawParameterValue("highCut");
  tiltParam = parameters.getRawParameterValue("tilt");
  wfAmountParam = parameters.getRawParameterValue("wowFlutterAmount");
  wfEnabledParam = parameters.getRawParameterValue("wowFlutterEnabled");
  outputLevelParam = parameters.getRawParameterValue("outputLevel");
  duckingAmountParam = parameters.getRawParameterValue("duckingAmount");
  bypassParam = parameters.getRawParameterValue("bypass");
}

ChaosverbAudioProcessor::~ChaosverbAudioProcessor() {
  // Stop timers before destruction to prevent callbacks into a partially
  // destroyed object. Must happen before any member destruction.
  glideTimerObj.stopTimer();
  mutationTimerObj.stopTimer();
}

//==============================================================================
void ChaosverbAudioProcessor::prepareToPlay(double sampleRate,
                                            int samplesPerBlock) {
  currentSampleRate = sampleRate;

  juce::dsp::ProcessSpec spec;
  spec.sampleRate = sampleRate;
  spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
  spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

  const double srRatio = sampleRate / 48000.0;

  // --- Pre-delay: max 250ms + 1 sample headroom ---
  // Max: 4000ms (accommodates 1/1 note at 60 BPM + free time 500ms)
  const int maxPreDelaySamples =
      static_cast<int>(std::ceil(4.0 * sampleRate)) + 1;
  preDelayLine.prepare(spec);
  preDelayLine.setMaximumDelayInSamples(maxPreDelaySamples);
  preDelayLine.setDelay(0.0f);
  preDelayLine.reset();

  // --- Allpass diffuser: scale delays to actual sample rate ---
  for (int i = 0; i < kNumDiffuserStages; ++i) {
    int allpassDelayLength =
        static_cast<int>(std::ceil(kAllpassLengths48k[i] * srRatio));
    diffLines[i].prepare(spec);
    diffLines[i].setMaximumDelayInSamples(allpassDelayLength + 1);
    diffLines[i].setDelay(static_cast<float>(allpassDelayLength));
    diffLines[i].reset();
  }

  // --- Dry/wet mixer ---
  dryWetMixer.prepare(spec);
  dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
  dryWetMixer.setWetMixProportion(0.5f);

  // --- Output EQ filters ---
  juce::dsp::ProcessSpec monoSpec{
      sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1};
  outputEQ.prepare(spec);

  // --- Haas delay: max 25ms on R channel for broadband stereo width ---
  const int maxHaasSamples =
      static_cast<int>(std::ceil(0.025 * sampleRate)) + 1;
  haasDelayLine.prepare(monoSpec);
  haasDelayLine.setMaximumDelayInSamples(maxHaasSamples);
  haasDelayLine.setDelay(0.0f);
  haasDelayLine.reset();

  // --- Wow & Flutter ---
  wowFlutter.prepare(spec);

  // --- Ducking envelope follower ---
  // Attack ~10ms: catches transients without tracking individual waveform
  // cycles Release ~250ms: smooth, transparent gain recovery (no AM-like
  // distortion)
  duckEnvelope = 0.0f;
  duckAttackCoeff =
      1.0f - std::exp(-1.0f / (0.010f * static_cast<float>(sampleRate)));
  duckReleaseCoeff =
      1.0f - std::exp(-1.0f / (0.250f * static_cast<float>(sampleRate)));

  // --- Both FDN instances: prepare together ---
  fdnA.prepare(spec);
  fdnB.prepare(spec);

  widthSmoother.reset(sampleRate, 0.05);     // 50ms smoothing
  haasDelaySmoother.reset(sampleRate, 0.05); // 50ms smoothing for Haas delay

  // --- Crossfade state machine: reset to Idle, A is active ---
  xfadeState = CrossfadeState::Idle;
  crossfadePhase = 0.0f;
  crossfadePhaseInc = 0.0f;
  fdnAIsActive = true;

  // Clear any pending mutation signal from previous session
  mutationPending.store(false, std::memory_order_relaxed);

  // --- Mutation timer: start (or restart) with current interval ---
  // Record when the timer starts so getRemainingTimeMs() has a valid baseline.
  // juce::Timer::startTimer() is thread-safe — it uses the internal TimerThread
  // lock (not the message thread) to register the timer. The *callback* fires
  // on the message thread. This pattern is standard in JUCE plugins where
  // prepareToPlay() may be called from the audio thread.
  lastMutationTimeMs = juce::Time::getMillisecondCounterHiRes();

  // Only start the mutation timer if it was enabled (default: stopped)
  if (mutationTimerRunning_.load()) {
    // 50ms poll — fast enough for ms-scale bipolar interval values
    mutationTimerObj.startTimer(50);
  }
}

void ChaosverbAudioProcessor::releaseResources() {
  preDelayLine.reset();
  for (int i = 0; i < kNumDiffuserStages; ++i)
    diffLines[i].reset();
  outputEQ.reset();
  haasDelayLine.reset();
  wowFlutter.reset();
  dryWetMixer.reset();
  fdnA.reset();
  fdnB.reset();
}

//==============================================================================
// Phase 4.4 — Mutation Timer System
//==============================================================================

void ChaosverbAudioProcessor::triggerMutation() {
  // Guard: if a crossfade trigger is already pending (not yet picked up by the
  // audio thread), skip this mutation. This prevents overlapping state changes
  // when the timer fires faster than the audio thread can process them.
  if (mutationPending.load(std::memory_order_acquire))
    return;

  // Record mutation timestamp for the countdown timer BEFORE setting params.
  lastMutationTimeMs = juce::Time::getMillisecondCounterHiRes();

  // Read all lock states (message thread — APVTS reads are always safe here)
  const bool topologyLocked =
      parameters.getRawParameterValue("topologyLock")->load() > 0.5f;
  const bool decayLocked =
      parameters.getRawParameterValue("decayLock")->load() > 0.5f;
  const bool preDelayLocked =
      parameters.getRawParameterValue("preDelayLock")->load() > 0.5f;
  const bool densityLocked =
      parameters.getRawParameterValue("densityLock")->load() > 0.5f;
  const bool spectralTiltLocked =
      parameters.getRawParameterValue("spectralTiltLock")->load() > 0.5f;
  const bool resonanceLocked =
      parameters.getRawParameterValue("resonanceLock")->load() > 0.5f;
  const bool modRateLocked =
      parameters.getRawParameterValue("modRateLock")->load() > 0.5f;
  const bool modDepthLocked =
      parameters.getRawParameterValue("modDepthLock")->load() > 0.5f;
  const bool widthLocked =
      parameters.getRawParameterValue("widthLock")->load() > 0.5f;
  const bool mixLocked =
      parameters.getRawParameterValue("mixLock")->load() > 0.5f;
  const bool lowCutLocked =
      parameters.getRawParameterValue("lowCutLock")->load() > 0.5f;
  const bool highCutLocked =
      parameters.getRawParameterValue("highCutLock")->load() > 0.5f;
  const bool tiltLocked =
      parameters.getRawParameterValue("tiltLock")->load() > 0.5f;
  const bool wfAmountLocked =
      parameters.getRawParameterValue("wowFlutterAmountLock")->load() > 0.5f;
  const bool outputLevelLocked =
      parameters.getRawParameterValue("outputLevelLock")->load() > 0.5f;
  const bool duckingLocked =
      parameters.getRawParameterValue("duckingAmountLock")->load() > 0.5f;

  // Use JUCE system random — uniform distribution, no musical weighting.
  juce::Random &rng = juce::Random::getSystemRandom();

  // Stop any existing glide before starting a new one
  glideTimerObj.stopTimer();
  numActiveGlides = 0;

  // Build glide targets: for each unlocked param, record current + random
  // target. setValueNotifyingHost uses normalized 0..1, respecting skew
  // automatically.
  auto addGlide = [&](const char *paramID, bool locked) {
    if (locked)
      return;
    if (auto *param = parameters.getParameter(paramID)) {
      glideTargets[static_cast<size_t>(numActiveGlides++)] = {
          param, param->getValue(), rng.nextFloat()};
    }
  };

  addGlide("topology", topologyLocked);
  addGlide("decay", decayLocked);
  addGlide("preDelay", preDelayLocked);
  addGlide("density", densityLocked);
  addGlide("spectralTilt", spectralTiltLocked);
  addGlide("resonance", resonanceLocked);
  addGlide("modRate", modRateLocked);
  addGlide("modDepth", modDepthLocked);
  addGlide("width", widthLocked);
  addGlide("mix", mixLocked);
  addGlide("lowCut", lowCutLocked);
  addGlide("highCut", highCutLocked);
  addGlide("tilt", tiltLocked);
  addGlide("wowFlutterAmount", wfAmountLocked);
  addGlide("outputLevel", outputLevelLocked);
  addGlide("duckingAmount", duckingLocked);

  // Glide duration matches crossfade speed for synchronized visual + audio
  // transition
  const float crossfadeMs = crossfadeSpeedParam->load();
  constexpr float kGlideTickMs = 16.0f; // ~60Hz update rate

  if (numActiveGlides > 0 && crossfadeMs > kGlideTickMs) {
    // Start smooth glide: knobs animate toward targets over crossfade duration
    glidePhase = 0.0f;
    glidePhaseInc = kGlideTickMs / crossfadeMs;
    glideTimerObj.startTimer(static_cast<int>(kGlideTickMs));
  } else {
    // Instant (crossfade <= 16ms): write targets immediately
    for (size_t i = 0; i < static_cast<size_t>(numActiveGlides); ++i)
      glideTargets[i].param->setValueNotifyingHost(glideTargets[i].targetNorm);
  }

  // Crossfade to the new parameter state (even if all params were locked —
  // the crossfade still fires, FDN-B gets the same values, no audible change).
  triggerCrossfade();
}

//==============================================================================
// Glide timer callback — smoothly interpolates parameters toward mutation
// targets
void ChaosverbAudioProcessor::GlideTimer::timerCallback() {
  processor.glidePhase += processor.glidePhaseInc;

  if (processor.glidePhase >= 1.0f) {
    // Glide complete: snap to final targets
    for (size_t i = 0; i < static_cast<size_t>(processor.numActiveGlides); ++i)
      processor.glideTargets[i].param->setValueNotifyingHost(
          processor.glideTargets[i].targetNorm);
    stopTimer();
    return;
  }

  // Smoothstep for natural-feeling motion (ease in/out)
  const float t = processor.glidePhase;
  const float smooth = t * t * (3.0f - 2.0f * t);

  for (size_t i = 0; i < static_cast<size_t>(processor.numActiveGlides); ++i) {
    const auto &g = processor.glideTargets[i];
    const float val = g.startNorm + smooth * (g.targetNorm - g.startNorm);
    g.param->setValueNotifyingHost(val);
  }
}

double ChaosverbAudioProcessor::getRemainingTimeMs() const {
  if (lastMutationTimeMs == 0.0 || !mutationTimerRunning_.load())
    return -1.0; // -1 signals "timer stopped" to the UI

  const double intervalMs = computeMutationIntervalMs();

  const double now = juce::Time::getMillisecondCounterHiRes();
  const double elapsed = now - lastMutationTimeMs;
  const double remaining = intervalMs - elapsed;

  return juce::jlimit(0.0, intervalMs, remaining);
}

//==============================================================================
double ChaosverbAudioProcessor::computeMutationIntervalMs() const {
  const float val = parameters.getRawParameterValue("mutationInterval")->load();

  double intervalMs;

  if (val >= 0.0f) {
    // Right side: absolute time in milliseconds (0–1000ms)
    intervalMs = static_cast<double>(val);
  } else {
    // Left side: BPM-synced note divisions
    const float bpm = hostBPM.load(std::memory_order_relaxed);
    const float quarterNoteMs = 60000.0f / bpm;
    const float t = -val / 1000.0f; // 0..1 (0=center, 1=far left)

    // Map snapped value to musical note division.
    // Snap points ensure t is exactly 0.1, 0.2, ..., 1.0 — use >= for exact
    // match.
    float beatFraction = 0.0f;
    if (t >= 0.975f)
      beatFraction = 16.0f; // 4 bars
    else if (t >= 0.95f)
      beatFraction = 12.0f; // 3 bars
    else if (t >= 0.925f)
      beatFraction = 8.0f; // 2 bars
    else if (t >= 0.9f)
      beatFraction = 4.0f; // 1/1 whole note
    else if (t >= 0.8f)
      beatFraction = 2.0f; // 1/2 half note
    else if (t >= 0.7f)
      beatFraction = 1.5f; // dotted 1/4
    else if (t >= 0.6f)
      beatFraction = 1.0f; // 1/4 quarter
    else if (t >= 0.5f)
      beatFraction = 0.75f; // dotted 1/8
    else if (t >= 0.4f)
      beatFraction = 0.5f; // 1/8 eighth
    else if (t >= 0.3f)
      beatFraction = 0.375f; // dotted 1/16
    else if (t >= 0.2f)
      beatFraction = 0.25f; // 1/16 sixteenth
    else if (t >= 0.1f)
      beatFraction = 0.125f; // 1/32

    intervalMs = static_cast<double>(quarterNoteMs * beatFraction);
  }

  // Minimum 50ms to prevent runaway mutations
  return juce::jmax(50.0, intervalMs);
}

void ChaosverbAudioProcessor::setMutationTimerRunning(bool running) {
  mutationTimerRunning_.store(running);

  if (running) {
    // Reset baseline so countdown starts fresh from current interval
    lastMutationTimeMs = juce::Time::getMillisecondCounterHiRes();
    // 50ms poll — fast enough for ms-scale bipolar interval values
    mutationTimerObj.startTimer(50);
  } else {
    mutationTimerObj.stopTimer();
  }
}

bool ChaosverbAudioProcessor::isMutationTimerRunning() const {
  return mutationTimerRunning_.load();
}

//==============================================================================
// Allpass diffuser helper (called from processBlock per-sample)
//==============================================================================

//==============================================================================

void ChaosverbAudioProcessor::applyDucking(int numSamples, float *dataL,
                                           float *dataR, float duckingNorm) {
  if (dataL == nullptr && dataR == nullptr)
    return;

  for (int n = 0; n < numSamples; ++n) {
    const float rawL = (dataL != nullptr) ? dataL[n] : 0.0f;
    const float rawR = (dataR != nullptr) ? dataR[n] : 0.0f;

    // Envelope follower for ducking
    const float inputPeak = std::max(std::abs(rawL), std::abs(rawR));
    if (inputPeak > duckEnvelope)
      duckEnvelope += duckAttackCoeff * (inputPeak - duckEnvelope);
    else
      duckEnvelope += duckReleaseCoeff * (inputPeak - duckEnvelope);

    if (duckingNorm >= 0.001f) {
      const float duckGain =
          juce::jlimit(0.0f, 1.0f, 1.0f - duckingNorm * duckEnvelope * 2.0f);
      if (dataL != nullptr)
        dataL[n] *= duckGain;
      if (dataR != nullptr)
        dataR[n] *= duckGain;
    }
  }
}

void ChaosverbAudioProcessor::applyStereoWidth(int numSamples, float *dataL,
                                               float *dataR, float widthGain) {
  if (dataL != nullptr && dataR != nullptr) {
    widthSmoother.setTargetValue(widthGain);
    for (int n = 0; n < numSamples; ++n) {
      const float currentWidth = widthSmoother.getNextValue();
      const float mid = (dataL[n] + dataR[n]) * 0.5f;
      const float side = (dataL[n] - dataR[n]) * 0.5f;
      dataL[n] = mid + side * currentWidth;
      dataR[n] = mid - side * currentWidth;
    }
  }
}

float ChaosverbAudioProcessor::processDiffuserSample(float input, int channel,
                                                     int numActiveStages) {
  float x = input;
  for (int stage = 0; stage < numActiveStages; ++stage) {
    auto &dl = diffLines[stage];
    const float delayed = dl.popSample(channel);
    const float v = x - kAllpassCoeff * delayed;
    dl.pushSample(channel, v);
    x = delayed + kAllpassCoeff * v;
  }
  return x;
}

//==============================================================================
void ChaosverbAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                           juce::MidiBuffer &midiMessages) {
  juce::ScopedNoDenormals noDenormals;
  juce::ignoreUnused(midiMessages);

  const int totalNumInputChannels = getTotalNumInputChannels();
  const int totalNumOutputChannels = getTotalNumOutputChannels();

  for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

  // -------------------------------------------------------------------------
  // Bypass — pass audio through unprocessed
  // -------------------------------------------------------------------------
  const bool bypassed = bypassParam->load() >= 0.5f;
  if (bypassed)
    return;

  const int numSamples = buffer.getNumSamples();
  const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

  // -------------------------------------------------------------------------
  // Read all parameters — atomic loads, fully real-time safe
  // -------------------------------------------------------------------------
  const float topologyPercent = topologyParam->load();
  const float decaySeconds = decayParam->load();
  const float preDelayVal = preDelayParam->load();
  const float densityPercent = densityParam->load();
  const float spectralTiltVal = spectralTiltParam->load();
  const float resonanceVal = resonanceParam->load();
  const float modRateHz = modRateParam->load();
  const float modDepthPercent = modDepthParam->load();
  const float widthPercent = widthParam->load();
  const float mixPercent = mixParam->load();
  const float crossfadeSpeedMs = crossfadeSpeedParam->load();
  const float lowCutHz = lowCutParam->load();
  const float highCutHz = highCutParam->load();
  const float tiltVal = tiltParam->load();
  const float wfAmount = wfAmountParam->load();
  const bool wfEnabled = wfEnabledParam->load() > 0.5f;
  const float outputLevelDb = outputLevelParam->load();
  const float duckingAmount = duckingAmountParam->load();

  // -------------------------------------------------------------------------
  // Check mutation pending flag (set by message thread, cleared here)
  // Guard: ignore if crossfade already in progress
  // -------------------------------------------------------------------------
  if (mutationPending.load(std::memory_order_acquire) &&
      xfadeState == CrossfadeState::Idle) {
    mutationPending.store(false, std::memory_order_release);
    xfadeState = CrossfadeState::Ramping;
    crossfadePhase = 0.0f;

    // Freeze current params as the outgoing snapshot.
    // activeSnapshot has last block's params (before mutation wrote new APVTS
    // values). The outgoing FDN keeps reverberating with these old params
    // during crossfade.
    outgoingSnapshot = activeSnapshot;

    // Do NOT reset the incoming FDN — it already has reverb energy from running
    // in parallel with the outgoing FDN. Keeping its state allows the crossfade
    // to genuinely blend between old and new reverb characters, making the
    // crossfade speed parameter audible. Resetting would cause the incoming FDN
    // to start from silence, producing a volume dip instead of a smooth morph.

    const float sr = static_cast<float>(currentSampleRate);
    if (crossfadeSpeedMs <= 0.0f) {
      crossfadePhaseInc = 1.0f;
    } else {
      crossfadePhaseInc = 1.0f / (crossfadeSpeedMs * 0.001f * sr);
    }
  }

  // -------------------------------------------------------------------------
  // Precompute per-block DSP parameters (avoids recomputing per-sample)
  // -------------------------------------------------------------------------

  const float sr = static_cast<float>(currentSampleRate);

  // Read host BPM unconditionally — used by pre-delay AND mutation interval.
  // Stored in atomic so the message-thread timer callback can also read it.
  {
    float bpm = 120.0f; // fallback when host doesn't report tempo
    if (auto *playhead = getPlayHead()) {
      if (auto pos = playhead->getPosition()) {
        if (auto hostBpm = pos->getBpm())
          bpm = juce::jmax(20.0f, static_cast<float>(*hostBpm));
      }
    }
    hostBPM.store(bpm, std::memory_order_relaxed);
  }

  // Pre-delay: bipolar — right of center=free time (ms), left=BPM-synced
  // divisions
  float preDelaySamples = 0.0f;
  if (preDelayVal >= 0.0f) {
    // Right of center: free time in ms (0-500ms)
    preDelaySamples = preDelayVal / 1000.0f * sr;
  } else {
    // Left of center: BPM-synced note divisions
    const float bpm = hostBPM.load(std::memory_order_relaxed);
    const float quarterNoteMs = 60000.0f / bpm;
    const float t = -preDelayVal / 500.0f; // 0..1 (0=off, 1=whole note)

    // Quantize to nearest note division
    float beatFraction = 0.0f;
    if (t > 0.9f)
      beatFraction = 4.0f; // 1/1
    else if (t > 0.8f)
      beatFraction = 2.0f; // 1/2
    else if (t > 0.7f)
      beatFraction = 1.5f; // 1/4d
    else if (t > 0.6f)
      beatFraction = 1.0f; // 1/4
    else if (t > 0.5f)
      beatFraction = 0.75f; // 1/8d
    else if (t > 0.4f)
      beatFraction = 0.5f; // 1/8
    else if (t > 0.3f)
      beatFraction = 0.375f; // 1/16d
    else if (t > 0.2f)
      beatFraction = 0.25f; // 1/16
    else if (t > 0.1f)
      beatFraction = 0.125f; // 1/32

    preDelaySamples = (quarterNoteMs * beatFraction) / 1000.0f * sr;
  }

  preDelaySamples = juce::jmax(0.0f, preDelaySamples);
  preDelayLine.setDelay(preDelaySamples);

  // Feedback gain from T60 (decay) formula
  float meanDelaySamples = 0.0f;
  for (int i = 0; i < ChaosverbFDN::kNumLines; ++i) {
    // Average L and R delay lengths (different primes per channel for
    // decorrelation)
    meanDelaySamples +=
        static_cast<float>(fdnA.delayLengthsL[i] + fdnA.delayLengthsR[i]) *
        0.5f;
  }
  meanDelaySamples /= static_cast<float>(ChaosverbFDN::kNumLines);

  const float meanLoopTime = meanDelaySamples / sr;
  const float safeDecay = juce::jmax(0.01f, decaySeconds);
  const float rawGain = std::exp(-6.91f * meanLoopTime / safeDecay);
  const float feedbackGain = juce::jmin(rawGain, 0.9999f);

  // Density -> diffuser stages + FDN in-loop allpass coefficient scaling
  // Wider thresholds = more dramatic jumps when sweeping the knob
  int numActiveDiffuserStages;
  if (densityPercent < 20.0f)
    numActiveDiffuserStages = 0;
  else if (densityPercent < 40.0f)
    numActiveDiffuserStages = 1;
  else if (densityPercent < 55.0f)
    numActiveDiffuserStages = 2;
  else if (densityPercent < 75.0f)
    numActiveDiffuserStages = 3;
  else
    numActiveDiffuserStages = 4;

  // Scale FDN in-loop allpass coefficients: sparse (0%) → dense wash (100%)
  // Quadratic curve for more extreme contrast between low and high density
  const float densityNorm = densityPercent / 100.0f;
  const float densityCurved = densityNorm * densityNorm;

  // Topology -> blend factor (0.3=minimum Hadamard mixing, 1.0=full Hadamard)
  // Never allows pure diagonal mode (parallel combs = metallic). Always some
  // cross-coupling.
  const float topologyBlend = 0.3f + 0.7f * (topologyPercent / 100.0f);

  // LFO depth in samples
  const float modDepthSamples =
      (modDepthPercent / 100.0f) * fdnA.maxLFODepthSamples;

  // Resonance smoothing coefficient (~50ms one-pole IIR)
  const float resoSmoothCoeff = 1.0f - std::exp(-1.0f / (0.050f * sr));

  // Stereo width gain: 0%=0.0, 100%=1.0, 400%=4.0
  const float widthGain = widthPercent / 100.0f;

  // Haas delay: scales from 0ms at width=0% to 25ms at width=400%
  // Adds temporal decorrelation to the R channel for broadband stereo width
  const float maxHaasMs = 25.0f;
  const float haasMs = (widthPercent / 400.0f) * maxHaasMs;
  const float haasDelaySamples = juce::jmax(0.0f, (haasMs / 1000.0f) * sr);

  // -------------------------------------------------------------------------
  // Update FDN coefficients: when idle both FDNs track live params;
  // during crossfade only the incoming FDN gets new coefficients so the
  // outgoing FDN keeps reverberating with its old character.
  // -------------------------------------------------------------------------
  if (xfadeState == CrossfadeState::Idle) {
    fdnA.updateDensity(densityCurved);
    fdnB.updateDensity(densityCurved);

    if (std::abs(spectralTiltVal - fdnA.cachedSpectralTilt) > 0.01f) {
      fdnA.updateShelfCoefficients(spectralTiltVal);
      fdnB.updateShelfCoefficients(spectralTiltVal);
    }
    if (std::abs(resonanceVal - fdnA.cachedResonance) > 0.01f) {
      fdnA.updateResonanceCoefficients(resonanceVal);
      fdnB.updateResonanceCoefficients(resonanceVal);
    }

    fdnA.prepareLFO(modRateHz, modDepthPercent);
    fdnB.prepareLFO(modRateHz, modDepthPercent);

    // Track live per-sample params for future crossfade snapshot
    activeSnapshot = {feedbackGain, topologyBlend, modDepthSamples,
                      resoSmoothCoeff};
  } else {
    // Crossfading: only update incoming FDN with new (mutated) params.
    // Outgoing FDN keeps its frozen coefficients from before the mutation.
    ChaosverbFDN &incomingFDN = fdnAIsActive ? fdnB : fdnA;
    incomingFDN.updateDensity(densityCurved);
    incomingFDN.updateShelfCoefficients(spectralTiltVal);
    incomingFDN.updateResonanceCoefficients(resonanceVal);
    incomingFDN.prepareLFO(modRateHz, modDepthPercent);
  }

  // Update output EQ coefficients when parameters change
  outputEQ.update(lowCutHz, highCutHz, tiltVal);

  // Set DryWetMixer mix ratio
  dryWetMixer.setWetMixProportion(
      juce::jlimit(0.0f, 1.0f, mixPercent / 100.0f));

  // Ducking amount: 0.0 = off, 1.0 = full ducking
  const float duckingNorm = duckingAmount / 100.0f;

  // -------------------------------------------------------------------------
  // Push dry signal into DryWetMixer before in-place wet processing
  // -------------------------------------------------------------------------
  {
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.pushDrySamples(block);
  }

  // Get write pointers
  float *dataL = (numChannels > 0) ? buffer.getWritePointer(0) : nullptr;
  float *dataR = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

  // -------------------------------------------------------------------------
  // Sample-by-sample processing loop
  //
  // Flow per sample:
  //   1. Pre-delay (shared, stereo)
  //   2. Allpass diffuser (shared, stereo)
  //   3. Both FDNs process diffused input with per-FDN params:
  //      - During crossfade: outgoing FDN uses frozen snapshot (old params),
  //        incoming FDN uses live params (new mutated values)
  //      - When idle: both FDNs use identical live params
  //   4. Equal-power crossfade blend:
  //      wetL = gainOut * outgoingL + gainIn * incomingL
  //      wetR = gainOut * outgoingR + gainIn * incomingR
  //   5. Advance crossfade phase; handle state transitions
  //
  // After all samples:
  //   6. Stereo width M/S matrix on wet buffer
  //   7. DryWetMixer blend
  // -------------------------------------------------------------------------
  for (int n = 0; n < numSamples; ++n) {
    // --- 1. Pre-delay (shared stereo) ---
    const float rawL = (dataL != nullptr) ? dataL[n] : 0.0f;
    const float rawR = (dataR != nullptr) ? dataR[n] : 0.0f;

    preDelayLine.pushSample(0, rawL);
    preDelayLine.pushSample(1, rawR);
    float sigL = preDelayLine.popSample(0);
    float sigR = preDelayLine.popSample(1);

    // --- 2. Allpass diffuser (shared stereo) ---
    if (numActiveDiffuserStages > 0) {
      sigL = processDiffuserSample(sigL, 0, numActiveDiffuserStages);
      sigR = processDiffuserSample(sigR, 1, numActiveDiffuserStages);
    }

    // --- 3. Both FDNs process with per-FDN params ---
    float wetAL = 0.0f, wetAR = 0.0f;
    float wetBL = 0.0f, wetBR = 0.0f;

    if (xfadeState == CrossfadeState::Ramping) {
      // During crossfade: outgoing uses frozen snapshot, incoming uses live
      const auto &outSnap = outgoingSnapshot;
      if (fdnAIsActive) {
        // A=outgoing (old params), B=incoming (new params)
        fdnA.processSample(sigL, sigR, outSnap.feedbackGain,
                           outSnap.topologyBlend, outSnap.modDepthSamples,
                           outSnap.resoSmoothCoeff, wetAL, wetAR);
        fdnB.processSample(sigL, sigR, feedbackGain, topologyBlend,
                           modDepthSamples, resoSmoothCoeff, wetBL, wetBR);
      } else {
        // A=incoming (new params), B=outgoing (old params)
        fdnA.processSample(sigL, sigR, feedbackGain, topologyBlend,
                           modDepthSamples, resoSmoothCoeff, wetAL, wetAR);
        fdnB.processSample(sigL, sigR, outSnap.feedbackGain,
                           outSnap.topologyBlend, outSnap.modDepthSamples,
                           outSnap.resoSmoothCoeff, wetBL, wetBR);
      }
    } else {
      // Idle: both FDNs use identical live params
      fdnA.processSample(sigL, sigR, feedbackGain, topologyBlend,
                         modDepthSamples, resoSmoothCoeff, wetAL, wetAR);
      fdnB.processSample(sigL, sigR, feedbackGain, topologyBlend,
                         modDepthSamples, resoSmoothCoeff, wetBL, wetBR);
    }

    // --- 4. Equal-power crossfade blend ---
    float wetL, wetR;
    if (xfadeState == CrossfadeState::Ramping) {
      // Equal-power: outgoing cos-fades out, incoming sin-fades in
      const float halfPi = juce::MathConstants<float>::halfPi;
      const float gainOut = std::cos(crossfadePhase * halfPi);
      const float gainIn = std::sin(crossfadePhase * halfPi);

      if (fdnAIsActive) {
        // A fading out, B fading in
        wetL = gainOut * wetAL + gainIn * wetBL;
        wetR = gainOut * wetAR + gainIn * wetBR;
      } else {
        // B fading out, A fading in
        wetL = gainOut * wetBL + gainIn * wetAL;
        wetR = gainOut * wetBR + gainIn * wetAR;
      }
    } else {
      wetL = fdnAIsActive ? wetAL : wetBL;
      wetR = fdnAIsActive ? wetAR : wetBR;
    }

    if (dataL != nullptr)
      dataL[n] = wetL;
    if (dataR != nullptr)
      dataR[n] = wetR;

    // --- 5. Advance crossfade phase and handle state transitions ---
    if (xfadeState == CrossfadeState::Ramping) {
      crossfadePhase += crossfadePhaseInc;

      if (crossfadePhase >= 1.0f) {
        crossfadePhase = 0.0f;

        // Swap roles: incoming FDN becomes active
        fdnAIsActive = !fdnAIsActive;
        xfadeState = CrossfadeState::Idle;
      }
    }
  }

  // -------------------------------------------------------------------------
  // 5b. Haas delay on R channel — temporal decorrelation for broadband width.
  //     Applied AFTER crossfade, BEFORE M/S width processing.
  //     At width=0% delay is 0 (pass-through), scaling to 12ms at 300%.
  // -------------------------------------------------------------------------
  if (dataR != nullptr) {
    haasDelaySmoother.setTargetValue(haasDelaySamples);
    for (int n = 0; n < numSamples; ++n) {
      haasDelayLine.pushSample(0, dataR[n]);
      const float smoothedHaas = haasDelaySmoother.getNextValue();
      dataR[n] = haasDelayLine.popSample(0, juce::jmax(0.0f, smoothedHaas));
    }
  }

  applyStereoWidth(numSamples, dataL, dataR, widthGain);

  outputEQ.process(numSamples, dataL, dataR);

  wowFlutter.process(numSamples, dataL, dataR, wfAmount, wfEnabled);

  applyDucking(numSamples, dataL, dataR, duckingNorm);

  // -------------------------------------------------------------------------
  // 7. Apply output level gain trim to WET signal only (before dry/wet mix)
  // -------------------------------------------------------------------------
  if (std::abs(outputLevelDb) > 0.01f) {
    const float outputGain = juce::Decibels::decibelsToGain(outputLevelDb);
    buffer.applyGain(outputGain);
  }

  // -------------------------------------------------------------------------
  // 8. Mix wet signal back with dry via DryWetMixer
  // -------------------------------------------------------------------------
  {
    juce::dsp::AudioBlock<float> block(buffer);
    dryWetMixer.mixWetSamples(block);
  }
}

//==============================================================================
juce::AudioProcessorEditor *ChaosverbAudioProcessor::createEditor() {
  return new ChaosverbAudioProcessorEditor(*this);
}

//==============================================================================
void ChaosverbAudioProcessor::getStateInformation(juce::MemoryBlock &destData) {
  auto state = parameters.copyState();
  std::unique_ptr<juce::XmlElement> xml(state.createXml());
  copyXmlToBinary(*xml, destData);
}

void ChaosverbAudioProcessor::setStateInformation(const void *data,
                                                  int sizeInBytes) {
  std::unique_ptr<juce::XmlElement> xmlState(
      getXmlFromBinary(data, sizeInBytes));

  if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
    parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Plugin factory function — required by JUCE plugin infrastructure
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new ChaosverbAudioProcessor();
}
