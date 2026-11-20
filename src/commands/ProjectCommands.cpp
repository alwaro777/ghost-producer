#include "commands/ProjectCommands.h"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

#include "ProjectCommandsInternal.h"

namespace gp::commands {
namespace {

using json = nlohmann::ordered_json;
using model::ProjectState;
using model::TrackId;

Status invalid(std::string msg) { return Status::error(ErrorCode::invalidArgument, std::move(msg)); }

json envelope(std::string_view type, json params) {
    return json{{"type", type}, {"version", 1}, {"params", std::move(params)}};
}

bool validName(const std::string& n) {
    return !n.empty() && n.size() <= model::limits::kMaxTrackNameBytes;
}

bool isPowerOfTwo(int v) { return v > 0 && (v & (v - 1)) == 0; }

auto findTrack(ProjectState& s, TrackId id) {
    return std::find_if(s.tracks.begin(), s.tracks.end(),
                        [&](const model::TrackState& t) { return t.id == id; });
}

// --- Comandos de un solo valor -------------------------------------------------------------

template <class Traits>
class SetValueCommand final : public Command {
public:
    using Value = typename Traits::Value;
    explicit SetValueCommand(Value v) : value_(v) {}

    std::string_view typeId() const noexcept override { return Traits::kType; }
    std::string descriptionKey() const override { return std::string("cmd.") + Traits::kType; }

    Status apply(ProjectState& s) override {
        if (!Traits::valid(value_)) return invalid(std::string(Traits::kType) + ": value out of range");
        auto& ref = Traits::ref(s);
        old_ = ref;
        ref = value_;
        return Status::ok();
    }
    void revert(ProjectState& s) override { Traits::ref(s) = old_; }
    json toJson() const override { return envelope(Traits::kType, json{{"value", value_}}); }

    bool mergeWith(const Command& next) override {
        if constexpr (Traits::kMergeable) {
            const auto* o = dynamic_cast<const SetValueCommand*>(&next);
            if (o == nullptr) return false;
            value_ = o->value_;
            return true;
        } else {
            (void)next;
            return false;
        }
    }

private:
    Value value_;
    Value old_{};
};

struct TempoTraits {
    using Value = double;
    static constexpr const char* kType = "transport.setTempo";
    static constexpr bool kMergeable = true;
    static bool valid(double v) {
        return v >= model::limits::kMinTempoBpm && v <= model::limits::kMaxTempoBpm;
    }
    static double& ref(ProjectState& s) { return s.transport.tempoBpm; }
};
struct EnabledTraits {
    using Value = bool;
    static constexpr const char* kType = "testTone.setEnabled";
    static constexpr bool kMergeable = false;
    static bool valid(bool) { return true; }
    static bool& ref(ProjectState& s) { return s.testTone.enabled; }
};
struct FrequencyTraits {
    using Value = double;
    static constexpr const char* kType = "testTone.setFrequency";
    static constexpr bool kMergeable = true;
    static bool valid(double v) {
        return v >= model::limits::kMinToneFrequencyHz && v <= model::limits::kMaxToneFrequencyHz;
    }
    static double& ref(ProjectState& s) { return s.testTone.frequencyHz; }
};
struct GainTraits {
    using Value = float;
    static constexpr const char* kType = "testTone.setGain";
    static constexpr bool kMergeable = true;
    static bool valid(float v) {
        return v >= model::limits::kMinToneGainDb && v <= model::limits::kMaxToneGainDb;
    }
    static float& ref(ProjectState& s) { return s.testTone.gainDb; }
};

// --- Compás --------------------------------------------------------------------------------

class SetTimeSignatureCommand final : public Command {
public:
    SetTimeSignatureCommand(int n, int d) : num_(n), den_(d) {}
    std::string_view typeId() const noexcept override { return "transport.setTimeSignature"; }
    std::string descriptionKey() const override { return "cmd.transport.setTimeSignature"; }
    Status apply(ProjectState& s) override {
        if (num_ < model::limits::kMinTimeSigNumerator || num_ > model::limits::kMaxTimeSigNumerator)
            return invalid("time signature numerator out of range");
        if (!isPowerOfTwo(den_) || den_ > model::limits::kMaxTimeSigDenominator)
            return invalid("time signature denominator must be a power of 2 in [1, 32]");
        oldNum_ = s.transport.timeSigNumerator;
        oldDen_ = s.transport.timeSigDenominator;
        s.transport.timeSigNumerator = num_;
        s.transport.timeSigDenominator = den_;
        return Status::ok();
    }
    void revert(ProjectState& s) override {
        s.transport.timeSigNumerator = oldNum_;
        s.transport.timeSigDenominator = oldDen_;
    }
    json toJson() const override {
        return envelope(typeId(), json{{"numerator", num_}, {"denominator", den_}});
    }

private:
    int num_, den_;
    int oldNum_ = 4, oldDen_ = 4;
};

// --- Pistas --------------------------------------------------------------------------------

class AddTrackCommand final : public Command {
public:
    AddTrackCommand(std::string name, std::optional<std::size_t> index, TrackId id = {})
        : name_(std::move(name)), index_(index), id_(id) {}
    std::string_view typeId() const noexcept override { return "track.add"; }
    std::string descriptionKey() const override { return "cmd.track.add"; }

    Status apply(ProjectState& s) override {
        if (!validName(name_)) return invalid("track name must be 1..64 bytes");
        if (s.tracks.size() >= model::limits::kMaxTracks) return invalid("too many tracks");
        const std::size_t at = index_.value_or(s.tracks.size());
        if (at > s.tracks.size()) return invalid("track index out of range");
        TrackId id = id_.isValid() ? id_ : TrackId{s.nextTrackId};
        if (id.value < s.nextTrackId) return invalid("track id already used");
        if (id.value >= (std::uint64_t{1} << 53)) return invalid("track id out of range");
        prevNextId_ = s.nextTrackId;
        id_ = id;
        index_ = at;
        s.tracks.insert(s.tracks.begin() + static_cast<std::ptrdiff_t>(at),
                        model::TrackState{id, name_});
        s.nextTrackId = id.value + 1;
        return Status::ok();
    }
    void revert(ProjectState& s) override {
        s.tracks.erase(s.tracks.begin() + static_cast<std::ptrdiff_t>(*index_));
        s.nextTrackId = prevNextId_;
    }
    json toJson() const override {
        json p{{"name", name_}};
        if (index_) p["index"] = *index_;
        if (id_.isValid()) p["id"] = id_.value;
        return envelope(typeId(), std::move(p));
    }

private:
    std::string name_;
    std::optional<std::size_t> index_;
    TrackId id_;
    std::uint64_t prevNextId_ = 1;
};

class RemoveTrackCommand final : public Command {
public:
    explicit RemoveTrackCommand(TrackId id) : id_(id) {}
    std::string_view typeId() const noexcept override { return "track.remove"; }
    std::string descriptionKey() const override { return "cmd.track.remove"; }
    Status apply(ProjectState& s) override {
        auto it = findTrack(s, id_);
        if (it == s.tracks.end()) return invalid("track does not exist");
        index_ = static_cast<std::size_t>(it - s.tracks.begin());
        removed_ = *it;
        s.tracks.erase(it);
        return Status::ok();
    }
    void revert(ProjectState& s) override {
        s.tracks.insert(s.tracks.begin() + static_cast<std::ptrdiff_t>(index_), removed_);
    }
    json toJson() const override { return envelope(typeId(), json{{"id", id_.value}}); }

private:
    TrackId id_;
    std::size_t index_ = 0;
    model::TrackState removed_;
};

class RenameTrackCommand final : public Command {
public:
    RenameTrackCommand(TrackId id, std::string name) : id_(id), name_(std::move(name)) {}
    std::string_view typeId() const noexcept override { return "track.rename"; }
    std::string descriptionKey() const override { return "cmd.track.rename"; }
    Status apply(ProjectState& s) override {
        if (!validName(name_)) return invalid("track name must be 1..64 bytes");
        auto it = findTrack(s, id_);
        if (it == s.tracks.end()) return invalid("track does not exist");
        old_ = it->name;
        it->name = name_;
        return Status::ok();
    }
    void revert(ProjectState& s) override {
        auto it = findTrack(s, id_);
        if (it != s.tracks.end()) it->name = old_;
    }
    json toJson() const override {
        return envelope(typeId(), json{{"id", id_.value}, {"name", name_}});
    }
    bool mergeWith(const Command& next) override {
        const auto* o = dynamic_cast<const RenameTrackCommand*>(&next);
        if (o == nullptr || o->id_ != id_) return false;
        name_ = o->name_;
        return true;
    }

private:
    TrackId id_;
    std::string name_;
    std::string old_;
};

// --- Factorías -----------------------------------------------------------------------------

Status badParam(const char* what) {
    return Status::error(ErrorCode::invalidArgument, std::string("bad or missing param: ") + what);
}

bool getNumber(const json& p, const char* key, double& out) {
    if (!p.contains(key) || !p[key].is_number()) return false;
    out = p[key].get<double>();
    return true;
}
bool getInt(const json& p, const char* key, int& out) {
    if (!p.contains(key) || !p[key].is_number_integer()) return false;
    const auto v = p[key].get<long long>();
    if (v < -1000000 || v > 1000000) return false;
    out = static_cast<int>(v);
    return true;
}
bool getUint(const json& p, const char* key, std::uint64_t& out) {
    if (!p.contains(key) || !p[key].is_number_integer()) return false;
    if (p[key].is_number_unsigned()) {
        out = p[key].get<std::uint64_t>();
        return true;
    }
    return false;
}
bool getString(const json& p, const char* key, std::string& out) {
    if (!p.contains(key) || !p[key].is_string()) return false;
    out = p[key].get<std::string>();
    return true;
}

template <class Traits>
void addSetter(CommandRegistry& r) {
    r.add(Traits::kType, [](const json& p, Status& err) -> std::unique_ptr<Command> {
        typename Traits::Value v{};
        if constexpr (std::is_same_v<typename Traits::Value, bool>) {
            if (!p.contains("value") || !p["value"].is_boolean()) {
                err = badParam("value");
                return nullptr;
            }
            v = p["value"].get<bool>();
        } else {
            double d = 0;
            if (!getNumber(p, "value", d)) {
                err = badParam("value");
                return nullptr;
            }
            v = static_cast<typename Traits::Value>(d);
        }
        return std::make_unique<SetValueCommand<Traits>>(v);
    });
}

}  // namespace

std::unique_ptr<Command> makeSetTempo(double bpm) {
    return std::make_unique<SetValueCommand<TempoTraits>>(bpm);
}
std::unique_ptr<Command> makeSetTimeSignature(int n, int d) {
    return std::make_unique<SetTimeSignatureCommand>(n, d);
}
std::unique_ptr<Command> makeSetTestToneEnabled(bool e) {
    return std::make_unique<SetValueCommand<EnabledTraits>>(e);
}
std::unique_ptr<Command> makeSetTestToneFrequency(double f) {
    return std::make_unique<SetValueCommand<FrequencyTraits>>(f);
}
std::unique_ptr<Command> makeSetTestToneGain(float g) {
    return std::make_unique<SetValueCommand<GainTraits>>(g);
}
std::unique_ptr<Command> makeAddTrack(std::string name, std::optional<std::size_t> index) {
    return std::make_unique<AddTrackCommand>(std::move(name), index);
}
std::unique_ptr<Command> makeRemoveTrack(model::TrackId id) {
    return std::make_unique<RemoveTrackCommand>(id);
}
std::unique_ptr<Command> makeRenameTrack(model::TrackId id, std::string newName) {
    return std::make_unique<RenameTrackCommand>(id, std::move(newName));
}

void registerProjectCommands(CommandRegistry& r) {
    addSetter<TempoTraits>(r);
    addSetter<EnabledTraits>(r);
    addSetter<FrequencyTraits>(r);
    addSetter<GainTraits>(r);

    r.add("transport.setTimeSignature", [](const json& p, Status& err) -> std::unique_ptr<Command> {
        int n = 0, d = 0;
        if (!getInt(p, "numerator", n)) { err = badParam("numerator"); return nullptr; }
        if (!getInt(p, "denominator", d)) { err = badParam("denominator"); return nullptr; }
        return makeSetTimeSignature(n, d);
    });
    r.add("track.add", [](const json& p, Status& err) -> std::unique_ptr<Command> {
        std::string name;
        if (!getString(p, "name", name)) { err = badParam("name"); return nullptr; }
        std::optional<std::size_t> index;
        if (p.contains("index")) {
            std::uint64_t i = 0;
            if (!getUint(p, "index", i)) { err = badParam("index"); return nullptr; }
            index = static_cast<std::size_t>(i);
        }
        TrackId id;
        if (p.contains("id")) {
            if (!getUint(p, "id", id.value)) { err = badParam("id"); return nullptr; }
        }
        return std::make_unique<AddTrackCommand>(std::move(name), index, id);
    });
    r.add("track.remove", [](const json& p, Status& err) -> std::unique_ptr<Command> {
        TrackId id;
        if (!getUint(p, "id", id.value)) { err = badParam("id"); return nullptr; }
        return makeRemoveTrack(id);
    });
    r.add("track.rename", [](const json& p, Status& err) -> std::unique_ptr<Command> {
        TrackId id;
        std::string name;
        if (!getUint(p, "id", id.value)) { err = badParam("id"); return nullptr; }
        if (!getString(p, "name", name)) { err = badParam("name"); return nullptr; }
        return makeRenameTrack(id, std::move(name));
    });
}

}  // namespace gp::commands
