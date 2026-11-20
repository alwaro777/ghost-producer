// F0-T9: matriz de render {bloque} x {sample rate} con detector RT, NaN y nivel.
#include <cmath>
#include <vector>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "engine/EngineSnapshot.h"
#include "engine/RenderCore.h"
#include "rt/RtGuard.h"

using namespace gp;
using namespace gp::engine;

TEST_CASE("render matrix: blocks x sample rates, 0 RT violations, no NaN, bounded level",
          "[integration][rt][render]") {
    if (!rt::checksEnabled()) SKIP("detector RT desactivado (GP_RT_CHECKS=OFF)");

    const int blocks[] = {32, 64, 128, 512, 2048};
    const double rates[] = {44100.0, 48000.0, 96000.0};
    const float gainDb = -12.0f;
    const double gainLin = std::pow(10.0, gainDb / 20.0);

    for (double sr : rates) {
        for (int block : blocks) {
            DYNAMIC_SECTION(sr << " Hz / " << block) {
                model::ProjectState st;
                st.testTone = {true, 440.0, gainDb};
                auto core = createRenderCore();
                core->prepare({sr, block, 2});
                core->publish(std::make_unique<const EngineSnapshot>(buildSnapshot(st, 1)));

                std::vector<std::vector<float>> bufs(2, std::vector<float>(std::size_t(block)));
                float* ptrs[2] = {bufs[0].data(), bufs[1].data()};
                const int total = static_cast<int>(sr);  // 1 s
                double peak = 0.0;
                bool finite = true;

                rt::resetViolationCount();
                for (int pos = 0; pos < total; pos += block) {
                    const int n = std::min(block, total - pos);
                    {
                        rt::ScopedRealtimeSection rtSection;
                        core->process(ptrs, 2, n);
                    }
                    for (int c = 0; c < 2; ++c)
                        for (int i = 0; i < n; ++i) {
                            const float v = bufs[std::size_t(c)][std::size_t(i)];
                            if (!std::isfinite(v)) finite = false;
                            peak = std::max(peak, double(std::fabs(v)));
                        }
                }
                CHECK(rt::violationCount() == 0);
                CHECK(finite);
                CHECK(peak <= gainLin * 1.001);          // sin clipping inesperado
                CHECK(peak >= gainLin * 0.95);           // y realmente suena
                const auto p = core->takePeaks();
                CHECK(p.left == Catch::Approx(gainLin).epsilon(0.05));
                core->release();
            }
        }
    }
}
