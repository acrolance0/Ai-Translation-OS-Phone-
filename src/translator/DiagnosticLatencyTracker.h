#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <cstdint>
#include <android-base/logging.h>

// Tracks latency milestones for a single spoken utterance.
struct UtteranceMetrics {
    uint64_t speech_onset_ms = 0;
    uint64_t first_asr_partial_ms = 0;
    uint64_t first_translation_chunk_ms = 0; // The first COMMITTED chunk meant for TTS
    uint64_t first_tts_synthesis_ms = 0;     // When the audio was generated
    uint64_t first_bt_output_ms = 0;         // When AAudio output callback pulled it
    uint64_t end_of_source_ms = 0;           // When endpoint (silence) was detected
    uint64_t end_of_output_ms = 0;           // When the last TTS chunk was pulled
};

// A thread-safe diagnostic tool to prove incremental translation capabilities
// and measure latency at each stage of the pipeline.
class DiagnosticLatencyTracker {
public:
    static DiagnosticLatencyTracker& GetInstance() {
        static DiagnosticLatencyTracker instance;
        return instance;
    }

    uint64_t GetNowMs() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    void RecordSpeechOnset(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_[utterance_id].speech_onset_ms == 0) {
            metrics_[utterance_id].speech_onset_ms = GetNowMs();
        }
    }

    void RecordFirstAsrPartial(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_[utterance_id].first_asr_partial_ms == 0) {
            metrics_[utterance_id].first_asr_partial_ms = GetNowMs();
        }
    }

    void RecordFirstTranslationChunk(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_[utterance_id].first_translation_chunk_ms == 0) {
            metrics_[utterance_id].first_translation_chunk_ms = GetNowMs();
        }
    }

    void RecordFirstTtsSynthesis(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_[utterance_id].first_tts_synthesis_ms == 0) {
            metrics_[utterance_id].first_tts_synthesis_ms = GetNowMs();
        }
    }

    void RecordFirstBtOutput(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_[utterance_id].first_bt_output_ms == 0) {
            metrics_[utterance_id].first_bt_output_ms = GetNowMs();
        }
    }

    void RecordEndOfSource(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[utterance_id].end_of_source_ms = GetNowMs();
    }

    void RecordEndOfOutput(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[utterance_id].end_of_output_ms = GetNowMs();
    }

    void PrintSummary(uint64_t utterance_id) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = metrics_.find(utterance_id);
        if (it == metrics_.end()) return;

        const auto& m = it->second;

        LOG(INFO) << "=== LATENCY REPORT FOR UTTERANCE " << utterance_id << " ===";

        if (m.first_translation_chunk_ms > 0 && m.end_of_source_ms > 0) {
            int64_t incremental_margin = m.end_of_source_ms - m.first_translation_chunk_ms;
            if (incremental_margin > 0) {
                LOG(INFO) << "[SUCCESS] True Streaming Confirmed: First translation committed "
                          << incremental_margin << "ms BEFORE source speech ended.";
            } else {
                LOG(ERROR) << "[FAIL] Blocking detected! Translation committed "
                           << -incremental_margin << "ms AFTER source speech ended.";
            }
        }

        LOG(INFO) << "Speech Onset to First ASR Partial: "
                  << (m.first_asr_partial_ms - m.speech_onset_ms) << " ms";
        LOG(INFO) << "Speech Onset to First Trans Chunk: "
                  << (m.first_translation_chunk_ms - m.speech_onset_ms) << " ms";
        LOG(INFO) << "Speech Onset to First TTS Output : "
                  << (m.first_tts_synthesis_ms - m.speech_onset_ms) << " ms";
        LOG(INFO) << "Speech Onset to BT Output Buffer : "
                  << (m.first_bt_output_ms - m.speech_onset_ms) << " ms";

        if (m.end_of_output_ms > m.end_of_source_ms) {
            LOG(INFO) << "Total Lag (Speaker stop to Output stop): "
                      << (m.end_of_output_ms - m.end_of_source_ms) << " ms";
        }

        LOG(INFO) << "===========================================";
    }

private:
    DiagnosticLatencyTracker() = default;
    std::mutex mutex_;
    std::map<uint64_t, UtteranceMetrics> metrics_;
};
