#include "commands/CommandHistory.h"

#include <algorithm>
#include <cassert>
#include <optional>

#include "model/Document.h"

namespace gp::commands {

namespace {
struct Entry {
    std::vector<std::unique_ptr<Command>> commands;
    std::string descriptionKey;
};
}  // namespace

struct CommandHistory::Impl {
    model::Document& doc;
    std::vector<Entry> undoStack, redoStack;
    std::optional<std::size_t> savedPos = 0;  // nullopt = posición guardada inalcanzable
    bool mergeable = false;                   // la cima se creó con el último execute
    bool inTxn = false;
    Entry txn;
    std::vector<Listener*> listeners;

    explicit Impl(model::Document& d) : doc(d) {}

    void notifyHistory(const CommandHistory& h) {
        const auto copy = listeners;
        for (auto* l : copy)
            if (std::find(listeners.begin(), listeners.end(), l) != listeners.end())
                l->historyChanged(h);
    }
    void dropRedo() {
        redoStack.clear();
        if (savedPos && *savedPos > undoStack.size()) savedPos.reset();
    }
};

CommandHistory::CommandHistory(model::Document& document)
    : impl_(std::make_unique<Impl>(document)) {}
CommandHistory::~CommandHistory() = default;

Status CommandHistory::execute(std::unique_ptr<Command> command, MergePolicy merge) {
    if (!command) return Status::error(ErrorCode::invalidArgument, "null command");
    auto& s = *impl_;
    Status st = command->apply(s.doc.mutableState());
    if (!st.isOk()) return st;

    if (s.inTxn) {
        s.txn.commands.push_back(std::move(command));
        s.dropRedo();
        return Status::ok();
    }

    s.dropRedo();
    bool merged = false;
    if (merge == MergePolicy::withPrevious && s.mergeable && !s.undoStack.empty() &&
        s.undoStack.back().commands.size() == 1) {
        merged = s.undoStack.back().commands.front()->mergeWith(*command);
        if (merged && s.savedPos && *s.savedPos == s.undoStack.size()) s.savedPos.reset();
    }
    if (!merged) {
        Entry e;
        e.descriptionKey = command->descriptionKey();
        e.commands.push_back(std::move(command));
        s.undoStack.push_back(std::move(e));
    }
    s.mergeable = true;
    s.doc.notifyChanged();
    s.notifyHistory(*this);
    return Status::ok();
}

bool CommandHistory::canUndo() const noexcept { return !impl_->inTxn && !impl_->undoStack.empty(); }
bool CommandHistory::canRedo() const noexcept { return !impl_->inTxn && !impl_->redoStack.empty(); }

bool CommandHistory::undo() {
    auto& s = *impl_;
    if (!canUndo()) return false;
    Entry e = std::move(s.undoStack.back());
    s.undoStack.pop_back();
    auto& state = s.doc.mutableState();
    for (auto it = e.commands.rbegin(); it != e.commands.rend(); ++it) (*it)->revert(state);
    s.redoStack.push_back(std::move(e));
    s.mergeable = false;
    s.doc.notifyChanged();
    s.notifyHistory(*this);
    return true;
}

bool CommandHistory::redo() {
    auto& s = *impl_;
    if (!canRedo()) return false;
    Entry e = std::move(s.redoStack.back());
    s.redoStack.pop_back();
    auto& state = s.doc.mutableState();
    std::size_t applied = 0;
    for (; applied < e.commands.size(); ++applied)
        if (!e.commands[applied]->apply(state).isOk()) break;
    if (applied != e.commands.size()) {  // no debería ocurrir: deshacer lo aplicado
        assert(false && "redo failed");
        for (std::size_t i = applied; i-- > 0;) e.commands[i]->revert(state);
        s.redoStack.push_back(std::move(e));
        return false;
    }
    s.undoStack.push_back(std::move(e));
    s.mergeable = false;
    s.doc.notifyChanged();
    s.notifyHistory(*this);
    return true;
}

std::string CommandHistory::undoDescriptionKey() const {
    return impl_->undoStack.empty() ? std::string{} : impl_->undoStack.back().descriptionKey;
}
std::string CommandHistory::redoDescriptionKey() const {
    return impl_->redoStack.empty() ? std::string{} : impl_->redoStack.back().descriptionKey;
}

void CommandHistory::beginTransaction(std::string descriptionKey) {
    auto& s = *impl_;
    assert(!s.inTxn && "nested transactions not supported");
    if (s.inTxn) return;
    s.inTxn = true;
    s.txn = Entry{{}, std::move(descriptionKey)};
}

void CommandHistory::commitTransaction() {
    auto& s = *impl_;
    assert(s.inTxn);
    if (!s.inTxn) return;
    s.inTxn = false;
    if (s.txn.commands.empty()) return;
    s.undoStack.push_back(std::move(s.txn));
    s.txn = {};
    s.mergeable = false;
    s.doc.notifyChanged();
    s.notifyHistory(*this);
}

void CommandHistory::abortTransaction() {
    auto& s = *impl_;
    assert(s.inTxn);
    if (!s.inTxn) return;
    s.inTxn = false;
    Entry e = std::move(s.txn);
    s.txn = {};
    if (e.commands.empty()) return;
    auto& state = s.doc.mutableState();
    for (auto it = e.commands.rbegin(); it != e.commands.rend(); ++it) (*it)->revert(state);
    s.doc.notifyChanged();
    s.notifyHistory(*this);
}

bool CommandHistory::inTransaction() const noexcept { return impl_->inTxn; }

void CommandHistory::clear() {
    auto& s = *impl_;
    s.undoStack.clear();
    s.redoStack.clear();
    s.txn = {};
    s.inTxn = false;
    s.mergeable = false;
    s.savedPos = 0;
    s.notifyHistory(*this);
}

void CommandHistory::markSaved() {
    impl_->savedPos = impl_->undoStack.size();
    impl_->notifyHistory(*this);
}

bool CommandHistory::isDirty() const noexcept {
    const auto& s = *impl_;
    return !(s.savedPos && *s.savedPos == s.undoStack.size()) || (s.inTxn && !s.txn.commands.empty());
}

std::vector<HistoryEntryInfo> CommandHistory::undoEntries() const {
    std::vector<HistoryEntryInfo> out;
    for (const auto& e : impl_->undoStack) out.push_back({e.descriptionKey, e.commands.size()});
    return out;
}
std::size_t CommandHistory::undoCount() const noexcept { return impl_->undoStack.size(); }
std::size_t CommandHistory::redoCount() const noexcept { return impl_->redoStack.size(); }

void CommandHistory::addListener(Listener* l) {
    if (l != nullptr) impl_->listeners.push_back(l);
}
void CommandHistory::removeListener(Listener* l) { std::erase(impl_->listeners, l); }

}  // namespace gp::commands
