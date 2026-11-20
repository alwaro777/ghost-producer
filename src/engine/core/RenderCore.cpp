#include "engine/RenderCore.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>

#include "rt/RtGuard.h"
#include "rt/SnapshotExchange.h"

namespace gp::engine {
namespace {

constexpr double kRampSeconds = 0.010;  // dentro de 5-20 ms
constexpr double kTwoPi = 6.283185307179586476925286766559;

/// Rampa lineal por muestra hacia un objetivo. Sin reservas; determinista.
struct LinearRamp {
    double value = 0.0;
    double target = 0.0;
    double step = 0.0;
    int remaining = 0;

    void reset(double v) noexcept {
        value = target = v;
        step = 0.0;
        remaining = 0;
    }
    void setTarget(double t, int rampSamples) noexcept {
        if (t == target)
            return;
        target = t;
        remaining = std::max(1, rampSamples);
        step = (target - value) / static_cast<double>(remaining);
    }
    double next() noexcept {
        if (remaining > 0) {
            if (--remaining == 0)
                value = target;
            else
                value += step;
        }
        return value;
    }
};

class RenderCore final : public IRenderCore {
public:
    void prepare(const ProcessSpec& spec) override {
        spec_ = spec;
        rampSamples_ = std::max(1, static_cast<int>(std::lround(kRampSeconds * spec.sampleRate)));
        resetState();
    }

    void release() override { resetState(); }

    void publish(std::unique_ptr<const EngineSnapshot> snapshot) override {
        GP_RT_ASSERT_NOT_REALTIME();
        exchange_.publish(std::move(snapshot));
    }

    void collectGarbage() override {
        GP_RT_ASSERT_NOT_REALTIME();
        exchange_.collectGarbage();
    }

    void process(float* const* outputs, int numChannels, int numSamples) noexcept override {
        if (numChannels <= 0 || numSamples <= 0)
            return;

        const EngineSnapshot* snap = exchange_.acquire();
        float* const out0 = outputs[0];

        if (snap != nullptr) {
            if (!freqInit_) {
                freq_.reset(snap->testToneFrequencyHz);
                freqInit_ = true;
            } else {
                freq_.setTarget(snap->testToneFrequencyHz, rampSamples_);
            }
            gain_.setTarget(snap->testToneEnabled ? static_cast<double>(snap->testToneGainLinear)
                                                  : 0.0,
                            rampSamples_);
        }

        float peak = 0.0f;
        if (!freqInit_ || (gain_.value == 0.0 && gain_.remaining == 0)) {
            for (int i = 0; i < numSamples; ++i)
                out0[i] = 0.0f;
        } else {
            const double invSr = 1.0 / spec_.sampleRate;
            for (int i = 0; i < numSamples; ++i) {
                const double g = gain_.next();
                const double f = freq_.next();
                const float v = (g == 0.0) ? 0.0f : static_cast<float>(g * std::sin(phase_));
                out0[i] = v;
                peak = std::max(peak, std::fabs(v));
                phase_ += kTwoPi * f * invSr;
                if (phase_ >= kTwoPi)
                    phase_ -= kTwoPi;
            }
        }

        for (int c = 1; c < numChannels; ++c)
            std::memcpy(outputs[c], out0, sizeof(float) * static_cast<std::size_t>(numSamples));

        updatePeak(peakL_, peak);
        updatePeak(peakR_, peak);  // mismo contenido en todos los canales
    }

    PeakReadings takePeaks() noexcept override {
        PeakReadings p;
        p.left = peakL_.exchange(0.0f, std::memory_order_acq_rel);
        p.right = peakR_.exchange(0.0f, std::memory_order_acq_rel);
        if (spec_.numOutputChannels == 1)
            p.right = p.left;
        return p;
    }

private:
    static void updatePeak(std::atomic<float>& slot, float v) noexcept {
        float cur = slot.load(std::memory_order_relaxed);
        while (v > cur && !slot.compare_exchange_weak(cur, v, std::memory_order_acq_rel)) {
        }
    }

    void resetState() {
        phase_ = 0.0;
        freq_.reset(440.0);
        gain_.reset(0.0);
        freqInit_ = false;
        peakL_.store(0.0f);
        peakR_.store(0.0f);
    }

    ProcessSpec spec_{};
    int rampSamples_ = 480;
    double phase_ = 0.0;
    LinearRamp freq_;
    LinearRamp gain_;
    bool freqInit_ = false;
    std::atomic<float> peakL_{0.0f};
    std::atomic<float> peakR_{0.0f};
    rt::SnapshotExchange<EngineSnapshot> exchange_;
};

}  // namespace

std::unique_ptr<IRenderCore> createRenderCore() {
    return std::make_unique<RenderCore>();
}

}  // namespace gp::engine
