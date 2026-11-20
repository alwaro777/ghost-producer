// Tests de rt::SnapshotExchange (F0-T3): protocolo, cola de retiro llena y estrés con TSan.

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

#include "rt/RtGuard.h"
#include "rt/SnapshotExchange.h"

namespace {

constexpr std::uint32_t kAlive = 0xA11CEu;

// Tipo de prueba: cuenta destrucciones y registra si alguna ocurre en el hilo lector.
struct Probe {
    static inline std::atomic<std::uint64_t> destroyed{0};
    static inline std::atomic<std::uint64_t> destroyedOnReader{0};
    static inline std::atomic<std::thread::id> readerId{};

    explicit Probe(std::uint64_t s) noexcept : seq(s) {}
    ~Probe() {
        if (std::this_thread::get_id() == readerId.load(std::memory_order_relaxed))
            destroyedOnReader.fetch_add(1, std::memory_order_relaxed);
        destroyed.fetch_add(1, std::memory_order_relaxed);
        magic = 0;
    }
    Probe(const Probe&) = delete;
    Probe& operator=(const Probe&) = delete;

    std::uint64_t seq;
    std::uint32_t magic = kAlive;

    static void reset() noexcept {
        destroyed = 0;
        destroyedOnReader = 0;
        readerId = std::thread::id{};
    }
};

}  // namespace

TEST_CASE("SnapshotExchange: nullptr antes del primer publish", "[rt][snapshot]") {
    gp::rt::SnapshotExchange<int> ex;
    REQUIRE(ex.acquire() == nullptr);
    ex.collectGarbage();
    REQUIRE(ex.acquire() == nullptr);
}

TEST_CASE("SnapshotExchange: protocolo publish/acquire/collect", "[rt][snapshot]") {
    Probe::reset();
    {
        gp::rt::SnapshotExchange<Probe, 8> ex;
        ex.publish(std::make_unique<const Probe>(1));
        const Probe* a = ex.acquire();
        REQUIRE(a != nullptr);
        REQUIRE(a->seq == 1);
        REQUIRE(ex.acquire() == a);  // sin pendiente: la misma

        // Dos publish seguidos: la intermedia la destruye el escritor sin que el lector la vea.
        ex.publish(std::make_unique<const Probe>(2));
        ex.publish(std::make_unique<const Probe>(3));
        REQUIRE(Probe::destroyed == 1);
        const Probe* b = ex.acquire();
        REQUIRE(b->seq == 3);
        REQUIRE(Probe::destroyed == 1);  // la 1 está retirada, no destruida

        ex.collectGarbage();
        REQUIRE(Probe::destroyed == 2);
        REQUIRE(ex.acquire() == b);

        ex.publish(std::make_unique<const Probe>(4));  // queda pendiente al destruir
    }
    REQUIRE(Probe::destroyed == 4);  // destructor: actual (3) + pendiente (4)
}

TEST_CASE("SnapshotExchange: cola de retiro llena mantiene la actual y no pierde la pendiente",
          "[rt][snapshot]") {
    Probe::reset();
    {
        gp::rt::SnapshotExchange<Probe, 2> ex;
        ex.publish(std::make_unique<const Probe>(1));
        REQUIRE(ex.acquire()->seq == 1);
        ex.publish(std::make_unique<const Probe>(2));
        REQUIRE(ex.acquire()->seq == 2);  // retira 1 (1/2)
        ex.publish(std::make_unique<const Probe>(3));
        REQUIRE(ex.acquire()->seq == 3);  // retira 2 (2/2: llena)

        ex.publish(std::make_unique<const Probe>(4));
        const Probe* stuck = nullptr;
        {
            gp::rt::ScopedRealtimeSection rt;
            stuck = ex.acquire();
        }
        REQUIRE(stuck->seq == 3);  // conserva la actual
        REQUIRE(ex.acquire()->seq == 3);
        REQUIRE(Probe::destroyed == 0);  // la pendiente (4) no se ha perdido ni destruido

        ex.collectGarbage();
        REQUIRE(Probe::destroyed == 2);
        REQUIRE(ex.acquire()->seq == 4);  // ahora sí
    }
    REQUIRE(Probe::destroyed == 4);
}

TEST_CASE("SnapshotExchange: estrés escritor/lector 10^5 publicaciones", "[rt][snapshot][stress]") {
    constexpr std::uint64_t kPublications = 100'000;
    Probe::reset();
    gp::rt::resetViolationCount();

    auto ex = std::make_unique<gp::rt::SnapshotExchange<Probe, 64>>();
    std::atomic<bool> writerDone{false};
    std::atomic<bool> readerReady{false};

    std::uint64_t corrupt = 0;
    std::uint64_t wentBackwards = 0;
    std::uint64_t lastSeen = 0;
    std::uint64_t distinctSeen = 0;

    std::thread reader([&] {
        Probe::readerId.store(std::this_thread::get_id(), std::memory_order_relaxed);
        readerReady.store(true, std::memory_order_release);
        gp::rt::ScopedRealtimeSection rt;
        const Probe* prev = nullptr;
        auto readOnce = [&] {
            const Probe* p = ex->acquire();
            if (p == nullptr)
                return;
            if (p->magic != kAlive)
                ++corrupt;
            if (p->seq < lastSeen)
                ++wentBackwards;
            if (p != prev)
                ++distinctSeen;
            lastSeen = p->seq;
            prev = p;
        };
        while (!writerDone.load(std::memory_order_acquire))
            readOnce();
        readOnce();  // toma la última pendiente
    });

    while (!readerReady.load(std::memory_order_acquire)) {
    }
    for (std::uint64_t i = 1; i <= kPublications; ++i) {
        ex->collectGarbage();
        ex->publish(std::make_unique<const Probe>(i));
        if ((i & 0xFF) == 0)
            std::this_thread::yield();
    }
    writerDone.store(true, std::memory_order_release);
    reader.join();

    const std::uint64_t destroyedBeforeTeardown = Probe::destroyed.load();
    ex->collectGarbage();
    ex.reset();  // lector parado: destruye la actual

    REQUIRE(corrupt == 0);
    REQUIRE(wentBackwards == 0);
    REQUIRE(lastSeen == kPublications);
    REQUIRE(distinctSeen >= 1);
    REQUIRE(destroyedBeforeTeardown < kPublications);
    REQUIRE(Probe::destroyed.load() == kPublications);
    REQUIRE(Probe::destroyedOnReader.load() == 0);
    REQUIRE(gp::rt::violationCount() == 0);  // lector en sección RT: ni new ni delete
    Probe::reset();
}
