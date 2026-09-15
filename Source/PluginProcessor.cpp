#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ID
{
    // Linear EQ (4 bandas)
    static const juce::String linF[4] = { "linFreq0", "linFreq1", "linFreq2", "linFreq3" };
    static const juce::String linG[4] = { "linGain0", "linGain1", "linGain2", "linGain3" };
    static const juce::String linQ[4] = { "linQ0", "linQ1", "linQ2", "linQ3" };
    // Analog EQ (3 bandas)
    static const juce::String anF[3] = { "anFreqLow", "anFreqMid", "anFreqHigh" };
    static const juce::String anG[3] = { "anGainLow", "anGainMid", "anGainHigh" };
    static const juce::String anQmid  = "anQMid";
    static const juce::String anWarmth = "anWarmth";
    // Synth layer
    static const juce::String synMix      = "synMix";
    static const juce::String synBaseFreq = "synBaseFreq";
    static const juce::String synDrop     = "synPitchDrop";
    static const juce::String synDecay    = "synDecayMs";
    static const juce::String synCutoff   = "synCutoff";
    static const juce::String synThresh   = "synThreshold";
    static const juce::String synWave     = "synWave";
    // Bypass / global
    static const juce::String bypLin = "bypassLinear";
    static const juce::String bypAn  = "bypassAnalog";
    static const juce::String bypSyn = "bypassSynth";
    static const juce::String outGain = "outputGain";
}

KickForgeAudioProcessor::KickForgeAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout KickForgeAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    // ---- EQ Lineal: 4 bandas ----
    struct LinDefault { float f, g, q; const char* name; };
    LinDefault linDefaults[4] = {
        { 55.0f,   2.0f, 0.7f, "Sub Shelf" },
        { 85.0f,   0.0f, 1.0f, "Punch" },
        { 350.0f, -3.0f, 1.1f, "Mud Cut" },
        { 5000.0f, 1.5f, 0.7f, "Air Shelf" }
    };
    for (int i = 0; i < 4; ++i)
    {
        p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::linF[i], juce::String (linDefaults[i].name) + " Freq",
            juce::NormalisableRange<float> (20.0f, 12000.0f, 1.0f, 0.3f), linDefaults[i].f));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::linG[i], juce::String (linDefaults[i].name) + " Gain",
            juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), linDefaults[i].g));
        p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::linQ[i], juce::String (linDefaults[i].name) + " Q",
            juce::NormalisableRange<float> (0.1f, 8.0f, 0.01f, 0.5f), linDefaults[i].q));
    }

    // ---- EQ Analogico: 3 bandas + warmth ----
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anF[0], "Analog Low Freq", juce::NormalisableRange<float> (20.0f, 500.0f, 1.0f, 0.4f), 70.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anG[0], "Analog Low Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 1.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anF[1], "Analog Mid Freq", juce::NormalisableRange<float> (100.0f, 4000.0f, 1.0f, 0.3f), 800.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anG[1], "Analog Mid Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.5f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anQmid, "Analog Mid Q", juce::NormalisableRange<float> (0.2f, 5.0f, 0.01f, 0.5f), 0.8f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anF[2], "Analog High Freq", juce::NormalisableRange<float> (1000.0f, 16000.0f, 1.0f, 0.3f), 4500.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anG[2], "Analog High Gain", juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.8f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::anWarmth, "Warmth (Saturation)", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));

    // ---- Capa de sintetizador (basica) ----
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::synMix, "Synth Mix", juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::synBaseFreq, "Synth Base Freq", juce::NormalisableRange<float> (30.0f, 150.0f, 0.1f), 55.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::synDrop, "Synth Pitch Drop (st)", juce::NormalisableRange<float> (0.0f, 48.0f, 0.1f), 24.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::synDecay, "Synth Decay (ms)", juce::NormalisableRange<float> (20.0f, 800.0f, 1.0f, 0.5f), 220.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::synCutoff, "Synth Filter Cutoff", juce::NormalisableRange<float> (60.0f, 8000.0f, 1.0f, 0.4f), 900.0f));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::synThresh, "Synth Trigger Threshold", juce::NormalisableRange<float> (0.02f, 0.9f, 0.001f), 0.35f));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (ID::synWave, "Synth Waveform", juce::StringArray { "Sine", "Triangle" }, 0));

    // ---- Bypass / global ----
    p.push_back (std::make_unique<juce::AudioParameterBool> (ID::bypLin, "Bypass Linear EQ", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (ID::bypAn,  "Bypass Analog EQ", false));
    p.push_back (std::make_unique<juce::AudioParameterBool> (ID::bypSyn, "Bypass Synth Layer", false));
    p.push_back (std::make_unique<juce::AudioParameterFloat> (ID::outGain, "Output Gain", juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f));

    return { p.begin(), p.end() };
}

void KickForgeAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, (juce::uint32) getTotalNumOutputChannels() };
    linearEQ.prepare (spec);
    analogEQ.prepare (spec);
    synthLayer.prepare (spec);
    setLatencySamples (linearEQ.getLatencySamples());
}

bool KickForgeAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    auto mono = juce::AudioChannelSet::mono();
    auto stereo = juce::AudioChannelSet::stereo();
    auto set = layouts.getMainOutputChannelSet();
    return (set == mono || set == stereo) && layouts.getMainInputChannelSet() == set;
}

void KickForgeAudioProcessor::pullParametersIntoDSP()
{
    for (int i = 0; i < 4; ++i)
    {
        linearEQ.bands[i].freq  = apvts.getRawParameterValue (ID::linF[i])->load();
        linearEQ.bands[i].gainDb = apvts.getRawParameterValue (ID::linG[i])->load();
        linearEQ.bands[i].q     = apvts.getRawParameterValue (ID::linQ[i])->load();
    }
    linearEQ.markDirty();

    analogEQ.low.freq   = apvts.getRawParameterValue (ID::anF[0])->load();
    analogEQ.low.gainDb  = apvts.getRawParameterValue (ID::anG[0])->load();
    analogEQ.mid.freq   = apvts.getRawParameterValue (ID::anF[1])->load();
    analogEQ.mid.gainDb  = apvts.getRawParameterValue (ID::anG[1])->load();
    analogEQ.mid.q      = apvts.getRawParameterValue (ID::anQmid)->load();
    analogEQ.high.freq  = apvts.getRawParameterValue (ID::anF[2])->load();
    analogEQ.high.gainDb = apvts.getRawParameterValue (ID::anG[2])->load();
    analogEQ.warmth     = apvts.getRawParameterValue (ID::anWarmth)->load();
    analogEQ.update();

    synthLayer.mix          = apvts.getRawParameterValue (ID::synMix)->load();
    synthLayer.baseFreq     = apvts.getRawParameterValue (ID::synBaseFreq)->load();
    synthLayer.pitchDrop    = apvts.getRawParameterValue (ID::synDrop)->load();
    synthLayer.ampDecayMs   = apvts.getRawParameterValue (ID::synDecay)->load();
    synthLayer.filterCutoff = apvts.getRawParameterValue (ID::synCutoff)->load();
    synthLayer.threshold    = apvts.getRawParameterValue (ID::synThresh)->load();
    synthLayer.useTriangle  = apvts.getRawParameterValue (ID::synWave)->load() > 0.5f;

    bypassLinear = apvts.getRawParameterValue (ID::bypLin)->load() > 0.5f;
    bypassAnalog = apvts.getRawParameterValue (ID::bypAn)->load() > 0.5f;
    bypassSynth  = apvts.getRawParameterValue (ID::bypSyn)->load() > 0.5f;
}

void KickForgeAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    pullParametersIntoDSP();

    juce::dsp::AudioBlock<float> block (buffer);

    // ---- 1) EQ lineal (precision quirurgica, fase cero) ----
    if (! bypassLinear)
        linearEQ.process (block);

    // ---- 2) EQ analogico EN SERIE (calidez / saturacion suave) ----
    if (! bypassAnalog)
        analogEQ.process (block);

    // ---- 3) Capa de sintetizador (enriquecimiento adicional) ----
    if (! bypassSynth)
        synthLayer.process (block);

    // ---- Ganancia de salida ----
    float outGainDb = apvts.getRawParameterValue (ID::outGain)->load();
    if (std::abs (outGainDb) > 0.001f)
        block.multiplyBy (juce::Decibels::decibelsToGain (outGainDb));
}

juce::AudioProcessorEditor* KickForgeAudioProcessor::createEditor()
{
    return new KickForgeAudioProcessorEditor (*this);
}

void KickForgeAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); true)
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void KickForgeAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new KickForgeAudioProcessor();
}
