#include "model/ProjectState.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace gp::model {

namespace {
constexpr std::uint64_t kMaxExactJsonInt = 1ULL << 53;

bool fail(std::string* whyNot, const char* msg) {
    if (whyNot != nullptr) {
        *whyNot = msg;
    }
    return false;
}

bool isPowerOfTwo(int v) {
    return v > 0 && (v & (v - 1)) == 0;
}
}  // namespace

bool isValid(const ProjectState& s, std::string* whyNot) {
    const auto& t = s.transport;
    if (!std::isfinite(t.tempoBpm) || t.tempoBpm < limits::kMinTempoBpm ||
        t.tempoBpm > limits::kMaxTempoBpm) {
        return fail(whyNot, "tempoBpm out of range");
    }
    if (t.timeSigNumerator < limits::kMinTimeSigNumerator ||
        t.timeSigNumerator > limits::kMaxTimeSigNumerator) {
        return fail(whyNot, "timeSigNumerator out of range");
    }
    if (!isPowerOfTwo(t.timeSigDenominator) || t.timeSigDenominator > limits::kMaxTimeSigDenominator) {
        return fail(whyNot, "timeSigDenominator must be a power of 2 in [1, 32]");
    }
    const auto& tone = s.testTone;
    if (!std::isfinite(tone.frequencyHz) || tone.frequencyHz < limits::kMinToneFrequencyHz ||
        tone.frequencyHz > limits::kMaxToneFrequencyHz) {
        return fail(whyNot, "testTone.frequencyHz out of range");
    }
    if (!std::isfinite(tone.gainDb) || tone.gainDb < limits::kMinToneGainDb ||
        tone.gainDb > limits::kMaxToneGainDb) {
        return fail(whyNot, "testTone.gainDb out of range");
    }
    if (s.tracks.size() > limits::kMaxTracks) {
        return fail(whyNot, "too many tracks");
    }
    std::set<std::uint64_t> seen;
    std::uint64_t maxId = 0;
    for (const auto& track : s.tracks) {
        if (!track.id.isValid()) {
            return fail(whyNot, "track id 0 is invalid");
        }
        if (track.id.value >= kMaxExactJsonInt) {
            return fail(whyNot, "track id must be < 2^53");
        }
        if (!seen.insert(track.id.value).second) {
            return fail(whyNot, "duplicate track id");
        }
        if (track.name.empty() || track.name.size() > limits::kMaxTrackNameBytes) {
            return fail(whyNot, "track name must be 1..64 bytes");
        }
        maxId = std::max(maxId, track.id.value);
    }
    if (s.nextTrackId <= maxId) {
        return fail(whyNot, "nextTrackId must be greater than every track id");
    }
    if (s.nextTrackId >= kMaxExactJsonInt) {
        return fail(whyNot, "nextTrackId must be < 2^53");
    }
    return true;
}

}  // namespace gp::model
