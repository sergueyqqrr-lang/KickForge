#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    SynthLayer
    ----------
    Capa de sintesis BASICA (a proposito minimal, para no inflar el
    proyecto) que enriquece el bombo ya ecualizado:

      - Detector de transiente simple (envelope follower + umbral) que
        dispara una nota cada vez que llega un golpe de bombo.
      - Oscilador seno/triangulo con "pitch envelope" (barrido de tono,
        estilo 808) y envolvente de amplitud (Attack/Decay simple).
      - Filtro paso-bajo de 1 polo para suavizar el oscilador.
      - Control de mezcla (mix) para sumarlo a la señal ya procesada.

    No es un sintetizador completo (sin polifonia, sin modulaciones
    avanzadas): es justo lo necesario para añadir cuerpo/sub extra.
*/
class SynthLayer
{
public:
    // --- Parametros expuestos ---
    std::atomic<float> mix          { 0.25f };  // 0..1, cuanto se suma a la señal original
    std::atomic<float> baseFreq     { 55.0f };  // Hz, tono de partida del oscilador
    std::atomic<float> pitchDrop    { 24.0f };  // semitonos que cae el pitch en el decay
    std::atomic<float> ampDecayMs   { 220.0f }; // duracion del golpe sintetizado
    std::atomic<float> filterCutoff { 900.0f }; // Hz
    std::atomic<float> threshold    { 0.35f };  // 0..1, sensibilidad del detector de transiente
    bool useTriangle = false;                   // false = seno, true = triangulo

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        envFollower = 0.0f;
        retrigGuardSamples = (int) (sampleRate * 0.06); // 60 ms anti-doble-disparo
        reset();
    }

    void reset()
    {
        phase = 0.0f;
        ampEnv = 0.0f;
        pitchEnvPos = 1.0f;
        active = false;
        retrigCounter = 0;
    }

    // Se llama DESPUES del EQ lineal + analogico, con el bloque ya procesado.
    // Detecta transientes en ese mismo bloque y suma la capa sintetica.
    void process (juce::dsp::AudioBlock<float>& block)
    {
        const int numCh = (int) block.getNumChannels();
        const int numSmp = (int) block.getNumSamples();
        const float thr = threshold.load();
        const float decaySec = juce::jmax (10.0f, ampDecayMs.load()) / 1000.0f;
        const float ampCoeff = std::exp (-1.0f / (decaySec * (float) sampleRate));
        const float pitchCoeff = std::exp (-1.0f / (0.5f * decaySec * (float) sampleRate));
        const float dropSemis = pitchDrop.load();
        const float base = baseFreq.load();
        const float cutoff = juce::jlimit (40.0f, 18000.0f, filterCutoff.load());
        const float rc = 1.0f / (2.0f * juce::MathConstants<float>::pi * cutoff);
        const float lpCoeff = (float) ((1.0 / sampleRate) / (rc + 1.0 / sampleRate));
        const float m = mix.load();

        for (int i = 0; i < numSmp; ++i)
        {
            // --- Deteccion de transiente sobre el canal 0 (mono-sum simple) ---
            float inSample = 0.0f;
            for (int ch = 0; ch < numCh; ++ch)
                inSample += std::abs (block.getChannelPointer ((size_t) ch)[i]);
            inSample /= (float) juce::jmax (1, numCh);

            envFollower += (inSample - envFollower) * 0.05f; // seguimiento rapido
            if (retrigCounter > 0) --retrigCounter;

            if (envFollower > thr && retrigCounter == 0)
            {
                active = true;
                ampEnv = 1.0f;
                pitchEnvPos = 1.0f;
                phase = 0.0f;
                retrigCounter = retrigGuardSamples;
            }

            float synthSample = 0.0f;
            if (active)
            {
                float freq = base * std::pow (2.0f, (dropSemis * pitchEnvPos) / 12.0f);
                float inc = juce::MathConstants<float>::twoPi * freq / (float) sampleRate;
                phase += inc;
                if (phase > juce::MathConstants<float>::twoPi)
                    phase -= juce::MathConstants<float>::twoPi;

                float raw = useTriangle
                              ? (2.0f / juce::MathConstants<float>::pi) * std::asin (std::sin (phase))
                              : std::sin (phase);

                lpState += (raw - lpState) * lpCoeff;
                synthSample = lpState * ampEnv;

                ampEnv *= ampCoeff;
                pitchEnvPos *= pitchCoeff;
                if (ampEnv < 0.0005f)
                    active = false;
            }

            // La capa sintetica se SUMA (no reemplaza) a la señal ya ecualizada,
            // escalada por "mix". Asi se "enriquece" el bombo sin perder el original.
            for (int ch = 0; ch < numCh; ++ch)
            {
                auto* data = block.getChannelPointer ((size_t) ch);
                data[i] += synthSample * m;
            }
        }
    }

private:
    double sampleRate = 44100.0;
    float phase = 0.0f, ampEnv = 0.0f, pitchEnvPos = 1.0f, lpState = 0.0f;
    float envFollower = 0.0f;
    bool active = false;
    int retrigGuardSamples = 0;
    int retrigCounter = 0;
};
