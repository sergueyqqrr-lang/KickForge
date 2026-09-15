#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    AnalogEQ
    --------
    Etapa de "color" analógico que se aplica EN SERIE después del EQ lineal.
    Usa 3 filtros IIR (low shelf, bell, high shelf) tipo console/API-ish,
    cada uno seguido de una saturación suave (tanh) que emula la no
    linealidad de un EQ analógico real y da "suavidad" al sonido.

    - warmth (0-1): cantidad de saturación aplicada en cada banda.
    - Los 3 controles freq/gain/q por banda permiten dar forma al color.
*/
class AnalogEQ
{
public:
    struct Band
    {
        std::atomic<float> freq { 100.0f };
        std::atomic<float> gainDb { 0.0f };
        std::atomic<float> q { 0.7f };
    };

    Band low, mid, high;
    std::atomic<float> warmth { 0.35f }; // 0..1

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        low.freq = 70.0f;  low.gainDb = 1.0f;
        mid.freq = 800.0f; mid.gainDb = 0.5f; mid.q = 0.8f;
        high.freq = 4500.0f; high.gainDb = 0.8f;

        lowFilter.prepare (spec);
        midFilter.prepare (spec);
        highFilter.prepare (spec);
        update();
    }

    void reset()
    {
        lowFilter.reset();
        midFilter.reset();
        highFilter.reset();
    }

    // Llamar tras cambiar parámetros
    void update()
    {
        *lowFilter.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sampleRate, low.freq.load(),  0.707f, juce::Decibels::decibelsToGain (low.gainDb.load()));
        *midFilter.state  = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, mid.freq.load(),  mid.q.load(), juce::Decibels::decibelsToGain (mid.gainDb.load()));
        *highFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, high.freq.load(), 0.707f, juce::Decibels::decibelsToGain (high.gainDb.load()));
    }

    void process (juce::dsp::AudioBlock<float>& block)
    {
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        lowFilter.process (ctx);
        saturateBlock (block);
        midFilter.process (ctx);
        saturateBlock (block);
        highFilter.process (ctx);
        saturateBlock (block);
    }

private:
    double sampleRate = 44100.0;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> lowFilter, midFilter, highFilter;

    void saturateBlock (juce::dsp::AudioBlock<float>& block)
    {
        float w = warmth.load();
        if (w < 0.001f) return;
        float drive = 1.0f + w * 4.0f; // más warmth -> más unidad de drive
        float makeup = 1.0f / std::tanh (drive * 0.6f); // compensa pérdida de nivel percibida

        for (size_t ch = 0; ch < block.getNumChannels(); ++ch)
        {
            auto* data = block.getChannelPointer (ch);
            for (size_t i = 0; i < block.getNumSamples(); ++i)
            {
                float x = data[i] * drive;
                float y = std::tanh (x);
                data[i] = juce::jmap (w, data[i], y * makeup * (1.0f / drive));
            }
        }
    }
};
