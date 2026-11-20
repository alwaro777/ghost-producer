// Tests del detector RT (F0-T3). Provocan violaciones a propósito.
// Regla: nada de REQUIRE/CHECK dentro de una sección RT (Catch2 reserva memoria); se capturan
// los valores dentro y se comprueban fuera.

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <new>
#include <thread>

#include "rt/RtGuard.h"
#include "rt/SnapshotExchange.h"
#include "rt/SpscQueue.h"

namespace {

using gp::rt::ViolationKind;

std::atomic<int> gLastKind{-1};
std::atomic<int> gHandlerCalls{0};

void recordingHandler(ViolationKind kind) noexcept {
    gLastKind.store(static_cast<int>(kind), std::memory_order_relaxed);
    gHandlerCalls.fetch_add(1, std::memory_order_relaxed);
}

// volatile: impide que el compilador elimine el par new/delete (elisión permitida).
int* volatile gSink = nullptr;

struct HandlerFixture {
    HandlerFixture() {
        gp::rt::resetViolationCount();
        gLastKind = -1;
        gHandlerCalls = 0;
        gp::rt::setViolationHandler(&recordingHandler);
    }
    ~HandlerFixture() {
        gp::rt::setViolationHandler(nullptr);
        gp::rt::resetViolationCount();
    }
    HandlerFixture(const HandlerFixture&) = delete;
    HandlerFixture& operator=(const HandlerFixture&) = delete;
};

#define GP_REQUIRE_CHECKS()                                            \
    do {                                                               \
        if (!gp::rt::checksEnabled())                                  \
            SKIP("GP_RT_CHECKS=0 en esta build: detector desactivado"); \
    } while (false)

}  // namespace

TEST_CASE("RtGuard: new dentro de sección RT cuenta una violación de tipo allocation", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    std::uint64_t count = 0;
    {
        gp::rt::ScopedRealtimeSection rt;
        gSink = new int(7);
        count = gp::rt::violationCount();
    }
    delete gSink;
    gSink = nullptr;
    REQUIRE(count == 1);
    REQUIRE(gLastKind.load() == static_cast<int>(ViolationKind::allocation));
    REQUIRE(gp::rt::violationCount() == 1);
}

TEST_CASE("RtGuard: delete dentro de sección RT cuenta deallocation", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    gSink = new int(1);
    {
        gp::rt::ScopedRealtimeSection rt;
        delete gSink;
    }
    gSink = nullptr;
    REQUIRE(gp::rt::violationCount() == 1);
    REQUIRE(gLastKind.load() == static_cast<int>(ViolationKind::deallocation));
}

TEST_CASE("RtGuard: todas las variantes de new/delete se detectan", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    struct alignas(128) Wide {
        char bytes[128];
    };
    std::uint64_t count = 0;
    {
        gp::rt::ScopedRealtimeSection rt;
        int* volatile a = new int[4];                                   // new[]
        delete[] a;                                                     // delete[]
        int* volatile b = new (std::nothrow) int;                       // nothrow
        delete b;                                                       // delete (sized)
        int* volatile c = new (std::nothrow) int[2];                    // nothrow[]
        delete[] c;
        Wide* volatile d = new Wide;                                    // aligned
        delete d;                                                       // aligned (sized)
        Wide* volatile e = new Wide[2];                                 // aligned[]
        delete[] e;
        Wide* volatile f = new (std::nothrow) Wide;                     // aligned nothrow
        ::operator delete(f, std::align_val_t{alignof(Wide)}, std::nothrow);
        void* volatile g = ::operator new(16, std::align_val_t{64});    // aligned explícito
        ::operator delete(g, std::align_val_t{64});
        void* volatile h = ::operator new[](16, std::align_val_t{64}, std::nothrow);
        ::operator delete[](h, std::align_val_t{64}, std::nothrow);
        void* volatile i = ::operator new(8);
        ::operator delete(i, std::size_t{8});
        delete static_cast<int*>(nullptr);  // liberar nullptr no cuenta
        count = gp::rt::violationCount();
    }
    // 9 reservas + 9 liberaciones no nulas.
    REQUIRE(count == 18);
}

TEST_CASE("RtGuard: fuera de sección RT no hay violaciones", "[rt]") {
    HandlerFixture fx;
    gSink = new int(3);
    delete gSink;
    gSink = nullptr;
    REQUIRE(gp::rt::violationCount() == 0);
    REQUIRE(gHandlerCalls.load() == 0);
    REQUIRE_FALSE(gp::rt::isInRealtimeSection());
}

TEST_CASE("RtGuard: ScopedAllowViolations suspende la detección", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    bool inRtWhileAllowed = true;
    bool inRtAfterAllow = false;
    {
        gp::rt::ScopedRealtimeSection rt;
        {
            gp::rt::ScopedAllowViolations allow;
            inRtWhileAllowed = gp::rt::isInRealtimeSection();
            gSink = new int(5);
            delete gSink;
            GP_RT_ASSERT_NOT_REALTIME();
        }
        inRtAfterAllow = gp::rt::isInRealtimeSection();
    }
    gSink = nullptr;
    REQUIRE(gp::rt::violationCount() == 0);
    REQUIRE_FALSE(inRtWhileAllowed);
    REQUIRE(inRtAfterAllow);
}

TEST_CASE("RtGuard: secciones anidadas", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    bool innerActive = false;
    bool outerStillActive = false;
    std::uint64_t countInner = 0;
    {
        gp::rt::ScopedRealtimeSection outer;
        {
            gp::rt::ScopedRealtimeSection inner;
            innerActive = gp::rt::isInRealtimeSection();
            gSink = new int(1);
            countInner = gp::rt::violationCount();
        }
        outerStillActive = gp::rt::isInRealtimeSection();
    }
    const bool activeAfter = gp::rt::isInRealtimeSection();
    delete gSink;  // fuera: no cuenta
    gSink = nullptr;
    REQUIRE(innerActive);
    REQUIRE(outerStillActive);
    REQUIRE_FALSE(activeAfter);
    REQUIRE(countInner == 1);
    REQUIRE(gp::rt::violationCount() == 1);
}

TEST_CASE("RtGuard: GP_RT_ASSERT_NOT_REALTIME dentro de sección → forbiddenCall", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    GP_RT_ASSERT_NOT_REALTIME();  // fuera: nada
    {
        gp::rt::ScopedRealtimeSection rt;
        GP_RT_ASSERT_NOT_REALTIME();
    }
    REQUIRE(gp::rt::violationCount() == 1);
    REQUIRE(gLastKind.load() == static_cast<int>(ViolationKind::forbiddenCall));
}

TEST_CASE("RtGuard: un handler que reserva no provoca recursión", "[rt]") {
    GP_REQUIRE_CHECKS();
    gp::rt::resetViolationCount();
    gp::rt::setViolationHandler([](ViolationKind) noexcept {
        int* volatile p = new (std::nothrow) int(1);  // mal handler: reserva
        delete p;
    });
    {
        gp::rt::ScopedRealtimeSection rt;
        GP_RT_ASSERT_NOT_REALTIME();
    }
    gp::rt::setViolationHandler(nullptr);
    REQUIRE(gp::rt::violationCount() == 1);
    gp::rt::resetViolationCount();
}

TEST_CASE("RtGuard: el detector es por hilo", "[rt]") {
    GP_REQUIRE_CHECKS();
    HandlerFixture fx;
    std::atomic<bool> go{false};
    std::atomic<bool> done{false};
    std::thread other([&] {
        while (!go.load(std::memory_order_acquire)) {
        }
        auto block = std::make_unique<int[]>(64);  // hilo no RT: no cuenta
        block[0] = 1;
        done.store(true, std::memory_order_release);
    });
    {
        gp::rt::ScopedRealtimeSection rt;  // solo este hilo es RT
        go.store(true, std::memory_order_release);
        while (!done.load(std::memory_order_acquire)) {
        }
    }
    other.join();
    REQUIRE(gp::rt::violationCount() == 0);
}

TEST_CASE("RtGuard: SpscQueue y SnapshotExchange::acquire son RT-safe", "[rt]") {
    HandlerFixture fx;
    gp::rt::SpscQueue<int, 8> queue;
    auto exchange = std::make_unique<gp::rt::SnapshotExchange<int, 4>>();
    exchange->publish(std::make_unique<const int>(1));

    const int* first = nullptr;
    const int* second = nullptr;
    int popped = 0;
    bool pushedOk = true;
    bool poppedOk = true;
    {
        gp::rt::ScopedRealtimeSection rt;
        for (int i = 0; i < 8; ++i)
            pushedOk = queue.tryPush(i) && pushedOk;
        const bool full = !queue.tryPush(99);
        for (int i = 0; i < 8; ++i)
            poppedOk = queue.tryPop(popped) && poppedOk;
        pushedOk = pushedOk && full;
        first = exchange->acquire();
        second = exchange->acquire();
    }
    exchange->publish(std::make_unique<const int>(2));
    const int* third = nullptr;
    std::uint64_t countAfterSwap = 0;
    {
        gp::rt::ScopedRealtimeSection rt;
        third = exchange->acquire();  // retira la 1 a la cola: sin delete
        countAfterSwap = gp::rt::violationCount();
    }
    REQUIRE(pushedOk);
    REQUIRE(poppedOk);
    REQUIRE(popped == 7);
    REQUIRE(first != nullptr);
    REQUIRE(*first == 1);
    REQUIRE(second == first);
    REQUIRE(third != nullptr);
    REQUIRE(*third == 2);
    REQUIRE(countAfterSwap == 0);
    exchange->collectGarbage();
    exchange.reset();
    REQUIRE(gp::rt::violationCount() == 0);
}
