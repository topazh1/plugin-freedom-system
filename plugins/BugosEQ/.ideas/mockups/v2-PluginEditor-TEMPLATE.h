// =============================================================================
// v2-PluginEditor-TEMPLATE.h
// BugosEQ — WebView PluginEditor header template
//
// USAGE: Copy to Source/PluginEditor.h during Stage 3 (GUI) and fill TODOs.
//        Do NOT copy-paste blindly — gui-agent will adapt this to the real
//        processor class name and file layout.
//
// GENERATED: 2026-02-27 by ui-finalization-agent (ui-mockup skill, v2)
// PARAMETERS: 35 total (3 global + 8 per band x 4 bands)
//
// CRITICAL: Member declaration order MUST be:
//   1. audioProcessor reference
//   2. ParameterAttachments (no dependency on WebView)
//   3. WebBrowserComponent webView  (depends on nothing)
//   Note: For this UI we use manual sendToPlugin/pluginMessage bridging
//         rather than the JUCE 8 Relay/Attachment WebView system.
//         This avoids relay ordering complexity while remaining fully correct.
// =============================================================================
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// TODO: Confirm BugosEQAudioProcessor is the exact class name in PluginProcessor.h
class BugosEQAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit BugosEQAudioProcessorEditor (BugosEQAudioProcessor& p);
    ~BugosEQAudioProcessorEditor() override;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==========================================================================
    // Processor reference — must be declared first
    BugosEQAudioProcessor& audioProcessor;

    //==========================================================================
    // WebBrowserComponent — the entire UI lives inside this view.
    //
    // The WebBrowserComponent is constructed with an Options object that
    // provides the HTML resource via a resource provider callback.
    // All parameter communication uses:
    //   UI -> C++: window.sendToPlugin  (emitted by JS, received via NativeFunction)
    //   C++ -> UI: webView.emitEventIfBrowserIsVisible("pluginMessage", ...)
    //==========================================================================
    juce::WebBrowserComponent webView;

    //==========================================================================
    // Timer for pushing meter updates to the UI at ~30 Hz
    //==========================================================================
    class MeterTimer : public juce::Timer
    {
    public:
        explicit MeterTimer (BugosEQAudioProcessorEditor& e) : editor (e) {}
        void timerCallback() override;
    private:
        BugosEQAudioProcessorEditor& editor;
    };

    MeterTimer meterTimer;

    //==========================================================================
    // Internal helpers
    //==========================================================================

    // Receives a parameter change message from the WebView JS side.
    // msg is a juce::var parsed from the JSON sent by window.sendToPlugin.
    void handlePluginMessage (const juce::var& msg);

    // Pushes a single parameter value back to the UI (e.g. on preset load).
    // paramId: APVTS parameter ID string
    // normValue: 0..1 normalized value
    void sendParameterToUI (const juce::String& paramId, float normValue);

    // Pushes meter levels to the UI.
    void sendMeterLevels();

    // Provides binary resource files (index.html etc.) to the WebView.
    // Returns the correct MIME type for each resource path.
    static juce::WebBrowserComponent::Resource getResourceForURL (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BugosEQAudioProcessorEditor)
};
