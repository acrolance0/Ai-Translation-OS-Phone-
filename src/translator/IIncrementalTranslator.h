#pragma once

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

// Represents the state of a translated segment.
enum class TranslationStatus {
    // A draft translation based on partial source context.
    // May be revoked or revised. Suitable for UI display, but NOT for TTS.
    PROVISIONAL,

    // A finalized chunk of translation. The source context has been locked in.
    // Safe to send to the Text-to-Speech (TTS) engine.
    COMMITTED,

    // Indicates that a previously emitted PROVISIONAL translation has been invalidated
    // by new source context and should be removed from the UI.
    REVOKED
};

// Represents an event emitted by the incremental translation engine.
struct TranslationEvent {
    // Unique identifier for the utterance segment being translated
    uint64_t utterance_id;

    // The sequence number of this event within the utterance
    uint32_t sequence_id;

    // The status of the translated text
    TranslationStatus status;

    // The translated text (empty if status == REVOKED)
    std::string translated_text;

    // Timing metadata (e.g., when the source audio for this chunk was captured)
    uint64_t source_timestamp_ms;
};

// Callback type for receiving translation events
using TranslationCallback = std::function<void(const TranslationEvent&)>;

// Configuration for the translation engine
struct TranslationConfig {
    std::string model_dir;
    std::string source_language; // e.g., "en"
    std::string target_language; // e.g., "es"
};

// The core interface for incremental translation
class IIncrementalTranslator {
public:
    virtual ~IIncrementalTranslator() = default;

    // Initializes the engine with models and language pairs
    virtual bool Initialize(const TranslationConfig& config) = 0;

    // Registers the callback that will receive translation events
    virtual void SetEventCallback(TranslationCallback callback) = 0;

    // Called when the ASR engine produces a new partial hypothesis.
    // The translator may use this to update its internal state and emit PROVISIONAL events.
    virtual void PushPartialSource(uint64_t utterance_id, const std::string& partial_text, uint64_t timestamp_ms) = 0;

    // Called when the ASR engine detects an endpoint and finalizes a sentence/segment.
    // The translator will flush its state and emit COMMITTED events.
    virtual void PushCommittedSource(uint64_t utterance_id, const std::string& final_text, uint64_t timestamp_ms) = 0;
};
