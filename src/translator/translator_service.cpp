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
#include "MockIncrementalTranslator.h"
#include "TtsChunkScheduler.h"
#include "MockStreamingTtsEngine.h"

// Input: 16kHz mono 16-bit audio
constexpr size_t INPUT_RING_BUFFER_CAPACITY = 16000 * 2;
AudioRingBuffer audio_buffer(INPUT_RING_BUFFER_CAPACITY);

// Output: 24kHz mono 16-bit audio (TTS pipeline)
constexpr size_t OUTPUT_SCHEDULER_CAPACITY = 24000 * 5; // 5 seconds of audio
TtsChunkScheduler tts_scheduler(OUTPUT_SCHEDULER_CAPACITY);

// Diagnostic counters
std::atomic<int64_t> total_frames_read{0};
std::atomic<int32_t> input_xrun_count{0};
std::atomic<int64_t> total_frames_written{0};
std::atomic<int32_t> output_xrun_count{0};

// ASR thread control
std::atomic<bool> keep_asr_running{true};

// AAudio Input callback for microphone capture
aaudio_data_callback_result_t inputDataCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {

    total_frames_read += numFrames;

    int32_t current_xruns = AAudioStream_getXRunCount(stream);
    if (current_xruns > input_xrun_count) {
        LOG(WARNING) << "Input Audio XRun detected! Total: " << current_xruns;
        input_xrun_count = current_xruns;
    }

    // Write to lock-free ring buffer
    bool success = audio_buffer.Write(static_cast<const int16_t*>(audioData), numFrames);
    if (!success) {
        LOG(ERROR) << "Input ring buffer overflow! Dropping frames.";
    }

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// AAudio Output callback for TTS playback (Bluetooth/Speaker)
aaudio_data_callback_result_t outputDataCallback(
        AAudioStream *stream,
        void *userData,
        void *audioData,
        int32_t numFrames) {

    total_frames_written += numFrames;

    int32_t current_xruns = AAudioStream_getXRunCount(stream);
    if (current_xruns > output_xrun_count) {
        LOG(WARNING) << "Output Audio XRun detected! Total: " << current_xruns;
        output_xrun_count = current_xruns;
    }

    // Pull from our TTS Chunk Scheduler
    tts_scheduler.PullAudio(static_cast<int16_t*>(audioData), numFrames);

    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}

// Helper to get current timestamp
uint64_t GetCurrentTimeMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

// Background thread that continuously reads from the ring buffer and feeds the ASR engine
void AsrWorkerThread() {
    LOG(INFO) << "ASR Worker Thread started.";

    // Initialize the ASR engine
    std::unique_ptr<IStreamingAsrEngine> asr_engine = std::make_unique<MockStreamingAsrEngine>();
    asr_engine->Initialize("/vendor/etc/models/asr/sherpa-onnx-zipformer");

    // Initialize the Translation engine
    std::unique_ptr<IIncrementalTranslator> translator = std::make_unique<MockIncrementalTranslator>();
    TranslationConfig trans_config{"/vendor/etc/models/translate/nllb-200-qint8", "en", "es"};
    translator->Initialize(trans_config);

    // Initialize the TTS engine
    std::shared_ptr<IStreamingTtsEngine> tts_engine = std::make_shared<MockStreamingTtsEngine>();
    TtsConfig tts_config{"/vendor/etc/models/tts/piper-es", 24000, 1};
    tts_engine->Initialize(tts_config);

    // Setup the translation event callback
    translator->SetEventCallback([tts_engine](const TranslationEvent& event) {
        if (event.status == TranslationStatus::PROVISIONAL) {
            LOG(INFO) << "[Translate UI - PROV]: " << event.translated_text;
        } else if (event.status == TranslationStatus::REVOKED) {
            LOG(INFO) << "[Translate UI - REVOKED]: (Clear previous)";
        } else if (event.status == TranslationStatus::COMMITTED) {
            LOG(INFO) << "[Translate TTS - COMMITTED]: " << event.translated_text;

            // Synthesize audio and push to scheduler
            std::vector<int16_t> audio_chunk;
            if (tts_engine->SynthesizeChunk(event.translated_text, audio_chunk)) {
                if (!tts_scheduler.PushAudio(audio_chunk)) {
                    LOG(ERROR) << "TTS Scheduler buffer full! Dropping generated audio.";
                }
            }
        }
    });

    std::vector<int16_t> local_buffer(1600); // 100ms chunks

    std::string last_partial = "";
    uint64_t current_utterance_id = 1;

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

                    // Push to translation stage
                    translator->PushPartialSource(current_utterance_id, current_partial, GetCurrentTimeMs());

                    last_partial = current_partial;
                }

                // 3. Check for endpoint (sentence boundary)
                if (asr_engine->IsEndpointDetected()) {
                    std::string final_text = asr_engine->GetFinalHypothesis();
                    LOG(INFO) << "[ASR Final]: " << final_text;

                    // Push final text to translation stage
                    translator->PushCommittedSource(current_utterance_id, final_text, GetCurrentTimeMs());

                    last_partial = ""; // Reset
                    current_utterance_id++;
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

    // --- Setup Input Stream (Microphone) ---
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_INPUT);
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(builder, 1);
    AAudioStreamBuilder_setSampleRate(builder, 16000);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setInputPreset(builder, AAUDIO_INPUT_PRESET_VOICE_RECOGNITION);
    AAudioStreamBuilder_setDataCallback(builder, inputDataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(builder, errorCallback, nullptr);

    AAudioStream *input_stream = nullptr;
    aaudio_result_t result = AAudioStreamBuilder_openStream(builder, &input_stream);

    if (result != AAUDIO_OK) {
        LOG(ERROR) << "Failed to open AAudio input stream: " << AAudio_convertResultToText(result);
        AAudioStreamBuilder_delete(builder);
        return 1;
    }

    result = AAudioStream_requestStart(input_stream);
    if (result != AAUDIO_OK) {
        LOG(ERROR) << "Failed to start AAudio input stream: " << AAudio_convertResultToText(result);
        AAudioStream_close(input_stream);
        AAudioStreamBuilder_delete(builder);
        return 1;
    }

    // --- Setup Output Stream (Speaker/Bluetooth) ---
    AAudioStreamBuilder *out_builder = nullptr;
    AAudio_createStreamBuilder(&out_builder);

    // Configure for TTS Output: 24kHz, Mono, 16-bit PCM
    AAudioStreamBuilder_setDirection(out_builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setFormat(out_builder, AAUDIO_FORMAT_PCM_I16);
    AAudioStreamBuilder_setChannelCount(out_builder, 1);
    AAudioStreamBuilder_setSampleRate(out_builder, 24000);

    // Let Android route it to the active media device (e.g., Bluetooth)
    AAudioStreamBuilder_setPerformanceMode(out_builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setUsage(out_builder, AAUDIO_USAGE_MEDIA);

    AAudioStreamBuilder_setDataCallback(out_builder, outputDataCallback, nullptr);
    AAudioStreamBuilder_setErrorCallback(out_builder, errorCallback, nullptr);

    AAudioStream *output_stream = nullptr;
    result = AAudioStreamBuilder_openStream(out_builder, &output_stream);

    if (result != AAUDIO_OK) {
        LOG(ERROR) << "Failed to open AAudio output stream: " << AAudio_convertResultToText(result);
        AAudioStreamBuilder_delete(out_builder);
        return 1;
    }

    result = AAudioStream_requestStart(output_stream);
    if (result != AAUDIO_OK) {
        LOG(ERROR) << "Failed to start AAudio output stream: " << AAudio_convertResultToText(result);
        AAudioStream_close(output_stream);
        AAudioStreamBuilder_delete(out_builder);
        return 1;
    }

    LOG(INFO) << "Audio capture started. Audio is passed via ring buffer to streaming ASR thread.";

    // Start ASR worker thread
    std::thread asr_thread(AsrWorkerThread);

    // Main loop: Print diagnostic metrics periodically
    while (true) {
        sleep(5);
        LOG(INFO) << "--- Diagnostic Audio Metrics ---";
        LOG(INFO) << "Input Frames Read: " << total_frames_read;
        LOG(INFO) << "Input XRuns: " << input_xrun_count;
        LOG(INFO) << "Output Frames Written: " << total_frames_written;
        LOG(INFO) << "Output XRuns: " << output_xrun_count;
        LOG(INFO) << "Input Buffer Backlog: " << audio_buffer.AvailableData() << " frames";
        LOG(INFO) << "TTS Scheduler Backlog: " << tts_scheduler.AvailableData() << " frames";
    }

    // Cleanup (unreachable in current loop, but good practice)
    keep_asr_running = false;
    asr_thread.join();
    AAudioStream_requestStop(input_stream);
    AAudioStream_close(input_stream);
    AAudioStreamBuilder_delete(builder);

    AAudioStream_requestStop(output_stream);
    AAudioStream_close(output_stream);
    AAudioStreamBuilder_delete(out_builder);

    return 0;
}
