#include <iostream>
#include <unistd.h>
#include <android-base/logging.h>
#include <aaudio/AAudio.h>
#include <atomic>
#include <chrono>
#include <mutex>

// Diagnostic counters
std::atomic<int64_t> total_frames_read{0};
std::atomic<int32_t> xrun_count{0};
std::atomic<int64_t> max_latency_ns{0};

// AAudio callback for processing audio
aaudio_data_callback_result_t dataCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {

    // Increment total frames
    total_frames_read += numFrames;

    // Check for xruns (overruns/underruns)
    int32_t current_xruns = AAudioStream_getXRunCount(stream);
    if (current_xruns > xrun_count) {
        LOG(WARNING) << "Audio XRun detected! Total: " << current_xruns;
        xrun_count = current_xruns;
    }

    // Since this is diagnostic mode, we don't save the audioData anywhere.
    // In a real ASR pipeline, we would push 'audioData' to a ring buffer here.

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
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

    LOG(INFO) << "Audio capture started in diagnostic mode. Audio is NOT being recorded to disk.";

    // Main loop: Print diagnostic metrics periodically
    while (true) {
        sleep(5);
        LOG(INFO) << "--- Diagnostic Audio Metrics ---";
        LOG(INFO) << "Total Frames Read: " << total_frames_read;
        LOG(INFO) << "Total XRuns: " << xrun_count;
        LOG(INFO) << "Stream State: " << AAudioStream_getState(stream);
    }

    // Cleanup (unreachable in current loop, but good practice)
    AAudioStream_requestStop(stream);
    AAudioStream_close(stream);
    AAudioStreamBuilder_delete(builder);

    return 0;
}
