#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Configuration for the TTS engine
struct TtsConfig {
    std::string model_path;
    int32_t sample_rate; // e.g., 22050 or 24000
    int32_t num_channels; // 1 (Mono)
};

// Interface for a streaming Text-To-Speech engine.
class IStreamingTtsEngine {
public:
    virtual ~IStreamingTtsEngine() = default;

    // Initializes the TTS engine with the given model
    virtual bool Initialize(const TtsConfig& config) = 0;

    // Synthesizes a chunk of text (e.g., a phrase or sentence) into PCM audio.
    // In a production streaming system (like Piper), this could also yield audio
    // incrementally via a callback, but for chunked processing, returning a vector
    // of synthesized 16-bit PCM samples is highly efficient.
    virtual bool SynthesizeChunk(const std::string& text, std::vector<int16_t>& out_audio) = 0;
};
