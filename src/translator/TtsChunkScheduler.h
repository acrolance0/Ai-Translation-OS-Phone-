#pragma once

#include <vector>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <algorithm>

// A thread-safe queue-like buffer that holds generated TTS audio chunks.
// The TTS generation thread pushes synthesized audio here.
// The AAudio output callback thread pulls audio from here for playback.
class TtsChunkScheduler {
public:
    TtsChunkScheduler(size_t max_capacity)
        : buffer_(max_capacity), capacity_(max_capacity), head_(0), tail_(0) {}

    // Called by the TTS Worker Thread to push generated PCM data.
    // Blocks if the buffer is full (providing backpressure to synthesis).
    bool PushAudio(const std::vector<int16_t>& data) {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t available_space = capacity_ - (head_ - tail_);
        if (available_space < data.size()) {
            // Buffer overflow - in a real system we might block on a condition_variable,
            // but for simplicity we'll just fail the push if full.
            return false;
        }

        for (int16_t sample : data) {
            buffer_[head_ % capacity_] = sample;
            head_++;
        }
        return true;
    }

    // Called by the AAudio Output Callback to read data for playback.
    // If there isn't enough data, it fills the remainder with silence (0s).
    void PullAudio(int16_t* out_data, size_t num_frames) {
        std::lock_guard<std::mutex> lock(mutex_);

        size_t available = head_ - tail_;
        size_t frames_to_read = std::min(num_frames, available);

        for (size_t i = 0; i < frames_to_read; ++i) {
            out_data[i] = buffer_[tail_ % capacity_];
            tail_++;
        }

        // Fill remaining requested frames with silence to avoid xruns
        if (frames_to_read < num_frames) {
            std::fill(out_data + frames_to_read, out_data + num_frames, 0);
        }
    }

    size_t AvailableData() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return head_ - tail_;
    }

    void Clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = 0;
        tail_ = 0;
    }

private:
    std::vector<int16_t> buffer_;
    size_t capacity_;
    size_t head_;
    size_t tail_;
    mutable std::mutex mutex_;
};
