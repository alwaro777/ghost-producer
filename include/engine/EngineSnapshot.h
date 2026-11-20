#pragma once
/// @file EngineSnapshot.h
/// Lo que el hilo de audio necesita saber del proyecto, ya "compilado" (ADR 0004).
/// Inmutable una vez publicado. Se construye en el hilo de mensajes a partir del
/// model::ProjectState y se entrega con rt::SnapshotExchange.
///
/// Punto de extensión: en F1 contendrá el grafo de procesamiento compilado (nodos, buffers
/// preasignados, PDC). Debe seguir siendo un objeto que el audio solo LEE.

#include <cstdint>

#include "model/ProjectState.h"

namespace gp::engine {

struct EngineSnapshot {
    std::uint64_t documentRevision = 0;  ///< model::Document::revision() de origen.
    bool testToneEnabled = false;
    double testToneFrequencyHz = 440.0;
    float testToneGainLinear = 0.0f;  ///< Ya convertido de dB (dB → lineal fuera del audio).
};

/// Compilador modelo → motor. Puro y determinista. Hilo: cualquiera salvo audio.
[[nodiscard]] EngineSnapshot buildSnapshot(const model::ProjectState& state,
                                           std::uint64_t documentRevision);

}  // namespace gp::engine
