#pragma once
/// @file TrackListComponent.h
/// Lista de pistas: nombre (doble clic para renombrar), «Renombrar» y «Eliminar». No toca el
/// modelo: emite callbacks que MainComponent convierte en Commands. Hilo: mensajes.

#include <functional>
#include <memory>
#include <vector>

#include <juce_gui_basics/juce_gui_basics.h>

#include "model/ProjectState.h"
#include "ui/Localisation.h"
#include "ui/Theme.h"

namespace gp::ui {

class TrackListComponent final : public juce::Component {
public:
    std::function<void(model::TrackId, std::string)> onRename;
    std::function<void(model::TrackId)> onRemove;

    TrackListComponent() {
        setTitle(tr("panel.tracks.title"));
        emptyLabel_.setText(tr("panel.tracks.empty"), juce::dontSendNotification);
        emptyLabel_.setFont(theme::font::body());
        emptyLabel_.setColour(juce::Label::textColourId, theme::colour::textSecondary);
        addAndMakeVisible(emptyLabel_);
    }

    /// Actualiza la lista. Si solo cambian nombres se edita en sitio; si cambia la estructura se
    /// reconstruye en diferido (la fila que originó el cambio puede estar dentro de su callback).
    void setTracks(const std::vector<model::TrackState>& tracks) {
        if (tracks == desired_)
            return;
        const bool sameIds = sameStructure(tracks, desired_) && rows_.size() == tracks.size();
        desired_ = tracks;
        if (sameIds) {
            for (std::size_t i = 0; i < tracks.size(); ++i)
                rows_[i]->setShownName(tracks[i].name);
            shown_ = tracks;
            return;
        }
        if (rebuildPending_)
            return;
        rebuildPending_ = true;
        juce::MessageManager::callAsync([safe = juce::Component::SafePointer<TrackListComponent>(this)] {
            if (safe != nullptr)
                safe->rebuild();
        });
    }

    /// Se llama tras reconstruir las filas (el padre puede reajustar la altura).
    std::function<void()> onContentChanged;

    int preferredHeight() const {
        return std::max<int>(theme::space::rowHeight,
                             static_cast<int>(rows_.size()) * theme::space::rowHeight);
    }

    void resized() override {
        auto area = getLocalBounds();
        emptyLabel_.setBounds(area.removeFromTop(theme::space::rowHeight));
        area = getLocalBounds();
        for (auto& row : rows_)
            row->setBounds(area.removeFromTop(theme::space::rowHeight));
    }

private:
    class Row final : public juce::Component {
    public:
        Row(TrackListComponent& owner, std::size_t index, const model::TrackState& track)
            : id_(track.id), owner_(owner) {
            number_.setText(juce::String(static_cast<int>(index) + 1), juce::dontSendNotification);
            number_.setColour(juce::Label::textColourId, theme::colour::textSecondary);
            number_.setFont(theme::font::body());
            name_.setText(juce::String::fromUTF8(track.name.c_str()), juce::dontSendNotification);
            name_.setFont(theme::font::body());
            name_.setColour(juce::Label::textColourId, theme::colour::textPrimary);
            name_.setEditable(false, true, false);  // doble clic
            name_.setTitle(tr("panel.tracks.rename"));
            name_.onTextChange = [this] {
                const auto text = name_.getText().toStdString();
                if (text.empty()) {  // no se envía; se restaura el último nombre válido
                    name_.setText(juce::String::fromUTF8(shownName_.c_str()),
                                  juce::dontSendNotification);
                    return;
                }
                if (owner_.onRename)
                    owner_.onRename(id_, text);
            };
            shownName_ = track.name;
            rename_.setButtonText(tr("panel.tracks.rename"));
            rename_.setTitle(tr("panel.tracks.rename"));
            rename_.setTooltip(tr("panel.tracks.nameRule"));
            rename_.onClick = [this] { name_.showEditor(); };
            remove_.setButtonText(tr("panel.tracks.remove"));
            remove_.setTitle(tr("panel.tracks.remove"));
            remove_.onClick = [this] {
                if (owner_.onRemove)
                    owner_.onRemove(id_);
            };
            for (juce::Component* c : std::initializer_list<juce::Component*>{
                     &number_, &name_, &rename_, &remove_})
                addAndMakeVisible(c);
            owner.addAndMakeVisible(this);
        }

        void setShownName(const std::string& name) {
            shownName_ = name;
            name_.setText(juce::String::fromUTF8(name.c_str()), juce::dontSendNotification);
        }

        void resized() override {
            using namespace theme::space;
            auto area = getLocalBounds().reduced(0, xs / 2);
            number_.setBounds(area.removeFromLeft(xl + s));
            remove_.setBounds(area.removeFromRight(buttonWidth - xl));
            area.removeFromRight(s);
            rename_.setBounds(area.removeFromRight(buttonWidth));
            area.removeFromRight(s);
            name_.setBounds(area);
        }

    private:
        model::TrackId id_;
        TrackListComponent& owner_;
        std::string shownName_;
        juce::Label number_, name_;
        juce::TextButton rename_, remove_;
    };

    static bool sameStructure(const std::vector<model::TrackState>& a,
                              const std::vector<model::TrackState>& b) {
        if (a.size() != b.size())
            return false;
        for (std::size_t i = 0; i < a.size(); ++i)
            if (a[i].id != b[i].id)
                return false;
        return true;
    }

    void rebuild() {
        rebuildPending_ = false;
        shown_ = desired_;
        rows_.clear();
        for (std::size_t i = 0; i < shown_.size(); ++i)
            rows_.push_back(std::make_unique<Row>(*this, i, shown_[i]));
        emptyLabel_.setVisible(shown_.empty());
        resized();
        repaint();
        if (onContentChanged)
            onContentChanged();
    }

    std::vector<model::TrackState> shown_, desired_;
    bool rebuildPending_ = false;
    std::vector<std::unique_ptr<Row>> rows_;
    juce::Label emptyLabel_;
};

}  // namespace gp::ui
