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
        // We simulate a long sentence with self-repair to test incremental logic
        size_t frames = total_samples_processed_ / 1600; // 100ms chunks

        if (frames < 10) return "we";
        if (frames < 20) return "we are";
        if (frames < 30) return "we are building";
        if (frames < 40) return "we are building a system";
        // Self correction simulation
        if (frames < 50) return "we are building an offline system";
        if (frames < 60) return "we are building an offline system for the";
        if (frames < 70) return "we are building an offline system for the OnePlus";
        return "we are building an offline system for the OnePlus 12";
    }

    bool IsEndpointDetected() override {
        // Simulate an endpoint after 80 chunks (8 seconds) of audio
        size_t frames = total_samples_processed_ / 1600;
        return (frames >= 80);
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
