// =============================================================================
// v2-PluginEditor-TEMPLATE.cpp
// BugosEQ — WebView PluginEditor implementation template
//
// USAGE: Copy to Source/PluginEditor.cpp during Stage 3 (GUI).
//        Fill all TODO markers. gui-agent adapts to actual processor fields.
//
// GENERATED: 2026-02-27 by ui-finalization-agent (ui-mockup skill, v2)
//
// ARCHITECTURE NOTES:
//   - UI lives in Source/ui/public/index.html  (copy of v2-ui.html)
//   - The HTML is bundled as BinaryData via juce_add_binary_data
//   - Parameter flow:
//       Knob drag  -> JS sendToPlugin()  -> handlePluginMessage()  -> APVTS
//       APVTS load -> sendParameterToUI() -> webView.emitEvent...  -> JS pluginMessage()
//   - Meter flow:
//       MeterTimer fires at ~30 Hz -> sendMeterLevels() -> JS pluginMessage()
// =============================================================================

#include "PluginEditor.h"
// TODO: Add BinaryData include once juce_add_binary_data is configured
// #include <BinaryData.h>

//==============================================================================
// Resource provider — maps URL paths to BinaryData entries
//==============================================================================
juce::WebBrowserComponent::Resource
BugosEQAudioProcessorEditor::getResourceForURL (const juce::String& url)
{
    // Strip leading slash to get the relative path
    auto path = url.trimCharactersAtStart ("/");

    // Root path serves index.html
    if (path.isEmpty() || path == "index.html")
    {
        // TODO: Replace BinaryData::index_html with the actual generated symbol name.
        // juce_add_binary_data converts '/' to '_' and '.' to '_', e.g.:
        //   Source/ui/public/index.html  ->  BinaryData::index_html
        return { std::vector<std::byte> (
                     reinterpret_cast<const std::byte*> (BinaryData::index_html),
                     reinterpret_cast<const std::byte*> (BinaryData::index_html) + BinaryData::index_htmlSize),
                 "text/html" };
    }

    // TODO: Add additional resources here if the UI references external CSS or JS files.
    // Example for a JS file:
    // if (path == "js/juce/index.js")
    // {
    //     return { std::vector<std::byte> ( ... BinaryData::js_juce_index_js ... ),
    //              "application/javascript" };
    // }

    // Return empty resource for unrecognized paths
    return {};
}

//==============================================================================
// Constructor
//==============================================================================
BugosEQAudioProcessorEditor::BugosEQAudioProcessorEditor (BugosEQAudioProcessor& p)
    : AudioProcessorEditor (&p),
      audioProcessor (p),
      // Construct webView with resource provider and native function handler
      webView (juce::WebBrowserComponent::Options{}
                   .withResourceProvider ([] (const juce::String& url) -> juce::WebBrowserComponent::Resource
                   {
                       return BugosEQAudioProcessorEditor::getResourceForURL (url);
                   },
                   juce::URL ("http://localhost/"))
                   .withNativeFunction ("sendToPlugin",
                   [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                   {
                       if (args.size() > 0)
                           handlePluginMessage (args[0]);
                       completion ({});
                   })),
      meterTimer (*this)
{
    addAndMakeVisible (webView);

    // Window size (from v2-ui.yaml window dimensions)
    setSize (820, 620);
    setResizable (false, false);

    // Load the UI
    webView.goToURL ("http://localhost/");

    // Start meter update timer at ~30 Hz
    meterTimer.startTimerHz (30);

    // TODO: Register as APVTS listener to push preset changes to UI:
    // audioProcessor.parameters.addParameterListener("band1_freq", this);
    // ... (repeat for all 35 parameters)
}

//==============================================================================
// Destructor
//==============================================================================
BugosEQAudioProcessorEditor::~BugosEQAudioProcessorEditor()
{
    meterTimer.stopTimer();

    // TODO: Remove all APVTS listeners registered in constructor:
    // audioProcessor.parameters.removeParameterListener("band1_freq", this);
    // ... (repeat for all 35 parameters)
}

//==============================================================================
// paint — WebView fills entire window; nothing to draw here
//==============================================================================
void BugosEQAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff2a1a0a)); // Match HTML background colour
}

//==============================================================================
// resized
//==============================================================================
void BugosEQAudioProcessorEditor::resized()
{
    webView.setBounds (getLocalBounds());
}

//==============================================================================
// handlePluginMessage
// Receives JSON messages from window.sendToPlugin() in the WebView.
//
// Expected message format (parameterChange):
//   { "type": "parameterChange", "id": "band1_freq", "value": 0.42 }
//   where "value" is a 0..1 normalized float.
//==============================================================================
void BugosEQAudioProcessorEditor::handlePluginMessage (const juce::var& msg)
{
    if (!msg.isObject())
        return;

    const auto type = msg["type"].toString();

    if (type == "parameterChange")
    {
        const auto paramId   = msg["id"].toString();
        const float normValue = static_cast<float> (msg["value"]);

        // Look up the parameter in APVTS and set the normalised value.
        // setValueNotifyingHost() schedules an automation write and notifies listeners.
        if (auto* param = audioProcessor.parameters.getParameter (paramId))
        {
            param->beginChangeGesture();
            param->setValueNotifyingHost (normValue);
            param->endChangeGesture();
        }
        else
        {
            DBG ("BugosEQEditor: Unknown parameter ID: " + paramId);
        }
    }
    // TODO: Handle additional message types here if needed (e.g. "requestState")
}

//==============================================================================
// sendParameterToUI
// Pushes a single parameter value update to the WebView JS.
// Call this when presets are loaded (from AudioProcessorValueTreeState listener).
//
// JS side receives:
//   window.pluginMessage({ type: "parameterValue", id: "band1_freq", value: 0.42 })
//==============================================================================
void BugosEQAudioProcessorEditor::sendParameterToUI (const juce::String& paramId, float normValue)
{
    juce::DynamicObject::Ptr msgObj = new juce::DynamicObject();
    msgObj->setProperty ("type",  "parameterValue");
    msgObj->setProperty ("id",    paramId);
    msgObj->setProperty ("value", normValue);

    webView.emitEventIfBrowserIsVisible ("pluginMessage", juce::var (msgObj.get()));
}

//==============================================================================
// sendMeterLevels
// Called by MeterTimer at ~30 Hz.
// Reads level data from the processor and pushes to the WebView.
//
// JS side receives:
//   window.pluginMessage({ type: "meterLevel",
//                          inputLevel: 0.72,
//                          outputLevel: 0.65,
//                          grLevels: [0.1, 0.0, 0.05, 0.0] })
//
// inputLevel / outputLevel: 0..1 (linear amplitude, map to meter width %)
// grLevels[band]: 0..1 where 1 = 100% meter fill (maximum GR)
//==============================================================================
void BugosEQAudioProcessorEditor::sendMeterLevels()
{
    // TODO: Replace with real meter values from processor.
    // Example (assuming processor exposes these atomics or methods):
    //   float inLevel  = audioProcessor.getInputLevel();
    //   float outLevel = audioProcessor.getOutputLevel();
    //   float gr1      = audioProcessor.getBandGRLevel(0);
    //   ...

    // Placeholder: send zeros until DSP is implemented
    juce::DynamicObject::Ptr msgObj = new juce::DynamicObject();
    msgObj->setProperty ("type",        "meterLevel");
    msgObj->setProperty ("inputLevel",  0.0f);  // TODO: real value
    msgObj->setProperty ("outputLevel", 0.0f);  // TODO: real value

    juce::Array<juce::var> grArray;
    grArray.add (0.0f);  // band1 GR  TODO: real value
    grArray.add (0.0f);  // band2 GR  TODO: real value
    grArray.add (0.0f);  // band3 GR  TODO: real value
    grArray.add (0.0f);  // band4 GR  TODO: real value
    msgObj->setProperty ("grLevels", grArray);

    webView.emitEventIfBrowserIsVisible ("pluginMessage", juce::var (msgObj.get()));
}

//==============================================================================
// MeterTimer callback
//==============================================================================
void BugosEQAudioProcessorEditor::MeterTimer::timerCallback()
{
    editor.sendMeterLevels();
}

// =============================================================================
// PARAMETER ID REFERENCE (all 35 parameters)
// These must match EXACTLY what is registered in PluginProcessor.cpp APVTS.
// =============================================================================
//
// Global (3):
//   "processing_mode"   Choice [0=Stereo, 1=M-S]     default: 0
//   "drive"             Float  [0..100]    %          default: 20
//   "output_gain"       Float  [-18..18]   dB         default: 0
//
// Band 1 — LF Shelf (8):
//   "band1_freq"        Float  [20..20000] Hz         default: 80
//   "band1_gain"        Float  [-18..18]   dB         default: 0
//   "band1_q"           Float  [0.1..10]              default: 0.7
//   "band1_mode"        Choice [0=Static,1=Exp,2=Up,3=Dn]  default: 0
//   "band1_threshold"   Float  [-60..0]    dBFS       default: -24
//   "band1_ratio"       Float  [1..20]     :1         default: 4
//   "band1_attack"      Float  [0.1..300]  ms         default: 10
//   "band1_release"     Float  [5..2000]   ms         default: 100
//
// Band 2 — Lo Mid Peak (8):  (same suffixes, prefix "band2_")
//   "band2_freq"   default: 400 Hz
//   "band2_q"      default: 1.0
//   (all other defaults same as band1)
//
// Band 3 — Hi Mid Peak (8):  (same suffixes, prefix "band3_")
//   "band3_freq"   default: 2500 Hz
//   "band3_q"      default: 1.0
//
// Band 4 — HF Shelf (8):     (same suffixes, prefix "band4_")
//   "band4_freq"   default: 10000 Hz
//   "band4_q"      default: 0.7
// =============================================================================
