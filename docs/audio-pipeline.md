# Translator Audio Pipeline Architecture

## Overview
This document details the native audio capture pipeline integrated into the OnePlus 12 offline translator service. The goal of this phase is to establish a reliable, low-latency audio capture stream from the microphone to the native service, without introducing ASR or translation models yet.

## Architecture Decisions

### A. Audio API
We selected **AAudio** (via `libaaudio`) as the native audio API.
* **Why AAudio?** AAudio is Android's modern C API designed for high-performance, low-latency audio. By using AAudio directly in the C++ service, we bypass unnecessary Java framework overhead (like `AudioRecord` in the JVM) and communicate closely with the Android Audio HAL.

### B. Capture Format
The capture stream is configured directly for the requirements of modern streaming Automatic Speech Recognition (ASR) engines:
* **Sample Rate:** 16,000 Hz (16 kHz)
* **Channels:** 1 (Mono)
* **Format:** 16-bit PCM (`AAUDIO_FORMAT_PCM_I16`)

We request the `AAUDIO_INPUT_PRESET_VOICE_RECOGNITION` input preset. This is critical because it instructs the Android Audio HAL (and the underlying Qualcomm Snapdragon audio DSP) to bypass heavy processing like Automatic Gain Control (AGC) or aggressive noise suppression that often distorts audio for ASR models.

### C. Buffering & Latency
* **Buffering:** We use AAudio's asynchronous callback model (`AAudioStreamBuilder_setDataCallback`). When the audio hardware fills a small buffer, our C++ callback is executed to process `numFrames`.
* **Latency:** We explicitly request `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`. The expected latency on a Snapdragon 8 Gen 3 (OnePlus 12) using this path is generally in the 10-25ms range.

### D. Diagnostic Mode & Privacy
The service currently operates in a **Diagnostic Mode**.
* The audio callback receives the PCM data but **immediately discards it**.
* No audio is saved to disk, printed to logs, or uploaded to any network.
* Instead, the callback tracks metrics: total frames read, and xruns (overruns/underruns).
* A logging thread periodically prints these diagnostic metrics to logcat to verify the pipeline is alive and not dropping frames.

## Security & SELinux
To capture audio without breaking the minimal service confinement, the `translator_service` SELinux domain was granted specific permissions:
* Ability to find and bind to `audioserver`.
* Ability to read/write to `audio_device` character files.
Network access and file writing (beyond logd) remain strictly prohibited.
