#pragma once
/// @file RtGuard.h
/// Detector de violaciones de tiempo real en builds de depuración (ADR 0004).
///
/// Con GP_RT_CHECKS=1 (presets dev/asan/tsan): un hook global de operator new/delete marca
/// como violación toda reserva o liberación hecha por un hilo que esté dentro de un
/// ScopedRealtimeSection. GP_RT_ASSERT_NOT_REALTIME() marca como violación código prohibido
/// (locks, I/O, logs) de NUESTRO código. Con GP_RT_CHECKS=0 (release) todo es no-op.
///
/// Garantías: todas las funciones son RT-safe y noexcept (contadores atómicos y flags
/// thread_local). El handler se llama en el hilo que viola: no puede reservar memoria,
/// bloquear ni hacer I/O.

#include <cstdint>

#ifndef GP_RT_CHECKS
#define GP_RT_CHECKS 0
#endif

namespace gp::rt {

enum class ViolationKind : std::uint8_t {
    allocation,     ///< operator new / new[] en sección RT.
    deallocation,   ///< operator delete / delete[] en sección RT.
    forbiddenCall,  ///< GP_RT_ASSERT_NOT_REALTIME() alcanzado en sección RT.
};

/// Handler RT-safe. Por defecto: solo cuenta (violationCount). Los tests pueden instalar uno
/// que aborte para obtener la traza. nullptr restaura el comportamiento por defecto.
using ViolationHandler = void (*)(ViolationKind kind) noexcept;

/// Marca el hilo actual como de tiempo real durante su vida (anidable). El motor lo coloca
/// al principio de cada callback de audio y del render offline.
class ScopedRealtimeSection {
public:
    ScopedRealtimeSection() noexcept;
    ~ScopedRealtimeSection() noexcept;
    ScopedRealtimeSection(const ScopedRealtimeSection&) = delete;
    ScopedRealtimeSection& operator=(const ScopedRealtimeSection&) = delete;
};

/// Suspende la detección en el hilo actual (anidable). Solo para código de terceros
/// auditado y documentado con un comentario que explique por qué; nunca en código nuevo.
class ScopedAllowViolations {
public:
    ScopedAllowViolations() noexcept;
    ~ScopedAllowViolations() noexcept;
    ScopedAllowViolations(const ScopedAllowViolations&) = delete;
    ScopedAllowViolations& operator=(const ScopedAllowViolations&) = delete;
};

/// true si el hilo actual está en una sección RT y la detección no está suspendida.
[[nodiscard]] bool isInRealtimeSection() noexcept;

/// Registra una violación (llama al handler e incrementa el contador global).
void reportViolation(ViolationKind kind) noexcept;

/// Total de violaciones desde el arranque o el último reset (todas las clases, todos los
/// hilos). La UI de debug lo muestra; los tests exigen 0.
[[nodiscard]] std::uint64_t violationCount() noexcept;
void resetViolationCount() noexcept;
void setViolationHandler(ViolationHandler handler) noexcept;

/// true si esta build tiene el hook de memoria activo (GP_RT_CHECKS=1).
[[nodiscard]] bool checksEnabled() noexcept;

}  // namespace gp::rt

#if GP_RT_CHECKS
/// Poner al principio de toda función nuestra que bloquee o haga I/O (locks, disco, logs).
#define GP_RT_ASSERT_NOT_REALTIME()                                            \
    do {                                                                       \
        if (::gp::rt::isInRealtimeSection())                                   \
            ::gp::rt::reportViolation(::gp::rt::ViolationKind::forbiddenCall); \
    } while (false)
#else
#define GP_RT_ASSERT_NOT_REALTIME() \
    do {                            \
    } while (false)
#endif
