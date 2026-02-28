#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "ADAASaturator.h"
#include "DynamicsEngine.h"

class NBS_DynaDriveAudioProcessor : public juce::AudioProcessor
{
public:
    NBS_DynaDriveAudioProcessor();
    ~NBS_DynaDriveAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "NBS DynaDrive"; }

    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // APVTS — public so PluginEditor can access parameters
    juce::AudioProcessorValueTreeState parameters;

    //--------------------------------------------------------------------------
    // Phase 5.3: Level meter peaks (written by audio thread, read by UI timer)
    //   One atomic float per channel — lock-free, no blocking on audio thread.
    //   Values are linear peak amplitude (0.0 to ~1.0+).
    //   PluginEditor timer reads and decays these at ~30 Hz.
    //--------------------------------------------------------------------------
    std::atomic<float> meterInL  { 0.0f };
    std::atomic<float> meterInR  { 0.0f };
    std::atomic<float> meterOutL { 0.0f };
    std::atomic<float> meterOutR { 0.0f };

    // v1.3.0: Gain reduction meter (dB, negative = gain reduction, 0 = no compression)
    std::atomic<float> meterGR   { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //--------------------------------------------------------------------------
    // DSP Components — declared BEFORE parameters (JUCE initialisation order)
    //--------------------------------------------------------------------------

    // 4x oversampling for anti-aliased saturation (FIR equiripple — steepest rolloff)
    juce::dsp::Oversampling<float> oversampling { 2, 2, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true };

    // Phase 4.1: Input gain stage
    juce::dsp::Gain<float> inputGain;

    // Phase 4.1 + 4.2: ADAA waveshaping engine
    //   Handles h_curve blend, even/odd harmonic pre-distortion, DC blocking.
    //   In M/S mode, channel 0 = mid, channel 1 = side (independent state).
    ADAASaturator adaaSaturator;

    // Phase 4.2: Post-saturation tilt filter
    //   Tilt EQ: symmetric low-shelf + high-shelf pair sharing a pivot frequency.
    //   ProcessorDuplicator applies the same mono IIR to every stereo channel.
    using TiltFilter = juce::dsp::ProcessorDuplicator<
                           juce::dsp::IIR::Filter<float>,
                           juce::dsp::IIR::Coefficients<float>>;
    TiltFilter satTiltLowShelf;
    TiltFilter satTiltHighShelf;

    // Phase 4.2: Post-dynamics tilt filter (same architecture, different params)
    TiltFilter dynTiltLowShelf;
    TiltFilter dynTiltHighShelf;

    // Phase 4.3: Dynamics Engine
    //   stereoEngine  — linked stereo (detects max L/R) used in normal mode
    //   midEngine     — independent mid channel dynamics in M/S mode
    //   sideEngine    — independent side channel dynamics in M/S mode
    DynamicsEngine stereoEngine;
    DynamicsEngine midEngine;
    DynamicsEngine sideEngine;

    // Phase 4.1: Output gain stage
    juce::dsp::Gain<float> outputGain;

    // Phase 4.1: Dry/Wet mixer
    juce::dsp::DryWetMixer<float> dryWetMixer;

    // ProcessSpec shared across all JUCE DSP components
    juce::dsp::ProcessSpec processSpec;

    //--------------------------------------------------------------------------
    // Parameter smoothers (5 ms ramp — prevent zipper noise on knob changes)
    //--------------------------------------------------------------------------

    // Phase 4.1 smoothers
    juce::LinearSmoothedValue<float> inputGainSmoother;
    juce::LinearSmoothedValue<float> outputGainSmoother;
    juce::LinearSmoothedValue<float> driveOutSmoother;
    juce::LinearSmoothedValue<float> compOutSmoother;
    juce::LinearSmoothedValue<float> driveSmoother;

    // Phase 4.2 smoothers
    juce::LinearSmoothedValue<float> alphaSmoother;   // h_curve  (0–1)
    juce::LinearSmoothedValue<float> biasSmoother;    // even     (0–0.15)
    juce::LinearSmoothedValue<float> oddSmoother;     // odd      (0–0.05)

    // Phase 4.3: M/S per-channel drive smoothers
    juce::LinearSmoothedValue<float> midDriveSmoother;
    juce::LinearSmoothedValue<float> sideDriveSmoother;

    // M/S crossfade smoother (0=stereo, 1=M/S) — prevents click on toggle
    juce::LinearSmoothedValue<float> msSmoother;
    std::vector<float> msBlendBuffer;  // pre-allocated in prepareToPlay

    //--------------------------------------------------------------------------
    // Tilt filter cached state — avoid recomputing coefficients every block
    //--------------------------------------------------------------------------

    float cachedSatTiltFreq  = 1000.0f;
    float cachedSatTiltSlope = 0.0f;

    float cachedDynTiltFreq  = 1000.0f;
    float cachedDynTiltSlope = 0.0f;

    // Helper: recompute and apply tilt filter coefficients when params change
    void updateSatTiltCoefficients  (float freq, float slopeDb);
    void updateDynTiltCoefficients  (float freq, float slopeDb);

    //--------------------------------------------------------------------------
    // Phase 4.3: Helper methods for processing blocks
    //--------------------------------------------------------------------------

    // Run ADAA saturation on an AudioBlock (normal stereo mode)
    void runSaturation (juce::dsp::AudioBlock<float>& block,
                        float driveGain, float alpha,
                        float bias, float oddGain) noexcept;

    // Run ADAA saturation on an AudioBlock in M/S mode (per-channel drive)
    void runSaturationMS (juce::dsp::AudioBlock<float>& block,
                          float midDrive, float sideDrive,
                          float alpha, float bias, float oddGain) noexcept;

    // Apply the dynamics engine to the stereo buffer (normal mode, linked detection)
    void runDynamicsStereo (juce::AudioBuffer<float>& buf,
                            int numSamples,
                            float thresholdDb, float ratio,
                            float attackCoeff, float releaseCoeff,
                            float downAmount, float upAmount,
                            float dynamicsMacro) noexcept;

    // Apply sat tilt filter to buffer (block-level, no-op when slope==0)
    void applySatTilt  (juce::AudioBuffer<float>& buf, float slopeDb) noexcept;

    // Apply dyn tilt filter to buffer (block-level, no-op when slope==0)
    void applyDynTilt  (juce::AudioBuffer<float>& buf, float slopeDb) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NBS_DynaDriveAudioProcessor)
};
