# Incremental Translation Pipeline Architecture

## Overview
This document describes the design and integration of the incremental machine translation stage within the OnePlus 12 offline translation appliance. To achieve a simultaneous translation effect, the system cannot wait for an entire sentence to be spoken before translating. It must translate partial ASR hypotheses as they arrive, and gracefully revise them as new context changes the meaning.

## Event-Driven Architecture
The C++ interface `IIncrementalTranslator` defines an event-driven architecture using `TranslationEvent` structures. The engine emits three types of statuses:
1. **`PROVISIONAL`**: A draft translation based on an incomplete sentence. This is sent to the UI to give the user immediate visual feedback. It is **never** sent to the Text-to-Speech (TTS) engine, because spoken audio cannot be "un-spoken."
2. **`REVOKED`**: A signal that the previous `PROVISIONAL` translation is no longer accurate due to new source words changing the sentence context. The UI should clear or strike through the revoked text.
3. **`COMMITTED`**: A final, locked-in translation chunk. The source context for this chunk is finalized. This text is safe to be buffered and sent to the TTS engine for audio playback.

## Pipeline Integration
1. The **ASR Engine** processes audio and yields partial text (e.g., "hello", "hello world").
2. The Service calls `PushPartialSource()` on the translator.
3. The Translator emits a `PROVISIONAL` event.
4. The Service receives the event and updates the UI log.
5. When the ASR detects silence/endpoint, the Service calls `PushCommittedSource()`.
6. The Translator revokes any lingering provisional drafts and emits `COMMITTED` chunks (broken down appropriately for TTS breathing/pacing).

## Production Model Choice
For the final production firmware on the OnePlus 12, the targeted implementation is **CTranslate2** running the **NLLB-200** (No Language Left Behind) model.
- **Engine:** CTranslate2 (C++ inference engine specifically optimized for Transformer models).
- **Model:** NLLB-200 (Distilled 600M or 1.3B parameter version).
- **Quantization:** `int8` quantization. This is mandatory to fit the model comfortably within the memory bandwidth of the Snapdragon 8 Gen 3 while maintaining high tokens-per-second throughput.
- **Languages:** NLLB supports 200 languages natively without cascaded models.
- **License:** CTranslate2 is MIT licensed. NLLB-200 is CC-BY-NC (or standard MIT for specific distilled variants depending on the Meta release terms).
- **Hardware Acceleration:** CTranslate2 is highly optimized for ARM64 NEON. While Qualcomm Hexagon (QNN) integration is possible, baseline deployment will rely on the CPU to ensure absolute stability.

## Performance & Offline Verification
- **Storage:** The `int8` quantized 600M model requires ~600MB of storage.
- **RAM:** Expect ~800MB of resident memory during active translation.
- **Latency:** On the SD8 Gen 3 CPU, generating a provisional translation from a 5-word partial source is expected to take < 150ms.
- **Offline Guarantee:** CTranslate2 operates entirely via local file paths (e.g., `/vendor/etc/models/translate`). It has zero network bindings, satisfying the strict offline operational constraint.
