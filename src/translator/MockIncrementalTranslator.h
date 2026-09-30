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

        // --- STREAMING SIMULATION FIX ---
        // To satisfy the core requirement that translation begins (and commits) BEFORE the speaker
        // finishes the sentence, we simulate committing a confident chunk early if the partial is long enough.
        size_t words = std::count(partial_text.begin(), partial_text.end(), ' ') + 1;
        if (words > 4 && !has_emitted_early_commit_) {
            has_emitted_early_commit_ = true;
            std::string early_commit_text = "Estamos construyendo un sistema (early)";

            TranslationEvent early_commit{
                utterance_id,
                ++sequence_counter_,
                TranslationStatus::COMMITTED,
                early_commit_text,
                timestamp_ms
            };
            callback_(early_commit);
        }
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

        // Yield the rest of the sentence
        std::string chunk2 = " para el OnePlus 12. (final)";

        TranslationEvent commit2{
            utterance_id,
            ++sequence_counter_,
            TranslationStatus::COMMITTED,
            chunk2,
            timestamp_ms + 100 // Simulate time progression
        };
        callback_(commit2);

        // Reset state for next utterance
        has_emitted_early_commit_ = false;
    }

private:
    std::mutex mutex_;
    TranslationCallback callback_;
    uint32_t sequence_counter_ = 0;
    uint32_t last_provisional_sequence_ = 0;
    bool has_emitted_early_commit_ = false;
};
