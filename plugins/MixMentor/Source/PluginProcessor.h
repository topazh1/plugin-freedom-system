#pragma once
#include <JuceHeader.h>

#include "analysis/DynamicsAnalyzer.h"
#include "analysis/TonalityAnalyzer.h"
#include "analysis/StereoAnalyzer.h"
#include "analysis/MixAnalysisResult.h"

/**
 * MixMentorAudioProcessor
 *
 * Master-bus analyser.  Audio passes through unmodified; each block is
 * forwarded to three lightweight analyser objects that update atomic state.
 * The editor polls getLatestAnalysis() via a juce::Timer to refresh the GUI.
 *
 * Parameters
 * ----------
 * genre  (AudioParameterChoice) — selects the comparison genre profile
 */
class MixMentorAudioProcessor : public juce::AudioProcessor
{
public:
    MixMentorAudioProcessor();
    ~MixMentorAudioProcessor() override;

    // -------------------------------------------------------------------------
    // AudioProcessor interface
    // -------------------------------------------------------------------------
    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MixMentor"; }

    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int          getNumPrograms()                             override { return 1; }
    int          getCurrentProgram()                          override { return 0; }
    void         setCurrentProgram (int)                      override {}
    const juce::String getProgramName (int)                   override { return {}; }
    void         changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData)            override;
    void setStateInformation (const void* data, int sizeInBytes)      override;

    // -------------------------------------------------------------------------
    // Public API for the editor
    // -------------------------------------------------------------------------

    /** Returns a snapshot of the latest analysis results.
     *  Safe to call from the message thread (reads atomics). */
    MixAnalysisResult getLatestAnalysis() const;

    /** APVTS — public so the editor can create parameter attachments. */
    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // -------------------------------------------------------------------------
    // Analysers (audio thread only)
    // -------------------------------------------------------------------------
    DynamicsAnalyzer dynamicsAnalyzer;
    TonalityAnalyzer tonalityAnalyzer;
    StereoAnalyzer   stereoAnalyzer;

    // -------------------------------------------------------------------------
    // Atomic snapshot (written audio thread, read message thread)
    // -------------------------------------------------------------------------
    std::atomic<float> latestRmsDb           { -60.0f };
    std::atomic<float> latestLufs            { -60.0f };
    std::atomic<float> latestPeakDb          { -60.0f };
    std::atomic<float> latestSpectralCentroid {1000.0f };
    std::atomic<float> latestSpectralFlatness {   0.5f };
    std::atomic<float> latestCorrelation      {   1.0f };
    std::atomic<float> latestStereoWidth      {   0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixMentorAudioProcessor)
};
