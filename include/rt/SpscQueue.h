#pragma once
/// @file SpscQueue.h
/// Cola lock-free de un productor y un consumidor, capacidad fija (ADR 0004).
///
/// Garantías: tryPush/tryPop son wait-free, noexcept y no reservan memoria (RT-safe). El
/// almacenamiento está dentro del objeto: construir y destruir la cola FUERA del hilo de
/// audio. Exactamente un hilo productor y un hilo consumidor durante la vida de la cola
/// (pueden ser distintos en momentos distintos solo con sincronización externa, p. ej.
/// con el audio parado).
///
/// Implementación (motor-audio, F0-T3): definiciones inline al final de este archivo,
/// debajo del marcador. No cambiar las declaraciones sin pasar por el arquitecto.

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace gp::rt {

template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity >= 2 && (Capacity & (Capacity - 1)) == 0,
                  "Capacity debe ser potencia de 2");
    static_assert(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T> &&
                      std::is_nothrow_default_constructible_v<T>,
                  "T debe moverse y construirse por defecto sin lanzar");

public:
    SpscQueue() noexcept;
    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    /// Productor. false si está llena (el elemento no se mueve en ese caso).
    bool tryPush(T&& value) noexcept;
    bool tryPush(const T& value) noexcept;

    /// Consumidor. false si está vacía (`out` no cambia).
    bool tryPop(T& out) noexcept;

    /// Aproximado si lo llama un tercer hilo; exacto desde productor o consumidor para
    /// decidir "hay espacio"/"hay datos".
    [[nodiscard]] std::size_t sizeApprox() const noexcept;
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    // Índices en líneas de caché separadas para evitar false sharing.
    alignas(64) std::atomic<std::size_t> head_{0};  ///< Escribe el consumidor.
    alignas(64) std::atomic<std::size_t> tail_{0};  ///< Escribe el productor.
    alignas(64) std::array<T, Capacity> slots_{};
};

// ---- implementación (motor-audio, F0-T3) — solo definiciones debajo de esta línea ----

}  // namespace gp::rt
