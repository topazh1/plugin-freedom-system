#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
// Parameter layout
// =============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
MixMentorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray genres
    {
        "Pop", "Hip-Hop", "Electronic", "Rock",
        "Jazz", "Classical", "R&B", "Country"
    };

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "genre", 1 },
        "Genre",
        genres,
        0   // default: Pop
    ));

    return layout;
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

MixMentorAudioProcessor::MixMentorAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createParameterLayout())
{
}

MixMentorAudioProcessor::~MixMentorAudioProcessor() {}

// =============================================================================
// Lifecycle
// =============================================================================

void MixMentorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    dynamicsAnalyzer.prepare (sampleRate, samplesPerBlock);
    tonalityAnalyzer.prepare (sampleRate, samplesPerBlock);
    stereoAnalyzer.prepare   (sampleRate, samplesPerBlock);
}

void MixMentorAudioProcessor::releaseResources() {}

bool MixMentorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Require stereo in and stereo out — this is a master-bus tool
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainInputChannelSet()  != juce::AudioChannelSet::stereo()) return false;
    return true;
}

// =============================================================================
// Audio thread
// =============================================================================

void MixMentorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                             juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    // --- Analysis (buffer is not modified — pass-through analyser) -----------

    dynamicsAnalyzer.process (buffer);
    tonalityAnalyzer.process (buffer);
    stereoAnalyzer.process   (buffer);

    // --- Snapshot analyser results into atomics for the editor ---------------

    latestRmsDb.store            (dynamicsAnalyzer.getRmsDb());
    latestLufs.store             (dynamicsAnalyzer.getLufsIntegrated());
    latestPeakDb.store           (dynamicsAnalyzer.getPeakDb());
    latestSpectralCentroid.store (tonalityAnalyzer.getSpectralCentroidHz());
    latestSpectralFlatness.store (tonalityAnalyzer.getSpectralFlatness());
    latestCorrelation.store      (stereoAnalyzer.getStereoCorrelation());
    latestStereoWidth.store      (stereoAnalyzer.getStereoWidth());
}

// =============================================================================
// Editor
// =============================================================================

juce::AudioProcessorEditor* MixMentorAudioProcessor::createEditor()
{
    return new MixMentorAudioProcessorEditor (*this);
}

// =============================================================================
// Thread-safe snapshot for the editor
// =============================================================================

MixAnalysisResult MixMentorAudioProcessor::getLatestAnalysis() const
{
    MixAnalysisResult r;
    r.rmsDb              = latestRmsDb.load();
    r.lufsIntegrated     = latestLufs.load();
    r.peakDb             = latestPeakDb.load();
    r.spectralCentroidHz = latestSpectralCentroid.load();
    r.spectralFlatness   = latestSpectralFlatness.load();
    r.stereoCorrelation  = latestCorrelation.load();
    r.stereoWidth        = latestStereoWidth.load();
    return r;
}

// =============================================================================
// State persistence
// =============================================================================

void MixMentorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void MixMentorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// =============================================================================
// Plugin entry point
// =============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MixMentorAudioProcessor();
}
