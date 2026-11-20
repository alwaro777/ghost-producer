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

// Índices monótonos (no se envuelven a Capacity): tail_ - head_ es el número de elementos y
// cabe en [0, Capacity]. El desbordamiento de size_t es aritmética modular y sigue siendo
// correcto porque Capacity es potencia de 2. Cada índice lo escribe un solo hilo: el dueño lo
// lee con relaxed y publica con release; el otro lado lo lee con acquire.

template <typename T, std::size_t Capacity>
SpscQueue<T, Capacity>::SpscQueue() noexcept {}

template <typename T, std::size_t Capacity>
bool SpscQueue<T, Capacity>::tryPush(T&& value) noexcept {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t head = head_.load(std::memory_order_acquire);
    if (tail - head >= Capacity)
        return false;  // llena: `value` no se toca
    slots_[tail & (Capacity - 1)] = std::move(value);
    tail_.store(tail + 1, std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
bool SpscQueue<T, Capacity>::tryPush(const T& value) noexcept {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);
    const std::size_t head = head_.load(std::memory_order_acquire);
    if (tail - head >= Capacity)
        return false;
    slots_[tail & (Capacity - 1)] = value;
    tail_.store(tail + 1, std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
bool SpscQueue<T, Capacity>::tryPop(T& out) noexcept {
    const std::size_t head = head_.load(std::memory_order_relaxed);
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    if (head == tail)
        return false;  // vacía: `out` no cambia
    out = std::move(slots_[head & (Capacity - 1)]);
    head_.store(head + 1, std::memory_order_release);
    return true;
}

template <typename T, std::size_t Capacity>
std::size_t SpscQueue<T, Capacity>::sizeApprox() const noexcept {
    // head primero: tail solo crece y nunca es menor que head, así que tail - head >= 0.
    const std::size_t head = head_.load(std::memory_order_acquire);
    const std::size_t tail = tail_.load(std::memory_order_acquire);
    const std::size_t size = tail - head;
    return size > Capacity ? Capacity : size;
}


}  // namespace gp::rt
