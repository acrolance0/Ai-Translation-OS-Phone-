# Translator Service Architecture

## Overview
This document outlines the architecture for the initial native translator service skeleton for the OnePlus 12 offline speech-translation appliance.

## Components

### 1. Translator Binary
The initial service is located in `src/translator/`.
- **`translator_service.cpp`**: A minimal C++ application that currently only initializes logging and prints a startup message indicating it is running in a controlled environment. It then loops to keep the service alive.
- **`Android.bp`**: Soong build configuration specifying it builds as a native vendor binary (`translator_service`) and requires `libbase` and `liblog`.

### 2. Init Configuration
The `translator.rc` file configures the service for Android `init`.
- Runs the binary located at `/vendor/bin/translator_service`.
- Assigns to the `hal` class.
- Runs as the `system` user and includes `system`, `audio`, and `bluetooth` groups for future integration.
- Assigns the `u:r:translator_service:s0` SELinux domain.

### 3. SELinux Domain
Dedicated SELinux policies are defined in `src/sepolicy/vendor/`:
- **`translator.te`**: Defines the `translator_service` domain, transitions it to run as a daemon, and grants minimal permissions, specifically allowing it to write to Android's logd.
- **`file_contexts`**: Labels the binary `/vendor/bin/translator_service` with the `translator_service_exec` type.

### 4. Product Integration
The service and policies are integrated via `src/product/lineage_waffle_translator.mk`. This custom product overlay adds `translator_service` to `PRODUCT_PACKAGES` and includes the custom `src/sepolicy/vendor` path into `BOARD_VENDOR_SEPOLICY_DIRS`.

## Current Status (Skeleton Phase)
- No translation, ASR, or TTS models are loaded.
- No network capabilities.
- Minimal permissions.
- Prepares the foundation for future audio/bluetooth pipeline integration.
