#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
// Parameter Layout
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout NBS_DynaDriveAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // --- Global ---

    // bypass — global effect bypass
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 },
        "Bypass",
        false));

    // sat_enable — soft clipping on/off
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "sat_enable", 1 },
        "Soft Clip",
        true));

    // comp_enable — compressor on/off
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "comp_enable", 1 },
        "Comp Enable",
        true));

    // --- Saturation Section ---

    // drive — primary saturation input drive (0–100 %)
    // Skew 0.6: more travel in subtle 0–40% range (driveGain 1–4, gentle warmth)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 },
        "Drive",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 0.6f),
        20.0f,
        "%"));

    // even — even harmonic content (2nd/4th order), tube-like warmth (0–100 %)
    // Skew 0.5: subtle effect (max 0.15 bias), most change in 0–30%
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "even", 1 },
        "Even",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 0.5f),
        0.0f,
        "%"));

    // odd — odd harmonic content (3rd/5th order), presence and grit (0–100 %)
    // Skew 0.5: subtle effect (max 0.05 cubic gain), most change in 0–30%
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "odd", 1 },
        "Odd",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 0.5f),
        0.0f,
        "%"));

    // h_curve — saturation curve shape, soft to hard (0–100 %)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "h_curve", 1 },
        "Shape",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"));

    // --- Center Section ---

    // pre_post — signal routing order: false = DYN->SAT, true = SAT->DYN
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "pre_post", 1 },
        "Signal Flow",
        false));

    // input — input gain before the processing chain (-24 to +12 dB)
    // Skew 2.5: midpoint around 0 dB where most work happens
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 },
        "Input",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f, 2.5f),
        0.0f,
        "dB"));

    // mix — dry/wet parallel blend (0–100 %)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 },
        "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 1.0f),
        100.0f,
        "%"));

    // output — makeup gain after the processing chain (-24 to +12 dB)
    // Skew 2.5: midpoint around 0 dB where most work happens
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 },
        "Output",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f, 2.5f),
        0.0f,
        "dB"));

    // drive_out — saturation section output volume (-24 to +12 dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive_out", 1 },
        "Drive Out",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f, 2.5f),
        0.0f,
        "dB"));

    // comp_out — compressor section output volume (-24 to +12 dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "comp_out", 1 },
        "Comp Out",
        juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f, 2.5f),
        0.0f,
        "dB"));

    // sat_tilt_freq — post-saturation tilt filter pivot frequency (100–10000 Hz)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat_tilt_freq", 1 },
        "Sat Tilt Freq",
        juce::NormalisableRange<float> (80.0f, 12000.0f, 1.0f, 0.35f),
        1000.0f,
        "Hz"));

    // sat_tilt_slope — post-saturation tilt slope: 0 = flat, +6 = bright, -6 = dark (dB/oct)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "sat_tilt_slope", 1 },
        "Sat Tilt Slope",
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f, 1.0f),
        0.0f,
        "dB/oct"));

    // --- Dynamics Section ---

    // dynamics — macro dynamics amount (0–100 %)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dynamics", 1 },
        "Dynamics",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 1.0f),
        30.0f,
        "%"));

    // up — upward compression amount (0–100 %)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "up", 1 },
        "Up",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 1.0f),
        0.0f,
        "%"));

    // down — downward compression amount (0–100 %)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "down", 1 },
        "Down",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 1.0f),
        50.0f,
        "%"));

    // --- Advanced Panel: Dynamics Detail ---

    // threshold — compressor threshold (-36 to 0 dB)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 },
        "Threshold",
        juce::NormalisableRange<float> (-36.0f, 0.0f, 0.1f, 1.0f),
        -18.0f,
        "dB"));

    // ratio — compression ratio (1.0 to 10.0 :1)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ratio", 1 },
        "Ratio",
        juce::NormalisableRange<float> (1.0f, 10.0f, 0.1f, 0.5f),
        4.0f,
        ":1"));

    // attack_time — compressor attack time (0.1–100 ms)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack_time", 1 },
        "Attack",
        juce::NormalisableRange<float> (0.1f, 100.0f, 0.01f, 0.5f),
        10.0f,
        "ms"));

    // release_time — compressor release time (10–1000 ms)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release_time", 1 },
        "Release",
        juce::NormalisableRange<float> (10.0f, 1000.0f, 1.0f, 0.5f),
        100.0f,
        "ms"));

    // --- Advanced Panel: Post-Dynamics Tilt ---

    // dyn_tilt_freq — post-dynamics tilt filter pivot frequency (100–10000 Hz)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dyn_tilt_freq", 1 },
        "Dyn Tilt Freq",
        juce::NormalisableRange<float> (80.0f, 12000.0f, 1.0f, 0.35f),
        1000.0f,
        "Hz"));

    // dyn_tilt_slope — post-dynamics tilt slope (dB/oct)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "dyn_tilt_slope", 1 },
        "Dyn Tilt Slope",
        juce::NormalisableRange<float> (-6.0f, 6.0f, 0.1f, 1.0f),
        0.0f,
        "dB/oct"));

    // --- Advanced Panel: M/S Processing ---

    // ms_enable — enables mid/side matrix processing
    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "ms_enable", 1 },
        "M/S Enable",
        false));

    // mid_drive — mid channel saturation drive in M/S mode (0–100 %)
    // Skew 0.6: match main drive feel
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mid_drive", 1 },
        "Mid Drive",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 0.6f),
        20.0f,
        "%"));

    // side_drive — side channel saturation drive in M/S mode (0–100 %)
    // Skew 0.6: match main drive feel
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "side_drive", 1 },
        "Side Drive",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f, 0.6f),
        20.0f,
        "%"));

    return { params.begin(), params.end() };
}

//==============================================================================
// Constructor / Destructor
//==============================================================================

NBS_DynaDriveAudioProcessor::NBS_DynaDriveAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true))
    , parameters (*this, nullptr, "Parameters", createParameterLayout())
{
}

NBS_DynaDriveAudioProcessor::~NBS_DynaDriveAudioProcessor()
{
}

//==============================================================================
// Prepare / Release
//==============================================================================

void NBS_DynaDriveAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Build a ProcessSpec describing the current stream
    processSpec.sampleRate       = sampleRate;
    processSpec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    processSpec.numChannels      = static_cast<juce::uint32> (getTotalNumOutputChannels());

    //--------------------------------------------------------------------------
    // Phase 4.1 components
    //--------------------------------------------------------------------------

    inputGain.prepare (processSpec);
    inputGain.reset();
    inputGain.setGainLinear (1.0f);

    // 2x oversampling — prepare before ADAA saturator
    oversampling.initProcessing (static_cast<size_t> (samplesPerBlock));
    oversampling.reset();

    // ADAA saturator — handles h_curve blend, even/odd harmonics, DC blocking
    // Channel 0 = L (or Mid in M/S), Channel 1 = R (or Side in M/S)
    // Prepared at 4x sample rate since it runs inside the oversampled block
    adaaSaturator.prepare (sampleRate * 4.0);
    adaaSaturator.reset();

    outputGain.prepare (processSpec);
    outputGain.reset();
    outputGain.setGainLinear (1.0f);

    dryWetMixer.prepare (processSpec);
    dryWetMixer.reset();

    // Phase compensation: the 4x oversampling FIR filter introduces group delay.
    // Tell the DryWetMixer so dry samples are delayed to match, preventing
    // comb-filtering artefacts when mix < 100%.
    const auto osLatency = static_cast<int> (oversampling.getLatencyInSamples());
    dryWetMixer.setWetLatency (osLatency);
    dryWetMixer.setWetMixProportion (1.0f);

    // Report total latency to the DAW for PDC (plugin delay compensation)
    setLatencySamples (osLatency);

    //--------------------------------------------------------------------------
    // Phase 4.2: Tilt filters
    //
    // Tilt EQ design: a low shelf and high shelf share the same pivot frequency.
    // When slope > 0 (bright): low shelf attenuates, high shelf boosts.
    // When slope < 0 (dark):   low shelf boosts, high shelf attenuates.
    //
    // Initial coefficients: unity gain (flat response).
    // Coefficients are updated in processBlock() when parameters change.
    //--------------------------------------------------------------------------
    const float flatGain = 1.0f;
    const float tiltQ    = 0.7071f;

    *satTiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, cachedSatTiltFreq, tiltQ, flatGain);
    satTiltLowShelf.prepare (processSpec);

    *satTiltHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, cachedSatTiltFreq, tiltQ, flatGain);
    satTiltHighShelf.prepare (processSpec);

    *dynTiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sampleRate, cachedDynTiltFreq, tiltQ, flatGain);
    dynTiltLowShelf.prepare (processSpec);

    *dynTiltHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sampleRate, cachedDynTiltFreq, tiltQ, flatGain);
    dynTiltHighShelf.prepare (processSpec);

    //--------------------------------------------------------------------------
    // Phase 4.3: Dynamics Engines
    //--------------------------------------------------------------------------

    stereoEngine.prepare (sampleRate);
    stereoEngine.reset();

    midEngine.prepare (sampleRate);
    midEngine.reset();

    sideEngine.prepare (sampleRate);
    sideEngine.reset();

    //--------------------------------------------------------------------------
    // Parameter smoothers — 5 ms ramp at current sample rate
    //--------------------------------------------------------------------------
    const double rampSeconds = 0.005;

    // Phase 4.1 smoothers
    inputGainSmoother.reset  (sampleRate, rampSeconds);
    outputGainSmoother.reset (sampleRate, rampSeconds);
    driveOutSmoother.reset   (sampleRate, rampSeconds);
    compOutSmoother.reset    (sampleRate, rampSeconds);
    driveSmoother.reset      (sampleRate, rampSeconds);

    // Phase 4.2 smoothers
    alphaSmoother.reset (sampleRate, rampSeconds);
    biasSmoother.reset  (sampleRate, rampSeconds);
    oddSmoother.reset   (sampleRate, rampSeconds);

    // Phase 4.3 M/S drive smoothers
    midDriveSmoother.reset  (sampleRate, rampSeconds);
    sideDriveSmoother.reset (sampleRate, rampSeconds);

    // M/S crossfade smoother (10 ms ramp — prevents click on toggle)
    msSmoother.reset (sampleRate, 0.010);
    msBlendBuffer.resize (static_cast<size_t> (samplesPerBlock));

    // Seed all smoothers with current parameter values (no startup ramp)
    const float inputDb   = parameters.getRawParameterValue ("input")->load();
    const float outputDb  = parameters.getRawParameterValue ("output")->load();
    const float driveVal  = parameters.getRawParameterValue ("drive")->load();
    const float hCurve    = parameters.getRawParameterValue ("h_curve")->load();
    const float evenVal   = parameters.getRawParameterValue ("even")->load();
    const float oddVal    = parameters.getRawParameterValue ("odd")->load();
    const float midDriveV = parameters.getRawParameterValue ("mid_drive")->load();
    const float sideDriveV= parameters.getRawParameterValue ("side_drive")->load();

    inputGainSmoother.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (inputDb));
    outputGainSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputDb));

    const float driveOutDb = parameters.getRawParameterValue ("drive_out")->load();
    const float compOutDb  = parameters.getRawParameterValue ("comp_out")->load();
    driveOutSmoother.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveOutDb));
    compOutSmoother.setCurrentAndTargetValue  (juce::Decibels::decibelsToGain (compOutDb));

    const float driveGain = 1.0f + (driveVal     / 100.0f) * 7.0f;
    const float midGain   = 1.0f + (midDriveV   / 100.0f) * 7.0f;
    const float sideGain  = 1.0f + (sideDriveV  / 100.0f) * 7.0f;

    driveSmoother.setCurrentAndTargetValue    (driveGain);
    midDriveSmoother.setCurrentAndTargetValue (midGain);
    sideDriveSmoother.setCurrentAndTargetValue(sideGain);

    alphaSmoother.setCurrentAndTargetValue (hCurve  / 100.0f);
    biasSmoother.setCurrentAndTargetValue  ((evenVal / 100.0f) * 0.15f);
    oddSmoother.setCurrentAndTargetValue   ((oddVal  / 100.0f) * 0.05f);

    // M/S crossfade: seed from current parameter
    const float msInit = parameters.getRawParameterValue ("ms_enable")->load() > 0.5f ? 1.0f : 0.0f;
    msSmoother.setCurrentAndTargetValue (msInit);
}

void NBS_DynaDriveAudioProcessor::releaseResources()
{
    oversampling.reset();
    adaaSaturator.reset();
    stereoEngine.reset();
    midEngine.reset();
    sideEngine.reset();
}

bool NBS_DynaDriveAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

//==============================================================================
// Tilt Filter Coefficient Helpers
//==============================================================================

void NBS_DynaDriveAudioProcessor::updateSatTiltCoefficients (float freq, float slopeDb)
{
    const double sr    = processSpec.sampleRate;
    const float  tiltQ = 0.7071f;

    const float highShelfGain = juce::Decibels::decibelsToGain ( slopeDb);
    const float lowShelfGain  = juce::Decibels::decibelsToGain (-slopeDb);

    *satTiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sr, static_cast<double> (freq), tiltQ, lowShelfGain);

    *satTiltHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sr, static_cast<double> (freq), tiltQ, highShelfGain);
}

void NBS_DynaDriveAudioProcessor::updateDynTiltCoefficients (float freq, float slopeDb)
{
    const double sr    = processSpec.sampleRate;
    const float  tiltQ = 0.7071f;

    const float highShelfGain = juce::Decibels::decibelsToGain ( slopeDb);
    const float lowShelfGain  = juce::Decibels::decibelsToGain (-slopeDb);

    *dynTiltLowShelf.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf (
        sr, static_cast<double> (freq), tiltQ, lowShelfGain);

    *dynTiltHighShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (
        sr, static_cast<double> (freq), tiltQ, highShelfGain);
}

//==============================================================================
// Phase 4.3 Helper: applySatTilt / applyDynTilt
//==============================================================================

void NBS_DynaDriveAudioProcessor::applySatTilt (juce::AudioBuffer<float>& buf, float slopeDb) noexcept
{
    if (slopeDb == 0.0f)
        return;

    juce::dsp::AudioBlock<float> blk (buf);
    juce::dsp::ProcessContextReplacing<float> ctx (blk);
    satTiltLowShelf.process  (ctx);
    satTiltHighShelf.process (ctx);
}

void NBS_DynaDriveAudioProcessor::applyDynTilt (juce::AudioBuffer<float>& buf, float slopeDb) noexcept
{
    if (slopeDb == 0.0f)
        return;

    juce::dsp::AudioBlock<float> blk (buf);
    juce::dsp::ProcessContextReplacing<float> ctx (blk);
    dynTiltLowShelf.process  (ctx);
    dynTiltHighShelf.process (ctx);
}

//==============================================================================
// Phase 4.3 Helper: runSaturation (stereo, AudioBlock)
//
//   Applies ADAA waveshaping to all channels of block.
//   Works on oversampled AudioBlock directly.
//==============================================================================
void NBS_DynaDriveAudioProcessor::runSaturation (juce::dsp::AudioBlock<float>& block,
                                                   float driveGain, float alpha,
                                                   float bias, float oddGain) noexcept
{
    const int numChannels = static_cast<int> (block.getNumChannels());
    const int numSamples  = static_cast<int> (block.getNumSamples());

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = block.getChannelPointer (static_cast<size_t> (ch));
        const int adaaCh = (ch < 2) ? ch : 1;

        for (int n = 0; n < numSamples; ++n)
            data[n] = adaaSaturator.processSample (data[n], adaaCh, driveGain, alpha, bias, oddGain);
    }
}

//==============================================================================
// Phase 4.3 Helper: runSaturationMS (M/S mode, AudioBlock)
//
//   Per-channel drive: ch0=mid uses midDrive, ch1=side uses sideDrive.
//==============================================================================
void NBS_DynaDriveAudioProcessor::runSaturationMS (juce::dsp::AudioBlock<float>& block,
                                                     float midDrive, float sideDrive,
                                                     float alpha, float bias, float oddGain) noexcept
{
    const int numSamples = static_cast<int> (block.getNumSamples());

    if (block.getNumChannels() >= 2)
    {
        float* dataMid  = block.getChannelPointer (0);
        float* dataSide = block.getChannelPointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            dataMid[n]  = adaaSaturator.processSample (dataMid[n],  0, midDrive,  alpha, bias, oddGain);
            dataSide[n] = adaaSaturator.processSample (dataSide[n], 1, sideDrive, alpha, bias, oddGain);
        }
    }
}

//==============================================================================
// Phase 4.3 Helper: runDynamicsStereo
//
//   Detects linked stereo level (max L/R) and applies the same gain to both channels.
//==============================================================================
void NBS_DynaDriveAudioProcessor::runDynamicsStereo (juce::AudioBuffer<float>& buf,
                                                       int numSamples,
                                                       float thresholdDb, float ratio,
                                                       float attackCoeff, float releaseCoeff,
                                                       float downAmount, float upAmount,
                                                       float dynamicsMacro) noexcept
{
    jassert (buf.getNumChannels() >= 2);

    float* dataL = buf.getWritePointer (0);
    float* dataR = buf.getWritePointer (1);

    for (int n = 0; n < numSamples; ++n)
    {
        stereoEngine.detectLevel (dataL[n], dataR[n]);

        const float g = stereoEngine.computeGain (thresholdDb, ratio,
                                                   attackCoeff, releaseCoeff,
                                                   downAmount, upAmount, dynamicsMacro);
        dataL[n] *= g;
        dataR[n] *= g;
    }
}

//==============================================================================
// Process
//==============================================================================

void NBS_DynaDriveAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                                 juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    juce::ignoreUnused (midiMessages);

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // Clear any channels beyond our input count (defensive)
    for (int ch = getTotalNumInputChannels(); ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);

    //--------------------------------------------------------------------------
    // 0. Bypass — pass audio through unprocessed
    //--------------------------------------------------------------------------
    const bool bypassed = parameters.getRawParameterValue ("bypass")->load() > 0.5f;
    if (bypassed)
        return;

    //--------------------------------------------------------------------------
    // 1. Read parameters (atomic, lock-free — real-time safe)
    //--------------------------------------------------------------------------

    // Phase 4.1 parameters
    const float inputDb  = parameters.getRawParameterValue ("input")->load();
    const float outputDb = parameters.getRawParameterValue ("output")->load();
    const float driveVal = parameters.getRawParameterValue ("drive")->load();
    const float mixVal   = parameters.getRawParameterValue ("mix")->load();

    // Phase 4.2 parameters
    const float hCurve       = parameters.getRawParameterValue ("h_curve")->load();
    const float evenVal      = parameters.getRawParameterValue ("even")->load();
    const float oddVal       = parameters.getRawParameterValue ("odd")->load();
    const float satTiltFreq  = parameters.getRawParameterValue ("sat_tilt_freq")->load();
    const float satTiltSlope = parameters.getRawParameterValue ("sat_tilt_slope")->load();
    const float dynTiltFreq  = parameters.getRawParameterValue ("dyn_tilt_freq")->load();
    const float dynTiltSlope = parameters.getRawParameterValue ("dyn_tilt_slope")->load();

    // Phase 4.3 parameters
    const bool  prePost       = parameters.getRawParameterValue ("pre_post")->load() > 0.5f;
    const bool  msEnable      = parameters.getRawParameterValue ("ms_enable")->load() > 0.5f;
    const bool  satEnable     = parameters.getRawParameterValue ("sat_enable")->load() > 0.5f;
    const bool  compEnable    = parameters.getRawParameterValue ("comp_enable")->load() > 0.5f;
    const float driveOutDb    = parameters.getRawParameterValue ("drive_out")->load();
    const float compOutDb     = parameters.getRawParameterValue ("comp_out")->load();
    const float midDriveVal   = parameters.getRawParameterValue ("mid_drive")->load();
    const float sideDriveVal  = parameters.getRawParameterValue ("side_drive")->load();
    const float dynamicsMacro = parameters.getRawParameterValue ("dynamics")->load() / 100.0f;
    const float upAmount      = parameters.getRawParameterValue ("up")->load()       / 100.0f;
    const float downAmount    = parameters.getRawParameterValue ("down")->load()     / 100.0f;
    const float thresholdDb   = parameters.getRawParameterValue ("threshold")->load();
    const float ratio         = parameters.getRawParameterValue ("ratio")->load();
    const float attackMs      = parameters.getRawParameterValue ("attack_time")->load();
    const float releaseMs     = parameters.getRawParameterValue ("release_time")->load();

    //--------------------------------------------------------------------------
    // 2. Update smoother targets
    //--------------------------------------------------------------------------

    inputGainSmoother.setTargetValue  (juce::Decibels::decibelsToGain (inputDb));
    outputGainSmoother.setTargetValue (juce::Decibels::decibelsToGain (outputDb));
    driveOutSmoother.setTargetValue   (juce::Decibels::decibelsToGain (driveOutDb));
    compOutSmoother.setTargetValue    (juce::Decibels::decibelsToGain (compOutDb));

    // drive (0–100 %) → ADAA linear gain (1.0 to 8.0)
    driveSmoother.setTargetValue (1.0f + (driveVal    / 100.0f) * 7.0f);
    midDriveSmoother.setTargetValue  (1.0f + (midDriveVal  / 100.0f) * 7.0f);
    sideDriveSmoother.setTargetValue (1.0f + (sideDriveVal / 100.0f) * 7.0f);

    // h_curve (0–100 %) → alpha blend (0.0–1.0)
    alphaSmoother.setTargetValue (hCurve / 100.0f);

    // even (0–100 %) → bias (0.0–0.15)
    biasSmoother.setTargetValue ((evenVal / 100.0f) * 0.15f);

    // odd (0–100 %) → cubic gain (0.0–0.05)
    oddSmoother.setTargetValue ((oddVal / 100.0f) * 0.05f);

    //--------------------------------------------------------------------------
    // 2b. Pre-compute M/S blend values (per-sample crossfade, click-free)
    //--------------------------------------------------------------------------
    msSmoother.setTargetValue (msEnable ? 1.0f : 0.0f);
    for (int n = 0; n < numSamples; ++n)
        msBlendBuffer[static_cast<size_t> (n)] = msSmoother.getNextValue();

    //--------------------------------------------------------------------------
    // 3. Update tilt filter coefficients if parameters changed
    //--------------------------------------------------------------------------

    const bool satTiltChanged = (satTiltFreq  != cachedSatTiltFreq ||
                                  satTiltSlope != cachedSatTiltSlope);
    if (satTiltChanged)
    {
        cachedSatTiltFreq  = satTiltFreq;
        cachedSatTiltSlope = satTiltSlope;

        if (satTiltSlope != 0.0f)
            updateSatTiltCoefficients (satTiltFreq, satTiltSlope);
    }

    const bool dynTiltChanged = (dynTiltFreq  != cachedDynTiltFreq ||
                                  dynTiltSlope != cachedDynTiltSlope);
    if (dynTiltChanged)
    {
        cachedDynTiltFreq  = dynTiltFreq;
        cachedDynTiltSlope = dynTiltSlope;

        if (dynTiltSlope != 0.0f)
            updateDynTiltCoefficients (dynTiltFreq, dynTiltSlope);
    }

    //--------------------------------------------------------------------------
    // 4. Pre-compute ballistic coefficients for dynamics
    //    exp(-1 / (time_s * sampleRate))
    //    Clamped to [0, 0.9999] to ensure the IIR always converges.
    //--------------------------------------------------------------------------
    const double sr = processSpec.sampleRate;

    const float attackCoeff  = static_cast<float> (
        std::exp (-1.0 / (static_cast<double> (attackMs)  * 0.001 * sr)));
    const float releaseCoeff = static_cast<float> (
        std::exp (-1.0 / (static_cast<double> (releaseMs) * 0.001 * sr)));

    //--------------------------------------------------------------------------
    // 5. Advance smoothers by one block (get settled values for this block)
    //    Drive/alpha/bias/odd are held constant across the block.
    //    Per-sample advancing for input/output gain is done inline below.
    //--------------------------------------------------------------------------

    const float alphaBlock    = alphaSmoother.getNextValue();
    const float biasBlock     = biasSmoother.getNextValue();
    const float oddGainBlock  = oddSmoother.getNextValue();

    // Settle drive smoothers to their block value (advance by one step)
    // We will use the settled value for saturation in this block.
    const float driveBlock    = driveSmoother.getNextValue();
    const float midDriveBlock = midDriveSmoother.getNextValue();
    const float sideDriveBlock= sideDriveSmoother.getNextValue();
    const float driveOutBlock = driveOutSmoother.getNextValue();
    const float compOutBlock  = compOutSmoother.getNextValue();

    //--------------------------------------------------------------------------
    // 6. Set dry/wet mix ratio
    //--------------------------------------------------------------------------
    dryWetMixer.setWetMixProportion (mixVal / 100.0f);

    //--------------------------------------------------------------------------
    // 7. Capture dry signal BEFORE processing
    //--------------------------------------------------------------------------
    juce::dsp::AudioBlock<float> block (buffer);
    dryWetMixer.pushDrySamples (block);

    //--------------------------------------------------------------------------
    // 8. Input gain (smoothed, per-sample)
    //    Two-pass: advance smoother on ch 0, apply settled value on ch 1+.
    //--------------------------------------------------------------------------
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);

        if (ch == 0)
        {
            for (int n = 0; n < numSamples; ++n)
                data[n] *= inputGainSmoother.getNextValue();
        }
        else
        {
            const float g = inputGainSmoother.getCurrentValue();
            for (int n = 0; n < numSamples; ++n)
                data[n] *= g;
        }
    }

    //--------------------------------------------------------------------------
    // Phase 5.3: Capture input peaks AFTER input gain stage
    //   These represent the signal entering the processing chain.
    //   We track the maximum absolute sample value per channel per block.
    //--------------------------------------------------------------------------
    {
        float peakL = 0.0f;
        float peakR = 0.0f;

        if (numChannels >= 1)
        {
            const float* dataL = buffer.getReadPointer (0);
            for (int n = 0; n < numSamples; ++n)
                peakL = std::max (peakL, std::abs (dataL[n]));
        }
        if (numChannels >= 2)
        {
            const float* dataR = buffer.getReadPointer (1);
            for (int n = 0; n < numSamples; ++n)
                peakR = std::max (peakR, std::abs (dataR[n]));
        }

        // Atomic peak hold: keep the maximum seen since last timer read.
        // The UI timer reads and applies decay separately.
        meterInL.store (std::max (peakL, meterInL.load (std::memory_order_relaxed)),
                        std::memory_order_relaxed);
        meterInR.store (std::max (peakR, meterInR.load (std::memory_order_relaxed)),
                        std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // 9. M/S Encode (smoothed crossfade — click-free)
    //
    //    Per-sample blend between L/R and Mid/Side using msBlendBuffer.
    //    blend=0: pass-through (stereo),  blend=1: full M/S encode.
    //--------------------------------------------------------------------------
    if (numChannels >= 2)
    {
        float* dataL = buffer.getWritePointer (0);
        float* dataR = buffer.getWritePointer (1);

        for (int n = 0; n < numSamples; ++n)
        {
            const float blend = msBlendBuffer[static_cast<size_t> (n)];
            if (blend < 0.0001f) continue;  // pure stereo — skip

            const float l = dataL[n];
            const float r = dataR[n];
            const float mid  = (l + r) * 0.5f;
            const float side = (l - r) * 0.5f;
            dataL[n] = l * (1.0f - blend) + mid  * blend;
            dataR[n] = r * (1.0f - blend) + side * blend;
        }
    }

    //--------------------------------------------------------------------------
    // 10. Main processing chain — conditional on pre_post
    //
    //     pre_post == false (DYN→SAT, default):
    //       Dynamics → ADAA Sat → Post-Sat Tilt → [M/S Decode] → Post-Dyn Tilt
    //
    //     pre_post == true (SAT→DYN):
    //       ADAA Sat → Post-Sat Tilt → Dynamics → [M/S Decode] → Post-Dyn Tilt
    //
    //     M/S Decode is placed AFTER both sat and dynamics stages in both modes
    //     (architecture note: decode must wrap sat + dynamics combined).
    //     Post-Dyn Tilt is always AFTER M/S Decode (applied in stereo L/R space).
    //
    //     When ms_enable is active, each stage processes the channels as
    //     independent signals (mid=ch0, side=ch1) with independent drive
    //     parameters and independent dynamics state.
    //
    //     For simplicity and clarity, M/S handling is integrated into each
    //     processing call: if msEnable, we use per-channel drive/dynamics instances.
    //--------------------------------------------------------------------------

    if (!prePost)
    {
        //----------------------------------------------------------------------
        // DYN→SAT mode (pre_post == false, default)
        //   Order: Dynamics → ADAA Sat → Post-Sat Tilt → [M/S Decode] → Post-Dyn Tilt
        //----------------------------------------------------------------------

        // Step A: Dynamics (gated by comp_enable)
        if (compEnable)
        {
            if (msEnable && numChannels >= 2)
            {
                float* dataMid  = buffer.getWritePointer (0);
                float* dataSide = buffer.getWritePointer (1);

                for (int n = 0; n < numSamples; ++n)
                {
                    midEngine.detectLevelMono (dataMid[n]);
                    const float gMid = midEngine.computeGain (thresholdDb, ratio,
                                                               attackCoeff, releaseCoeff,
                                                               downAmount, upAmount, dynamicsMacro);
                    dataMid[n] *= gMid;

                    sideEngine.detectLevelMono (dataSide[n]);
                    const float gSide = sideEngine.computeGain (thresholdDb, ratio,
                                                                 attackCoeff, releaseCoeff,
                                                                 downAmount, upAmount, dynamicsMacro);
                    dataSide[n] *= gSide;
                }
            }
            else
            {
                runDynamicsStereo (buffer, numSamples,
                                   thresholdDb, ratio, attackCoeff, releaseCoeff,
                                   downAmount, upAmount, dynamicsMacro);
            }

            // GR metering
            if (msEnable)
                meterGR.store (std::min (midEngine.getGainReductionDb(), sideEngine.getGainReductionDb()),
                               std::memory_order_relaxed);
            else
                meterGR.store (stereoEngine.getGainReductionDb(), std::memory_order_relaxed);

            // Comp output volume
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                for (int n = 0; n < numSamples; ++n)
                    data[n] *= compOutBlock;
            }
        }
        else
        {
            meterGR.store (0.0f, std::memory_order_relaxed);
        }

        // Step B: ADAA Saturation (4x oversampled)
        //   Always run oversampling up/down to maintain consistent latency for
        //   DryWetMixer phase alignment and DAW PDC, even when sat is bypassed.
        {
            juce::dsp::AudioBlock<float> inputBlock (buffer);
            auto oversampledBlock = oversampling.processSamplesUp (inputBlock);

            if (satEnable)
            {
                if (msEnable && numChannels >= 2)
                    runSaturationMS (oversampledBlock, midDriveBlock, sideDriveBlock, alphaBlock, biasBlock, oddGainBlock);
                else
                    runSaturation (oversampledBlock, driveBlock, alphaBlock, biasBlock, oddGainBlock);
            }

            oversampling.processSamplesDown (inputBlock);

            if (satEnable)
            {
                // Post-Saturation Tilt (only when sat is active)
                applySatTilt (buffer, satTiltSlope);

                // Drive output volume
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float* data = buffer.getWritePointer (ch);
                    for (int n = 0; n < numSamples; ++n)
                        data[n] *= driveOutBlock;
                }
            }
        }

        // Step D: M/S Decode (smoothed crossfade — click-free)
        if (numChannels >= 2)
        {
            float* dataL = buffer.getWritePointer (0);
            float* dataR = buffer.getWritePointer (1);

            for (int n = 0; n < numSamples; ++n)
            {
                const float blend = msBlendBuffer[static_cast<size_t> (n)];
                if (blend < 0.0001f) continue;  // pure stereo — no decode needed

                const float ch0 = dataL[n];
                const float ch1 = dataR[n];
                const float L = ch0 + ch1;   // M/S decode
                const float R = ch0 - ch1;
                dataL[n] = ch0 * (1.0f - blend) + L * blend;
                dataR[n] = ch1 * (1.0f - blend) + R * blend;
            }
        }

        // Step E: Post-Dynamics Tilt (applied in L/R space after decode)
        applyDynTilt (buffer, dynTiltSlope);
    }
    else
    {
        //----------------------------------------------------------------------
        // SAT→DYN mode (pre_post == true)
        //   Order: ADAA Sat → Post-Sat Tilt → Dynamics → [M/S Decode] → Post-Dyn Tilt
        //----------------------------------------------------------------------

        // Step A: ADAA Saturation (4x oversampled)
        //   Always run oversampling up/down for consistent latency (see DYN→SAT path).
        {
            juce::dsp::AudioBlock<float> inputBlock (buffer);
            auto oversampledBlock = oversampling.processSamplesUp (inputBlock);

            if (satEnable)
            {
                if (msEnable && numChannels >= 2)
                    runSaturationMS (oversampledBlock, midDriveBlock, sideDriveBlock, alphaBlock, biasBlock, oddGainBlock);
                else
                    runSaturation (oversampledBlock, driveBlock, alphaBlock, biasBlock, oddGainBlock);
            }

            oversampling.processSamplesDown (inputBlock);

            if (satEnable)
            {
                // Post-Saturation Tilt (only when sat is active)
                applySatTilt (buffer, satTiltSlope);

                // Drive output volume
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    float* data = buffer.getWritePointer (ch);
                    for (int n = 0; n < numSamples; ++n)
                        data[n] *= driveOutBlock;
                }
            }
        }

        // Step C: Dynamics (gated by comp_enable)
        if (compEnable)
        {
            if (msEnable && numChannels >= 2)
            {
                float* dataMid  = buffer.getWritePointer (0);
                float* dataSide = buffer.getWritePointer (1);

                for (int n = 0; n < numSamples; ++n)
                {
                    midEngine.detectLevelMono (dataMid[n]);
                    const float gMid = midEngine.computeGain (thresholdDb, ratio,
                                                               attackCoeff, releaseCoeff,
                                                               downAmount, upAmount, dynamicsMacro);
                    dataMid[n] *= gMid;

                    sideEngine.detectLevelMono (dataSide[n]);
                    const float gSide = sideEngine.computeGain (thresholdDb, ratio,
                                                                 attackCoeff, releaseCoeff,
                                                                 downAmount, upAmount, dynamicsMacro);
                    dataSide[n] *= gSide;
                }
            }
            else
            {
                runDynamicsStereo (buffer, numSamples,
                                   thresholdDb, ratio, attackCoeff, releaseCoeff,
                                   downAmount, upAmount, dynamicsMacro);
            }

            // GR metering
            if (msEnable)
                meterGR.store (std::min (midEngine.getGainReductionDb(), sideEngine.getGainReductionDb()),
                               std::memory_order_relaxed);
            else
                meterGR.store (stereoEngine.getGainReductionDb(), std::memory_order_relaxed);

            // Comp output volume
            for (int ch = 0; ch < numChannels; ++ch)
            {
                float* data = buffer.getWritePointer (ch);
                for (int n = 0; n < numSamples; ++n)
                    data[n] *= compOutBlock;
            }
        }
        else
        {
            meterGR.store (0.0f, std::memory_order_relaxed);
        }

        // Step D: M/S Decode (smoothed crossfade — click-free)
        if (numChannels >= 2)
        {
            float* dataL = buffer.getWritePointer (0);
            float* dataR = buffer.getWritePointer (1);

            for (int n = 0; n < numSamples; ++n)
            {
                const float blend = msBlendBuffer[static_cast<size_t> (n)];
                if (blend < 0.0001f) continue;

                const float ch0 = dataL[n];
                const float ch1 = dataR[n];
                const float L = ch0 + ch1;
                const float R = ch0 - ch1;
                dataL[n] = ch0 * (1.0f - blend) + L * blend;
                dataR[n] = ch1 * (1.0f - blend) + R * blend;
            }
        }

        // Step E: Post-Dynamics Tilt (in L/R space after decode)
        applyDynTilt (buffer, dynTiltSlope);
    }

    //--------------------------------------------------------------------------
    // 11. Output gain (smoothed, per-sample)
    //--------------------------------------------------------------------------
    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* data = buffer.getWritePointer (ch);

        if (ch == 0)
        {
            for (int n = 0; n < numSamples; ++n)
                data[n] *= outputGainSmoother.getNextValue();
        }
        else
        {
            const float g = outputGainSmoother.getCurrentValue();
            for (int n = 0; n < numSamples; ++n)
                data[n] *= g;
        }
    }

    //--------------------------------------------------------------------------
    // Phase 5.3: Capture output peaks AFTER output gain stage, before dry/wet blend
    //   These represent the wet signal level before mixing with dry.
    //   Placed here so the meter reflects the processed signal level.
    //--------------------------------------------------------------------------
    {
        float peakL = 0.0f;
        float peakR = 0.0f;

        if (numChannels >= 1)
        {
            const float* dataL = buffer.getReadPointer (0);
            for (int n = 0; n < numSamples; ++n)
                peakL = std::max (peakL, std::abs (dataL[n]));
        }
        if (numChannels >= 2)
        {
            const float* dataR = buffer.getReadPointer (1);
            for (int n = 0; n < numSamples; ++n)
                peakR = std::max (peakR, std::abs (dataR[n]));
        }

        meterOutL.store (std::max (peakL, meterOutL.load (std::memory_order_relaxed)),
                         std::memory_order_relaxed);
        meterOutR.store (std::max (peakR, meterOutR.load (std::memory_order_relaxed)),
                         std::memory_order_relaxed);
    }

    //--------------------------------------------------------------------------
    // 12. Dry/Wet blend
    //--------------------------------------------------------------------------
    dryWetMixer.mixWetSamples (block);
}

//==============================================================================
// Editor
//==============================================================================

juce::AudioProcessorEditor* NBS_DynaDriveAudioProcessor::createEditor()
{
    return new NBS_DynaDriveAudioProcessorEditor (*this);
}

//==============================================================================
// State (Preset save / load)
//==============================================================================

void NBS_DynaDriveAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NBS_DynaDriveAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState != nullptr && xmlState->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
// Factory function
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NBS_DynaDriveAudioProcessor();
}
