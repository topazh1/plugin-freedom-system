#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "BinaryData.h"

//==============================================================================
ChaosverbAudioProcessorEditor::ChaosverbAudioProcessorEditor (ChaosverbAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // =========================================================================
    // STEP 1: Create all relays FIRST (no dependencies)
    // Construction order matches declaration order in .h file.
    // =========================================================================

    // Float relays (16)
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
    lowCutRelay           = std::make_unique<juce::WebSliderRelay>       ("lowCut");
    highCutRelay          = std::make_unique<juce::WebSliderRelay>       ("highCut");
    tiltRelay             = std::make_unique<juce::WebSliderRelay>       ("tilt");
    wowFlutterAmountRelay = std::make_unique<juce::WebSliderRelay>       ("wowFlutterAmount");
    outputLevelRelay      = std::make_unique<juce::WebSliderRelay>       ("outputLevel");
    duckingAmountRelay    = std::make_unique<juce::WebSliderRelay>       ("duckingAmount");

    // Bool relays (15)
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
    lowCutLockRelay       = std::make_unique<juce::WebToggleButtonRelay> ("lowCutLock");
    highCutLockRelay      = std::make_unique<juce::WebToggleButtonRelay> ("highCutLock");
    tiltLockRelay         = std::make_unique<juce::WebToggleButtonRelay> ("tiltLock");
    wowFlutterAmountLockRelay = std::make_unique<juce::WebToggleButtonRelay>("wowFlutterAmountLock");
    wowFlutterEnabledRelay = std::make_unique<juce::WebToggleButtonRelay>("wowFlutterEnabled");
    outputLevelLockRelay   = std::make_unique<juce::WebToggleButtonRelay>("outputLevelLock");
    duckingAmountLockRelay = std::make_unique<juce::WebToggleButtonRelay>("duckingAmountLock");
    bypassRelay            = std::make_unique<juce::WebToggleButtonRelay>("bypass");

    // =========================================================================
    // STEP 2: Create WebView SECOND (relays must exist first)
    // ALL 31 relays registered via .withOptionsFrom() in the Options chain.
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
            .withOptionsFrom (*lowCutRelay)
            .withOptionsFrom (*highCutRelay)
            .withOptionsFrom (*tiltRelay)
            .withOptionsFrom (*wowFlutterAmountRelay)
            .withOptionsFrom (*outputLevelRelay)
            .withOptionsFrom (*duckingAmountRelay)
            // Bool relays (16)
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
            .withOptionsFrom (*lowCutLockRelay)
            .withOptionsFrom (*highCutLockRelay)
            .withOptionsFrom (*tiltLockRelay)
            .withOptionsFrom (*wowFlutterAmountLockRelay)
            .withOptionsFrom (*wowFlutterEnabledRelay)
            .withOptionsFrom (*outputLevelLockRelay)
            .withOptionsFrom (*duckingAmountLockRelay)
            .withOptionsFrom (*bypassRelay)
            // Native functions: JS → C++ calls
            .withNativeFunction ("mutateNow", [this] (const juce::Array<juce::var>&,
                                                       std::function<void (juce::var)> complete)
            {
                processorRef.triggerMutation();
                complete ({});
            })
            .withNativeFunction ("toggleMutationTimer", [this] (const juce::Array<juce::var>&,
                                                                 std::function<void (juce::var)> complete)
            {
                bool newState = !processorRef.isMutationTimerRunning();
                processorRef.setMutationTimerRunning (newState);
                complete (juce::var (newState));
            })
            .withNativeFunction ("getMutationState", [this] (const juce::Array<juce::var>&,
                                                              std::function<void (juce::var)> complete)
            {
                auto* obj = new juce::DynamicObject();
                obj->setProperty ("remainingMs", processorRef.getRemainingTimeMs());
                obj->setProperty ("running",     processorRef.isMutationTimerRunning());
                complete (juce::var (obj));
            })
    );

    // =========================================================================
    // STEP 3: Create attachments LAST (relays and webView must exist first)
    // Pattern 12: 3-argument constructor (parameter, relay, undoManager=nullptr)
    // =========================================================================
    auto& apvts = processorRef.parameters;

    // Float attachments (12)
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
    lowCutAttachment       = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("lowCut"),           *lowCutRelay,           nullptr);
    highCutAttachment      = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("highCut"),          *highCutRelay,          nullptr);
    tiltAttachment         = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("tilt"),             *tiltRelay,             nullptr);
    wowFlutterAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("wowFlutterAmount"), *wowFlutterAmountRelay, nullptr);
    outputLevelAttachment  = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("outputLevel"),      *outputLevelRelay,      nullptr);
    duckingAmountAttachment = std::make_unique<juce::WebSliderParameterAttachment> (
        *apvts.getParameter ("duckingAmount"),    *duckingAmountRelay,    nullptr);

    // Bool attachments (15)
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
    lowCutLockAttachment       = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("lowCutLock"),       *lowCutLockRelay,       nullptr);
    highCutLockAttachment      = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("highCutLock"),      *highCutLockRelay,      nullptr);
    tiltLockAttachment         = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("tiltLock"),         *tiltLockRelay,         nullptr);
    wowFlutterAmountLockAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("wowFlutterAmountLock"), *wowFlutterAmountLockRelay, nullptr);
    wowFlutterEnabledAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("wowFlutterEnabled"), *wowFlutterEnabledRelay, nullptr);
    outputLevelLockAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("outputLevelLock"), *outputLevelLockRelay, nullptr);
    duckingAmountLockAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("duckingAmountLock"), *duckingAmountLockRelay, nullptr);
    bypassAttachment = std::make_unique<juce::WebToggleButtonParameterAttachment> (
        *apvts.getParameter ("bypass"), *bypassRelay, nullptr);

    // =========================================================================
    // STEP 4: Make WebView visible and load UI
    // =========================================================================
    addAndMakeVisible (*webView);
    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // =========================================================================
    // STEP 5: Set plugin window size + resizable with fixed aspect ratio
    // =========================================================================
    setSize (900, 380);
    setResizable (true, true);
    setResizeLimits (630, 266, 1800, 760);
    getConstrainer()->setFixedAspectRatio (900.0 / 380.0);
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
//
// BinaryData symbol names are derived from file names (not paths):
//   Source/ui/public/index.html               -> BinaryData::index_html
//   Source/ui/public/js/juce/index.js         -> BinaryData::index_js
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

    // JUCE WebView JavaScript bridge (Pattern 21: ES6 module)
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

    // Resource not found
    return std::nullopt;
}
