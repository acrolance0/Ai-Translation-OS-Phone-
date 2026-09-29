#pragma once

#include "IStreamingTtsEngine.h"
#include <android-base/logging.h>
#include <cmath>
#include <thread>
#include <chrono>
#include <algorithm>

// Mock TTS engine to simulate audio synthesis latency and generate dummy PCM audio (sine waves).
class MockStreamingTtsEngine : public IStreamingTtsEngine {
public:
    bool Initialize(const TtsConfig& config) override {
        sample_rate_ = config.sample_rate;
        LOG(INFO) << "MockStreamingTtsEngine initialized. Model: " << config.model_path
                  << ", Sample Rate: " << sample_rate_;
        return true;
    }

    bool SynthesizeChunk(const std::string& text, std::vector<int16_t>& out_audio) override {
        // Simulate synthesis latency (~50ms for a short chunk on modern ARM)
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Generate a 0.5-second beep for every word in the chunk, purely to verify
        // the audio pipeline makes sound.
        size_t words = std::count(text.begin(), text.end(), ' ') + 1;
        if (text.empty()) words = 0;

        size_t total_samples = static_cast<size_t>(sample_rate_ * 0.5 * words);
        out_audio.resize(total_samples);

        // Generate a 440Hz sine wave tone
        double frequency = 440.0;
        for (size_t i = 0; i < total_samples; ++i) {
            double t = static_cast<double>(i) / sample_rate_;
            out_audio[i] = static_cast<int16_t>(32767.0 * 0.5 * std::sin(2.0 * M_PI * frequency * t));
        }

        LOG(INFO) << "[TTS Engine] Synthesized " << words << " words into "
                  << total_samples << " samples.";
        return true;
    }

private:
    int32_t sample_rate_ = 24000;
};
