#include "PluginEditor.h"

// Colours
static const juce::Colour BG_DARK    { 0xff1e1e2e };
static const juce::Colour BG_SURFACE { 0xff313244 };
static const juce::Colour SEPARATOR  { 0xff45475a };
static const juce::Colour TEXT_MAIN  { 0xffcdd6f4 };
static const juce::Colour TEXT_DIM   { 0xff6c7086 };
static const juce::Colour ACCENT     { 0xff89b4fa }; // blue
static const juce::Colour ACCENT2    { 0xffa6e3a1 }; // green (meter fill)

// =============================================================================
// Constructor / Destructor
// =============================================================================

MixMentorAudioProcessorEditor::MixMentorAudioProcessorEditor (MixMentorAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    // --- Genre selector ------------------------------------------------------
    genreSelector.addItemList (
        { "Pop", "Hip-Hop", "Electronic", "Rock",
          "Jazz", "Classical", "R&B", "Country" }, 1);

    genreSelector.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (genreSelector);

    // Attach to APVTS — keeps ComboBox in sync with the "genre" parameter
    genreAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        processorRef.apvts, "genre", genreSelector);

    // --- Feedback text area --------------------------------------------------
    feedbackText.setMultiLine     (true);
    feedbackText.setReadOnly      (true);
    feedbackText.setScrollbarsShown (true);
    feedbackText.setColour (juce::TextEditor::backgroundColourId, BG_SURFACE);
    feedbackText.setColour (juce::TextEditor::textColourId,       TEXT_MAIN);
    feedbackText.setColour (juce::TextEditor::outlineColourId,    SEPARATOR);
    feedbackText.setText (
        "Feedback from your SNIPAI.co mentor will appear here.\n\n"
        "Select a genre profile, then press \"Send to SNIPAI.co\" to submit "
        "your current mix snapshot for review.");
    addAndMakeVisible (feedbackText);

    // --- Send button ---------------------------------------------------------
    sendButton.setColour (juce::TextButton::buttonColourId,   ACCENT);
    sendButton.setColour (juce::TextButton::textColourOffId,  BG_DARK);
    sendButton.onClick = [this]
    {
        // TODO Stage 4: build JSON payload from getLatestAnalysis() + genre
        //               POST to https://snipai.co/api/mix-snapshot
        feedbackText.setText (
            "[Placeholder] Mix data would be sent to SNIPAI.co here.\n\n"
            "Web integration will be implemented in Stage 4.");
    };
    addAndMakeVisible (sendButton);

    setSize (480, 420);
    startTimerHz (30); // Refresh analysis display ~30 times per second
}

MixMentorAudioProcessorEditor::~MixMentorAudioProcessorEditor()
{
    stopTimer();
}

// =============================================================================
// Timer — polls processor and queues a repaint for the meter area
// =============================================================================

void MixMentorAudioProcessorEditor::timerCallback()
{
    const auto analysis = processorRef.getLatestAnalysis();

    displayRmsDb            = analysis.rmsDb;
    displayLufs             = analysis.lufsIntegrated;
    displaySpectralCentroid = analysis.spectralCentroidHz;
    displayStereoWidth      = analysis.stereoWidth;

    // Only repaint the analysis section — avoids redrawing components
    repaint (0, 92, getWidth(), 110);
}

// =============================================================================
// Paint — background, title, and analysis meters
// =============================================================================

void MixMentorAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int W = getWidth();

    // Background
    g.fillAll (BG_DARK);

    // ---- Header (y 0-44) ----------------------------------------------------
    g.setColour (BG_SURFACE);
    g.fillRect (0, 0, W, 44);

    g.setColour (TEXT_MAIN);
    g.setFont (20.0f);
    g.drawText ("MixMentor", 16, 0, 200, 44, juce::Justification::centredLeft);

    g.setColour (TEXT_DIM);
    g.setFont (11.0f);
    g.drawText ("Master Bus Analyser", W - 160, 0, 148, 44, juce::Justification::centredRight);

    // ---- Separator ----------------------------------------------------------
    g.setColour (SEPARATOR);
    g.drawHorizontalLine (44, 0.0f, (float)W);

    // ---- Genre label (y 50-90) ----------------------------------------------
    g.setColour (TEXT_DIM);
    g.setFont (12.0f);
    g.drawText ("Genre", 16, 52, 58, 28, juce::Justification::centredLeft);

    // ---- Separator ----------------------------------------------------------
    g.setColour (SEPARATOR);
    g.drawHorizontalLine (90, 0.0f, (float)W);

    // ---- Analysis section (y 92-200) ----------------------------------------
    g.setColour (TEXT_MAIN);
    g.setFont (13.0f);
    g.drawText ("Analysis", 16, 94, 200, 18, juce::Justification::centredLeft);

    // Helper lambda: draws a labelled horizontal meter bar
    auto drawMeter = [&] (const juce::String& label,
                          const juce::String& valueText,
                          float normalisedFill,
                          int   yPos)
    {
        const int labelX = 16,  labelW = 100;
        const int barX   = 120, barW   = W - barX - 16, barH = 14;
        const int valX   = W - 72;

        g.setColour (TEXT_DIM);
        g.setFont (11.0f);
        g.drawText (label, labelX, yPos, labelW, barH, juce::Justification::centredLeft);

        // Track
        g.setColour (SEPARATOR);
        g.fillRoundedRectangle ((float)barX, (float)yPos,
                                (float)barW, (float)barH, 3.0f);

        // Fill
        const float fillW = juce::jlimit (0.0f, (float)barW,
                                          normalisedFill * (float)barW);
        if (fillW > 1.0f)
        {
            g.setColour (ACCENT2);
            g.fillRoundedRectangle ((float)barX, (float)yPos,
                                    fillW, (float)barH, 3.0f);
        }

        // Value text
        g.setColour (TEXT_DIM);
        g.setFont (10.0f);
        g.drawText (valueText, valX, yPos, 60, barH, juce::Justification::centredRight);
    };

    // Dynamics: normalise LUFS from -30 to 0 dBFS
    const float dynNorm = juce::jmap (displayLufs, -30.0f, 0.0f, 0.0f, 1.0f);
    drawMeter ("Dynamics",
               juce::String (displayLufs, 1) + " LUFS",
               dynNorm, 116);

    // Tonality: normalise centroid from 200 Hz to 10 kHz
    const float tonNorm = juce::jmap (displaySpectralCentroid, 200.0f, 10000.0f, 0.0f, 1.0f);
    const juce::String centroidStr = (displaySpectralCentroid >= 1000.0f)
        ? (juce::String (displaySpectralCentroid / 1000.0f, 1) + " kHz")
        : (juce::String ((int)displaySpectralCentroid) + " Hz");
    drawMeter ("Tonality", centroidStr, tonNorm, 140);

    // Stereo width: already 0-1
    drawMeter ("Stereo Width",
               juce::String ((int)(displayStereoWidth * 100.0f)) + " %",
               displayStereoWidth, 164);

    // ---- Separator ----------------------------------------------------------
    g.setColour (SEPARATOR);
    g.drawHorizontalLine (196, 0.0f, (float)W);

    // ---- Feedback label (y 200-220) -----------------------------------------
    g.setColour (TEXT_MAIN);
    g.setFont (13.0f);
    g.drawText ("Feedback", 16, 200, 200, 20, juce::Justification::centredLeft);
}

// =============================================================================
// Resized — lay out all JUCE components
// =============================================================================

void MixMentorAudioProcessorEditor::resized()
{
    const int margin = 16;
    const int W      = getWidth();

    // Genre row (y=52, height=28) — label drawn in paint, combo starts after
    genreSelector.setBounds (margin + 64, 52, W - margin * 2 - 64, 28);

    // Feedback text editor (y=224, height=112)
    feedbackText.setBounds (margin, 224, W - margin * 2, 112);

    // Send button (y=350, height=40)
    sendButton.setBounds (margin, 350, W - margin * 2, 40);
}
