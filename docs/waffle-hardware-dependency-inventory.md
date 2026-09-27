# OnePlus 12 (`waffle`) Hardware Dependency Inventory

## Executive Summary
This document classifies and analyzes the hardware, vendor, driver, HAL, and system dependencies present in the official LineageOS 23.2 device trees for the **OnePlus 12 (`waffle`)** and its common chipset base (**`sm8650-common`**).

The goal of this classification is to provide a rigorous, engineering-backed roadmap for transforming the standard LineageOS phone image into a dedicated, offline speech-translation appliance without breaking critical low-level subsystems (audio routing, Qualcomm Hexagon DSP, Bluetooth A2DP, thermal management, or hardware security).

---

## Component Classifications & Component Map

### 1. Required for Boot
* **Components:** Primary bootloaders (`xbl`, `xbl_config`, `abl`, `tz`, `hyp`, `devcfg`), Linux Kernel (`kernel/oneplus/sm8650`), Device Trees (`dtb`, `dtbo`), ramdisk (`init_boot`, `vendor_boot`), vendor DLKM kernel modules.
* **Makefiles / Configs:** `BoardConfigCommon.mk` (`BOARD_BOOT_HEADER_VERSION := 4`, `BOARD_RAMDISK_USE_LZ4 := true`), `init.qcom.rc`, `init.target.rc`, `init.oplus.rc`.
* **Rationale:** The Qualcomm Snapdragon 8 Gen 3 (SM8650 / Pineapple) requires low-level firmware partitions and signed vendor kernel modules (`system_dlkm`, `vendor_dlkm`) for hardware initialization and Android init stage 1/2 execution.

### 2. Required for Storage
* **Components:** UFS 4.0 storage controller drivers, `f2fs`/`ext4`/`erofs` filesystem modules, `checkpoint_gc`, metadata encryption.
* **Makefiles / Configs:** `BoardConfigCommon.mk` (`BOARD_USES_METADATA_PARTITION := true`), `fstab.qcom`.
* **Rationale:** Reliable read/write operations for dynamic partitions (`system`, `vendor`, `odm`, `product`, `system_ext`) and fast local model storage require full storage driver stack integrity.

### 3. Required for Display
* **Components:** Qualcomm SurfaceFlinger graphics stack (`libdrm`, `mesa-libgallium`, `libgbm`), Pixelworks Iris display processor firmware (`persist.vendor.display.pxlw.iris_feature`), display HAL services.
* **Makefiles / Configs:** `vendor.prop`, `odm.prop`, `device.mk` (`displayconfig.xml`).
* **Rationale:** Required for local rendering during initial setup, status display, and UI rendering (or minimal status screen).

### 4. Required for Microphone / Audio Input
* **Components:** Qualcomm Audio Reach / PAL (Platform Audio Layer) HAL, AGM (Audio Graph Manager), Qualcomm Hexagon ADSP audio firmware (`capi_v3_oprec.so`, `libaiboostubwc_skel.so`, `libQnnHtpV75Skel.so`), multi-mic noise reduction / beamforming.
* **Makefiles / Configs:** `BoardConfigCommon.mk` (`AUDIO_FEATURE_ENABLED_PAL_HIDL := true`, `AUDIO_FEATURE_ENABLED_AGM_HIDL := true`), `common.mk` (`android.hardware.audio.service`), `audio_policy_configuration.xml`.
* **Rationale:** **CRITICAL FOR TRANSLATOR.** Continuous microphone listening and streaming ASR require uninterrupted access to multi-mic hardware, ADSP noise cancellation, and low-latency PCM audio capture streams.

### 5. Required for Bluetooth
* **Components:** Qualcomm WCN7850 Bluetooth controller firmware (`bluetooth.img`), Bluetooth HAL (`android.hardware.bluetooth@1.1`), `bt_firmware` partition, HCI driver stack.
* **Makefiles / Configs:** `common.mk`, `BoardConfigCommon.mk`, `vendor.prop` (`bluetooth.device.default_name=OnePlus 12`).
* **Rationale:** Essential hardware transport layer needed to connect external wireless earbuds and headphones.

### 6. Required for Bluetooth Audio Output
* **Components:** Bluetooth Audio HAL (`audio.bluetooth.default`), A2DP / LDAC / aptX Adaptive codecs, PAL HFP/A2DP bridge (`libhfp_pal`).
* **Makefiles / Configs:** `common.mk` (`PRODUCT_PACKAGES += audio.bluetooth.default libhfp_pal`).
* **Rationale:** **CRITICAL FOR TRANSLATOR.** Streaming translated Text-to-Speech (TTS) must be routed with low latency to paired Bluetooth earbuds.

### 7. Required for Wi-Fi During Development / Setup
* **Components:** WCN7850 Wi-Fi firmware (`wlan` modules), Wi-Fi HAL (`android.hardware.wifi@1.0-service`), OPlus Wi-Fi overlays.
* **Makefiles / Configs:** `common.mk`, `OPlusWifiResTarget`.
* **Rationale:** Required during initial development, debugging, ADB over Wi-Fi, and model downloading before full offline transition.

### 8. Required for Qualcomm Hardware Initialization
* **Components:** Qualcomm Hexagon NPU/HTP firmware (`libQnnHtpV75.so`, `libSnpeHtpV75Skel.so`), QMI / QTI daemon infrastructure, `qupfw`, `shrm`.
* **Makefiles / Configs:** `proprietary-files.txt` (`odm/lib/rfsa/adsp/aiboost/signed/`), `init.qcom.rc`.
* **Rationale:** Hexagon Tensor Processor (HTP) / NPU hardware initialization is essential for running hardware-accelerated local ASR/translation/TTS inference models in later stages.

### 9. Required for Thermal / Power Management
* **Components:** Qualcomm Thermal Engine HAL, OPlus PowerShare service (`vendor.lineage.powershare-service.oplus`), OPlus battery listener (`libbatterylistener`, `libaudiochargerlistener`).
* **Makefiles / Configs:** `common.mk`, `BoardConfigCommon.mk`.
* **Rationale:** Continuous speech listening and streaming AI inference generate high compute load. Thermal throttling rules and power management services must remain active to prevent hardware damage.

### 10. Required for Security
* **Components:** KeyMint 3.0 StrongBox (`android.hardware.security.keymint3-service.strongbox.nxp`), Weaver HAL (`android.hardware.weaver-service.nxp`), NXP Secure Element (`com.android.se`), Qualcomm Trusted Execution Environment (QTEE / `tz`).
* **Makefiles / Configs:** `device.mk` (`PRODUCT_PACKAGES += android.hardware.security.keymint3-service.strongbox.nxp`), `BoardConfigCommon.mk`.
* **Rationale:** Android 16 Keystore, secure storage encryption, and hardware-backed credential verification depend on these services.

### 11. Required for Recovery / Update
* **Components:** LineageOS Recovery (`recovery.img`), Virtual A/B OTA post-install scripts (`checkpoint_gc`, `otapreopt_script`, `xbl_config_arb_check`), `bootloader` slot control HAL.
* **Makefiles / Configs:** `BoardConfigCommon.mk` (`AB_OTA_UPDATER := true`), `common.mk`.
* **Rationale:** Required for flashing initial firmware updates and performing safe offline system updates.

### 12. Required for Telephony but Potentially Removable Later
* **Components:** Qualcomm RIL (Radio Interface Layer) daemons, IMS services, SIM / eSIM EUICC resources (`FrameworksResEuicc_EU`, `FrameworksResEuicc_NA`), modem firmware (`modem.img`).
* **Makefiles / Configs:** `device.mk`, `FrameworksResTargetPhone`.
* **Rationale:** Telephony components can be isolated or disabled once the baseline build is verified, as the final appliance will not make cellular calls or use SMS.

### 13. Required for Normal Smartphone UI but Potentially Removable Later
* **Components:** Standard Android Launcher (Trebuchet), Default Phone/Dialer, Messaging app, Contacts, SetupWizard, DeskClock, ContactsProvider.
* **Makefiles / Configs:** `lineage_waffle.mk` (inherits `config/full_phone.mk`).
* **Rationale:** Standard smartphone applications will eventually be replaced by the native translator system service and custom dedicated boot UI.

### 14. Unknown — Requires Testing
* **Components:** OPlus Doze extensions (`OplusDozeResCommon`), In-display fingerprint (IDTP), IFAA service (`sys.oplus.ifaa.model`).
* **Rationale:** Interaction with minimal headless/kiosk mode needs to be validated empirically during stage-by-stage removal.

---

## Rules for Component Removal
1. **Never Disable Thermal Protection:** Thermal mitigation HALs and battery limits must remain intact.
2. **Never Set SELinux to Permissive:** All native translator services must be properly labeled with custom `.te` domain policies.
3. **Preserve Audio & DSP Pipelines:** Never remove `audio.primary`, PAL, AGM, or Hexagon HTP RFSA libraries.
