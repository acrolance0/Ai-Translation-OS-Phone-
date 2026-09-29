# Android Minimization Plan for OnePlus 12 (`waffle`) Translator Appliance

## Executive Summary
This document provides a systematic, dependency-analyzed plan for stripping standard Android smartphone applications and non-essential framework services from the custom `translator_waffle` firmware image.

The objective is to minimize system overhead, boot time, memory footprint, and attack surface while strictly preserving all low-level kernel drivers, Qualcomm SM8650 hardware abstraction layers (HALs), audio capture pipelines, Bluetooth A2DP/HFP audio stacks, power/thermal management daemons, and hardware-backed security infrastructure.

---

## 1. Subsystem Classification Matrix

| Category | Description | Primary Components & Services | Action |
| :--- | :--- | :--- | :--- |
| **1. Boot Critical** | Essential for Linux kernel init & Android init stages | `init`, `vold`, `servicemanager`, `hwservicemanager`, `vendor.dlkm`, `system_dlkm`, `ueventd` | **PRESERVE** |
| **2. Framework Core** | Essential for Binder IPC & app execution | `system_server`, `ActivityManager`, `PackageManager`, `SurfaceFlinger`, `installd`, `app_process` | **PRESERVE** |
| **3. Audio / Microphone** | Low-latency audio capture & routing | `audioserver`, `AudioFlinger`, `AudioPolicyService`, `android.hardware.audio.service`, PAL HAL, AGM plugins, `capi_v3_oprec.so` | **PRESERVE** |
| **4. Bluetooth Stack** | BT HCI transport & device pairing | `android.hardware.bluetooth@1.1-service`, `bluetooth.default`, WCN7850 BT firmware | **PRESERVE** |
| **5. Bluetooth Audio** | Wireless audio rendering & codecs | `audio.bluetooth.default`, `libhfp_pal`, A2DP/LDAC/aptX codecs | **PRESERVE** |
| **6. Thermal & Power** | System cooling & battery protection | Qualcomm Thermal Engine (`thermalserv`), `vendor.lineage.powershare-service.oplus`, `libbatterylistener` | **PRESERVE** |
| **7. Storage** | Storage encryption & filesystem GC | `vold`, `f2fs`/`ext4`/`erofs` drivers, `checkpoint_gc` | **PRESERVE** |
| **8. Security & Keystore** | Hardware crypto & encryption keys | `keystore2`, NXP KeyMint 3.0 StrongBox, Weaver HAL, QTEE (`tz`) | **PRESERVE** |
| **9. Recovery & OTA** | Firmware updates & recovery | `recovery.img`, Virtual A/B scripts (`otapreopt_script`, `xbl_config_arb_check`) | **PRESERVE** |
| **10. Telephony Only** | Cellular calls, SMS, & eSIM | `rild`, `telephony-common`, `CarrierConfig`, `FrameworksResEuicc_*`, `CellBroadcastReceiver` | **REMOVE (Stage 1)** |
| **11. Smartphone UI** | Standard user apps & launcher | `Trebuchet` (Launcher), `Dialer`, `Messaging`, `Contacts`, `DeskClock`, `Gallery`, `Browser` | **REMOVE (Stage 2)** |
| **12. Dev / Debugging** | USB/Network ADB & testing | `adbd`, `OPlusWifiResTarget`, `WifiResTarget` | **RETAIN (Dev Phase)** |
| **13. Unknown / Test** | Extended OPlus gestures & doze | `OplusDozeResCommon`, In-display fingerprint (IDTP), IFAA | **TEST BEFORE REMOVAL** |

---

## 2. Core Inter-Subsystem Dependencies

Careful analysis must be paid to hidden Binder IPC dependencies across system servers:
1. **`audioserver` & `system_server`:** `AudioPolicyService` inside `audioserver` relies on `ActivityManager` and `PackageManager` for permission checks and audio focus arbitration. Removing core Android framework components breaks audio capture.
2. **`audioserver` & Bluetooth:** Wireless audio playback relies on Binder IPC between `com.android.bluetooth` (Bluetooth app process) and `audio.bluetooth.default` HAL. Disabling the `Bluetooth` system package breaks A2DP streaming.
3. **`SurfaceFlinger` & Native UI:** Even in headless/kiosk mode, `SurfaceFlinger` must remain active to render the custom native translator interface.

---

## 3. Staged Removal Roadmap

### Stage 1: Cellular & Telephony Isolation
* **Components:** `rild`, `telephony-common.jar`, `FrameworksResEuicc_EU`, `FrameworksResEuicc_NA`, `CellBroadcastReceiver`, `CarrierConfigResCommon`.
* **Reason:** The appliance operates offline without cellular SIM or data capabilities.
* **Dependencies:** RIL daemons are standalone; removing them does not break AudioFlinger or Bluetooth.
* **Risk:** Low.
* **Verification Test:** Verify system boots cleanly, audio recording (`tinycap`) works, and no RIL crash loops appear in `logcat`.
* **Rollback Method:** Re-include `PRODUCT_PACKAGES += TelephonyProvider FrameworksResEuicc_EU` in `translator_waffle.mk`.

### Stage 2: Standard Smartphone User Applications
* **Components:** `Trebuchet` (Default Launcher), `Dialer`, `Messaging`, `Contacts`, `ContactsProvider`, `DeskClock`, `Camera2`.
* **Reason:** Replaced by the dedicated native translator boot service.
* **Dependencies:** None for standalone user apps.
* **Risk:** Low.
* **Verification Test:** Confirm native translator system service starts automatically at boot and receives focus.
* **Rollback Method:** Re-add application packages to `PRODUCT_PACKAGES` in `translator_waffle.mk`.

### Stage 3: Setup Wizard & Non-Essential System Utilities
* **Components:** `LineageSetupWizard`, `PrintSpooler`, `LiveWallpapersPicker`, `SearchLauncher`.
* **Reason:** Eliminates unnecessary background services and first-boot setup wizard prompts.
* **Dependencies:** `LineageSetupWizard` sets `USER_SETUP_COMPLETE`. Must ensure property `provisioned=1` is set via default build properties.
* **Risk:** Medium.
* **Verification Test:** Confirm device boots straight to the custom translator UI without getting stuck on an unprovisioned setup screen.
* **Rollback Method:** Restore `LineageSetupWizard` overlay.

---

## 4. Subsystems That Must NEVER Be Removed
1. **Qualcomm Thermal Engine & OPlus Battery Listeners:** Disabling thermal daemons risks hardware overheating and battery damage during continuous AI model inference.
2. **Audio Reach / PAL HAL & ADSP Firmware:** Required for multi-mic noise cancellation and streaming PCM audio capture.
3. **Bluetooth A2DP & HFP HALs:** Required for audio output to paired Bluetooth earbuds.
4. **Keystore2 & NXP KeyMint StrongBox:** Required for system encryption and secure boot validation.
5. **SELinux Policy Infrastructure:** SELinux must remain in **Enforcing** mode; custom translator services will be assigned dedicated `.te` domains.
