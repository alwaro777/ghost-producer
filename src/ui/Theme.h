#pragma once
/// @file Theme.h
/// Tokens del tema oscuro de GhostProducer: color, tipografía, espaciado y movimiento.
/// Los componentes NO usan valores sueltos: todo sale de aquí. Hilo: mensajes.

#include <juce_graphics/juce_graphics.h>

namespace gp::ui::theme {

namespace colour {
inline const juce::Colour background{0xff15171c};
inline const juce::Colour panel{0xff1e2128};
inline const juce::Colour panelBorder{0xff2e333d};
inline const juce::Colour textPrimary{0xffe8eaf0};
inline const juce::Colour textSecondary{0xff9aa1b0};
inline const juce::Colour accent{0xff4fd1c5};
inline const juce::Colour accentText{0xff0b1d1b};
inline const juce::Colour focus{0xfff2c14e};
inline const juce::Colour meterLow{0xff3fbf7f};
inline const juce::Colour meterHigh{0xffe0b341};
inline const juce::Colour meterClip{0xffe5534b};
inline const juce::Colour meterBack{0xff101216};
inline const juce::Colour ok{0xff3fbf7f};
inline const juce::Colour error{0xffe5534b};
inline const juce::Colour banner{0xff3a2f14};
}  // namespace colour

namespace space {
inline constexpr int xs = 4;
inline constexpr int s = 8;
inline constexpr int m = 12;
inline constexpr int l = 16;
inline constexpr int xl = 24;
inline constexpr int rowHeight = 28;
inline constexpr int controlHeight = 26;
inline constexpr int labelWidth = 96;
inline constexpr int valueWidth = 70;
inline constexpr int buttonWidth = 112;
inline constexpr int meterWidth = 90;
inline constexpr int meterBarHeight = 8;
inline constexpr int statusBarHeight = 52;
inline constexpr int panelHeight = 176;
inline constexpr int bannerHeight = 56;
inline constexpr float cornerRadius = 6.0f;
inline constexpr float focusThickness = 2.0f;
}  // namespace space

namespace font {
inline juce::Font body() { return juce::Font(juce::FontOptions(14.0f)); }
inline juce::Font title() { return juce::Font(juce::FontOptions(13.0f, juce::Font::bold)); }
inline juce::Font small() { return juce::Font(juce::FontOptions(12.0f)); }
}  // namespace font

namespace motion {
inline constexpr int uiTimerHz = 30;
/// Caída de los medidores por tick (lineal), ~0,3 s de máximo a cero.
inline constexpr float meterFallPerTick = 0.11f;
/// Los textos de la barra de estado se refrescan cada N ticks (~4 veces/s).
inline constexpr int statusRefreshTicks = 8;
inline constexpr float meterFloorDb = -60.0f;
}  // namespace motion

}  // namespace gp::ui::theme
