#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_extra/juce_gui_extra.h>

// Forward declaration
class ChaosverbAudioProcessor;

//==============================================================================
/**
 * ChaosverbAudioProcessorEditor — WebView-based plugin editor
 *
 * TEMPLATE FILE — For gui-agent reference during Stage 3 (GUI implementation).
 * This is NOT a copy-paste file. gui-agent adapts this to the actual plugin
 * structure and wires up the mutateNow native function callback.
 *
 * Source mockup: v1-ui.yaml (finalized: true)
 * Generated: 2026-02-27
 * Parameters: 22 total (12 Float + 10 Bool)
 */
class ChaosverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit ChaosverbAudioProcessorEditor (ChaosverbAudioProcessor&);
    ~ChaosverbAudioProcessorEditor() override;

    //==========================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // =========================================================================
    // Reference to processor
    ChaosverbAudioProcessor& audioProcessor;

    // =========================================================================
    // CRITICAL MEMBER DECLARATION ORDER
    // Members are destroyed in REVERSE order of declaration.
    // Order MUST be: Relays -> WebView -> Attachments
    //
    // Why: Attachments call evaluateJavascript() on destruction.
    //      WebView must still exist when attachments are destroyed.
    //      Relays must still exist when WebView is destroyed.
    //
    // Violation causes: undefined behaviour in release builds only
    // (debug builds may hide this with different destruction timing).
    // =========================================================================

    // 1. RELAYS FIRST — 12 Float relays + 10 Bool relays = 22 total
    // ─────────────────────────────────────────────────────────────
    // Float relays (WebSliderRelay)
    std::unique_ptr<juce::WebSliderRelay>         topologyRelay;
    std::unique_ptr<juce::WebSliderRelay>         decayRelay;
    std::unique_ptr<juce::WebSliderRelay>         preDelayRelay;
    std::unique_ptr<juce::WebSliderRelay>         densityRelay;
    std::unique_ptr<juce::WebSliderRelay>         spectralTiltRelay;
    std::unique_ptr<juce::WebSliderRelay>         resonanceRelay;
    std::unique_ptr<juce::WebSliderRelay>         modRateRelay;
    std::unique_ptr<juce::WebSliderRelay>         modDepthRelay;
    std::unique_ptr<juce::WebSliderRelay>         mutationIntervalRelay;
    std::unique_ptr<juce::WebSliderRelay>         crossfadeSpeedRelay;
    std::unique_ptr<juce::WebSliderRelay>         widthRelay;
    std::unique_ptr<juce::WebSliderRelay>         mixRelay;

    // Bool relays (WebToggleButtonRelay)
    std::unique_ptr<juce::WebToggleButtonRelay>   topologyLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   decayLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   preDelayLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   densityLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   spectralTiltLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   resonanceLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   modRateLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   modDepthLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   widthLockRelay;
    std::unique_ptr<juce::WebToggleButtonRelay>   mixLockRelay;

    // 2. WEBVIEW SECOND — depends on relays being alive
    // ─────────────────────────────────────────────────────────────
    std::unique_ptr<juce::WebBrowserComponent>    webView;

    // 3. ATTACHMENTS LAST — depend on both relays and webView
    // ─────────────────────────────────────────────────────────────
    // Float attachments (WebSliderParameterAttachment)
    std::unique_ptr<juce::WebSliderParameterAttachment>       topologyAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       decayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       preDelayAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       densityAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       spectralTiltAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       resonanceAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       modRateAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       modDepthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       mutationIntervalAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       crossfadeSpeedAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       widthAttachment;
    std::unique_ptr<juce::WebSliderParameterAttachment>       mixAttachment;

    // Bool attachments (WebToggleButtonParameterAttachment)
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> topologyLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> decayLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> preDelayLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> densityLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> spectralTiltLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> resonanceLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modRateLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> modDepthLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> widthLockAttachment;
    std::unique_ptr<juce::WebToggleButtonParameterAttachment> mixLockAttachment;

    //==========================================================================
    // Resource provider (Pattern 8: explicit URL mapping)
    std::optional<juce::WebBrowserComponent::Resource>
    getResource (const juce::String& url);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ChaosverbAudioProcessorEditor)
};
