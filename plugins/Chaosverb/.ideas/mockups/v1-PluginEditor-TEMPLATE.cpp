#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
ChaosverbAudioProcessorEditor::ChaosverbAudioProcessorEditor (ChaosverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // =========================================================================
    // STEP 1: Create all relays FIRST (no dependencies)
    // Construction order matches declaration order in .h file.
    // =========================================================================

    // Float relays
    topologyRelay         = std::make_unique<juce::WebSliderRelay>       ("topology");
    decayRelay            = std::make_unique<juce::WebSliderRelay>       ("decay");
    preDelayRelay         = std::make_unique<juce::WebSliderRelay>       ("preDelay");
    densityRelay          = std::make_unique<juce::WebSliderRelay>       ("density");
    spectralTiltRelay     = std::make_unique<juce::WebSliderRelay>       ("spectralTilt");
    resonanceRelay        = std::make_unique<juce::WebSliderRelay>       ("resonance");
    modRateRelay          = std::make_unique<juce::WebSliderRelay>       ("modRate");
    modDepthRelay         = std::make_unique<juce::WebSliderRelay>       ("modDepth");
    mutationIntervalRelay = std::make_unique<juce::WebSliderRelay>       ("mutationInterval");
    crossfadeSpeedRelay   = std::make_unique<juce::WebSliderRelay>       ("crossfadeSpeed");
    widthRelay            = std::make_unique<juce::WebSliderRelay>       ("width");
    mixRelay              = std::make_unique<juce::WebSliderRelay>       ("mix");

    // Bool relays
    topologyLockRelay     = std::make_unique<juce::WebToggleButtonRelay> ("topologyLock");
    decayLockRelay        = std::make_unique<juce::WebToggleButtonRelay> ("decayLock");
    preDelayLockRelay     = std::make_unique<juce::WebToggleButtonRelay> ("preDelayLock");
    densityLockRelay      = std::make_unique<juce::WebToggleButtonRelay> ("densityLock");
    spectralTiltLockRelay = std::make_unique<juce::WebToggleButtonRelay> ("spectralTiltLock");
    resonanceLockRelay    = std::make_unique<juce::WebToggleButtonRelay> ("resonanceLock");
    modRateLockRelay      = std::make_unique<juce::WebToggleButtonRelay> ("modRateLock");
    modDepthLockRelay     = std::make_unique<juce::WebToggleButtonRelay> ("modDepthLock");
    widthLockRelay        = std::make_unique<juce::WebToggleButtonRelay> ("widthLock");
    mixLockRelay          = std::make_unique<juce::WebToggleButtonRelay> ("mixLock");

    // =========================================================================
    // STEP 2: Create WebView SECOND (relays must exist first)
    // All relays registered via .withOptionsFrom() in the Options chain.
    // =========================================================================
    webView = std::make_unique<juce::WebBrowserComponent> (
        juce::WebBrowserComponent::Options{}
            .withNativeIntegrationEnabled()
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
            // Float relays (12)
            .withOptionsFrom (*topologyRelay)
            .withOptionsFrom (*decayRelay)
            .withOptionsFrom (*preDelayRelay)
            .withOptionsFrom (*densityRelay)
            .withOptionsFrom (*spectralTiltRelay)
            .withOptionsFrom (*resonanceRelay)
            .withOptionsFrom (*modRateRelay)
            .withOptionsFrom (*modDepthRelay)
            .withOptionsFrom (*mutationIntervalRelay)
            .withOptionsFrom (*crossfadeSpeedRelay)
            .withOptionsFrom (*widthRelay)
            .withOptionsFrom (*mixRelay)
            // Bool relays (10)
            .withOptionsFrom (*topologyLockRelay)
            .withOptionsFrom (*decayLockRelay)
            .withOptionsFrom (*preDelayLockRelay)
            .withOptionsFrom (*densityLockRelay)
            .withOptionsFrom (*spectralTiltLockRelay)
            .withOptionsFrom (*resonanceLockRelay)
            .withOptionsFrom (*modRateLockRelay)
            .withOptionsFrom (*modDepthLockRelay)
            .withOptionsFrom (*widthLockRelay)
            .withOptionsFrom (*mixLockRelay)
    );

    // =========================================================================
    // STEP 3: Create attachments LAST (relays and webView must exist first)
    // JUCE 8 requires 3-argument constructor: (parameter, relay, undoManager)
    // Pass nullptr for undoManager (Pattern 12).
    // =========================================================================
    auto& apvts = audioProcessor.parameters;  // Adjust to actual member name

    // Float attachments
    topologyAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("topology"),         *topologyRelay,         nullptr);
    decayAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("decay"),            *decayRelay,            nullptr);
    preDelayAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("preDelay"),         *preDelayRelay,         nullptr);
    densityAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("density"),          *densityRelay,          nullptr);
    spectralTiltAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("spectralTilt"),     *spectralTiltRelay,     nullptr);
    resonanceAttachment    = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("resonance"),        *resonanceRelay,        nullptr);
    modRateAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("modRate"),          *modRateRelay,          nullptr);
    modDepthAttachment     = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("modDepth"),         *modDepthRelay,         nullptr);
    mutationIntervalAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("mutationInterval"), *mutationIntervalRelay, nullptr);
    crossfadeSpeedAttachment   = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("crossfadeSpeed"),   *crossfadeSpeedRelay,   nullptr);
    widthAttachment        = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("width"),            *widthRelay,            nullptr);
    mixAttachment          = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("mix"),              *mixRelay,              nullptr);

    // Bool attachments
    topologyLockAttachment     = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("topologyLock"),     *topologyLockRelay,     nullptr);
    decayLockAttachment        = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("decayLock"),        *decayLockRelay,        nullptr);
    preDelayLockAttachment     = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("preDelayLock"),     *preDelayLockRelay,     nullptr);
    densityLockAttachment      = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("densityLock"),      *densityLockRelay,      nullptr);
    spectralTiltLockAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("spectralTiltLock"), *spectralTiltLockRelay, nullptr);
    resonanceLockAttachment    = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("resonanceLock"),    *resonanceLockRelay,    nullptr);
    modRateLockAttachment      = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("modRateLock"),      *modRateLockRelay,      nullptr);
    modDepthLockAttachment     = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("modDepthLock"),     *modDepthLockRelay,     nullptr);
    widthLockAttachment        = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("widthLock"),        *widthLockRelay,        nullptr);
    mixLockAttachment          = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("mixLock"),          *mixLockRelay,          nullptr);

    // =========================================================================
    // STEP 4: Make WebView visible and load UI
    // =========================================================================
    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // =========================================================================
    // STEP 5: Set plugin window size (from v1-ui.yaml dimensions)
    // =========================================================================
    setSize (780, 360);

    // NOTE for gui-agent: Wire up "mutateNow" native function here.
    // The JS calls window.__JUCE__.backend.emitEvent("mutateNow", {}) when
    // the Mutate Now button is clicked. Register a handler that calls
    // audioProcessor.triggerMutation() on the audio processor.
    // Example:
    //
    // webView->registerNativeFunction ("mutateNow",
    //     [this] (const juce::var&)
    //     {
    //         audioProcessor.triggerMutationNow();
    //         return juce::var();
    //     });
}

ChaosverbAudioProcessorEditor::~ChaosverbAudioProcessorEditor()
{
    // Destruction order is automatic — reverse of declaration order in .h file:
    // 1. Attachments destroyed first (call evaluateJavascript — webView still alive)
    // 2. WebView destroyed second (attachments already gone)
    // 3. Relays destroyed last (nothing using them)
    //
    // No explicit cleanup needed. std::unique_ptr handles everything correctly.
}

//==============================================================================
void ChaosverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Background — WebView covers the entire editor, so this is a fallback
    // color shown briefly during load or if WebView fails.
    g.fillAll (juce::Colour (0xff141414));
}

void ChaosverbAudioProcessorEditor::resized()
{
    // WebView fills the entire plugin window
    if (webView != nullptr)
        webView->setBounds (getLocalBounds());
}

//==============================================================================
// Resource provider — Pattern 8: explicit URL mapping
// BinaryData names are derived from file paths:
//   Source/ui/public/index.html      -> BinaryData::index_html
//   Source/ui/public/js/juce/index.js -> BinaryData::index_js
//   Source/ui/public/js/juce/check_native_interop.js -> BinaryData::check_native_interop_js
//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
ChaosverbAudioProcessorEditor::getResource (const juce::String& url)
{
    auto makeVector = [] (const char* data, int size)
    {
        return std::vector<std::byte> (
            reinterpret_cast<const std::byte*> (data),
            reinterpret_cast<const std::byte*> (data) + size);
    };

    // Main HTML document
    if (url == "/" || url == "/index.html")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_html, BinaryData::index_htmlSize),
            juce::String ("text/html")
        };
    }

    // JUCE WebView bridge (Pattern 21: ES6 module)
    if (url == "/js/juce/index.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::index_js, BinaryData::index_jsSize),
            juce::String ("application/javascript")
        };
    }

    // Native interop check script (Pattern 13: REQUIRED)
    if (url == "/js/juce/check_native_interop.js")
    {
        return juce::WebBrowserComponent::Resource {
            makeVector (BinaryData::check_native_interop_js,
                        BinaryData::check_native_interop_jsSize),
            juce::String ("application/javascript")
        };
    }

    // Add additional resources here if index.html references external CSS,
    // fonts, or images that are bundled via juce_add_binary_data.

    return std::nullopt;  // 404 — resource not found
}
