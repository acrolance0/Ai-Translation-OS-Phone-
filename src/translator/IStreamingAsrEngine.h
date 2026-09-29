#pragma once

#include <string>
#include <vector>
#include <cstdint>

// Clean interface separating the streaming ASR implementation from the audio pipeline
class IStreamingAsrEngine {
public:
    virtual ~IStreamingAsrEngine() = default;

    // Initializes the engine with paths to acoustic/language models
    virtual bool Initialize(const std::string& model_dir) = 0;

    // Feeds incremental audio PCM frames into the ASR engine
    virtual void AcceptAudio(const int16_t* data, size_t num_samples) = 0;

    // Retrieves the current partial transcription hypothesis (e.g. while the user is still speaking)
    virtual std::string GetPartialHypothesis() = 0;

    // Indicates whether the engine has detected an endpoint (the user finished a sentence)
    virtual bool IsEndpointDetected() = 0;

    // Retrieves the final, corrected transcription for the last speech segment and resets for the next
    virtual std::string GetFinalHypothesis() = 0;
};
