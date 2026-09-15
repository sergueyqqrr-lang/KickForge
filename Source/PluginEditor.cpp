#include "PluginEditor.h"

KickForgeAudioProcessorEditor::KickForgeAudioProcessorEditor (KickForgeAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    auto setupSection = [this] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setFont (juce::Font (16.0f, juce::Font::bold));
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };

    setupSection (sectionLinear, "EQ LINEAL (fase cero)");
    addRow ("linFreq0", "Sub Freq");   addRow ("linGain0", "Sub Gain");
    addRow ("linFreq1", "Punch Freq"); addRow ("linGain1", "Punch Gain"); addRow ("linQ1", "Punch Q");
    addRow ("linFreq2", "Mud Freq");   addRow ("linGain2", "Mud Gain");   addRow ("linQ2", "Mud Q");
    addRow ("linFreq3", "Air Freq");   addRow ("linGain3", "Air Gain");

    setupSection (sectionAnalog, "EQ ANALOGICO (serie / warmth)");
    addRow ("anFreqLow", "Low Freq");   addRow ("anGainLow", "Low Gain");
    addRow ("anFreqMid", "Mid Freq");   addRow ("anGainMid", "Mid Gain"); addRow ("anQMid", "Mid Q");
    addRow ("anFreqHigh", "High Freq"); addRow ("anGainHigh", "High Gain");
    addRow ("anWarmth", "Warmth");

    setupSection (sectionSynth, "SYNTH LAYER (basico)");
    addRow ("synMix", "Mix");
    addRow ("synBaseFreq", "Base Freq");
    addRow ("synPitchDrop", "Pitch Drop");
    addRow ("synDecayMs", "Decay");
    addRow ("synCutoff", "Cutoff");
    addRow ("synThreshold", "Threshold");

    addRow ("outputGain", "Output Gain");

    setSize (420, 40 * (int) rows.size() + 140);
}

void KickForgeAudioProcessorEditor::addRow (const juce::String& paramID, const juce::String& labelText)
{
    auto row = std::make_unique<Row>();
    row->label.setText (labelText, juce::dontSendNotification);
    row->label.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (row->label);

    row->slider.setSliderStyle (juce::Slider::LinearHorizontal);
    row->slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 70, 20);
    addAndMakeVisible (row->slider);

    row->attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, paramID, row->slider);
    rows.push_back (std::move (row));
}

void KickForgeAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1c1c1e));
}

void KickForgeAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    int y = area.getY();
    const int rowH = 24;
    const int sectionGap = 10;

    auto placeSection = [&] (juce::Label& l)
    {
        l.setBounds (area.getX(), y, area.getWidth(), rowH);
        y += rowH + 4;
    };

    // Nota: el orden aqui debe reflejar el orden en que se añadieron las filas
    // en el constructor para cada seccion. Simplificado con un contador.
    size_t idx = 0;
    auto placeRows = [&] (int count)
    {
        for (int i = 0; i < count; ++i)
        {
            auto& r = *rows[idx++];
            r.label.setBounds (area.getX(), y, 110, rowH);
            r.slider.setBounds (area.getX() + 115, y, area.getWidth() - 115, rowH);
            y += rowH + 2;
        }
        y += sectionGap;
    };

    placeSection (sectionLinear);
    placeRows (10);
    placeSection (sectionAnalog);
    placeRows (8);
    placeSection (sectionSynth);
    placeRows (6);

    if (idx < rows.size())
    {
        auto& r = *rows[idx++];
        r.label.setBounds (area.getX(), y, 110, rowH);
        r.slider.setBounds (area.getX() + 115, y, area.getWidth() - 115, rowH);
    }
}
