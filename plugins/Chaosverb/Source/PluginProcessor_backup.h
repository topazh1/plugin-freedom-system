#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>

#include "ChaosverbFDN.h"

//==============================================================================
/**
 * ChaosverbAudioProcessor
 *
 * Algorithmic reverb using an FDN (Feedback Delay Network) with impossible
 * topology. Features a mutation system that randomizes unlocked parameters
 * on a user-defined interval. Dual FDN instances support crossfade between
 * current and new reverb states.
 *
 * Plugin type: Audio Effect (stereo in / stereo out)
 * Parameters: 31 (16 Float + 15 Bool)
 *
 * Phase 4.3: Dual FDN + Crossfade System.
 * - FDN-A and FDN-B run continuously (both always process audio)
 * - Pre-delay, allpass diffuser, DryWetMixer, and stereo width are shared
 *   at PluginProcessor level (moved out of ChaosverbFDN)
 * - Equal-power crossfader blends FDN-A and FDN-B wet outputs
 * - mutationPending atomic flag: set by message thread, cleared by audio thread
 * - triggerCrossfade() public method: used by UI and mutation timer
 *
 * Phase 4.4: Mutation Timer + Lock System.
 * - MutationTimer fires every mutationInterval seconds on the message thread
 * - triggerMutation() reads 10 lock params, randomizes unlocked params, triggers crossfade
 * - getRemainingTimeMs() for UI countdown display
 */
class ChaosverbAudioProcessor : public juce::AudioProcessor
{
public:
    //==========================================================================
    ChaosverbAudioProcessor();
    ~ChaosverbAudioProcessor() override;

    //==========================================================================
    // AudioProcessor overrides
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==========================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==========================================================================
    const juce::String getName() const override { return "Chaosverb"; }
    bool acceptsMidi() const override  { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    // Report tail length as max decay (60s) so DAW knows to keep plugin alive
    double getTailLengthSeconds() const override { return 60.0; }

    //==========================================================================
    int getNumPrograms() override    { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==========================================================================
    // State management (DAW preset save/load)
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==========================================================================
    /**
     * Trigger a crossfade from the current active FDN to the other instance.
     *
     * Safe to call from any thread (message thread, UI, mutation timer).
     * Sets mutationPending = true. processBlock() reads and clears this flag
     * at the start of each block. If a crossfade is already in progress,
     * the pending flag is ignored until it completes.
     */
    void triggerCrossfade()
    {
        mutationPending.store(true, std::memory_order_release);
    }

    //==========================================================================
    /**
     * Trigger a full mutation cycle immediately.
     *
     * Must be called from the message thread only (timer callback, UI button).
     * Reads all 10 lock boolean parameters. For each unlocked parameter, generates
     * a uniform random normalized value (0..1) and writes it to APVTS via
     * setValueNotifyingHost(). Then calls triggerCrossfade() to blend to new state.
     *
     * Guard: if mutationPending is already true (previous crossfade not yet started),
     * skips the mutation to prevent overlapping state changes.
     */
    void triggerMutation();

    //==========================================================================
    /**
     * Returns the remaining time in milliseconds until the next mutation fires.
     *
     * Computed as: mutationInterval * 1000 - (now - lastMutationTimeMs)
     * Clamped to [0, mutationInterval * 1000].
     * Returns 0.0 if the timer has not been started yet.
     *
     * Safe to call from any thread (message or UI). lastMutationTimeMs is
     * written only from the message thread (timer callback + prepareToPlay).
     */
    double getRemainingTimeMs() const;

    //==========================================================================
    /**
     * Stop or start the automatic mutation timer.
     * When stopped, no automatic mutations fire. Manual "Mutate Now" still works.
     * Safe to call from the message thread (UI button handler).
     */
    void setMutationTimerRunning(bool running);
    bool isMutationTimerRunning() const;

    //==========================================================================
    // APVTS — public so PluginEditor can access for parameter attachments
    juce::AudioProcessorValueTreeState parameters;

    //==========================================================================
    // Crossfade state machine — values accessed from audio thread only (except
    // mutationPending which is accessed from any thread via atomic).

    enum class CrossfadeState { Idle, Ramping, Complete };

    // Set to true by message thread, cleared by audio thread when crossfade starts.
    // std::memory_order_release on store, acquire on load — no mutex needed.
    std::atomic<bool> mutationPending { false };

private:
    //==========================================================================
    // Phase 4.4 — Mutation timer system

    /**
     * MutationTimer
     *
     * juce::Timer subclass owned by PluginProcessor (as a member, NOT via
     * inheritance). Fires on the JUCE message thread at the current
     * mutationInterval. On each tick, calls processor.triggerMutation().
     *
     * Uses juce::Timer (not HighResolutionTimer) — message thread is fine for
     * 5–600 second intervals. Timer is started in prepareToPlay() and stopped
     * in the PluginProcessor destructor.
     */
    class MutationTimer : public juce::Timer
    {
    public:
        explicit MutationTimer(ChaosverbAudioProcessor& p) : processor(p) {}

        void timerCallback() override
        {
            // Polling approach: check elapsed time against CURRENT interval value.
            // This ensures interval slider changes take effect immediately,
            // not just on the next cycle.
            const float intervalSeconds = processor.parameters
                .getRawParameterValue("mutationInterval")->load();
            const double intervalMs = static_cast<double>(intervalSeconds) * 1000.0;
            const double now = juce::Time::getMillisecondCounterHiRes();
            const double elapsed = now - processor.lastMutationTimeMs;

            if (elapsed >= intervalMs)
                processor.triggerMutation();
        }

    private:
        ChaosverbAudioProcessor& processor;
    };

    MutationTimer mutationTimerObj { *this };

    // Whether the automatic mutation timer is running. Toggled by the UI start button.
    // Defaults to false — plugin loads with timer stopped (Start button visible).
    std::atomic<bool> mutationTimerRunning_ { false };

    // Timestamp of the last mutation (or prepareToPlay). Used for getRemainingTimeMs().
    // Written only from message thread (prepareToPlay + timer callback).
    // Read from message/UI thread via getRemainingTimeMs().
    double lastMutationTimeMs { 0.0 };

    //==========================================================================
    // Phase 4.3 DSP — Shared pre-processing stage (moved out of ChaosverbFDN)
    // These components are shared: both FDN-A and FDN-B receive the same
    // pre-delayed, diffused input signal.

    // Pre-delay line (bipolar: -100 BPM-sync to +500ms free, interpolated for smooth sweeps)
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayLine;

    // Allpass diffuser — 4 Schroeder stages with prime delays (scaled at prepare time)
    // Stage delays at 48kHz: 347, 557, 743, 1013 samples
    static constexpr int kNumDiffuserStages = 4;
    static constexpr int kAllpassLengths48k[kNumDiffuserStages] = { 347, 557, 743, 1013 };
    static constexpr float kAllpassCoeff = 0.7f;

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> diffLine0;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> diffLine1;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> diffLine2;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> diffLine3;

    int allpassDelayLengths[kNumDiffuserStages] = {};

    // Dry/wet mixer — applied after crossfader blends A and B wet outputs
    juce::dsp::DryWetMixer<float> dryWetMixer;

    //==========================================================================
    // Output EQ — applied to wet signal after crossfade/width, before dry/wet mix
    // Low cut (highpass), high cut (lowpass), tilt (shelf pair)
    juce::dsp::IIR::Filter<float> lowCutL, lowCutR;
    juce::dsp::IIR::Filter<float> highCutL, highCutR;
    juce::dsp::IIR::Filter<float> tiltLowL, tiltLowR;
    juce::dsp::IIR::Filter<float> tiltHighL, tiltHighR;

    float cachedLowCut  = -1.0f;
    float cachedHighCut = -1.0f;
    float cachedTilt    = -999.0f;

    // Haas delay — mono delay applied to R channel wet signal for broadband width.
    // Adds temporal decorrelation that complements the FDN's spectral decorrelation.
    // Delay time scales with width parameter: 0% = 0ms, 300% = 12ms.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> haasDelayLine;

    //==========================================================================
    // Wow & Flutter — modulated pitch shift applied to wet signal after EQ.
    // Uses two LFOs per channel (wow=slow+deep, flutter=fast+shallow) for
    // tape-like pitch instability. Entirely bypassed when disabled.
    // Separate L/R delay lines ensure full stereo independence (no mono collapse).
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wfDelayLineL;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> wfDelayLineR;
    float wfWowPhaseL     = 0.0f;
    float wfWowPhaseR     = 1.5707963f; // pi/2 offset for stereo decorrelation
    float wfFlutterPhaseL = 0.0f;
    float wfFlutterPhaseR = 1.5707963f;

    //==========================================================================
    // Ducking — envelope follower on dry input, applied as gain reduction to wet.
    // Attack: ~1ms (catches transients), Release: ~150ms (smooth return).
    float duckEnvelope      = 0.0f;
    float duckAttackCoeff   = 0.0f;
    float duckReleaseCoeff  = 0.0f;

    //==========================================================================
    // Dual FDN instances — both always run (no idle optimization)
    // FDN-A is the "active" instance (full gain) at startup.
    // When crossfade completes, roles swap: B becomes active, A becomes idle.
    ChaosverbFDN fdnA;
    ChaosverbFDN fdnB;

    //==========================================================================
    // Crossfade state machine — audio thread only
    CrossfadeState xfadeState   = CrossfadeState::Idle;

    // crossfadePhase ramps 0.0 -> 1.0 during Ramping state.
    // gainA = cos(phase * PI/2), gainB = sin(phase * PI/2)
    float crossfadePhase        = 0.0f;

    // Phase increment per sample: 1.0 / (crossfadeSpeed_ms * sampleRate / 1000)
    float crossfadePhaseInc     = 0.0f;

    // fdnAIsActive: when true, A is the current active FDN (gainA=1, gainB=0 at Idle).
    // After crossfade completes, this flips: B becomes active.
    bool fdnAIsActive           = true;

    //==========================================================================
    // Cached sample rate (set in prepareToPlay)
    double currentSampleRate    = 48000.0;

    //==========================================================================
    // Helper: process one sample through the 4-stage Schroeder allpass diffuser.
    // Only numActiveStages are applied (0-4), controlled by density parameter.
    float processDiffuserSample(float input, int channel, int numActiveStages);

    //==========================================================================
    // Helper: get diffuser delay line by index
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>& getDiffLine(int i)
    {
        switch (i)
        {
            case 0: return diffLine0;
            case 1: return diffLine1;
            case 2: return diffLine2;
            default: return diffLine3;
        }
    }

    //==========================================================================
    // Creates the full 22-parameter APVTS layout
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    //==========================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChaosverbAudioProcessor)
};
