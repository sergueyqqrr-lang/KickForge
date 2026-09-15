#pragma once
#include "PluginProcessor.h"

/**
    Editor simple organizado en 3 secciones (Linear EQ / Analog EQ / Synth).
    Usa sliders genericos ligados via APVTS::SliderAttachment para mantener
    el proyecto compacto; se puede reemplazar despues por una UI a medida.
*/
class KickForgeAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit KickForgeAudioProcessorEditor (KickForgeAudioProcessor&);
    ~KickForgeAudioProcessorEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    KickForgeAudioProcessor& processor;

    struct Row
    {
        juce::Label label;
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    std::vector<std::unique_ptr<Row>> rows;
    juce::Label sectionLinear, sectionAnalog, sectionSynth;

    void addRow (const juce::String& paramID, const juce::String& labelText);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickForgeAudioProcessorEditor)
};
