#include "engine/EngineSnapshot.h"

#include <cmath>

namespace gp::engine {

EngineSnapshot buildSnapshot(const model::ProjectState& state, std::uint64_t documentRevision) {
    EngineSnapshot s;
    s.documentRevision = documentRevision;
    s.testToneEnabled = state.testTone.enabled;
    s.testToneFrequencyHz = state.testTone.frequencyHz;
    s.testToneGainLinear =
        static_cast<float>(std::pow(10.0, static_cast<double>(state.testTone.gainDb) / 20.0));
    return s;
}

}  // namespace gp::engine
