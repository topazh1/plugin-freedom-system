#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * MixMentorAudioProcessorEditor
 *
 * Placeholder GUI — three interactive controls plus a live analysis display.
 *
 * Layout (480 x 420 px)
 * ─────────────────────
 *  [Header — "MixMentor"]
 *  Genre:  [Pop ▾]
 *  ─────────────────────────────────
 *  Analysis
 *  Dynamics  ████████░░  -14 LUFS
 *  Tonality  ██████░░░░  1.2 kHz
 *  Stereo    ████░░░░░░  45 %
 *  ─────────────────────────────────
 *  Feedback
 *  ┌─────────────────────────────┐
 *  │ (response from mentor)      │
 *  └─────────────────────────────┘
 *  [Send to SNIPAI.co]
 *
 * The three analysis meters are redrawn by a 30 Hz timer.
 */
class MixMentorAudioProcessorEditor : public juce::AudioProcessorEditor,
                                       public juce::Timer
{
public:
    explicit MixMentorAudioProcessorEditor (MixMentorAudioProcessor&);
    ~MixMentorAudioProcessorEditor() override;

    void paint   (juce::Graphics&) override;
    void resized ()                override;

    /** Called at ~30 Hz to poll the processor and refresh the meter display. */
    void timerCallback() override;

private:
    // -------------------------------------------------------------------------
    // Controls
    // -------------------------------------------------------------------------
    juce::ComboBox   genreSelector;
    juce::TextEditor feedbackText;
    juce::TextButton sendButton { "Send to SNIPAI.co" };

    // APVTS attachment — syncs genreSelector <-> "genre" parameter
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> genreAttachment;

    // -------------------------------------------------------------------------
    // Analysis display values (updated by timerCallback, read by paint)
    // -------------------------------------------------------------------------
    float displayRmsDb           = -60.0f;
    float displayLufs            = -60.0f;
    float displaySpectralCentroid = 1000.0f;
    float displayStereoWidth      = 0.0f;

    // -------------------------------------------------------------------------
    MixMentorAudioProcessor& processorRef;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixMentorAudioProcessorEditor)
};
