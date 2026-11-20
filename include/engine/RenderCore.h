#pragma once
/// @file RenderCore.h
/// Núcleo de render independiente del dispositivo: el MISMO código procesa el callback en
/// tiempo real y el render offline (tests golden, exportación futura). Sin JUCE en la API.
///
/// Hilos:
///  - prepare/release/publish/collectGarbage: hilo de mensajes (o el hilo de control del
///    render offline). prepare/release solo con process() parado.
///  - process: hilo de audio. RT-safe: sin reservas, locks, I/O ni logs; noexcept.
///  - takePeaks: cualquier hilo (atómicos).
///
/// Garantías de audio:
///  - Cambios de frecuencia y ganancia, y activar/desactivar el tono, con rampas de 5–20 ms:
///    sin discontinuidades (clics). Fase continua al cambiar la frecuencia.
///  - process() SOBRESCRIBE todos los canales de salida (no acumula).
///  - Determinista: misma secuencia de (snapshots, tamaños de bloque) ⇒ mismas muestras,
///    con independencia del tamaño de bloque salvo por el instante en que se toma cada
///    snapshot (al inicio de cada bloque).
///  - Antes del primer publish(), o con el tono desactivado, produce silencio (0.0f exacto).

#include <memory>

#include "engine/EngineSnapshot.h"

namespace gp::engine {

struct ProcessSpec {
    double sampleRate = 48000.0;  ///< 44100–192000.
    int maxBlockSize = 512;       ///< process() nunca recibirá más muestras que esto.
    int numOutputChannels = 2;    ///< 1..32.
};

/// Picos absolutos (lineales) desde la última lectura, para los medidores.
struct PeakReadings {
    float left = 0.0f;
    float right = 0.0f;  ///< Igual a left si la salida es mono.
};

class IRenderCore {
public:
    virtual ~IRenderCore() = default;

    /// Puede reservar memoria. Reinicia fase y rampas.
    virtual void prepare(const ProcessSpec& spec) = 0;
    virtual void release() = 0;

    /// Entrega una nueva instantánea; la toma el siguiente process(). No bloquea.
    virtual void publish(std::unique_ptr<const EngineSnapshot> snapshot) = 0;
    /// Libera instantáneas retiradas por el audio. Llamar ≥10 Hz desde el hilo de mensajes.
    virtual void collectGarbage() = 0;

    /// Rellena `numSamples` (≤ maxBlockSize) en `numChannels` (≤ numOutputChannels) canales.
    virtual void process(float* const* outputs, int numChannels, int numSamples) noexcept = 0;

    /// Devuelve y pone a cero los picos acumulados.
    [[nodiscard]] virtual PeakReadings takePeaks() noexcept = 0;
};

[[nodiscard]] std::unique_ptr<IRenderCore> createRenderCore();

}  // namespace gp::engine
