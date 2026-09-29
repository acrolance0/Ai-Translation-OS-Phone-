#pragma once

#include <vector>
#include <atomic>
#include <cstdint>
#include <algorithm>

// A simple, lock-free single-producer single-consumer ring buffer.
// Ideal for passing audio data from the AAudio callback to the ASR thread
// without risking thread blocking via mutexes.
class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacity)
        : buffer_(capacity), capacity_(capacity), head_(0), tail_(0) {}

    // Called by the producer (AAudio callback)
    bool Write(const int16_t* data, size_t num_frames) {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_acquire);

        size_t available_space = capacity_ - (h - t);
        if (available_space < num_frames) {
            return false; // Buffer overflow
        }

        size_t h_idx = h % capacity_;
        size_t chunk1 = std::min(num_frames, capacity_ - h_idx);
        size_t chunk2 = num_frames - chunk1;

        std::copy(data, data + chunk1, buffer_.begin() + h_idx);
        if (chunk2 > 0) {
            std::copy(data + chunk1, data + num_frames, buffer_.begin());
        }

        head_.store(h + num_frames, std::memory_order_release);
        return true;
    }

    // Called by the consumer (ASR Worker Thread)
    size_t Read(int16_t* data, size_t num_frames) {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_relaxed);

        size_t available_data = h - t;
        size_t frames_to_read = std::min(num_frames, available_data);
        if (frames_to_read == 0) {
            return 0; // Nothing to read
        }

        size_t t_idx = t % capacity_;
        size_t chunk1 = std::min(frames_to_read, capacity_ - t_idx);
        size_t chunk2 = frames_to_read - chunk1;

        std::copy(buffer_.begin() + t_idx, buffer_.begin() + t_idx + chunk1, data);
        if (chunk2 > 0) {
            std::copy(buffer_.begin(), buffer_.begin() + chunk2, data + chunk1);
        }

        tail_.store(t + frames_to_read, std::memory_order_release);
        return frames_to_read;
    }

    size_t AvailableData() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return h - t;
    }

private:
    std::vector<int16_t> buffer_;
    size_t capacity_;
    alignas(64) std::atomic<size_t> head_;
    alignas(64) std::atomic<size_t> tail_;
};
