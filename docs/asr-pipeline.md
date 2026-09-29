# Streaming ASR Pipeline Architecture

## Overview
This document outlines the architecture of the Automatic Speech Recognition (ASR) component integrated into the OnePlus 12 offline translator service. The primary requirement is **genuine streaming**, meaning the system incrementally processes audio and generates partial hypotheses before the speaker finishes their sentence.

## Architecture & Data Flow
1. **Audio Capture (AAudio):** High-priority audio callback reads 16kHz Mono 16-bit PCM data.
2. **Lock-Free Ring Buffer:** The callback writes audio chunks into a thread-safe, lock-free `AudioRingBuffer`. This ensures the audio thread is never blocked waiting for a mutex, avoiding frame drops (xruns).
3. **ASR Worker Thread:** A background thread continuously polls the ring buffer. It reads available data in chunks (e.g., 100ms blocks) and feeds them into the ASR engine interface.
4. **Streaming ASR Engine:** The engine processes the audio incrementally and yields partial text hypotheses.

## Engine Interface (`IStreamingAsrEngine`)
To avoid tightly coupling the service to a single ASR implementation, we define a clean C++ interface:
- `Initialize(model_dir)`: Loads acoustic and language models.
- `AcceptAudio(data, samples)`: Ingests raw PCM frames.
- `GetPartialHypothesis()`: Returns the current best guess for the spoken text.
- `IsEndpointDetected()`: Returns true if a sentence boundary/silence is detected.
- `GetFinalHypothesis()`: Returns the finalized transcription for the segment.

## Implementation & Production Choice
For this development phase, a `MockStreamingAsrEngine` is used to simulate streaming behavior (emitting partial words over time) and validate the multithreaded ring-buffer pipeline without requiring cross-compilation of a large ML runtime in the sandbox.

### Production Engine: Sherpa-ONNX (Next-gen Kaldi)
For the final appliance on the OnePlus 12, the intended production engine is **Sherpa-ONNX**.
- **Model Type:** Streaming Zipformer (Transducer architecture).
- **Why?** It natively supports C++ inference, operates highly efficiently on ARM64 CPUs, and is designed specifically for real-time incremental output.
- **License:** Apache 2.0.
- **Hardware Acceleration:** While Sherpa-ONNX runs fast on the CPU, future phases will explore delegating operations to the Snapdragon 8 Gen 3 Hexagon DSP via Qualcomm NNAPI/QNN, but *only after verification*. We currently assume CPU execution.
- **Model Storage:** Models are expected to be placed in the `/vendor/etc/models/asr/` partition. The uncompressed streaming zipformer models (e.g., `sherpa-onnx-streaming-zipformer-en`) typically require ~150-300MB of storage.

## Testing & Metrics
The diagnostic mode measures:
- **Audio Backlog:** The number of unprocessed frames in the ring buffer.
- **XRuns:** Drop counts from the hardware.
- **First-Token Latency (Estimated):** With Sherpa-ONNX on the SD8G3 CPU, first-token latency from audio ingest to partial text is expected to be under 80ms.
