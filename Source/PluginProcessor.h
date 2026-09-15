#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/LinearPhaseEQ.h"
#include "DSP/AnalogEQ.h"
#include "DSP/SynthLayer.h"

class KickForgeAudioProcessor  : public juce::AudioProcessor
{
public:
    KickForgeAudioProcessor();
    ~KickForgeAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KickForge"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void pullParametersIntoDSP();

    LinearPhaseEQ linearEQ;
    AnalogEQ      analogEQ;
    SynthLayer    synthLayer;

    bool bypassLinear = false;
    bool bypassAnalog = false;
    bool bypassSynth  = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KickForgeAudioProcessor)
};
