# OnePlus 12 Offline Speech-Translation Appliance (`oneplus12-offline-translator`)

## Project Goal & Overview
This repository contains the custom engineering code, device/product configurations, build scripts, SELinux policies, native system services, and documentation for building a dedicated, fully offline speech-translation appliance based on the **OnePlus 12**.

Rather than functioning as a standard Android smartphone, the target firmware is transformed into an offline appliance capable of continuous microphone listening, streaming Automatic Speech Recognition (ASR), incremental translation, streaming Text-to-Speech (TTS), and low-latency audio output over Bluetooth earbuds/headphones.

## Hardware & Operating System Baseline
* **Target Device:** OnePlus 12
* **Codename:** `waffle`
* **SoC:** Qualcomm Snapdragon 8 Gen 3 (SM8650)
* **Architecture:** ARM64
* **RAM Variants:** 12 GB / 16 GB / 24 GB
* **Storage:** UFS 4.0
* **Bluetooth:** Bluetooth 5.4 with A2DP audio support
* **Baseline OS Base:** LineageOS 23.2 (`lineage-23.2` / Android 16)

## Translation Requirement: True Streaming / Simultaneous Pipeline
The appliance implements a simultaneous streaming translation architecture rather than sentence-level batch processing:
```
[Source Speech] -> [Streaming ASR (Partial Text)] -> [Incremental Translation] -> [Streaming TTS] -> [Bluetooth Audio]
```
This enables near-instantaneous audio playback while the speaker is still talking.

## Offline Operational Constraint
* **Zero Runtime Cloud Dependencies:** Once configured and flashed, the firmware operates completely offline without Internet access, cellular data, Google Services, Play Store, or cloud translation APIs.
* **Minimal Appliance Firmware:** Standard smartphone components (browser, dialer, SMS, contacts, default smartphone launcher) are stripped out in favor of a native, system-level translation service starting automatically at boot.

## Codebase Architecture & Repository Separation
To maintain clean engineering boundaries and ensure seamless upstream syncs:
1. **Upstream Android/LineageOS Source Tree:** Managed independently via Google's `repo` tool (`~/android/lineage`). Full upstream Android source trees, device trees, and kernel repos remain in the `repo` checkout.
2. **Custom Engineering Repository (`oneplus12-offline-translator`):** Contains ONLY custom engineering additions, including:
   * Translator native system services
   * Firmware product Makefile overrides (`lineage_waffle.mk` overlays)
   * System init scripts & SELinux policies (`.te`)
   * Build automation scripts & patches
   * Documentation & verification tests

## Repository Structure
```
.
├── README.md           # Project architecture and development roadmap
├── .gitignore          # Excludes build outputs, proprietary blobs, and secrets
├── docs/               # Technical specifications and architectural diagrams
├── patches/            # Targeted patches for LineageOS source tree
├── scripts/            # Build automation and validation scripts
└── src/                # Custom native system services and translator code
```

## Licensing & Notice
Custom code in this repository is developed for the `oneplus12-offline-translator` project. All upstream LineageOS and Android source components retain their respective open-source licenses (Apache 2.0 / GPLv2).
