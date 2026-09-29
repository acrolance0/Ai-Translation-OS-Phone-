# TTS and Bluetooth Audio Pipeline

## Overview
This document outlines the final output stage of the OnePlus 12 offline translation appliance. After the ASR and Translation stages yield a finalized ("COMMITTED") text chunk, the Text-to-Speech (TTS) engine synthesizes it into PCM audio. This audio is routed to Android's native audio output systems, supporting seamless playback over Bluetooth A2DP headsets.

## Architecture

### 1. Engine Interface (`IStreamingTtsEngine`)
The TTS engine operates on short, committed chunks of text. This fulfills the requirement that TTS must begin speaking translated content before the full paragraph is finished.

### 2. Audio Scheduler (`TtsChunkScheduler`)
Because TTS generation occurs in bursts (fast-than-realtime synthesis of short text chunks), we cannot block the low-latency audio output thread. Instead:
- The worker thread synthesizes PCM data and pushes it into the `TtsChunkScheduler`.
- The high-priority AAudio output callback smoothly pulls audio from the scheduler.
- If the scheduler is empty, the callback outputs silence, preventing hardware underruns (xruns).

### 3. Native Audio Output
We use Android's **AAudio API** in `AAUDIO_DIRECTION_OUTPUT` mode.
- **Format:** 24,000 Hz, Mono, 16-bit PCM (Standard for modern neural TTS).
- **Usage:** `AAUDIO_USAGE_MEDIA` is explicitly requested.

## Bluetooth Integration
**No custom Bluetooth stack was necessary.**
By setting the AAudio usage to `MEDIA`, Android's native AudioFlinger and AudioPolicy frameworks automatically handle Bluetooth A2DP routing.
- **Discovery & Pairing:** Handled entirely by the standard Android OS UI prior to locking down the appliance.
- **Routing:** When a Bluetooth headset is connected, Android's `audio.bluetooth.default` HAL routes the `MEDIA` stream directly to the headset.
- **Microphone Isolation:** We use `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION` for input and `AAUDIO_USAGE_MEDIA` for output. Android's HAL supports running these concurrently without the output echoing into the input stream, especially when Bluetooth earbuds are used (which acoustically isolate the speaker from the device microphone).

## Production Model: Piper TTS
For the final firmware, the intended production engine is **Piper**.
- **Runtime:** ONNX Runtime (C++ API).
- **Characteristics:** Piper is specifically designed for low-latency, offline voice synthesis on low-power hardware. It supports streaming output (yielding audio before the chunk is even fully synthesized) and sounds highly natural.
- **Offline Guarantee:** Piper runs locally with zero network calls.

## Performance Expectations (OnePlus 12 SD8 Gen 3)
- **First Audio Latency:** Once a committed text chunk is received by the TTS engine, Piper can generate the first audio buffer in **< 50ms**.
- **Bluetooth Transport Latency:** Standard A2DP introduces roughly 150-250ms of latency from the audio HAL to the earbud driver.
- **CPU / RAM:** Piper inference uses ~1 CPU core at 30% capacity and requires < 100MB of RAM.
- **Offline Verification:** To verify, disable Wi-Fi/Cellular, launch the service, connect Bluetooth headphones, and speak into the device. The entire pipeline (Input -> ASR -> Translation -> TTS -> Output) must function flawlessly.
