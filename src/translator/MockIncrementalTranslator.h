#pragma once

#include "IIncrementalTranslator.h"
#include <android-base/logging.h>
#include <map>
#include <mutex>
#include <thread>
#include <chrono>

// A mock implementation of the incremental translator to validate the architecture,
// state-machine transitions (Provisional -> Revoked -> Committed), and integration
// without requiring heavy ML inference on-device.
class MockIncrementalTranslator : public IIncrementalTranslator {
public:
    bool Initialize(const TranslationConfig& config) override {
        LOG(INFO) << "MockIncrementalTranslator initialized. Src: " << config.source_language
                  << " -> Tgt: " << config.target_language;
        return true;
    }

    void SetEventCallback(TranslationCallback callback) override {
        std::lock_guard<std::mutex> lock(mutex_);
        callback_ = callback;
    }

    void PushPartialSource(uint64_t utterance_id, const std::string& partial_text, uint64_t timestamp_ms) override {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!callback_) return;

        // Simulate a revision logic:
        // If the partial text is "hello world this is", we might provisionally translate it.
        // If it changes, we revoke the old one.

        // Revoke the last provisional if it exists
        if (last_provisional_sequence_ > 0) {
            TranslationEvent revoke_event{
                utterance_id,
                ++sequence_counter_,
                TranslationStatus::REVOKED,
                "",
                timestamp_ms
            };
            callback_(revoke_event);
        }

        // Simulate translation of the new partial text
        std::string mock_translation = "[Provisional]: " + partial_text + " (es)";

        TranslationEvent prov_event{
            utterance_id,
            ++sequence_counter_,
            TranslationStatus::PROVISIONAL,
            mock_translation,
            timestamp_ms
        };

        last_provisional_sequence_ = prov_event.sequence_id;
        callback_(prov_event);
    }

    void PushCommittedSource(uint64_t utterance_id, const std::string& final_text, uint64_t timestamp_ms) override {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!callback_) return;

        // Revoke any lingering provisional translations before committing
        if (last_provisional_sequence_ > 0) {
            TranslationEvent revoke_event{
                utterance_id,
                ++sequence_counter_,
                TranslationStatus::REVOKED,
                "",
                timestamp_ms
            };
            callback_(revoke_event);
            last_provisional_sequence_ = 0;
        }

        // Simulate processing time for final translation
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        // Break final text into chunks suitable for TTS (simulated)
        std::string chunk1 = "Hola mundo, ";
        std::string chunk2 = "esta es una prueba definitiva.";

        TranslationEvent commit1{
            utterance_id,
            ++sequence_counter_,
            TranslationStatus::COMMITTED,
            chunk1,
            timestamp_ms
        };
        callback_(commit1);

        TranslationEvent commit2{
            utterance_id,
            ++sequence_counter_,
            TranslationStatus::COMMITTED,
            chunk2,
            timestamp_ms + 100 // Simulate time progression
        };
        callback_(commit2);
    }

private:
    std::mutex mutex_;
    TranslationCallback callback_;
    uint32_t sequence_counter_ = 0;
    uint32_t last_provisional_sequence_ = 0;
};
