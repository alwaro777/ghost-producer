/// @file RtGuard.cpp
/// Detector RT (ADR 0004). Contiene, en la MISMA unidad de compilación que
/// ScopedRealtimeSection, el reemplazo global de operator new/delete: cualquier ejecutable que
/// use una sección RT arrastra este .o desde la librería estática y, con él, el hook.
///
/// Con GP_RT_CHECKS == 0 no se reemplaza nada y todas las funciones son no-op.

#include "rt/RtGuard.h"

#include <atomic>
#include <cstdint>

#if GP_RT_CHECKS
#include <cstddef>
#include <cstdlib>
#include <new>
#endif

namespace gp::rt {

#if GP_RT_CHECKS

namespace {

// constinit + tipo trivial: el acceso TLS no necesita inicializador dinámico ni reserva, así
// que es seguro dentro de operator new y antes de main.
constinit thread_local std::uint32_t tlRealtimeDepth = 0;
constinit thread_local std::uint32_t tlAllowDepth = 0;

constinit std::atomic<std::uint64_t> gViolations{0};
constinit std::atomic<ViolationHandler> gHandler{nullptr};

}  // namespace

ScopedRealtimeSection::ScopedRealtimeSection() noexcept { ++tlRealtimeDepth; }
ScopedRealtimeSection::~ScopedRealtimeSection() noexcept { --tlRealtimeDepth; }

ScopedAllowViolations::ScopedAllowViolations() noexcept { ++tlAllowDepth; }
ScopedAllowViolations::~ScopedAllowViolations() noexcept { --tlAllowDepth; }

bool isInRealtimeSection() noexcept { return tlRealtimeDepth > 0 && tlAllowDepth == 0; }

void reportViolation(ViolationKind kind) noexcept {
    gViolations.fetch_add(1, std::memory_order_relaxed);
    if (const ViolationHandler handler = gHandler.load(std::memory_order_acquire)) {
        // Si el handler reserva (no debería), no queremos recursión infinita en el hook.
        ++tlAllowDepth;
        handler(kind);
        --tlAllowDepth;
    }
}

std::uint64_t violationCount() noexcept { return gViolations.load(std::memory_order_relaxed); }
void resetViolationCount() noexcept { gViolations.store(0, std::memory_order_relaxed); }
void setViolationHandler(ViolationHandler handler) noexcept {
    gHandler.store(handler, std::memory_order_release);
}
bool checksEnabled() noexcept { return true; }

#else  // GP_RT_CHECKS == 0: coste cero, sin hook.

ScopedRealtimeSection::ScopedRealtimeSection() noexcept {}
ScopedRealtimeSection::~ScopedRealtimeSection() noexcept {}
ScopedAllowViolations::ScopedAllowViolations() noexcept {}
ScopedAllowViolations::~ScopedAllowViolations() noexcept {}
bool isInRealtimeSection() noexcept { return false; }
void reportViolation(ViolationKind) noexcept {}
std::uint64_t violationCount() noexcept { return 0; }
void resetViolationCount() noexcept {}
void setViolationHandler(ViolationHandler) noexcept {}
bool checksEnabled() noexcept { return false; }

#endif

}  // namespace gp::rt

#if GP_RT_CHECKS

// ---- Reemplazo global de operator new/delete ---------------------------------------------
// Todas las variantes replaceable (C++17/20): simple, array, nothrow, aligned, sized. Las
// variantes por defecto de la biblioteca no se mezclan con las nuestras: todo pasa por
// std::malloc / std::free (o _aligned_malloc/_aligned_free en Windows para las alineadas).
// liberar nullptr no cuenta como violación (no toca el asignador).

namespace {

inline void onAllocate() noexcept {
    if (gp::rt::isInRealtimeSection())
        gp::rt::reportViolation(gp::rt::ViolationKind::allocation);
}

inline void onDeallocate(void* ptr) noexcept {
    if (ptr != nullptr && gp::rt::isInRealtimeSection())
        gp::rt::reportViolation(gp::rt::ViolationKind::deallocation);
}

void* rawAllocate(std::size_t size) noexcept { return std::malloc(size == 0 ? 1 : size); }

void* rawAllocateAligned(std::size_t size, std::size_t alignment) noexcept {
    if (size == 0)
        size = 1;
#if defined(_WIN32)
    return _aligned_malloc(size, alignment);
#else
    if (alignment < sizeof(void*))
        alignment = sizeof(void*);
    void* ptr = nullptr;
    return posix_memalign(&ptr, alignment, size) == 0 ? ptr : nullptr;
#endif
}

void rawFree(void* ptr) noexcept { std::free(ptr); }

void rawFreeAligned(void* ptr) noexcept {
#if defined(_WIN32)
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

// Semántica estándar: reintentar con new_handler; sin handler, bad_alloc.
void* allocateOrThrow(std::size_t size) {
    onAllocate();
    for (;;) {
        if (void* ptr = rawAllocate(size))
            return ptr;
        std::new_handler handler = std::get_new_handler();
        if (handler == nullptr)
            throw std::bad_alloc();
        handler();
    }
}

void* allocateAlignedOrThrow(std::size_t size, std::align_val_t alignment) {
    onAllocate();
    for (;;) {
        if (void* ptr = rawAllocateAligned(size, static_cast<std::size_t>(alignment)))
            return ptr;
        std::new_handler handler = std::get_new_handler();
        if (handler == nullptr)
            throw std::bad_alloc();
        handler();
    }
}

void* allocateNoThrow(std::size_t size) noexcept {
    try {
        return allocateOrThrow(size);
    } catch (...) {
        return nullptr;
    }
}

void* allocateAlignedNoThrow(std::size_t size, std::align_val_t alignment) noexcept {
    try {
        return allocateAlignedOrThrow(size, alignment);
    } catch (...) {
        return nullptr;
    }
}

void deallocate(void* ptr) noexcept {
    onDeallocate(ptr);
    rawFree(ptr);
}

void deallocateAligned(void* ptr) noexcept {
    onDeallocate(ptr);
    rawFreeAligned(ptr);
}

}  // namespace

// NOLINTBEGIN(misc-new-delete-overloads)
void* operator new(std::size_t size) { return allocateOrThrow(size); }
void* operator new[](std::size_t size) { return allocateOrThrow(size); }
void* operator new(std::size_t size, const std::nothrow_t&) noexcept { return allocateNoThrow(size); }
void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    return allocateNoThrow(size);
}
void* operator new(std::size_t size, std::align_val_t al) { return allocateAlignedOrThrow(size, al); }
void* operator new[](std::size_t size, std::align_val_t al) {
    return allocateAlignedOrThrow(size, al);
}
void* operator new(std::size_t size, std::align_val_t al, const std::nothrow_t&) noexcept {
    return allocateAlignedNoThrow(size, al);
}
void* operator new[](std::size_t size, std::align_val_t al, const std::nothrow_t&) noexcept {
    return allocateAlignedNoThrow(size, al);
}

void operator delete(void* ptr) noexcept { deallocate(ptr); }
void operator delete[](void* ptr) noexcept { deallocate(ptr); }
void operator delete(void* ptr, const std::nothrow_t&) noexcept { deallocate(ptr); }
void operator delete[](void* ptr, const std::nothrow_t&) noexcept { deallocate(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { deallocate(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { deallocate(ptr); }
void operator delete(void* ptr, std::align_val_t) noexcept { deallocateAligned(ptr); }
void operator delete[](void* ptr, std::align_val_t) noexcept { deallocateAligned(ptr); }
void operator delete(void* ptr, std::align_val_t, const std::nothrow_t&) noexcept {
    deallocateAligned(ptr);
}
void operator delete[](void* ptr, std::align_val_t, const std::nothrow_t&) noexcept {
    deallocateAligned(ptr);
}
void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept { deallocateAligned(ptr); }
void operator delete[](void* ptr, std::size_t, std::align_val_t) noexcept {
    deallocateAligned(ptr);
}
// NOLINTEND(misc-new-delete-overloads)

#endif  // GP_RT_CHECKS
