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

// Propiedad: cada puntero lo posee exactamente un lado. pending_ se intercambia con exchange
// (RMW atómico), así que una instantánea pendiente la recoge o el lector (acquire) o el
// escritor (publish siguiente), nunca ambos. Las retiradas viajan lector → escritor por la
// SPSC (release/acquire), de modo que el escritor ve terminadas todas las lecturas antes de
// destruir.

template <typename T, std::size_t RetireCapacity>
SnapshotExchange<T, RetireCapacity>::SnapshotExchange() noexcept {}

template <typename T, std::size_t RetireCapacity>
SnapshotExchange<T, RetireCapacity>::~SnapshotExchange() {
    collectGarbage();
    delete pending_.exchange(nullptr, std::memory_order_acquire);
    delete current_;
    current_ = nullptr;
}

template <typename T, std::size_t RetireCapacity>
void SnapshotExchange<T, RetireCapacity>::publish(std::unique_ptr<const T> snapshot) {
    const T* fresh = snapshot.release();
    // acq_rel: release publica la construcción de `fresh`; acquire sincroniza con el lector
    // por si la anterior la había dejado él (no ocurre: el lector solo pone nullptr).
    const T* unseen = pending_.exchange(fresh, std::memory_order_acq_rel);
    delete unseen;  // nunca la vio el lector
}

template <typename T, std::size_t RetireCapacity>
const T* SnapshotExchange<T, RetireCapacity>::acquire() noexcept {
    if (pending_.load(std::memory_order_relaxed) == nullptr)
        return current_;
    // Desde el productor, sizeApprox es exacto para decidir "hay espacio" (el consumidor
    // solo puede liberar más). Sin hueco: seguimos con la actual y la pendiente espera.
    if (current_ != nullptr && retired_.sizeApprox() >= RetireCapacity)
        return current_;
    const T* next = pending_.exchange(nullptr, std::memory_order_acq_rel);
    if (next == nullptr)
        return current_;  // defensivo: solo el lector vacía pending_
    if (current_ != nullptr) {
        const bool pushed = retired_.tryPush(current_);
        (void)pushed;  // garantizado por la comprobación de espacio anterior
    }
    current_ = next;
    return current_;
}

template <typename T, std::size_t RetireCapacity>
void SnapshotExchange<T, RetireCapacity>::collectGarbage() {
    const T* old = nullptr;
    while (retired_.tryPop(old))
        delete old;
}


}  // namespace gp::rt
