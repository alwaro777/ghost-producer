#pragma once
/// @file SnapshotExchange.h
/// Entrega de instantáneas inmutables de un escritor (no RT) a un lector de audio, sin
/// locks ni liberaciones de memoria en el hilo de audio (ADR 0004).
///
/// Protocolo:
///  - publish(): el escritor deja la nueva instantánea "pendiente". Si había otra pendiente
///    que el lector aún no tomó, el escritor la destruye (nunca la vio el lector).
///  - acquire(): al empezar cada bloque, el lector toma la pendiente (si la hay), la hace
///    "actual" y mete la anterior en la cola de retiro. Si la cola de retiro está llena,
///    conserva la actual y deja la pendiente para el siguiente bloque.
///  - collectGarbage(): el escritor destruye lo retirado. Llamarlo con frecuencia (≥10 Hz)
///    y siempre antes de publish().
/// Invariante: el hilo de audio nunca ejecuta el destructor de T.
///
/// Implementación (motor-audio, F0-T3): definiciones inline debajo del marcador.

#include <atomic>
#include <cstddef>
#include <memory>

#include "rt/SpscQueue.h"

namespace gp::rt {

template <typename T, std::size_t RetireCapacity = 64>
class SnapshotExchange {
public:
    SnapshotExchange() noexcept;
    /// Destruye actual, pendiente y retiradas. Solo con el lector parado.
    ~SnapshotExchange();
    SnapshotExchange(const SnapshotExchange&) = delete;
    SnapshotExchange& operator=(const SnapshotExchange&) = delete;

    /// Escritor (un único hilo no RT). Toma la propiedad. Nunca bloquea.
    void publish(std::unique_ptr<const T> snapshot);

    /// Lector (hilo de audio). RT-safe, wait-free. Devuelve la instantánea actual
    /// (nullptr antes del primer publish). El puntero es válido hasta la siguiente
    /// llamada a acquire() desde el lector.
    [[nodiscard]] const T* acquire() noexcept;

    /// Escritor. Libera las instantáneas retiradas por el lector.
    void collectGarbage();

private:
    std::atomic<const T*> pending_{nullptr};  ///< Propiedad: escritor ↔ lector vía exchange.
    const T* current_ = nullptr;              ///< Solo lo toca el lector.
    SpscQueue<const T*, RetireCapacity> retired_;  ///< Lector → escritor.
};

// ---- implementación (motor-audio, F0-T3) — solo definiciones debajo de esta línea ----

}  // namespace gp::rt
