# OnePlus 12 Offline Speech-Translation Appliance (`oneplus12-offline-translator`)

## Project Goal & Architecture Overview
This repository contains the custom engineering code, device/product configurations, build scripts, SELinux policies, native system services, and technical specifications for building a dedicated, fully offline speech-translation appliance based on the **OnePlus 12 (`waffle`)**.

Rather than functioning as a standard Android smartphone, the target firmware is transformed into a dedicated appliance capable of continuous microphone listening, streaming Automatic Speech Recognition (ASR), incremental translation, streaming Text-to-Speech (TTS), and low-latency audio output over Bluetooth earbuds/headphones.

```
[Source Speech] -> [Streaming ASR (Partial Text)] -> [Incremental Translation] -> [Streaming TTS] -> [Bluetooth Audio Output]
```

### Key Technical Constraints
* **Zero Runtime Cloud Dependency:** Operates 100% offline without Internet access, cellular data, Google Play Services, or cloud APIs.
* **Minimal Appliance Firmware:** Standard smartphone components (cellular telephony, browser, dialer, SMS, contacts, default smartphone launcher) are isolated and removed in favor of a native system-level translation service starting automatically at boot.

---

## Hardware & Operating System Baseline
* **Target Device:** OnePlus 12
* **Codename:** `waffle`
* **SoC:** Qualcomm Snapdragon 8 Gen 3 (SM8650 / Pineapple)
* **Architecture:** ARM64 (Kryo 300 CPU runtime)
* **RAM Variants:** 12 GB / 16 GB / 24 GB LPDDR5X
* **Storage:** UFS 4.0
* **Bluetooth:** Bluetooth 5.4 with A2DP and HFP audio profiles
* **Baseline OS Base:** LineageOS 23.2 (`lineage-23.2` / Android 16)

---

## Codebase & Repository Architecture
To maintain clean engineering boundaries and ensure seamless upstream LineageOS syncs:
1. **Upstream Android/LineageOS Source Tree (`~/android/lineage`):** Managed independently via Google's `repo` tool. Full upstream Android source trees, device trees (`device/oneplus/waffle`, `device/oneplus/sm8650-common`), and kernel repos remain in the `repo` checkout.
2. **Custom Engineering Repository (`oneplus12-offline-translator`):** Contains ONLY custom engineering additions, including product makefiles, build scripts, SELinux domain policies, native system services, and technical documentation.

```
.
├── .gitignore          # Filters build outputs, vendor blobs, keys, and temporary files
├── README.md           # Project summary and comprehensive engineering report
├── docs/               # Technical specifications, hardware inventories, and build plans
│   ├── android-minimization-plan.md
│   ├── baseline-build-plan.md
│   ├── baseline-build-result.md
│   ├── proprietary-dependency-status.md
│   └── waffle-hardware-dependency-inventory.md
├── init/               # Custom init scripts and system service launch definitions
├── patches/            # Targeted patches for LineageOS source tree
├── products/           # Custom product definitions (AndroidProducts.mk, translator_waffle.mk)
├── scripts/            # Build automation and environment validation scripts
├── sepolicy/           # Custom SELinux domain policy rules (.te)
├── src/                # Custom native system service source code
└── tests/              # Automated verification and integration test suites
```

---

## Engineering Status Report: Accomplishments Completed So Far

### 1. Host Machine & Build Environment Audit
* **System Assessment:** Audited KVM virtual machine host running Ubuntu 24.04.4 LTS (Linux 6.8.0 x86_64, 4 logical CPU cores, 7.8 GB RAM, 92 GB free disk space).
* **Requirements Comparison:** Verified requirements against official LineageOS 23.2 `waffle` documentation. Documented hardware/resource constraints (disk and swap requirements) and user sudo privileges.

### 2. Host Build Dependencies Installation
* **Toolchain & Compilers:** Installed GNU GCC/G++ 13.3.0 (with multilib 32-bit support), Clang/LLVM 18.1.3, GNU Make 4.3, Ninja 1.11.1, OpenJDK 21, and Python 3.12.
* **Build Utilities:** Installed `flex`, `bison`, `bc`, `lz4`, `erofs-utils`, `squashfs-tools`, `rsync`, `zip`, `unzip`, `xsltproc`, `protobuf-compiler`, `python3-protobuf`, `schedtool`, `imagemagick`, `pngcrush`, `xxd`, and GitHub CLI (`gh`).
* **Source Control & Caching:** Installed Google `repo` launcher (`~/bin/repo`), initialized global `git-lfs` (version 3.4.1), and configured `ccache` (version 4.9.1) with a conservative 15.0 GB limit (`USE_CCACHE=1`).

### 3. Workspace Establishment & Source Manifest Sync
* **Workspace Isolation:** Created dedicated directories `~/android/lineage` for the upstream Android tree and `~/android/oneplus12-offline-translator` for custom engineering assets.
* **Manifest Initialization:** Initialized LineageOS 23.2 manifest (`repo init -u https://github.com/LineageOS/android.git -b lineage-23.2 --git-lfs --no-clone-bundle`).
* **Target Device Sync:** Created local manifest (`.repo/local_manifests/waffle.xml`) and synchronized target device trees:
  * `device/oneplus/waffle` (`lineage-23.2`)
  * `device/oneplus/sm8650-common` (`lineage-23.2`)
  * `hardware/oplus` (`lineage-23.2`)

### 4. Hardware Dependency Classification & Inventory
* **Technical Inventory:** Produced `docs/waffle-hardware-dependency-inventory.md`, classifying all device/chipset components into 14 distinct categories.
* **Preservation Directives:** Established strict engineering rules ensuring continuous microphone capture, PAL/AGM audio routing, WCN7850 Bluetooth A2DP/HFP profiles, Qualcomm Thermal Engine HAL, NXP KeyMint 3.0 StrongBox, and SELinux Enforcing mode are preserved.

### 5. Proprietary Blob & Firmware Dependency Analysis
* **Analysis:** Produced `docs/proprietary-dependency-status.md`, documenting extraction specifications from stock OnePlus 12 Android 16 firmware (`CPH2573_16.0.9.400(EX01)`).
* **Manifest Specifications:** Analyzed 2,382 lines in `waffle/proprietary-files.txt`, 28 boot/hardware partitions in `waffle/proprietary-firmware.txt`, and 1,088 lines in `sm8650-common/proprietary-files.txt`.
* **Version Control Compliance:** Enforced strict `.gitignore` rules ensuring vendor binaries and signing keys are never committed to Git.

### 6. Baseline Build Execution & Diagnostic Report
* **Execution Attempt:** Executed control build initialization (`breakfast waffle` -> `brunch waffle`).
* **Diagnostic Report:** Produced `docs/baseline-build-result.md`, capturing exact error output, diagnosing root causes (unpopulated vendor blob paths `vendor/oneplus/` and unpopulated upstream HAL/kernel repos `vendor/lineage`, `hardware/qcom-caf/sm8650`, `kernel/oneplus/sm8650`), and detailing host disk/swap requirements.

### 7. Custom Product Definition (`translator_waffle`)
* **Product Configuration:** Created `products/AndroidProducts.mk` and `products/translator_waffle.mk`, defining the custom product `translator_waffle` derived from `device/oneplus/waffle/lineage_waffle.mk`.
* **Hardware Inheritance:** Inherited 100% of the baseline device configuration, Qualcomm common definitions, Virtual A/B OTA, and ARM64 architecture specs while setting custom branding properties (`ro.translator.version=1.0.0-baseline`).
* **Build System Integration:** Integrated products into the LineageOS build environment via symlink at `~/android/lineage/vendor/translator/products`.
* **Documentation:** Produced `docs/custom-product-baseline.md` detailing inherited makefiles and retained hardware dependencies.

### 8. Android Minimization Strategy & Removal Roadmap
* **Minimization Specification:** Produced `docs/android-minimization-plan.md`, establishing a 13-category classification matrix for stripping non-essential smartphone packages.
* **Dependency & Risk Analysis:** Evaluated Binder IPC inter-subsystem dependencies between `audioserver`, `system_server`, `ActivityManager`, `SurfaceFlinger`, and `com.android.bluetooth`.
* **Staged Roadmap:** Formulated a 3-stage removal roadmap (Stage 1: Cellular/RIL isolation, Stage 2: Standard user app stripping, Stage 3: Setup wizard elimination) with explicit verification tests and rollback procedures for each candidate package.

---

## Licensing & Notice
Custom code and documentation in this repository are developed for the `oneplus12-offline-translator` project. All upstream LineageOS and Android source components retain their respective open-source licenses (Apache 2.0 / GPLv2).
