#include "model/Document.h"

#include <algorithm>
#include <cassert>
#include <thread>
#include <utility>
#include <vector>

#include "rt/RtGuard.h"

namespace gp::model {

struct Document::Impl {
    ProjectState state;
    std::uint64_t revision = 0;
    std::vector<Listener*> listeners;
    std::thread::id owner = std::this_thread::get_id();

    void checkThread() const { assert(owner == std::this_thread::get_id()); }

    void notify(const Document& doc) {
        ++revision;
        // Copia: un listener podría quitarse durante la notificación.
        const auto snapshot = listeners;
        for (auto* l : snapshot)
            if (std::find(listeners.begin(), listeners.end(), l) != listeners.end())
                l->documentChanged(doc);
    }
};

Document::Document(ProjectState initial) : impl_(std::make_unique<Impl>()) {
    impl_->state = std::move(initial);
}

Document::~Document() = default;

const ProjectState& Document::state() const noexcept { return impl_->state; }

std::uint64_t Document::revision() const noexcept { return impl_->revision; }

void Document::reset(ProjectState newState) {
    impl_->checkThread();
    impl_->state = std::move(newState);
    impl_->notify(*this);
}

void Document::addListener(Listener* listener) {
    impl_->checkThread();
    if (listener != nullptr &&
        std::find(impl_->listeners.begin(), impl_->listeners.end(), listener) ==
            impl_->listeners.end())
        impl_->listeners.push_back(listener);
}

void Document::removeListener(Listener* listener) {
    impl_->checkThread();
    std::erase(impl_->listeners, listener);
}

ProjectState& Document::mutableState() noexcept { return impl_->state; }

void Document::notifyChanged() {
    GP_RT_ASSERT_NOT_REALTIME();
    impl_->checkThread();
    impl_->notify(*this);
}

}  // namespace gp::model
