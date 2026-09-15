# KickForge — EQ lineal + EQ analógico en serie + capa de síntesis para bombo

Plugin VST3/AU pensado para trabajar bombo (kick) en Studio One, con tres etapas **en serie**:

1. **EQ Lineal (fase cero)** — 4 bandas fijas orientadas a bombo (Sub Shelf, Punch, Corte de barro, Air Shelf). Al ser FIR de fase lineal, corrige frecuencias con total precisión sin torcer la fase ni "mover" los transientes.
2. **EQ Analógico (en serie)** — 3 bandas (low shelf / bell / high shelf) con saturación suave (control `Warmth`) que emula el comportamiento no lineal de un EQ analógico real, para dar calidez después de la corrección quirúrgica.
3. **Synth Layer (básica, a propósito)** — detecta el transiente del propio bombo y dispara un oscilador seno/triángulo con barrido de tono (pitch envelope) tipo 808, mezclado con `Mix` para reforzar el cuerpo/sub sin necesitar MIDI.

> ⚠️ **Importante sobre este entregable**: te doy el proyecto **fuente en JUCE (C++)**. No puedo generar el binario `.vst3`/`.component` final desde aquí porque compilar y firmar plugins de audio requiere Xcode (macOS) o Visual Studio (Windows) — herramientas que no están disponibles en este entorno. El código está completo y listo para compilar.

## Requisitos

- **CMake** ≥ 3.22
- **Xcode** (macOS) o **Visual Studio 2022** (Windows) con soporte C++17
- Conexión a internet la primera vez (CMake descarga JUCE automáticamente vía `FetchContent`)

## Compilar

```bash
cd KickForge
cmake -B build -G Xcode          # en macOS
# o en Windows:
# cmake -B build -G "Visual Studio 17 2022"

cmake --build build --config Release
```

Esto genera automáticamente los formatos `VST3`, `AU` (solo macOS) y `Standalone`. JUCE instala el `.vst3` en la carpeta estándar del sistema (`~/Library/Audio/Plug-Ins/VST3` en macOS o `C:\Program Files\Common Files\VST3` en Windows) al compilar en modo Release; si no aparece ahí, cópialo manualmente desde `build/KickForge_artefacts/Release/VST3/`.

Después, en Studio One: **Studio One → Preferencias → Ubicaciones → VST Plug-Ins** → verifica que la carpeta esté incluida → **Reescanear plugins**.

## Estructura del proyecto

```
KickForge/
├── CMakeLists.txt
└── Source/
    ├── PluginProcessor.h/.cpp   → cadena de señal y parámetros (APVTS)
    ├── PluginEditor.h/.cpp      → interfaz (sliders agrupados por sección)
    └── DSP/
        ├── LinearPhaseEQ.h      → FIR de fase lineal (diseño por muestreo en frecuencia)
        ├── AnalogEQ.h           → IIR + saturación tanh (calidez)
        └── SynthLayer.h         → detector de transiente + oscilador + envolventes
```

## Notas de diseño / cosas a ajustar a gusto

- El **EQ lineal** usa 2049 taps → añade **~23 ms de latencia** a 44.1 kHz (se reporta al host automáticamente con `setLatencySamples`, así que Studio One compensa el delay solo). Si quieres menos latencia a costa de algo menos de precisión en graves, baja `numTaps` en `LinearPhaseEQ.h`.
- El **detector de transiente** del Synth Layer es intencionalmente simple (envelope follower + umbral). Si el bombo tiene mucho bleed de otros elementos, puede disparar de más — ajusta `Threshold` o, si quieres, lo cambiamos a un input side-chain por MIDI en vez de detección de audio.
- Cada banda del EQ analógico satura de forma independiente (low → mid → high en cadena), lo que da un carácter más "consola" que si se saturara todo junto al final.
- La UI actual es funcional pero minimalista (sliders horizontales por parámetro) para mantener el proyecto compacto, tal como pediste para la sección de síntesis. Si luego quieres una interfaz gráfica más vistosa (knobs, medidor de reducción, etc.), se puede hacer como una segunda fase sin tocar el DSP.

## Próximos pasos posibles

- Añadir un analizador de espectro visual (antes/después de cada etapa).
- Hacer el número de bandas del EQ lineal configurable.
- Exportar presets de fábrica (Trap Kick, Rock Kick, 808 Sub, etc.).

Si quieres, puedo ayudarte a depurar errores de compilación si te aparecen al construirlo en Xcode/Visual Studio — solo pega el mensaje de error.
