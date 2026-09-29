#pragma once

#include "IStreamingAsrEngine.h"
#include <string>
#include <android-base/logging.h>

// A mock implementation of a streaming ASR engine to prove the audio pipeline, threading,
// and incremental delivery architecture without cross-compiling a heavy ONNX runtime.
class MockStreamingAsrEngine : public IStreamingAsrEngine {
public:
    bool Initialize(const std::string& model_dir) override {
        LOG(INFO) << "MockStreamingAsrEngine initialized with model path: " << model_dir;
        return true;
    }

    void AcceptAudio(const int16_t* data, size_t num_samples) override {
        // In a real engine (like Sherpa-ONNX), this feeds the C++ API.
        // Here, we just count samples to simulate progress.
        total_samples_processed_ += num_samples;
    }

    std::string GetPartialHypothesis() override {
        // Simulate incremental word output based on how much audio we've processed
        size_t seconds_processed = total_samples_processed_ / 16000;

        if (seconds_processed < 2) return "hello";
        if (seconds_processed < 4) return "hello world";
        if (seconds_processed < 6) return "hello world this is";
        return "hello world this is a streaming test";
    }

    bool IsEndpointDetected() override {
        // Simulate an endpoint after 8 seconds of audio
        return (total_samples_processed_ >= (8 * 16000));
    }

    std::string GetFinalHypothesis() override {
        std::string final_text = GetPartialHypothesis() + ".";
        // Reset for next utterance
        total_samples_processed_ = 0;
        return final_text;
    }

private:
    size_t total_samples_processed_ = 0;
};
