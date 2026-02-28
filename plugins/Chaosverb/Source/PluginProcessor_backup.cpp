#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter layout — 22 parameters: 12 Float + 10 Bool
// JUCE 8 requires juce::ParameterID { "id", 1 } format (not bare strings)
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
ChaosverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // -------------------------------------------------------------------------
    // Float parameters (12) — Audio controls, all lockable via mutation system
    // (except mutationInterval and crossfadeSpeed which are never randomized)
    // -------------------------------------------------------------------------

    // topology — FDN feedback matrix structure (0–100, linear, default 50)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "topology", 1 },
        "Topology",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        50.0f
    ));

    // decay — Reverb tail length (0.1–60s, logarithmic skew 0.35, default 4.0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "decay", 1 },
        "Decay",
        juce::NormalisableRange<float>(0.1f, 60.0f, 0.0f, 0.35f),
        4.0f,
        "s"
    ));

    // preDelay — Bipolar: left=BPM-synced divisions, center=0ms, right=free time
    // Range: -500 to +500 (symmetric = 12 o'clock at 0ms). Linear skew.
    // Negative = BPM sync mode, Positive = milliseconds.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "preDelay", 1 },
        "Pre-Delay",
        juce::NormalisableRange<float>(-500.0f, 500.0f, 0.0f, 1.0f),
        0.0f
    ));

    // density — Echo density / diffusion (0–100%, linear, default 60)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "density", 1 },
        "Density",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        60.0f,
        "%"
    ));

    // spectralTilt — Per-band independent decay scaling (-100 to +100, linear, default 0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "spectralTilt", 1 },
        "Spectral Tilt",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.0f, 1.0f),
        0.0f
    ));

    // resonance — Narrow resonant feedback peaks in tail (0–100%, linear, default 0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "resonance", 1 },
        "Resonance",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        0.0f,
        "%"
    ));

    // modRate — LFO speed for delay line modulation (0.01–10Hz, logarithmic skew 0.35, default 0.3)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "modRate", 1 },
        "Mod Rate",
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.0f, 0.35f),
        0.3f,
        "Hz"
    ));

    // modDepth — Amount of delay line length modulation (0–100%, linear, default 20)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "modDepth", 1 },
        "Mod Depth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        40.0f,
        "%"
    ));

    // mutationInterval — Timer interval between mutation events (5–120s, linear, default 30)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mutationInterval", 1 },
        "Mutation Interval",
        juce::NormalisableRange<float>(5.0f, 120.0f, 0.0f, 1.0f),
        30.0f,
        "s"
    ));

    // crossfadeSpeed — Wet signal crossfade duration at mutation (0–500ms, linear, default 100)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "crossfadeSpeed", 1 },
        "Crossfade Speed",
        juce::NormalisableRange<float>(0.0f, 500.0f, 0.0f, 1.0f),
        100.0f,
        "ms"
    ));

    // width — Stereo spread of wet signal (0–400%, linear, default 200)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "width", 1 },
        "Width",
        juce::NormalisableRange<float>(0.0f, 400.0f, 0.0f, 1.0f),
        200.0f,
        "%"
    ));

    // mix — Dry/wet balance (0–100%, linear, default 50)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        50.0f,
        "%"
    ));

    // outputLevel — Output gain trim (-12 to +12 dB, linear, default 0)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "outputLevel", 1 },
        "Output Level",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.0f, 1.0f),
        0.0f,
        "dB"
    ));

    // -------------------------------------------------------------------------
    // Output EQ parameters (3) — post-FDN wet signal shaping
    // -------------------------------------------------------------------------

    // lowCut — Highpass frequency on wet output (20–500Hz, log skew, default 80)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "lowCut", 1 },
        "Low Cut",
        juce::NormalisableRange<float>(20.0f, 500.0f, 0.0f, 0.35f),
        80.0f,
        "Hz"
    ));

    // highCut — Lowpass frequency on wet output (1k–20kHz, log skew, default 10k)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "highCut", 1 },
        "High Cut",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 0.0f, 0.35f),
        10000.0f,
        "Hz"
    ));

    // tilt — Output tilt EQ (-100 to +100, negative=warm/dark, positive=bright, default -10)
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "tilt", 1 },
        "Tilt",
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.0f, 1.0f),
        -10.0f
    ));

    // -------------------------------------------------------------------------
    // Wow & Flutter parameter (1) — combined depth+speed control
    // -------------------------------------------------------------------------

    // wowFlutterAmount — Combined wow/flutter intensity (0–100%, default 0)
    // Scales both modulation depth and rate together for a single-knob control.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "wowFlutterAmount", 1 },
        "Wow & Flutter",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        0.0f,
        "%"
    ));

    // duckingAmount — Wet signal ducking based on input envelope (0–100%, default 0)
    // When input is loud, the wet reverb signal is reduced proportionally.
    // 0% = no ducking, 100% = full ducking (wet goes silent during loud input).
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { "duckingAmount", 1 },
        "Ducking",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.0f, 1.0f),
        0.0f,
        "%"
    ));

    // -------------------------------------------------------------------------
    // Bool parameters (16) — Mutation lock controls + toggles
    // -------------------------------------------------------------------------

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "topologyLock", 1 },
        "Topology Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "decayLock", 1 },
        "Decay Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "preDelayLock", 1 },
        "Pre-Delay Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "densityLock", 1 },
        "Density Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "spectralTiltLock", 1 },
        "Spectral Tilt Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "resonanceLock", 1 },
        "Resonance Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "modRateLock", 1 },
        "Mod Rate Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "modDepthLock", 1 },
        "Mod Depth Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "widthLock", 1 },
        "Width Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "mixLock", 1 },
        "Mix Lock",
        false
    ));

    // Tone lock controls (3) — protect tone params from mutation
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "lowCutLock", 1 },
        "Low Cut Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "highCutLock", 1 },
        "High Cut Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "tiltLock", 1 },
        "Tilt Lock",
        false
    ));

    // Wow & Flutter lock + enable toggle
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "wowFlutterAmountLock", 1 },
        "Wow & Flutter Amount Lock",
        false
    ));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "wowFlutterEnabled", 1 },
        "Wow & Flutter Enabled",
        false
    ));

    // Output level lock
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "outputLevelLock", 1 },
        "Output Level Lock",
        false
    ));

    // Ducking amount lock
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "duckingAmountLock", 1 },
        "Ducking Lock",
        false
    ));

    // bypass — Global effect bypass (default: off = effect active)
    layout.add(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false
    ));

    return layout;
}

//==============================================================================
ChaosverbAudioProcessor::ChaosverbAudioProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , parameters(*this, nullptr, "Parameters", createParameterLayout())
{
}

ChaosverbAudioProcessor::~ChaosverbAudioProcessor()
{
    // Stop timer before destruction to prevent callbacks into a partially
    // destroyed object. Must happen before any member destruction.
    mutationTimerObj.stopTimer();
}

//==============================================================================
void ChaosverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels      = static_cast<juce::uint32>(getTotalNumOutputChannels());

    const double srRatio = sampleRate / 48000.0;

    // --- Pre-delay: max 250ms + 1 sample headroom ---
    // Max: 4000ms (accommodates 1/1 note at 60 BPM + free time 500ms)
    const int maxPreDelaySamples = static_cast<int>(std::ceil(4.0 * sampleRate)) + 1;
    preDelayLine.prepare(spec);
    preDelayLine.setMaximumDelayInSamples(maxPreDelaySamples);
    preDelayLine.setDelay(0.0f);
    preDelayLine.reset();

    // --- Allpass diffuser: scale delays to actual sample rate ---
    for (int i = 0; i < kNumDiffuserStages; ++i)
        allpassDelayLengths[i] = static_cast<int>(std::ceil(kAllpassLengths48k[i] * srRatio));

    diffLine0.prepare(spec); diffLine0.setMaximumDelayInSamples(allpassDelayLengths[0] + 1); diffLine0.setDelay(static_cast<float>(allpassDelayLengths[0])); diffLine0.reset();
    diffLine1.prepare(spec); diffLine1.setMaximumDelayInSamples(allpassDelayLengths[1] + 1); diffLine1.setDelay(static_cast<float>(allpassDelayLengths[1])); diffLine1.reset();
    diffLine2.prepare(spec); diffLine2.setMaximumDelayInSamples(allpassDelayLengths[2] + 1); diffLine2.setDelay(static_cast<float>(allpassDelayLengths[2])); diffLine2.reset();
    diffLine3.prepare(spec); diffLine3.setMaximumDelayInSamples(allpassDelayLengths[3] + 1); diffLine3.setDelay(static_cast<float>(allpassDelayLengths[3])); diffLine3.reset();

    // --- Dry/wet mixer ---
    dryWetMixer.prepare(spec);
    dryWetMixer.setMixingRule(juce::dsp::DryWetMixingRule::sin3dB);
    dryWetMixer.setWetMixProportion(0.5f);

    // --- Output EQ filters: prepare with mono spec (we call processSample manually) ---
    juce::dsp::ProcessSpec monoSpec { sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 };
    lowCutL.prepare(monoSpec);  lowCutR.prepare(monoSpec);
    highCutL.prepare(monoSpec); highCutR.prepare(monoSpec);
    tiltLowL.prepare(monoSpec); tiltLowR.prepare(monoSpec);
    tiltHighL.prepare(monoSpec); tiltHighR.prepare(monoSpec);

    // Initialize with defaults
    cachedLowCut = -1.0f; cachedHighCut = -1.0f; cachedTilt = -999.0f;

    // --- Haas delay: max 25ms on R channel for broadband stereo width ---
    const int maxHaasSamples = static_cast<int>(std::ceil(0.025 * sampleRate)) + 1;
    haasDelayLine.prepare(monoSpec);
    haasDelayLine.setMaximumDelayInSamples(maxHaasSamples);
    haasDelayLine.setDelay(0.0f);
    haasDelayLine.reset();

    // --- Wow & Flutter delay lines: separate L/R for full stereo independence ---
    // Max 6ms (3ms center + 3ms modulation) — reduced for subtlety
    const int maxWfDelaySamples = static_cast<int>(std::ceil(0.006 * sampleRate)) + 1;
    wfDelayLineL.prepare(monoSpec);
    wfDelayLineL.setMaximumDelayInSamples(maxWfDelaySamples);
    wfDelayLineL.setDelay(0.0f);
    wfDelayLineL.reset();
    wfDelayLineR.prepare(monoSpec);
    wfDelayLineR.setMaximumDelayInSamples(maxWfDelaySamples);
    wfDelayLineR.setDelay(0.0f);
    wfDelayLineR.reset();

    // Reset wow/flutter LFO phases (R channels offset by pi/2 for stereo)
    wfWowPhaseL     = 0.0f;
    wfWowPhaseR     = juce::MathConstants<float>::halfPi;
    wfFlutterPhaseL = 0.0f;
    wfFlutterPhaseR = juce::MathConstants<float>::halfPi;

    // --- Ducking envelope follower ---
    duckEnvelope = 0.0f;
    duckAttackCoeff  = 1.0f - std::exp(-1.0f / (0.0001f * static_cast<float>(sampleRate)));
    duckReleaseCoeff = 1.0f - std::exp(-1.0f / (0.015f * static_cast<float>(sampleRate)));

    // --- Both FDN instances: prepare together ---
    fdnA.prepare(spec);
    fdnB.prepare(spec);

    // --- Crossfade state machine: reset to Idle, A is active ---
    xfadeState       = CrossfadeState::Idle;
    crossfadePhase   = 0.0f;
    crossfadePhaseInc = 0.0f;
    fdnAIsActive     = true;

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
    if (mutationTimerRunning_.load())
    {
        // Short polling interval — timerCallback checks elapsed vs current param
        mutationTimerObj.startTimer(200);
    }
}

void ChaosverbAudioProcessor::releaseResources()
{
    preDelayLine.reset();
    diffLine0.reset(); diffLine1.reset(); diffLine2.reset(); diffLine3.reset();
    lowCutL.reset(); lowCutR.reset();
    highCutL.reset(); highCutR.reset();
    tiltLowL.reset(); tiltLowR.reset();
    tiltHighL.reset(); tiltHighR.reset();
    haasDelayLine.reset();
    wfDelayLineL.reset();
    wfDelayLineR.reset();
    dryWetMixer.reset();
    fdnA.reset();
    fdnB.reset();
}

//==============================================================================
// Phase 4.4 — Mutation Timer System
//==============================================================================

void ChaosverbAudioProcessor::triggerMutation()
{
    // Guard: if a crossfade trigger is already pending (not yet picked up by the
    // audio thread), skip this mutation. This prevents overlapping state changes
    // when the timer fires faster than the audio thread can process them.
    if (mutationPending.load(std::memory_order_acquire))
        return;

    // Record mutation timestamp for the countdown timer BEFORE setting params.
    lastMutationTimeMs = juce::Time::getMillisecondCounterHiRes();

    // Read all lock states (message thread — APVTS reads are always safe here)
    const bool topologyLocked    = parameters.getRawParameterValue("topologyLock")->load()    > 0.5f;
    const bool decayLocked       = parameters.getRawParameterValue("decayLock")->load()       > 0.5f;
    const bool preDelayLocked    = parameters.getRawParameterValue("preDelayLock")->load()    > 0.5f;
    const bool densityLocked     = parameters.getRawParameterValue("densityLock")->load()     > 0.5f;
    const bool spectralTiltLocked = parameters.getRawParameterValue("spectralTiltLock")->load() > 0.5f;
    const bool resonanceLocked   = parameters.getRawParameterValue("resonanceLock")->load()   > 0.5f;
    const bool modRateLocked     = parameters.getRawParameterValue("modRateLock")->load()     > 0.5f;
    const bool modDepthLocked    = parameters.getRawParameterValue("modDepthLock")->load()    > 0.5f;
    const bool widthLocked       = parameters.getRawParameterValue("widthLock")->load()       > 0.5f;
    const bool mixLocked         = parameters.getRawParameterValue("mixLock")->load()         > 0.5f;
    const bool lowCutLocked      = parameters.getRawParameterValue("lowCutLock")->load()      > 0.5f;
    const bool highCutLocked     = parameters.getRawParameterValue("highCutLock")->load()     > 0.5f;
    const bool tiltLocked        = parameters.getRawParameterValue("tiltLock")->load()        > 0.5f;
    const bool wfAmountLocked    = parameters.getRawParameterValue("wowFlutterAmountLock")->load() > 0.5f;
    const bool outputLevelLocked = parameters.getRawParameterValue("outputLevelLock")->load()     > 0.5f;
    const bool duckingLocked     = parameters.getRawParameterValue("duckingAmountLock")->load()  > 0.5f;

    // Use JUCE system random — uniform distribution, no musical weighting.
    // True chaos: every value in the full parameter range is equally likely.
    juce::Random& rng = juce::Random::getSystemRandom();

    // Helper: write a normalized value (0..1) to an APVTS parameter.
    // setValueNotifyingHost() normalises the value into JUCE's 0..1 range,
    // respecting the skew factor declared in createParameterLayout(). This is
    // why we pass a raw nextFloat() — JUCE maps it through the skew automatically.
    //
    // Thread safety: setValueNotifyingHost is safe on the message thread.
    auto randomize = [&](const char* paramID)
    {
        if (auto* param = parameters.getParameter(paramID))
            param->setValueNotifyingHost(rng.nextFloat());
    };

    if (!topologyLocked)    randomize("topology");
    if (!decayLocked)       randomize("decay");
    if (!preDelayLocked)    randomize("preDelay");
    if (!densityLocked)     randomize("density");
    if (!spectralTiltLocked) randomize("spectralTilt");
    if (!resonanceLocked)   randomize("resonance");
    if (!modRateLocked)     randomize("modRate");
    if (!modDepthLocked)    randomize("modDepth");
    if (!widthLocked)       randomize("width");
    if (!mixLocked)         randomize("mix");
    if (!lowCutLocked)      randomize("lowCut");
    if (!highCutLocked)     randomize("highCut");
    if (!tiltLocked)        randomize("tilt");
    if (!wfAmountLocked)    randomize("wowFlutterAmount");
    if (!outputLevelLocked) randomize("outputLevel");
    if (!duckingLocked)     randomize("duckingAmount");

    // Crossfade to the new parameter state (even if all params were locked —
    // the crossfade still fires, FDN-B gets the same values, no audible change).
    triggerCrossfade();
}

double ChaosverbAudioProcessor::getRemainingTimeMs() const
{
    if (lastMutationTimeMs == 0.0 || !mutationTimerRunning_.load())
        return -1.0; // -1 signals "timer stopped" to the UI

    const float intervalSeconds = parameters.getRawParameterValue("mutationInterval")->load();
    const double intervalMs     = static_cast<double>(intervalSeconds) * 1000.0;

    const double now      = juce::Time::getMillisecondCounterHiRes();
    const double elapsed  = now - lastMutationTimeMs;
    const double remaining = intervalMs - elapsed;

    return juce::jlimit(0.0, intervalMs, remaining);
}

void ChaosverbAudioProcessor::setMutationTimerRunning(bool running)
{
    mutationTimerRunning_.store(running);

    if (running)
    {
        // Reset baseline so countdown starts fresh from current interval
        lastMutationTimeMs = juce::Time::getMillisecondCounterHiRes();
        // Short polling interval — timerCallback checks elapsed vs current param
        mutationTimerObj.startTimer(200);
    }
    else
    {
        mutationTimerObj.stopTimer();
    }
}

bool ChaosverbAudioProcessor::isMutationTimerRunning() const
{
    return mutationTimerRunning_.load();
}

//==============================================================================
// Allpass diffuser helper (called from processBlock per-sample)
//==============================================================================
float ChaosverbAudioProcessor::processDiffuserSample(float input, int channel, int numActiveStages)
{
    float x = input;
    for (int stage = 0; stage < numActiveStages; ++stage)
    {
        auto& dl = getDiffLine(stage);
        const float delayed = dl.popSample(channel);
        const float v = x - kAllpassCoeff * delayed;
        dl.pushSample(channel, v);
        x = delayed + kAllpassCoeff * v;
    }
    return x;
}

//==============================================================================
void ChaosverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused(midiMessages);

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // -------------------------------------------------------------------------
    // Bypass — pass audio through unprocessed
    // -------------------------------------------------------------------------
    const bool bypassed = parameters.getRawParameterValue("bypass")->load() >= 0.5f;
    if (bypassed)
        return;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // -------------------------------------------------------------------------
    // Read all parameters — atomic loads, fully real-time safe
    // -------------------------------------------------------------------------
    const float topologyPercent  = parameters.getRawParameterValue("topology")->load();
    const float decaySeconds     = parameters.getRawParameterValue("decay")->load();
    const float preDelayParam    = parameters.getRawParameterValue("preDelay")->load();
    const float densityPercent   = parameters.getRawParameterValue("density")->load();
    const float spectralTiltVal  = parameters.getRawParameterValue("spectralTilt")->load();
    const float resonanceVal     = parameters.getRawParameterValue("resonance")->load();
    const float modRateHz        = parameters.getRawParameterValue("modRate")->load();
    const float modDepthPercent  = parameters.getRawParameterValue("modDepth")->load();
    const float widthPercent     = parameters.getRawParameterValue("width")->load();
    const float mixPercent       = parameters.getRawParameterValue("mix")->load();
    const float crossfadeSpeedMs = parameters.getRawParameterValue("crossfadeSpeed")->load();
    const float lowCutHz         = parameters.getRawParameterValue("lowCut")->load();
    const float highCutHz        = parameters.getRawParameterValue("highCut")->load();
    const float tiltVal          = parameters.getRawParameterValue("tilt")->load();
    const float wfAmount         = parameters.getRawParameterValue("wowFlutterAmount")->load();
    const bool  wfEnabled        = parameters.getRawParameterValue("wowFlutterEnabled")->load() > 0.5f;
    const float outputLevelDb    = parameters.getRawParameterValue("outputLevel")->load();
    const float duckingAmount    = parameters.getRawParameterValue("duckingAmount")->load();

    // -------------------------------------------------------------------------
    // Check mutation pending flag (set by message thread, cleared here)
    // Guard: ignore if crossfade already in progress
    // -------------------------------------------------------------------------
    if (mutationPending.load(std::memory_order_acquire) &&
        xfadeState == CrossfadeState::Idle)
    {
        mutationPending.store(false, std::memory_order_release);
        xfadeState     = CrossfadeState::Ramping;
        crossfadePhase = 0.0f;

        // Reset the incoming (inactive) FDN so it starts fresh.
        // This makes the crossfade audible: the outgoing FDN's old reverb
        // tail fades out while the incoming FDN builds new energy.
        if (fdnAIsActive)
            fdnB.reset();   // B is incoming — will be faded in
        else
            fdnA.reset();   // A is incoming — will be faded in

        const float sr = static_cast<float>(currentSampleRate);
        if (crossfadeSpeedMs <= 0.0f)
        {
            // Instant switch: jump phase directly to 1 (will complete this block)
            crossfadePhaseInc = 1.0f; // will reach 1.0 in one step
        }
        else
        {
            // ramp over crossfadeSpeedMs milliseconds
            crossfadePhaseInc = 1.0f / (crossfadeSpeedMs * 0.001f * sr);
        }
    }

    // -------------------------------------------------------------------------
    // Precompute per-block DSP parameters (avoids recomputing per-sample)
    // -------------------------------------------------------------------------

    const float sr = static_cast<float>(currentSampleRate);

    // Pre-delay: bipolar — right of center=free time (ms), left=BPM-synced divisions
    float preDelaySamples = 0.0f;
    if (preDelayParam >= 0.0f)
    {
        // Right of center: free time in ms (0-500ms)
        preDelaySamples = preDelayParam / 1000.0f * sr;
    }
    else
    {
        // Left of center: BPM-synced note divisions
        float bpm = 120.0f;
        if (auto* playhead = getPlayHead())
        {
            if (auto pos = playhead->getPosition())
            {
                if (auto hostBpm = pos->getBpm())
                    bpm = juce::jmax(20.0f, static_cast<float>(*hostBpm));
            }
        }

        const float quarterNoteMs = 60000.0f / bpm;
        const float t = -preDelayParam / 500.0f; // 0..1 (0=off, 1=whole note)

        // Quantize to nearest note division
        float beatFraction = 0.0f;
        if      (t > 0.9f) beatFraction = 4.0f;    // 1/1
        else if (t > 0.8f) beatFraction = 2.0f;    // 1/2
        else if (t > 0.7f) beatFraction = 1.5f;    // 1/4d
        else if (t > 0.6f) beatFraction = 1.0f;    // 1/4
        else if (t > 0.5f) beatFraction = 0.75f;   // 1/8d
        else if (t > 0.4f) beatFraction = 0.5f;    // 1/8
        else if (t > 0.3f) beatFraction = 0.375f;  // 1/16d
        else if (t > 0.2f) beatFraction = 0.25f;   // 1/16
        else if (t > 0.1f) beatFraction = 0.125f;  // 1/32

        preDelaySamples = (quarterNoteMs * beatFraction) / 1000.0f * sr;
    }

    preDelaySamples = juce::jmax(0.0f, preDelaySamples);
    preDelayLine.setDelay(preDelaySamples);

    // Feedback gain from T60 (decay) formula
    float meanDelaySamples = 0.0f;
    for (int i = 0; i < ChaosverbFDN::kNumLines; ++i)
    {
        // Average L and R delay lengths (different primes per channel for decorrelation)
        meanDelaySamples += static_cast<float>(fdnA.delayLengthsL[i] + fdnA.delayLengthsR[i]) * 0.5f;
    }
    meanDelaySamples /= static_cast<float>(ChaosverbFDN::kNumLines);

    const float meanLoopTime = meanDelaySamples / sr;
    const float safeDecay    = juce::jmax(0.01f, decaySeconds);
    const float rawGain      = std::exp(-6.91f * meanLoopTime / safeDecay);
    const float feedbackGain = juce::jmin(rawGain, 0.9999f);

    // Density -> diffuser stages + FDN in-loop allpass coefficient scaling
    // Wider thresholds = more dramatic jumps when sweeping the knob
    int numActiveDiffuserStages;
    if      (densityPercent <  20.0f) numActiveDiffuserStages = 0;
    else if (densityPercent <  40.0f) numActiveDiffuserStages = 1;
    else if (densityPercent <  55.0f) numActiveDiffuserStages = 2;
    else if (densityPercent <  75.0f) numActiveDiffuserStages = 3;
    else                              numActiveDiffuserStages = 4;

    // Scale FDN in-loop allpass coefficients: sparse (0%) → dense wash (100%)
    // Quadratic curve for more extreme contrast between low and high density
    const float densityNorm = densityPercent / 100.0f;
    const float densityCurved = densityNorm * densityNorm;
    fdnA.updateDensity(densityCurved);
    fdnB.updateDensity(densityCurved);

    // Topology -> blend factor (0.3=minimum Hadamard mixing, 1.0=full Hadamard)
    // Never allows pure diagonal mode (parallel combs = metallic). Always some cross-coupling.
    const float topologyBlend = 0.3f + 0.7f * (topologyPercent / 100.0f);

    // LFO depth in samples (both FDNs use same modDepth since params are shared)
    const float modDepthSamples = (modDepthPercent / 100.0f) * fdnA.maxLFODepthSamples;

    // Resonance smoothing coefficient (~50ms one-pole IIR)
    const float resoSmoothCoeff = 1.0f - std::exp(-1.0f / (0.050f * sr));

    // Stereo width gain: 0%=0.0, 100%=1.0, 400%=4.0
    const float widthGain = widthPercent / 100.0f;

    // Haas delay: scales from 0ms at width=0% to 25ms at width=400%
    // Adds temporal decorrelation to the R channel for broadband stereo width
    const float maxHaasMs = 25.0f;
    const float haasMs = (widthPercent / 400.0f) * maxHaasMs;
    const float haasDelaySamples = juce::jmax(0.0f, (haasMs / 1000.0f) * sr);

    // Update shelf and resonance coefficients in both FDNs when params change
    if (std::abs(spectralTiltVal - fdnA.cachedSpectralTilt) > 0.01f)
    {
        fdnA.updateShelfCoefficients(spectralTiltVal);
        fdnB.updateShelfCoefficients(spectralTiltVal);
    }
    if (std::abs(resonanceVal - fdnA.cachedResonance) > 0.01f)
    {
        fdnA.updateResonanceCoefficients(resonanceVal);
        fdnB.updateResonanceCoefficients(resonanceVal);
    }

    // Update output EQ coefficients when parameters change
    if (std::abs(lowCutHz - cachedLowCut) > 0.5f)
    {
        cachedLowCut = lowCutHz;
        auto hpCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(
            currentSampleRate, lowCutHz, 0.707f);
        *lowCutL.coefficients = *hpCoeffs;
        *lowCutR.coefficients = *hpCoeffs;
    }
    if (std::abs(highCutHz - cachedHighCut) > 1.0f)
    {
        cachedHighCut = highCutHz;
        auto lpCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            currentSampleRate, highCutHz, 0.707f);
        *highCutL.coefficients = *lpCoeffs;
        *highCutR.coefficients = *lpCoeffs;
    }
    if (std::abs(tiltVal - cachedTilt) > 0.1f)
    {
        cachedTilt = tiltVal;
        const float t = tiltVal / 100.0f; // -1..+1
        // Tilt: boost highs + cut lows (positive), or boost lows + cut highs (negative)
        const float tiltGainDb = t * 6.0f; // +-6dB max
        auto lowShelf = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            currentSampleRate, 600.0f, 0.707f, juce::Decibels::decibelsToGain(-tiltGainDb));
        auto highShelf = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            currentSampleRate, 3000.0f, 0.707f, juce::Decibels::decibelsToGain(tiltGainDb));
        *tiltLowL.coefficients  = *lowShelf;
        *tiltLowR.coefficients  = *lowShelf;
        *tiltHighL.coefficients = *highShelf;
        *tiltHighR.coefficients = *highShelf;
    }

    // Update LFO increments in both FDNs
    fdnA.prepareLFO(modRateHz, modDepthPercent);
    fdnB.prepareLFO(modRateHz, modDepthPercent);

    // Set DryWetMixer mix ratio
    dryWetMixer.setWetMixProportion(juce::jlimit(0.0f, 1.0f, mixPercent / 100.0f));

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
    float* dataL = (numChannels > 0) ? buffer.getWritePointer(0) : nullptr;
    float* dataR = (numChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    // -------------------------------------------------------------------------
    // Sample-by-sample processing loop
    //
    // Flow per sample:
    //   1. Pre-delay (shared, stereo)
    //   2. Allpass diffuser (shared, stereo)
    //   3. Both FDNs receive identical diffused L+R input
    //      fdnA.processSample(diffL, diffR, ...) -> wetAL, wetAR
    //      fdnB.processSample(diffL, diffR, ...) -> wetBL, wetBR
    //      (processSample advances LFO once per stereo sample — correct)
    //   4. Equal-power crossfade blend:
    //      wetL = gainA * wetAL + gainB * wetBL
    //      wetR = gainA * wetAR + gainB * wetBR
    //   5. Advance crossfade phase; handle state transitions
    //
    // After all samples:
    //   6. Stereo width M/S matrix on wet buffer
    //   7. DryWetMixer blend
    // -------------------------------------------------------------------------
    for (int n = 0; n < numSamples; ++n)
    {
        // --- 1. Pre-delay (shared stereo) ---
        const float rawL = (dataL != nullptr) ? dataL[n] : 0.0f;
        const float rawR = (dataR != nullptr) ? dataR[n] : 0.0f;

        // --- Envelope follower for ducking (track dry input amplitude) ---
        {
            const float inputPeak = std::max(std::abs(rawL), std::abs(rawR));
            if (inputPeak > duckEnvelope)
                duckEnvelope += duckAttackCoeff * (inputPeak - duckEnvelope);
            else
                duckEnvelope += duckReleaseCoeff * (inputPeak - duckEnvelope);
        }

        preDelayLine.pushSample(0, rawL);
        preDelayLine.pushSample(1, rawR);
        float sigL = preDelayLine.popSample(0);
        float sigR = preDelayLine.popSample(1);

        // --- 2. Allpass diffuser (shared stereo) ---
        if (numActiveDiffuserStages > 0)
        {
            sigL = processDiffuserSample(sigL, 0, numActiveDiffuserStages);
            sigR = processDiffuserSample(sigR, 1, numActiveDiffuserStages);
        }

        // --- 3. Both FDNs process identical diffused input ---
        // processSample processes L+R together, advancing LFO once per call.
        float wetAL = 0.0f, wetAR = 0.0f;
        float wetBL = 0.0f, wetBR = 0.0f;

        fdnA.processSample(sigL, sigR, feedbackGain, topologyBlend,
                           modDepthSamples, resoSmoothCoeff,
                           wetAL, wetAR);

        fdnB.processSample(sigL, sigR, feedbackGain, topologyBlend,
                           modDepthSamples, resoSmoothCoeff,
                           wetBL, wetBR);

        // --- 4. Use active FDN output + crossfade volume dip ---
        // Only listen to whichever FDN is currently active.
        float wetL = fdnAIsActive ? wetAL : wetBL;
        float wetR = fdnAIsActive ? wetAR : wetBR;

        // During mutation crossfade, apply a smooth V-shaped volume dip (1→0→1).
        // This makes the crossfade speed directly control the audible transition:
        // short = quick dip, long = dramatic fade-out and back-in.
        if (xfadeState == CrossfadeState::Ramping)
        {
            const float twoPi = juce::MathConstants<float>::twoPi;
            const float dip = 0.5f + 0.5f * std::cos(crossfadePhase * twoPi);
            wetL *= dip;
            wetR *= dip;
        }

        // --- Apply ducking: reduce wet signal based on input envelope ---
        if (duckingNorm > 0.001f)
        {
            // 4x sensitivity scaling so typical mix levels (-18 to -6 dBFS)
            // produce meaningful ducking without requiring a threshold knob
            const float duckGain = juce::jlimit(0.0f, 1.0f,
                1.0f - duckingNorm * duckEnvelope * 4.0f);
            wetL *= duckGain;
            wetR *= duckGain;
        }

        if (dataL != nullptr) dataL[n] = wetL;
        if (dataR != nullptr) dataR[n] = wetR;

        // --- 5. Advance crossfade phase and handle state transitions ---
        if (xfadeState == CrossfadeState::Ramping)
        {
            crossfadePhase += crossfadePhaseInc;

            if (crossfadePhase >= 1.0f)
            {
                crossfadePhase = 0.0f;

                // Swap roles: inactive FDN becomes active
                fdnAIsActive = !fdnAIsActive;
                xfadeState   = CrossfadeState::Idle;
            }
        }
    }

    // -------------------------------------------------------------------------
    // 5b. Haas delay on R channel — temporal decorrelation for broadband width.
    //     Applied AFTER crossfade, BEFORE M/S width processing.
    //     At width=0% delay is 0 (pass-through), scaling to 12ms at 300%.
    // -------------------------------------------------------------------------
    if (dataR != nullptr && haasDelaySamples > 0.01f)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            haasDelayLine.pushSample(0, dataR[n]);
            dataR[n] = haasDelayLine.popSample(0, haasDelaySamples);
        }
    }

    // -------------------------------------------------------------------------
    // 6. Apply stereo width M/S matrix to wet buffer (in-place)
    //    Applied BEFORE DryWetMixer::mixWetSamples per architecture spec.
    //    width=0%: mono, width=100%: normal stereo, width=200%: exaggerated.
    //    0.5 encode factor prevents clipping at >100% width.
    // -------------------------------------------------------------------------
    if (dataL != nullptr && dataR != nullptr && numChannels == 2)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            const float mid  = (dataL[n] + dataR[n]) * 0.5f;
            const float side = (dataL[n] - dataR[n]) * 0.5f;
            dataL[n] = mid + side * widthGain;
            dataR[n] = mid - side * widthGain;
        }
    }

    // -------------------------------------------------------------------------
    // 6b. Apply output EQ (low cut, high cut, tilt) to wet buffer
    //     Applied AFTER stereo width, BEFORE DryWetMixer (wet signal only).
    // -------------------------------------------------------------------------
    if (dataL != nullptr && dataR != nullptr)
    {
        for (int n = 0; n < numSamples; ++n)
        {
            // Low cut (highpass) — removes rumble
            dataL[n] = lowCutL.processSample(dataL[n]);
            dataR[n] = lowCutR.processSample(dataR[n]);

            // High cut (lowpass) — removes harshness
            dataL[n] = highCutL.processSample(dataL[n]);
            dataR[n] = highCutR.processSample(dataR[n]);

            // Tilt EQ — overall brightness/darkness
            dataL[n] = tiltLowL.processSample(dataL[n]);
            dataL[n] = tiltHighL.processSample(dataL[n]);
            dataR[n] = tiltLowR.processSample(dataR[n]);
            dataR[n] = tiltHighR.processSample(dataR[n]);
        }
    }

    // -------------------------------------------------------------------------
    // 6c. Wow & Flutter — modulated pitch shift on wet signal
    //     Applied AFTER output EQ, BEFORE dry/wet mix.
    //     Two LFOs per channel: wow (slow+deep) + flutter (fast+shallow).
    //     Entirely bypassed when disabled — no latency, no coloration.
    //     Does NOT touch the FDN core — only modulates the post-FDN wet signal.
    // -------------------------------------------------------------------------
    if (wfEnabled && wfAmount > 0.001f && dataL != nullptr && dataR != nullptr)
    {
        const float wfNorm       = wfAmount / 100.0f;
        const float maxDepthMs   = 2.0f;   // reduced from 5ms for subtlety
        const float depthSamples = wfNorm * maxDepthMs * 0.001f * sr;

        // Wow: slow, deep pitch drift (tape transport instability)
        // Flutter: fast, shallow pitch wobble (head vibration)
        const float wowRate      = 0.3f + wfNorm * 0.8f;    // 0.3–1.1 Hz
        const float flutterRate  = 5.0f + wfNorm * 6.0f;    // 5–11 Hz

        const float twoPi      = juce::MathConstants<float>::twoPi;
        const float wowInc     = twoPi * wowRate / sr;
        const float flutterInc = twoPi * flutterRate / sr;

        // Center delay: always at max depth so modulation stays within buffer
        const float centerDelay = maxDepthMs * 0.001f * sr;

        for (int n = 0; n < numSamples; ++n)
        {
            const float modL = depthSamples * (0.7f * std::sin(wfWowPhaseL)
                                              + 0.3f * std::sin(wfFlutterPhaseL));
            const float modR = depthSamples * (0.7f * std::sin(wfWowPhaseR)
                                              + 0.3f * std::sin(wfFlutterPhaseR));

            // Separate L/R delay lines — guarantees full stereo independence
            wfDelayLineL.pushSample(0, dataL[n]);
            wfDelayLineR.pushSample(0, dataR[n]);
            dataL[n] = wfDelayLineL.popSample(0, juce::jmax(1.0f, centerDelay + modL));
            dataR[n] = wfDelayLineR.popSample(0, juce::jmax(1.0f, centerDelay + modR));

            wfWowPhaseL     += wowInc;
            wfWowPhaseR     += wowInc;
            wfFlutterPhaseL += flutterInc;
            wfFlutterPhaseR += flutterInc;

            if (wfWowPhaseL     >= twoPi) wfWowPhaseL     -= twoPi;
            if (wfWowPhaseR     >= twoPi) wfWowPhaseR     -= twoPi;
            if (wfFlutterPhaseL >= twoPi) wfFlutterPhaseL -= twoPi;
            if (wfFlutterPhaseR >= twoPi) wfFlutterPhaseR -= twoPi;
        }
    }

    // -------------------------------------------------------------------------
    // 7. Apply output level gain trim to WET signal only (before dry/wet mix)
    // -------------------------------------------------------------------------
    if (std::abs(outputLevelDb) > 0.01f)
    {
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
juce::AudioProcessorEditor* ChaosverbAudioProcessor::createEditor()
{
    return new ChaosverbAudioProcessorEditor(*this);
}

//==============================================================================
void ChaosverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChaosverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Plugin factory function — required by JUCE plugin infrastructure
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ChaosverbAudioProcessor();
}
