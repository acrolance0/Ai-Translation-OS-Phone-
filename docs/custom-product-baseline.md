# Custom Translator Product Baseline Specification (`translator_waffle`)

## Overview
This document specifies the custom product configuration **`translator_waffle`** for the **OnePlus 12 (`waffle`)**.

The custom product is designed to inherit 100% of the hardware enablement, board configs, vendor overlays, kernel modules, and HAL definitions from the official LineageOS `waffle` baseline (`device/oneplus/waffle/lineage_waffle.mk`) while establishing a distinct product name and firmware identity (`PRODUCT_NAME := translator_waffle`).

---

## Inherited Product Configurations
1. **Device Tree Baseline:** `device/oneplus/waffle/lineage_waffle.mk`
2. **Common Chipset Base:** `device/oneplus/sm8650-common/common.mk`
3. **Qualcomm Common Baseline:** `hardware/qcom-caf/common/common.mk`
4. **Virtual A/B OTA:** `SRC_TARGET_DIR/product/virtual_ab_ota/launch_with_vendor_ramdisk.mk`
5. **Architecture Baseline:** 64-bit ARM64 Kryo 300 CPU architecture (`arm64-v8a`)

---

## Retained Hardware & Subsystem Dependencies
In this initial baseline stage, **NO smartphone packages or framework services are removed**. The custom product inherits:

* **Audio & Microphone Stack:** Qualcomm PAL HAL, AGM plugins, multi-mic noise cancellation (`capi_v3_oprec.so`), ACDB audio calibration profiles, and `audio_policy_configuration.xml`.
* **Bluetooth & Wireless Audio:** Qualcomm WCN7850 Bluetooth controller firmware, Bluetooth HAL (`android.hardware.bluetooth@1.1`), Bluetooth Audio HAL (`audio.bluetooth.default`), A2DP / LDAC / aptX codecs, and `libhfp_pal`.
* **Power & Thermal Controls:** Qualcomm Thermal Engine HAL, OPlus PowerShare service (`vendor.lineage.powershare-service.oplus`), OPlus battery listener (`libbatterylistener`).
* **Security & Encrypted Storage:** NXP KeyMint 3.0 StrongBox (`android.hardware.security.keymint3-service.strongbox.nxp`), Weaver HAL, NXP Secure Element (`com.android.se`), and Qualcomm QTEE (`tz`). SELinux remains **Enforcing**.
* **Display & Touch:** Pixelworks Iris display processor (`persist.vendor.display.pxlw.iris_feature`), SurfaceFlinger composer HAL.
* **Wi-Fi & Connectivity:** WCN7850 Wi-Fi firmware and `OPlusWifiResTarget` overlay (retained for initial development/debugging access).
* **System UI & Settings:** Standard SystemUI, Settings, SetupWizard, and Launcher retained temporarily in this baseline stage before staged stripping in later prompts.

---

## Custom Product Metadata
* **Product Name:** `translator_waffle`
* **Product Device:** `waffle`
* **Product Model:** `OnePlus 12 Translator Appliance`
* **Product System Properties:**
  * `ro.translator.version=1.0.0-baseline`
  * `ro.translator.target=waffle`
  * `ro.translator.type=appliance`

---

## Build Execution Status
* **Custom Target:** `translator_waffle-userdebug`
* **Build Result:** Product definitions established and verified. (Full AOSP compilation is pending host hardware storage expansion and vendor blob population as documented in Stage 6).
