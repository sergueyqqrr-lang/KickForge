#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    LinearPhaseEQ
    -------------
    EQ de 4 bandas con fase lineal (FIR, diseño por muestreo en frecuencia).
    No introduce distorsión de fase: ideal para correcciones quirúrgicas
    antes de aplicar color analógico.

    Bandas pensadas para bombo:
      Band 0: Sub Shelf   (ej. 40-80 Hz)  -> peso/sub
      Band 1: Punch Bell  (ej. 60-120 Hz) -> cuerpo/pegada
      Band 2: Mud Bell    (ej. 250-500Hz) -> corte de "barro"
      Band 3: Air Shelf   (ej. 3-10 kHz)  -> click del batidor / aire

    Introduce latencia = (numTaps-1)/2 muestras, reportada automáticamente
    al host mediante setLatencySamples() en el processor.
*/
class LinearPhaseEQ
{
public:
    struct Band
    {
        std::atomic<float> freq { 100.0f };
        std::atomic<float> gainDb { 0.0f };
        std::atomic<float> q { 0.7f };
        bool isShelf = false;
        bool isLowShelf = false;
    };

    static constexpr int numBands = 4;
    static constexpr int fftOrder = 12;            // 4096 puntos
    static constexpr int fftSize  = 1 << fftOrder;
    static constexpr int numTaps  = 2049;          // impar -> fase lineal tipo I

    Band bands[numBands];

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        bands[0].isShelf = true;  bands[0].isLowShelf = true;  bands[0].freq = 55.0f;  bands[0].gainDb = 2.0f;
        bands[1].isShelf = false;                               bands[1].freq = 85.0f;  bands[1].gainDb = 0.0f; bands[1].q = 1.0f;
        bands[2].isShelf = false;                               bands[2].freq = 350.0f; bands[2].gainDb = -3.0f; bands[2].q = 1.1f;
        bands[3].isShelf = true;  bands[3].isLowShelf = false; bands[3].freq = 5000.0f; bands[3].gainDb = 1.5f;

        filter.prepare (spec);
        recomputeTaps();
        filter.coefficients = coeffs;
    }

    void reset() { filter.reset(); }

    int getLatencySamples() const { return (numTaps - 1) / 2; }

    void process (juce::dsp::AudioBlock<float>& block)
    {
        if (needsUpdate.exchange (false))
        {
            recomputeTaps();
            filter.coefficients = coeffs;
        }
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        filter.process (ctx);
    }

    // Llamar cuando cambie algún parámetro desde el editor/APVTS
    void markDirty() { needsUpdate.store (true); }

private:
    double sampleRate = 44100.0;
    std::atomic<bool> needsUpdate { true };
    juce::dsp::FIR::Coefficients<float>::Ptr coeffs = new juce::dsp::FIR::Coefficients<float> (numTaps);
    juce::dsp::FIR::Filter<float> filter;

    // Respuesta en dB de una banda tipo "shelf" en la frecuencia f (Hz)
    static float shelfDb (float f, float freq, float gainDb, bool isLow)
    {
        float ratio = f / juce::jmax (1.0f, freq);
        float x = isLow ? (1.0f / (1.0f + ratio * ratio)) : (ratio * ratio / (1.0f + ratio * ratio));
        return gainDb * x;
    }

    // Respuesta en dB de una campana (bell) tipo peaking, aproximación gaussiana en escala log
    static float bellDb (float f, float freq, float gainDb, float q)
    {
        if (f <= 0.0f || freq <= 0.0f) return 0.0f;
        float bw = 1.0f / juce::jmax (0.05f, q); // ancho relativo en octavas aprox.
        float logRatio = std::log2 (f / freq);
        float x = std::exp (-(logRatio * logRatio) / (2.0f * bw * bw));
        return gainDb * x;
    }

    void recomputeTaps()
    {
        const int half = fftSize / 2;
        std::vector<float> magDb (half + 1, 0.0f);

        for (int bin = 0; bin <= half; ++bin)
        {
            float f = (float) bin * (float) sampleRate / (float) fftSize;
            float db = 0.0f;
            for (auto& b : bands)
            {
                float freq = b.freq.load();
                float gain = b.gainDb.load();
                if (std::abs (gain) < 0.001f) continue;
                if (b.isShelf)
                    db += shelfDb (f, freq, gain, b.isLowShelf);
                else
                    db += bellDb (f, freq, gain, b.q.load());
            }
            magDb[(size_t) bin] = db;
        }

        // Construir espectro completo (simétrico, fase cero) y hacer IFFT
        juce::dsp::FFT fft (fftOrder);
        std::vector<std::complex<float>> spectrum (fftSize, { 0.0f, 0.0f });
        for (int bin = 0; bin <= half; ++bin)
        {
            float mag = juce::Decibels::decibelsToGain (magDb[(size_t) bin]);
            spectrum[(size_t) bin] = { mag, 0.0f };
            if (bin > 0 && bin < half)
                spectrum[(size_t) (fftSize - bin)] = { mag, 0.0f }; // simetría conjugada (real)
        }

        std::vector<std::complex<float>> timeDomain (fftSize);
        fft.perform (spectrum.data(), timeDomain.data(), true); // IFFT

        // Circular shift para hacerlo causal y tomar numTaps centrales, con ventana Hann
        std::vector<float> full (fftSize, 0.0f);
        for (int n = 0; n < fftSize; ++n)
            full[(size_t) n] = timeDomain[(size_t) ((n + fftSize / 2) % fftSize)].real();

        int start = fftSize / 2 - numTaps / 2;
        auto newCoeffs = new juce::dsp::FIR::Coefficients<float> (numTaps);
        float sum = 0.0f;
        for (int i = 0; i < numTaps; ++i)
        {
            float window = 0.5f - 0.5f * std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (numTaps - 1)); // Hann
            float val = full[(size_t) (start + i)] * window;
            newCoeffs->coefficients.getRawDataPointer()[i] = val;
            sum += val;
        }
        // Normalizar ganancia DC a ~unidad relativa (evita drift de nivel por el enventanado)
        if (std::abs (sum) > 0.0001f)
        {
            float norm = 1.0f / sum;
            for (int i = 0; i < numTaps; ++i)
                newCoeffs->coefficients.getRawDataPointer()[i] *= norm;
        }

        coeffs = newCoeffs;
    }
};
