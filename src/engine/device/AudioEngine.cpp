#include "engine/AudioEngine.h"

#include <algorithm>
#include <atomic>
#include <memory>

#include <juce_audio_devices/juce_audio_devices.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_events/juce_events.h>

#include "engine/EngineSnapshot.h"
#include "engine/RenderCore.h"
#include "rt/RtGuard.h"

namespace gp::engine {
namespace {

constexpr int kMaxCoreChannels = 2;
constexpr int kGarbageTimerHz = 20;

class AudioEngine final : public IAudioEngine,
                          private juce::AudioIODeviceCallback,
                          private juce::MidiInputCallback,
                          private juce::Timer {
public:
    AudioEngine() : core_(createRenderCore()) {}
    ~AudioEngine() override { shutdown(); }

    Status initialise(const std::string& savedDeviceState) override {
        shutdown();  // idempotente; permite reinicializar
        lastError_.clear();

        std::unique_ptr<juce::XmlElement> xml;
        if (!savedDeviceState.empty()) {
            xml = juce::XmlDocument::parse(juce::String::fromUTF8(savedDeviceState.c_str()));
            if (xml == nullptr) {
                lastError_ = "Saved device state is not valid XML";
                // Motor utilizable pero sin dispositivo.
                startHousekeeping();
                return Status::error(ErrorCode::deviceError, lastError_);
            }
        }

        juce::String err;
        try {
            err = manager_.initialise(0, 2, xml.get(), true);
        } catch (...) {
            err = "Unexpected exception while opening the audio device";
        }
        if (err.isNotEmpty())
            lastError_ = err.toStdString();

        manager_.addAudioCallback(this);
        manager_.addMidiInputDeviceCallback({}, this);  // todas las entradas activas
        callbacksAdded_ = true;
        startHousekeeping();

        if (manager_.getCurrentAudioDevice() == nullptr) {
            if (lastError_.empty())
                lastError_ = "No audio device available";
            return Status::error(ErrorCode::deviceError, lastError_);
        }
        if (!lastError_.empty())
            return Status::error(ErrorCode::deviceError, lastError_);
        return Status::ok();
    }

    void shutdown() override {
        stopTimer();
        if (callbacksAdded_) {
            manager_.removeMidiInputDeviceCallback({}, this);
            manager_.removeAudioCallback(this);
            callbacksAdded_ = false;
        }
        manager_.closeAudioDevice();
    }

    std::string saveDeviceState() const override {
        if (auto xml = manager_.createStateXml())
            return xml->toString().toStdString();
        return {};
    }

    void setProject(const model::ProjectState& state, std::uint64_t documentRevision) override {
        GP_RT_ASSERT_NOT_REALTIME();
        const EngineSnapshot snap = buildSnapshot(state, documentRevision);
        const juce::ScopedLock sl(writerLock_);
        lastSnapshot_ = snap;
        hasSnapshot_ = true;
        core_->collectGarbage();
        core_->publish(std::make_unique<const EngineSnapshot>(snap));
    }

    AudioDeviceStatus deviceStatus() const override {
        AudioDeviceStatus s;
        s.lastError = lastError_;
        if (auto* dev = manager_.getCurrentAudioDevice()) {
            s.open = dev->isOpen();
            s.deviceTypeName = dev->getTypeName().toStdString();
            s.outputDeviceName = dev->getName().toStdString();
            s.sampleRate = dev->getCurrentSampleRate();
            s.bufferSize = dev->getCurrentBufferSizeSamples();
            s.outputLatencySamples = dev->getOutputLatencyInSamples();
        }
        return s;
    }

    EngineMetrics takeMetrics() override {
        EngineMetrics m;
        m.cpuLoad = static_cast<float>(loadMeasurer_.getLoadAsProportion());
        const auto peaks = core_->takePeaks();
        m.peakLeft = peaks.left;
        m.peakRight = peaks.right;
        if (auto* dev = manager_.getCurrentAudioDevice()) {
            const int x = dev->getXRunCount();
            if (x >= 0)
                m.xrunCount = static_cast<std::uint64_t>(x);
        }
        m.midiEventCount = midiCount_.load(std::memory_order_relaxed);
        m.rtViolationCount = rt::violationCount();
        return m;
    }

    juce::AudioDeviceManager& deviceManagerForSettingsUI() override { return manager_; }

private:
    void startHousekeeping() { startTimerHz(kGarbageTimerHz); }

    void timerCallback() override {
        GP_RT_ASSERT_NOT_REALTIME();
        const juce::ScopedLock sl(writerLock_);
        core_->collectGarbage();
    }

    // ---- AudioIODeviceCallback ----
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override {
        GP_RT_ASSERT_NOT_REALTIME();
        const double sr = device->getCurrentSampleRate();
        const int block = std::max(1, device->getCurrentBufferSizeSamples());
        const int outs = device->getActiveOutputChannels().countNumberOfSetBits();

        ProcessSpec spec;
        spec.sampleRate = sr;
        spec.maxBlockSize = block;
        spec.numOutputChannels = std::clamp(outs, 1, kMaxCoreChannels);
        coreChannels_ = spec.numOutputChannels;
        maxBlock_ = block;

        loadMeasurer_.reset(sr, block);
        {
            const juce::ScopedLock sl(writerLock_);
            core_->prepare(spec);
            if (hasSnapshot_)
                core_->publish(std::make_unique<const EngineSnapshot>(lastSnapshot_));
        }
    }

    void audioDeviceStopped() override {
        GP_RT_ASSERT_NOT_REALTIME();
        const juce::ScopedLock sl(writerLock_);
        core_->release();
    }

    void audioDeviceIOCallbackWithContext(const float* const* /*inputs*/, int /*numIn*/,
                                          float* const* outputs, int numOut, int numSamples,
                                          const juce::AudioIODeviceCallbackContext&) override {
        const rt::ScopedRealtimeSection rtSection;
        const juce::ScopedNoDenormals noDenormals;
        const juce::AudioProcessLoadMeasurer::ScopedTimer timer(loadMeasurer_, numSamples);

        if (numOut <= 0 || numSamples <= 0)
            return;

        const int active = std::min(numOut, coreChannels_);
        const int block = std::max(1, maxBlock_);
        float* ptrs[kMaxCoreChannels];

        for (int start = 0; start < numSamples; start += block) {
            const int n = std::min(block, numSamples - start);
            for (int c = 0; c < active; ++c)
                ptrs[c] = outputs[c] + start;
            core_->process(ptrs, active, n);
        }
        for (int c = active; c < numOut; ++c)
            if (outputs[c] != nullptr)
                juce::FloatVectorOperations::clear(outputs[c], numSamples);
    }

    // ---- MidiInputCallback ----
    void handleIncomingMidiMessage(juce::MidiInput*, const juce::MidiMessage&) override {
        midiCount_.fetch_add(1, std::memory_order_relaxed);
    }

    juce::AudioDeviceManager manager_;
    std::unique_ptr<IRenderCore> core_;
    juce::AudioProcessLoadMeasurer loadMeasurer_;
    juce::CriticalSection writerLock_;  // solo hilos no RT (publish/collect/prepare/release)
    EngineSnapshot lastSnapshot_;
    bool hasSnapshot_ = false;
    bool callbacksAdded_ = false;
    std::string lastError_;
    std::atomic<std::uint64_t> midiCount_{0};
    int coreChannels_ = kMaxCoreChannels;  // escrito antes de arrancar el callback
    int maxBlock_ = 512;
};

}  // namespace

std::unique_ptr<IAudioEngine> createAudioEngine() {
    return std::make_unique<AudioEngine>();
}

}  // namespace gp::engine
