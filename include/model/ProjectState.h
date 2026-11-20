#pragma once
/// @file ProjectState.h
/// Estado del documento del proyecto (F0). Tipos de valor puros: sin JUCE, sin punteros
/// compartidos, copiables. Es lo que se guarda en disco (ADR 0002) y lo que mutan los
/// Commands (ADR 0003).
///
/// Hilo: un ProjectState vivo pertenece al hilo de mensajes (a través de model::Document).
/// Las copias pueden leerse en cualquier hilo NO de audio. El motor nunca lee ProjectState
/// desde el callback: recibe un engine::EngineSnapshot compilado (ADR 0004).
///
/// Punto de extensión: F1 añade clips, volumen/pan/mute por pista, etc. Cada campo nuevo
/// que cambie el significado de un archivo existente exige subir kCurrentSchemaVersion y
/// registrar una migración (model/Migrations.h). Campos nuevos con valor por defecto
/// neutro pueden añadirse sin migración, pero sí subiendo la versión.

#include <compare>
#include <cstdint>
#include <string>
#include <vector>

namespace gp::model {

/// Rangos válidos. Los Commands rechazan (ErrorCode::invalidArgument) valores fuera de rango;
/// el deserializador también. Nunca se "clampa" en silencio.
namespace limits {
inline constexpr double kMinTempoBpm = 20.0;
inline constexpr double kMaxTempoBpm = 999.0;
inline constexpr int kMinTimeSigNumerator = 1;
inline constexpr int kMaxTimeSigNumerator = 32;
/// El denominador debe ser potencia de 2 en [1, 32].
inline constexpr int kMaxTimeSigDenominator = 32;
inline constexpr double kMinToneFrequencyHz = 20.0;
inline constexpr double kMaxToneFrequencyHz = 20000.0;
inline constexpr float kMinToneGainDb = -60.0f;
inline constexpr float kMaxToneGainDb = 0.0f;
inline constexpr std::size_t kMaxTrackNameBytes = 64;  ///< UTF-8, no vacío.
inline constexpr std::size_t kMaxTracks = 1024;
}  // namespace limits

/// Identificador estable de pista. Único dentro del proyecto, nunca se reutiliza
/// (ver ProjectState::nextTrackId). Invariante: value ∈ [1, 2^53) para que sea exacto en JSON.
struct TrackId {
    std::uint64_t value = 0;  ///< 0 = inválido.
    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    friend auto operator<=>(const TrackId&, const TrackId&) = default;
};

/// Pista. En F0 es solo modelo (no produce audio); sirve para probar comandos estructurales.
struct TrackState {
    TrackId id;
    std::string name;  ///< UTF-8, 1..kMaxTrackNameBytes bytes.
    friend bool operator==(const TrackState&, const TrackState&) = default;
};

/// Tempo y compás globales (el mapa de tempo llega en F1+).
struct TransportState {
    double tempoBpm = 120.0;
    int timeSigNumerator = 4;
    int timeSigDenominator = 4;
    friend bool operator==(const TransportState&, const TransportState&) = default;
};

/// Tono de prueba de F0 (seno). Desaparece del modelo cuando haya instrumentos; su
/// eliminación será una migración.
struct TestToneState {
    bool enabled = false;
    double frequencyHz = 440.0;
    float gainDb = -18.0f;
    friend bool operator==(const TestToneState&, const TestToneState&) = default;
};

/// Raíz del documento. El nombre del proyecto NO se guarda: es el nombre del archivo.
/// Invariantes:
///  - Todos los TrackId de `tracks` son válidos y distintos.
///  - nextTrackId > max(tracks[i].id.value) (o 1 si no hay pistas).
///  - El orden de `tracks` es el orden visible.
struct ProjectState {
    TransportState transport;
    TestToneState testTone;
    std::vector<TrackState> tracks;
    std::uint64_t nextTrackId = 1;
    friend bool operator==(const ProjectState&, const ProjectState&) = default;
};

/// Comprueba todas las invariantes y rangos anteriores. Lo usan el deserializador y los
/// tests. Hilo: cualquiera salvo audio.
[[nodiscard]] bool isValid(const ProjectState& state, std::string* whyNot = nullptr);

}  // namespace gp::model
