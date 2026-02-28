#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

// Forward declaration
class NBS_DynaDriveAudioProcessor;

//==============================================================================
/**
    NBS_DynaDriveAudioProcessorEditor
    WebView-based plugin editor for NBS DynaDrive v8.

    UI: 740x400px (collapsed), 740x548px (expanded)
    Design: v8-ui.html — production HTML with JUCE parameter bindings

    GENERATED: 2026-02-27
    SOURCE:    plugins/NBS_DynaDrive/.ideas/mockups/v8-ui.html
*/
class NBS_DynaDriveAudioProcessorEditor
    : public juce::AudioProcessorEditor
{
public:
    explicit NBS_DynaDriveAudioProcessorEditor (NBS_DynaDriveAudioProcessor&);
    ~NBS_DynaDriveAudioProcessorEditor() override;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    //==========================================================================
    // Reference to processor
    NBS_DynaDriveAudioProcessor& audioProcessor;

    //==========================================================================
    // ⚠️  CRITICAL MEMBER DECLARATION ORDER  ⚠️
    //
    //  C++ destroys members in REVERSE order of declaration.
    //  Attachments call evaluateJavascript() during destruction.
    //  That requires the webView to still be alive.
    //
    //  CORRECT ORDER:
    //    1. Relays      — no dependencies
    //    2. webView     — depends on relays (registered via .withOptionsFrom)
    //    3. Attachments — depend on both relays AND webView
    //
    //  Destruction order is the reverse:
    //    1. Attachments destroyed FIRST  (webView still alive — safe)
    //    2. webView    destroyed SECOND  (relays still alive — safe)
    //    3. Relays     destroyed LAST    (nothing using them — safe)
    //
    //  ❌ ANY OTHER ORDER CAUSES UNDEFINED BEHAVIOUR IN RELEASE BUILDS.
    //==========================================================================

    // 1️⃣  RELAYS FIRST (20 parameters — all sliders except 2 toggles)
    // ── Saturation ──
    std::unique_ptr<juce::WebSliderRelay> driveRelay;
    std::unique_ptr<juce::WebSliderRelay> evenRelay;
    std::unique_ptr<juce::WebSliderRelay> oddRelay;
    std::unique_ptr<juce::WebSliderRelay> hCurveRelay;

    // ── Center ──
    std::unique_ptr<juce::WebToggleButtonRelay> prePostRelay;
    std::unique_ptr<juce::WebSliderRelay>       inputRelay;
    std::unique_ptr<juce::WebSliderRelay>       mixRelay;
    std::unique_ptr<juce::WebSliderRelay>       outputRelay;
    std::unique_ptr<juce::WebSliderRelay>       satTiltFreqRelay;
    std::unique_ptr<juce::WebSliderRelay>       satTiltSlopeRelay;

    // ── Dynamics ──
    std::unique_ptr<juce::WebSliderRelay> dynamicsRelay;
    std::unique_ptr<juce::WebSliderRelay> upRelay;
    std::unique_ptr<juce::WebSliderRelay> downRelay;

    // ── Advanced — Dynamics Detail ──
    std::unique_ptr<juce::WebSliderRelay> thresholdRelay;
    std::unique_ptr<juce::WebSliderRelay> ratioRelay;
    std::unique_ptr<juce::WebSliderRelay> attackTimeRelay;
    std::unique_ptr<juce::WebSliderRelay> releaseTimeRelay;

    // ── Advanced — Post-Dyn Tilt ──
    std::unique_ptr<juce::WebSliderRelay> dynTiltFreqRelay;
    std::unique_ptr<juce::WebSliderRelay> dynTiltSlopeRelay;

    // ── Advanced — M/S ──
    std::unique_ptr<juce::WebToggleButtonRelay> msEnableRelay;
    std::unique_ptr<juce::WebSliderRelay>       midDriveRelay;
    std::unique_ptr<juce::WebSliderRelay>       sideDriveRelay;

    // 2️⃣  WEBVIEW SECOND (depends on all relays via .withOptionsFrom())
    std::unique_ptr<juce::WebBrowserComponent> webView;

    // 3️⃣  ATTACHMENTS LAST (depend on relays + webView)
    // ── Saturation ──
    std::unique_ptr<juce::WebSliderParameterAttachment> driveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> evenAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> oddAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> hCurveAttachment;

    // ── Center ──
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> prePostAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       inputAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       mixAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       outputAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       satTiltFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       satTiltSlopeAttachment;

    // ── Dynamics ──
    std::unique_ptr<juce::WebSliderParameterAttachment> dynamicsAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> upAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> downAttachment;

    // ── Advanced — Dynamics Detail ──
    std::unique_ptr<juce::WebSliderParameterAttachment> thresholdAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> ratioAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> attackTimeAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> releaseTimeAttachment;

    // ── Advanced — Post-Dyn Tilt ──
    std::unique_ptr<juce::WebSliderParameterAttachment> dynTiltFreqAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment> dynTiltSlopeAttachment;

    // ── Advanced — M/S ──
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> msEnableAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       midDriveAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       sideDriveAttachment;

    //==========================================================================
    // WebView resource provider
    std::optional<juce::WebBrowserComponent::Resource>
        getResource (const juce::String& url);

    // Window size state (tracks collapsed/expanded)
    bool isExpanded = false;

    static constexpr int kCollapsedWidth  = 740;
    static constexpr int kCollapsedHeight = 400;
    static constexpr int kExpandedWidth   = 740;
    static constexpr int kExpandedHeight  = 548;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NBS_DynaDriveAudioProcessorEditor)
};
