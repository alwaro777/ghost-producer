#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "engine/RenderCore.h"
#include "rt/RtGuard.h"

using namespace gp;
using namespace gp::engine;

namespace {

std::unique_ptr<const EngineSnapshot> snap(bool on, double hz, float gainDb) {
    model::ProjectState st;
    st.testTone = {on, hz, gainDb};
    return std::make_unique<const EngineSnapshot>(buildSnapshot(st, 1));
}

struct Step {
    int atSample;  // al inicio del bloque que contenga esta muestra (múltiplo del bloque)
    bool on;
    double hz;
    float db;
};

/// Render offline mono/estéreo; devuelve el canal 0. Publica pasos en límites de bloque.
std::vector<float> render(double sr, int block, int total, const std::vector<Step>& steps,
                          int channels = 2, PeakReadings* peaks = nullptr) {
    auto core = createRenderCore();
    core->prepare({sr, block, channels});
    std::vector<float> out(static_cast<std::size_t>(total));
    std::vector<std::vector<float>> bufs(static_cast<std::size_t>(channels),
                                         std::vector<float>(static_cast<std::size_t>(block)));
    std::vector<float*> ptrs;
    for (auto& b : bufs) ptrs.push_back(b.data());
    std::size_t next = 0;
    for (int pos = 0; pos < total; pos += block) {
        while (next < steps.size() && steps[next].atSample <= pos) {
            core->collectGarbage();
            core->publish(snap(steps[next].on, steps[next].hz, steps[next].db));
            ++next;
        }
        const int n = std::min(block, total - pos);
        core->process(ptrs.data(), channels, n);
        std::memcpy(out.data() + pos, ptrs[0], sizeof(float) * static_cast<std::size_t>(n));
    }
    if (peaks) *peaks = core->takePeaks();
    return out;
}

double zeroCrossFreq(const std::vector<float>& x, double sr, std::size_t from, std::size_t to) {
    // cruces ascendentes con interpolación lineal
    double first = -1, last = -1;
    int count = 0;
    for (std::size_t i = from + 1; i < to; ++i) {
        if (x[i - 1] < 0.0f && x[i] >= 0.0f) {
            const double t = static_cast<double>(i - 1) + (-x[i - 1]) / double(x[i] - x[i - 1]);
            if (first < 0) first = t;
            last = t;
            ++count;
        }
    }
    return (count - 1) * sr / (last - first);
}

double maxDiff(const std::vector<float>& x) {
    double m = 0;
    for (std::size_t i = 1; i < x.size(); ++i) m = std::max(m, double(std::fabs(x[i] - x[i - 1])));
    return m;
}

}  // namespace

TEST_CASE("buildSnapshot convierte dB a lineal y copia campos") {
    model::ProjectState st;
    st.testTone = {true, 523.25, -18.0f};
    const auto s = buildSnapshot(st, 42);
    CHECK(s.documentRevision == 42);
    CHECK(s.testToneEnabled);
    CHECK(s.testToneFrequencyHz == 523.25);
    CHECK(s.testToneGainLinear == Catch::Approx(0.12589f).margin(1e-5));
    st.testTone.gainDb = 0.0f;
    CHECK(buildSnapshot(st, 0).testToneGainLinear == Catch::Approx(1.0f));
}

TEST_CASE("silencio exacto antes de publicar y con el tono apagado") {
    auto a = render(48000, 128, 4800, {});
    CHECK(std::all_of(a.begin(), a.end(), [](float v) { return v == 0.0f && !std::signbit(v); }));
    auto b = render(48000, 128, 4800, {{0, false, 440.0, 0.0f}});
    CHECK(std::all_of(b.begin(), b.end(), [](float v) { return v == 0.0f && !std::signbit(v); }));
}

TEST_CASE("seno 440 Hz: pico y frecuencia, independiente de bloque y sample rate") {
    for (double sr : {44100.0, 48000.0, 96000.0}) {
        std::vector<float> ref;
        for (int block : {32, 64, 128, 512, 2048}) {
            PeakReadings pk;
            const int total = static_cast<int>(sr);
            auto x = render(sr, block, total, {{0, true, 440.0, 0.0f}}, 2, &pk);
            const float peak = *std::max_element(x.begin(), x.end());
            CHECK(peak >= 0.99f);
            CHECK(peak <= 1.0f);
            CHECK(pk.left == pk.right);
            CHECK(pk.left >= 0.99f);
            const double f = zeroCrossFreq(x, sr, static_cast<std::size_t>(sr * 0.05), x.size());
            CHECK(std::fabs(f - 440.0) <= 1.0);
            if (ref.empty()) ref = x;
            else CHECK(x == ref);  // mismo resultado con cualquier bloque
        }
    }
}

TEST_CASE("sin clics en cambios de frecuencia, ganancia y on/off") {
    const double sr = 48000;
    const int block = 64;
    const std::vector<Step> steps = {{0, true, 440.0, 0.0f},      {4800, true, 880.0, 0.0f},
                                     {9600, true, 880.0, -60.0f}, {14400, false, 880.0, -60.0f},
                                     {19200, true, 880.0, 0.0f}};
    auto x = render(sr, block, 28800, steps);
    const double limit = 2.0 * std::sin(3.14159265358979 * 880.0 / sr) * 1.05;
    CHECK(maxDiff(x) <= limit);

    // la rampa dura 5-20 ms: el apagado (0 dB -> silencio) tarda ~10 ms
    auto y = render(sr, block, 9600, {{0, true, 440.0, 0.0f}, {4800, false, 440.0, 0.0f}});
    std::size_t lastNonZero = 0;
    for (std::size_t i = 4800; i < y.size(); ++i) if (y[i] != 0.0f) lastNonZero = i;
    const double rampMs = (static_cast<double>(lastNonZero) - 4800.0) * 1000.0 / sr;
    CHECK(rampMs >= 5.0);
    CHECK(rampMs <= 20.0);
}

TEST_CASE("determinismo: bytes iguales") {
    const std::vector<Step> steps = {{0, true, 440.0, -6.0f}, {2048, true, 1000.0, -12.0f},
                                     {6144, false, 1000.0, -12.0f}};
    auto a = render(48000, 128, 12000, steps);
    auto b = render(48000, 128, 12000, steps);
    REQUIRE(a.size() == b.size());
    CHECK(std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0);
}

TEST_CASE("process en sección RT con publicaciones intercaladas: 0 violaciones") {
    auto core = createRenderCore();
    core->prepare({48000.0, 256, 2});
    std::vector<float> l(256), r(256);
    float* ptrs[2] = {l.data(), r.data()};
    rt::resetViolationCount();
    for (int i = 0; i < 200; ++i) {
        core->collectGarbage();
        core->publish(snap(i % 3 != 0, 200.0 + i * 10.0, -3.0f * float(i % 10)));
        if (i % 2 == 0) core->publish(snap(true, 300.0, -6.0f));
        {
            rt::ScopedRealtimeSection rt;
            core->process(ptrs, 2, 256);
            (void)core->takePeaks();
        }
    }
    CHECK(rt::violationCount() == 0);
}

TEST_CASE("takePeaks devuelve y pone a cero; mono: right == left") {
    auto core = createRenderCore();
    core->prepare({48000.0, 512, 1});
    core->publish(snap(true, 440.0, 0.0f));
    std::vector<float> m(512);
    float* ptrs[1] = {m.data()};
    for (int i = 0; i < 20; ++i) core->process(ptrs, 1, 512);
    auto p = core->takePeaks();
    CHECK(p.left > 0.9f);
    CHECK(p.right == p.left);
    auto z = core->takePeaks();
    CHECK(z.left == 0.0f);
    CHECK(z.right == 0.0f);
}

TEST_CASE("publish y collectGarbage dentro de sección RT registran violación") {
    if (!rt::checksEnabled()) SUCCEED("detector desactivado en esta build");
    else {
        auto core = createRenderCore();
        core->prepare({48000.0, 64, 2});
        rt::resetViolationCount();
        {
            rt::ScopedRealtimeSection section;
            core->collectGarbage();
        }
        CHECK(rt::violationCount() >= 1);
        rt::resetViolationCount();
        {
            rt::ScopedRealtimeSection section;
            core->publish(snap(true, 440.0, 0.0f));
        }
        CHECK(rt::violationCount() >= 1);
        rt::resetViolationCount();
    }
}
