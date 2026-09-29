#include <iostream>
#include <unistd.h>
#include <android-base/logging.h>
#include <aaudio/AAudio.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <memory>

#include "AudioRingBuffer.h"
#include "MockStreamingAsrEngine.h"

// Define a 2-second buffer for 16kHz mono 16-bit audio
constexpr size_t RING_BUFFER_CAPACITY_FRAMES = 16000 * 2;
AudioRingBuffer audio_buffer(RING_BUFFER_CAPACITY_FRAMES);

// Diagnostic counters
std::atomic<int64_t> total_frames_read{0};
std::atomic<int32_t> xrun_count{0};

// ASR thread control
std::atomic<bool> keep_asr_running{true};

// AAudio callback for processing audio
aaudio_data_callback_result_t dataCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {

    total_frames_read += numFrames;

    int32_t current_xruns = AAudioStream_getXRunCount(stream);
    if (current_xruns > xrun_count) {
        LOG(WARNING) << "Audio XRun detected! Total: " << current_xruns;
        xrun_count = current_xruns;
    }

    // Write to lock-free ring buffer
    bool success = audio_buffer.Write(static_cast<const int16_t*>(audioData), numFrames);
    if (!success) {
        LOG(ERROR) << "Ring buffer overflow! Dropping frames.";
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// Background thread that continuously reads from the ring buffer and feeds the ASR engine
void AsrWorkerThread() {
    LOG(INFO) << "ASR Worker Thread started.";

    // Initialize the engine
    std::unique_ptr<IStreamingAsrEngine> asr_engine = std::make_unique<MockStreamingAsrEngine>();
    asr_engine->Initialize("/vendor/etc/models/asr/sherpa-onnx-zipformer");

    std::vector<int16_t> local_buffer(1600); // 100ms chunks

    std::string last_partial = "";

    while (keep_asr_running) {
        size_t available = audio_buffer.AvailableData();
        if (available > 0) {
            size_t frames_to_read = std::min(available, local_buffer.size());
            size_t frames_read = audio_buffer.Read(local_buffer.data(), frames_to_read);

            if (frames_read > 0) {
                // 1. Feed incremental audio
                asr_engine->AcceptAudio(local_buffer.data(), frames_read);

                // 2. Get incremental text
                std::string current_partial = asr_engine->GetPartialHypothesis();
                if (current_partial != last_partial) {
                    LOG(INFO) << "[ASR Partial]: " << current_partial;
                    last_partial = current_partial;
                }

                // 3. Check for endpoint (sentence boundary)
                if (asr_engine->IsEndpointDetected()) {
                    std::string final_text = asr_engine->GetFinalHypothesis();
                    LOG(INFO) << "[ASR Final]: " << final_text;
                    LOG(INFO) << "--- Passing to Translation Stage ---";
                    last_partial = ""; // Reset
                }
            }
        } else {
            // No data, sleep briefly to avoid burning CPU
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void errorCallback(AAudioStream *stream, void *userData, aaudio_result_t error) {
    LOG(ERROR) << "AAudio error: " << AAudio_convertResultToText(error);
}

int main(int argc, char** argv) {
    android::base::InitLogging(argv, android::base::LogdLogger(android::base::SYSTEM));

    LOG(INFO) << "Translator service skeleton starting up.";
    LOG(INFO) << "Initializing native audio capture pipeline...";

    AAudioStreamBuilder *builder = nullptr;
    AAudio_createStreamBuilder(&builder);

    // Configure for ASR: 16kHz, Mono, 16-bit PCM
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, 16000);

    // Request low latency and use voice recognition preset (bypasses heavy processing)
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_VOICE_RECOGNITION);

    // Setup callbacks
    AAudioStreamBuilder_setDataCallback(builder, dataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    AAudioStream *stream = nullptr;
    aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &stream);

    if (result != AAUDIO_OK) {
        LOG(ERROR) << "Failed to open AAudio stream: " << AAudio_convertResultToText(result);
        AAudioStreamBuilder_delete(builder);
        return 1;
    }

    LOG(INFO) << "AAudio stream opened successfully.";
    LOG(INFO) << "Sample Rate: " << AAudioStream_getSampleRate(stream);
    LOG(INFO) << "Channel Count: " << AAudioStream_getChannelCount(stream);
    LOG(INFO) << "Buffer Capacity: " << AAudioStream_getBufferCapacityInFrames(stream);

    result = AAudioStream_requestStart(stream);
    if (result != AAUDIO_OK) {
        LOG(ERROR) << "Failed to start AAudio stream: " << AAudio_convertResultToText(result);
        AAudioStream_close(stream);
        AAudioStreamBuilder_delete(builder);
        return 1;
    }

    LOG(INFO) << "Audio capture started. Audio is passed via ring buffer to streaming ASR thread.";

    // Start ASR worker thread
    std::thread asr_thread(AsrWorkerThread);

    // Main loop: Print diagnostic metrics periodically
    while (true) {
        sleep(5);
        LOG(INFO) << "--- Diagnostic Audio Metrics ---";
        LOG(INFO) << "Total Frames Read: " << total_frames_read;
        LOG(INFO) << "Total XRuns: " << xrun_count;
        LOG(INFO) << "Ring Buffer Backlog: " << audio_buffer.AvailableData() << " frames";
        LOG(INFO) << "Stream State: " << AAudioStream_getState(stream);
    }

    // Cleanup (unreachable in current loop, but good practice)
    keep_asr_running = false;
    asr_thread.join();
    AAudioStream_requestStop(stream);
    AAudioStream_close(stream);
    AAudioStreamBuilder_delete(builder);

    return 0;
}
