// Tests de rt::SpscQueue (F0-T3): FIFO, llena/vacía y estrés productor/consumidor (TSan).

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <thread>

#include "rt/SpscQueue.h"

namespace {

// Tipo solo-movible para comprobar que "llena → no se mueve".
struct MoveOnly {
    std::unique_ptr<int> value;
    MoveOnly() noexcept = default;
    explicit MoveOnly(int v) : value(std::make_unique<int>(v)) {}
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;
};

}  // namespace

TEST_CASE("SpscQueue: FIFO básico", "[rt][spsc]") {
    gp::rt::SpscQueue<int, 4> q;
    REQUIRE(q.capacity() == 4);
    REQUIRE(q.sizeApprox() == 0);
    REQUIRE(q.tryPush(1));
    REQUIRE(q.tryPush(2));
    const int three = 3;
    REQUIRE(q.tryPush(three));  // sobrecarga const&
    REQUIRE(q.sizeApprox() == 3);
    int out = 0;
    REQUIRE(q.tryPop(out));
    REQUIRE(out == 1);
    REQUIRE(q.tryPop(out));
    REQUIRE(out == 2);
    REQUIRE(q.tryPop(out));
    REQUIRE(out == 3);
    REQUIRE(q.sizeApprox() == 0);
}

TEST_CASE("SpscQueue: vacía → false y out no cambia", "[rt][spsc]") {
    gp::rt::SpscQueue<int, 2> q;
    int out = 42;
    REQUIRE_FALSE(q.tryPop(out));
    REQUIRE(out == 42);
    REQUIRE(q.tryPush(1));
    REQUIRE(q.tryPop(out));
    REQUIRE_FALSE(q.tryPop(out));
    REQUIRE(out == 1);
}

TEST_CASE("SpscQueue: llena → false sin mover el elemento", "[rt][spsc]") {
    gp::rt::SpscQueue<MoveOnly, 2> q;
    REQUIRE(q.tryPush(MoveOnly(1)));
    REQUIRE(q.tryPush(MoveOnly(2)));
    REQUIRE(q.sizeApprox() == 2);
    MoveOnly extra(3);
    REQUIRE_FALSE(q.tryPush(std::move(extra)));
    REQUIRE(extra.value != nullptr);  // NOLINT(bugprone-use-after-move): no se movió
    REQUIRE(*extra.value == 3);
    MoveOnly out;
    REQUIRE(q.tryPop(out));
    REQUIRE(*out.value == 1);
    REQUIRE(q.tryPush(std::move(extra)));
    REQUIRE(q.tryPop(out));
    REQUIRE(*out.value == 2);
    REQUIRE(q.tryPop(out));
    REQUIRE(*out.value == 3);
}

TEST_CASE("SpscQueue: los índices dan muchas vueltas al buffer", "[rt][spsc]") {
    gp::rt::SpscQueue<std::uint32_t, 4> q;
    std::uint32_t expected = 0;
    std::uint32_t next = 0;
    for (int round = 0; round < 10000; ++round) {
        while (q.tryPush(next))
            ++next;
        std::uint32_t out = 0;
        REQUIRE(q.tryPop(out));
        REQUIRE(out == expected++);
        REQUIRE(q.tryPop(out));
        REQUIRE(out == expected++);
    }
}

TEST_CASE("SpscQueue: estrés 2 hilos, 10^6 elementos en orden y sin pérdidas", "[rt][spsc][stress]") {
    constexpr std::uint64_t kCount = 1'000'000;
    auto q = std::make_unique<gp::rt::SpscQueue<std::uint64_t, 1024>>();

    std::thread producer([&q] {
        for (std::uint64_t i = 0; i < kCount;) {
            if (q->tryPush(i))
                ++i;
            else
                std::this_thread::yield();
        }
    });

    std::uint64_t expected = 0;
    std::uint64_t outOfOrder = 0;
    std::uint64_t sum = 0;
    while (expected < kCount) {
        std::uint64_t v = 0;
        if (q->tryPop(v)) {
            if (v != expected)
                ++outOfOrder;
            sum += v;
            ++expected;
        } else {
            std::this_thread::yield();
        }
    }
    producer.join();

    std::uint64_t leftover = 0;
    REQUIRE_FALSE(q->tryPop(leftover));
    REQUIRE(outOfOrder == 0);
    REQUIRE(expected == kCount);
    REQUIRE(sum == kCount * (kCount - 1) / 2);
}
