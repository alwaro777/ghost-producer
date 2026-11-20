#pragma once
/// @file AudioEngine.h
/// Fachada del motor para la app: dispositivos de audio/MIDI, callback y métricas. Envuelve
/// un IRenderCore. Hilo: TODAS las funciones solo desde el hilo de mensajes.
///
/// Reglas:
///  - La UI no llama al motor salvo a través de esta interfaz (vía app::AppServices); los
///    cambios del proyecto le llegan con setProject(), nunca escribiendo en el motor.
///  - Sin dispositivo disponible (CI, interfaz desconectada) el motor sigue vivo: status
///    open=false y la app funciona (sin sonido). Nunca lanza excepciones ni aborta.
///  - El callback de audio usa rt::ScopedRealtimeSection y desactiva denormales (FTZ/DAZ).

#include <cstdint>
#include <memory>
#include <string>

#include "core/Status.h"
#include "model/ProjectState.h"

namespace juce {
class AudioDeviceManager;
}

namespace gp::engine {

struct AudioDeviceStatus {
    bool open = false;
    std::string deviceTypeName;  ///< "CoreAudio", "Windows Audio", "ALSA"…
    std::string outputDeviceName;
    double sampleRate = 0.0;
    int bufferSize = 0;
    int outputLatencySamples = 0;
    std::string lastError;  ///< Detalle técnico del último fallo al abrir (inglés).
};

struct EngineMetrics {
    float cpuLoad = 0.0f;   ///< 0..1+ : tiempo de callback / duración del bloque, suavizado.
    float peakLeft = 0.0f;  ///< Lineal, desde la última llamada.
    float peakRight = 0.0f;
    std::uint64_t xrunCount = 0;  ///< Según el driver, si lo reporta (si no, 0).
    std::uint64_t midiEventCount =
        0;  ///< Mensajes MIDI recibidos de entradas activas (F0: solo cuenta).
    std::uint64_t rtViolationCount = 0;  ///< rt::violationCount(); 0 en release.
};

class IAudioEngine {
public:
    virtual ~IAudioEngine() = default;

    /// Abre el dispositivo. `savedDeviceState` es lo devuelto por saveDeviceState() en una
    /// sesión anterior (vacío ⇒ dispositivo por defecto, 2 salidas, 0 entradas). Si falla,
    /// devuelve deviceError pero el motor queda utilizable (ver reglas).
    [[nodiscard]] virtual Status initialise(const std::string& savedDeviceState) = 0;
    /// Para el audio y cierra dispositivos. Idempotente. Obligatorio antes de destruir.
    virtual void shutdown() = 0;

    /// Estado opaco (XML de JUCE) del dispositivo y entradas MIDI elegidos. Preferencia de
    /// la app, NO del proyecto.
    [[nodiscard]] virtual std::string saveDeviceState() const = 0;

    /// Compila y publica el proyecto (buildSnapshot + IRenderCore::publish). Llamar tras
    /// cada model::Document::Listener::documentChanged.
    virtual void setProject(const model::ProjectState& state, std::uint64_t documentRevision) = 0;

    [[nodiscard]] virtual AudioDeviceStatus deviceStatus() const = 0;
    /// Lee y reinicia los picos; el resto son acumulados. Pensado para un timer de UI (30 Hz).
    [[nodiscard]] virtual EngineMetrics takeMetrics() = 0;

    /// EXCEPCIÓN documentada (ADR 0004): solo para alojar juce::AudioDeviceSelectorComponent
    /// en el panel de ajustes. Nadie más debe usarlo.
    [[nodiscard]] virtual juce::AudioDeviceManager& deviceManagerForSettingsUI() = 0;
};

[[nodiscard]] std::unique_ptr<IAudioEngine> createAudioEngine();

}  // namespace gp::engine
