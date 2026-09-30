#pragma once

#include <vector>
#include <atomic>
#include <cstdint>
#include <algorithm>

// A thread-safe, lock-free queue-like buffer that holds generated TTS audio chunks.
// The TTS generation thread pushes synthesized audio here.
// The AAudio output callback thread pulls audio from here for playback.
// Lock-free design prevents priority inversion in the audio callback.
class TtsChunkScheduler {
public:
    TtsChunkScheduler(size_t max_capacity)
        : buffer_(max_capacity), capacity_(max_capacity), head_(0), tail_(0) {}

    // Called by the TTS Worker Thread to push generated PCM data.
    bool PushAudio(const std::vector<int16_t>& data) {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_acquire);

        size_t available_space = capacity_ - (h - t);
        if (available_space < data.size()) {
            return false;
        }

        for (int16_t sample : data) {
            buffer_[h % capacity_] = sample;
            h++;
        }

        head_.store(h, std::memory_order_release);
        return true;
    }

    // Called by the AAudio Output Callback to read data for playback.
    void PullAudio(int16_t* out_data, size_t num_frames) {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_relaxed);

        size_t available = h - t;
        size_t frames_to_read = std::min(num_frames, available);

        for (size_t i = 0; i < frames_to_read; ++i) {
            out_data[i] = buffer_[t % capacity_];
            t++;
        }

        tail_.store(t, std::memory_order_release);

        // Fill remaining requested frames with silence to avoid xruns
        if (frames_to_read < num_frames) {
            std::fill(out_data + frames_to_read, out_data + num_frames, 0);
        }
    }

    size_t AvailableData() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return h - t;
    }

    void Clear() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

private:
    std::vector<int16_t> buffer_;
    size_t capacity_;
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};
