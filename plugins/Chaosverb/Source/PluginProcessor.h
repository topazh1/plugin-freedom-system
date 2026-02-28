#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include <atomic>

#include "ChaosverbEQ.h"
#include "ChaosverbFDN.h"
#include "ChaosverbWowFlutter.h"

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
 * - triggerMutation() reads 10 lock params, randomizes unlocked params,
 * triggers crossfade
 * - getRemainingTimeMs() for UI countdown display
 */
class ChaosverbAudioProcessor : public juce::AudioProcessor {
public:
  //==========================================================================
  ChaosverbAudioProcessor();
  ~ChaosverbAudioProcessor() override;

  //==========================================================================
  // AudioProcessor overrides
  void prepareToPlay(double sampleRate, int samplesPerBlock) override;
  void releaseResources() override;
  void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

  //==========================================================================
  juce::AudioProcessorEditor *createEditor() override;
  bool hasEditor() const override { return true; }

  //==========================================================================
  const juce::String getName() const override { return "Chaosverb"; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  bool isMidiEffect() const override { return false; }

  // Report tail length as max decay (60s) so DAW knows to keep plugin alive
  double getTailLengthSeconds() const override { return 60.0; }

  //==========================================================================
  int getNumPrograms() override { return 1; }
  int getCurrentProgram() override { return 0; }
  void setCurrentProgram(int) override {}
  const juce::String getProgramName(int) override { return {}; }
  void changeProgramName(int, const juce::String &) override {}

  //==========================================================================
  // State management (DAW preset save/load)
  void getStateInformation(juce::MemoryBlock &destData) override;
  void setStateInformation(const void *data, int sizeInBytes) override;

  //==========================================================================
  /**
   * Trigger a crossfade from the current active FDN to the other instance.
   *
   * Safe to call from any thread (message thread, UI, mutation timer).
   * Sets mutationPending = true. processBlock() reads and clears this flag
   * at the start of each block. If a crossfade is already in progress,
   * the pending flag is ignored until it completes.
   */
  void triggerCrossfade() {
    mutationPending.store(true, std::memory_order_release);
  }

  //==========================================================================
  /**
   * Trigger a full mutation cycle immediately.
   *
   * Must be called from the message thread only (timer callback, UI button).
   * Reads all 10 lock boolean parameters. For each unlocked parameter,
   * generates a uniform random normalized value (0..1) and writes it to APVTS
   * via setValueNotifyingHost(). Then calls triggerCrossfade() to blend to new
   * state.
   *
   * Guard: if mutationPending is already true (previous crossfade not yet
   * started), skips the mutation to prevent overlapping state changes.
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
  // Cached Parameter Pointers (Optimization)
  std::atomic<float> *topologyParam = nullptr;
  std::atomic<float> *decayParam = nullptr;
  std::atomic<float> *preDelayParam = nullptr;
  std::atomic<float> *densityParam = nullptr;
  std::atomic<float> *spectralTiltParam = nullptr;
  std::atomic<float> *resonanceParam = nullptr;
  std::atomic<float> *modRateParam = nullptr;
  std::atomic<float> *modDepthParam = nullptr;
  std::atomic<float> *widthParam = nullptr;
  std::atomic<float> *mixParam = nullptr;
  std::atomic<float> *crossfadeSpeedParam = nullptr;
  std::atomic<float> *lowCutParam = nullptr;
  std::atomic<float> *highCutParam = nullptr;
  std::atomic<float> *tiltParam = nullptr;
  std::atomic<float> *wfAmountParam = nullptr;
  std::atomic<float> *wfEnabledParam = nullptr;
  std::atomic<float> *outputLevelParam = nullptr;
  std::atomic<float> *duckingAmountParam = nullptr;
  std::atomic<float> *bypassParam = nullptr;

  //==========================================================================
  // Crossfade state machine — values accessed from audio thread only (except
  // mutationPending which is accessed from any thread via atomic).

  enum class CrossfadeState { Idle, Ramping, Complete };

  // Set to true by message thread, cleared by audio thread when crossfade
  // starts. std::memory_order_release on store, acquire on load — no mutex
  // needed.
  std::atomic<bool> mutationPending{false};

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
  class MutationTimer : public juce::Timer {
  public:
    explicit MutationTimer(ChaosverbAudioProcessor &p) : processor(p) {}

    void timerCallback() override {
      // Polling approach: check elapsed time against CURRENT interval value.
      // computeMutationIntervalMs() handles bipolar interpretation:
      // positive = absolute ms, negative = BPM-synced note divisions.
      const double intervalMs = processor.computeMutationIntervalMs();
      const double now = juce::Time::getMillisecondCounterHiRes();
      const double elapsed = now - processor.lastMutationTimeMs;

      if (elapsed >= intervalMs)
        processor.triggerMutation();
    }

  private:
    ChaosverbAudioProcessor &processor;
  };

  MutationTimer mutationTimerObj{*this};

  // Whether the automatic mutation timer is running. Toggled by the UI start
  // button. Defaults to false — plugin loads with timer stopped (Start button
  // visible).
  std::atomic<bool> mutationTimerRunning_{false};

  // Host BPM — updated from processBlock via AudioPlayHead, read by timer
  // callback. Fallback: 120 BPM when host doesn't report tempo.
  std::atomic<float> hostBPM{120.0f};

  // Compute the effective mutation interval in milliseconds from the bipolar
  // mutationInterval parameter. Positive = absolute ms, negative = BPM-synced
  // note divisions. Returns minimum 50ms to prevent runaway mutations.
  double computeMutationIntervalMs() const;

  // Timestamp of the last mutation (or prepareToPlay). Used for
  // getRemainingTimeMs(). Written only from message thread (prepareToPlay +
  // timer callback). Read from message/UI thread via getRemainingTimeMs().
  double lastMutationTimeMs{0.0};

  //==========================================================================
  // Glide system — smooth parameter transitions during mutation.
  // Instead of snapping params instantly, glide toward targets over
  // crossfadeSpeed duration on the message thread (~60Hz timer).

  static constexpr int kMaxGlideParams = 16;

  struct ParamGlideTarget {
    juce::RangedAudioParameter *param = nullptr;
    float startNorm = 0.0f;
    float targetNorm = 0.0f;
  };

  std::array<ParamGlideTarget, kMaxGlideParams> glideTargets{};
  int numActiveGlides = 0;
  float glidePhase = 0.0f;
  float glidePhaseInc = 0.0f;

  class GlideTimer : public juce::Timer {
  public:
    explicit GlideTimer(ChaosverbAudioProcessor &p) : processor(p) {}
    void timerCallback() override;

  private:
    ChaosverbAudioProcessor &processor;
  };

  GlideTimer glideTimerObj{*this};

  //==========================================================================
  // Phase 4.3 DSP — Shared pre-processing stage (moved out of ChaosverbFDN)
  // These components are shared: both FDN-A and FDN-B receive the same
  // pre-delayed, diffused input signal.

  // Pre-delay line (bipolar: -100 BPM-sync to +500ms free, interpolated for
  // smooth sweeps)
  juce::dsp::DelayLine<float,
                       juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
      preDelayLine;

  // Allpass diffuser — 4 Schroeder stages with prime delays (scaled at prepare
  // time) Stage delays at 48kHz: 347, 557, 743, 1013 samples
  static constexpr int kNumDiffuserStages = 4;
  static constexpr int kAllpassLengths48k[kNumDiffuserStages] = {347, 557, 743,
                                                                 1013};
  static constexpr float kAllpassCoeff = 0.7f;

  juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None>
      diffLines[kNumDiffuserStages];

  // Dry/wet mixer — applied after crossfader blends A and B wet outputs
  juce::dsp::DryWetMixer<float> dryWetMixer;

  //==========================================================================
  // Output EQ — applied to wet signal after crossfade/width, before dry/wet mix
  nbs::ChaosverbEQ outputEQ;

  // Haas delay — mono delay applied to R channel wet signal for broadband
  // width. Adds temporal decorrelation that complements the FDN's spectral
  // decorrelation. Delay time scales with width parameter: 0% = 0ms, 300% =
  // 12ms.
  juce::dsp::DelayLine<float,
                       juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>
      haasDelayLine;

  // Per-sample smoothers for zipper-free parameter changes
  juce::SmoothedValue<float> widthSmoother;
  juce::SmoothedValue<float> haasDelaySmoother;

  //==========================================================================
  // Wow & Flutter — modulated pitch shift applied to wet signal after EQ.
  nbs::ChaosverbWowFlutter wowFlutter;

  //==========================================================================
  // Ducking — envelope follower on dry input, applied as gain reduction to wet.
  // Attack: ~10ms (catches transients), Release: ~250ms (smooth return).
  float duckEnvelope = 0.0f;
  float duckAttackCoeff = 0.0f;
  float duckReleaseCoeff = 0.0f;

  //==========================================================================
  // Per-FDN parameter snapshot for true crossfade between old/new reverb
  // states. Captures per-sample DSP params so outgoing FDN keeps old reverb
  // character while incoming FDN uses new mutated params during crossfade.
  struct FDNParamSnapshot {
    float feedbackGain = 0.0f;
    float topologyBlend = 0.65f;
    float modDepthSamples = 0.0f;
    float resoSmoothCoeff = 0.0f;
  };

  FDNParamSnapshot activeSnapshot;   // Tracks live params when idle
  FDNParamSnapshot outgoingSnapshot; // Frozen old params during crossfade

  //==========================================================================
  // Dual FDN instances — both always run (no idle optimization)
  // FDN-A is the "active" instance (full gain) at startup.
  // When crossfade completes, roles swap: B becomes active, A becomes idle.
  ChaosverbFDN fdnA;
  ChaosverbFDN fdnB;

  //==========================================================================
  // Crossfade state machine — audio thread only
  CrossfadeState xfadeState = CrossfadeState::Idle;

  // crossfadePhase ramps 0.0 -> 1.0 during Ramping state.
  // gainA = cos(phase * PI/2), gainB = sin(phase * PI/2)
  float crossfadePhase = 0.0f;

  // Phase increment per sample: 1.0 / (crossfadeSpeed_ms * sampleRate / 1000)
  float crossfadePhaseInc = 0.0f;

  // fdnAIsActive: when true, A is the current active FDN (gainA=1, gainB=0 at
  // Idle). After crossfade completes, this flips: B becomes active.
  bool fdnAIsActive = true;

  //==========================================================================
  // Cached sample rate (set in prepareToPlay)
  double currentSampleRate = 48000.0;

  //==========================================================================
  // Helper: process one sample through the 4-stage Schroeder allpass diffuser.
  // Only numActiveStages are applied (0-4), controlled by density parameter.
  float processDiffuserSample(float input, int channel, int numActiveStages);

  //==========================================================================
  // DSP Helper Functions
  void applyDucking(int numSamples, float *dataL, float *dataR,
                    float duckingNorm);
  void applyStereoWidth(int numSamples, float *dataL, float *dataR,
                        float widthGain);

  //==========================================================================
  // Creates the full 22-parameter APVTS layout
  static juce::AudioProcessorValueTreeState::ParameterLayout
  createParameterLayout();

  //==========================================================================
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChaosverbAudioProcessor)
};
