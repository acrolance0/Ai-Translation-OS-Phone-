# Baseline Build Result Report for OnePlus 12 (`waffle`)

## Build Summary
* **Target Device:** OnePlus 12 (`waffle`)
* **Target Architecture:** Qualcomm Snapdragon 8 Gen 3 (SM8650) / ARM64
* **LineageOS Branch:** `lineage-23.2`
* **Build Target Command:** `breakfast waffle` -> `brunch waffle`
* **Host System:** Ubuntu 24.04 LTS (KVM VM, 4 CPU threads, 7.8 GB RAM, 0 B Swap, 92 GB free disk space on `/`)
* **Build Result:** **BLOCKED / HALTED**

---

## Detailed Failure Analysis & Root Causes

### 1. Missing Proprietary Vendor Blobs (`vendor/oneplus/waffle` & `vendor/oneplus/sm8650-common`)
* **Status:** Missing.
* **Impact:** Android build system (`build/envsetup.sh` / `breakfast`) requires `vendor/oneplus/waffle/BoardConfigVendor.mk` and `vendor/oneplus/sm8650-common/BoardConfigVendor.mk` to assemble vendor, ODM, and hardware HAL dependencies.

### 2. Missing Upstream Vendor & HAL Repositories
* `vendor/lineage`: Missing.
* `hardware/qcom-caf/sm8650`: Missing.
* `kernel/oneplus/sm8650`: Missing.

### 3. Critical Host System Resource Constraints
* **Disk Capacity:** The complete LineageOS 23.2 repository sync requires ~150–200 GB, and compilation outputs (`out/`) require an additional 150–200 GB (total ~400 GB recommended). Available free disk space on the host is currently 92 GB.
* **RAM & Swap:** Compilation requires significant RAM (64 GB recommended for LineageOS 21+). The current host has 7.8 GB RAM and 0 B swap, which would trigger Out-Of-Memory (`oom-killer`) termination during C++/Rust compilation.

---

## Required Remediation Steps Before Next Build Attempt
1. **Host Disk Expansion:** Expand virtual disk or mount a dedicated storage block with at least 400 GB free space.
2. **Host Memory & Swap Provisioning:** Configure a 32 GB – 64 GB swap file (`/swapfile`) to prevent compiler memory crashes.
3. **Full Source Tree Sync:** Execute `repo sync` across all required upstream LineageOS and Qualcomm CAF repositories (`vendor/lineage`, `hardware/qcom-caf/sm8650`, `kernel/oneplus/sm8650`).
4. **Proprietary Blob Population:** Populate `vendor/oneplus/waffle` and `vendor/oneplus/sm8650-common` from stock `CPH2573_16.0.9.400` payload/image extraction via `extract-files.py`.

---

## Hardware Safety Verification
* **ONEPLUS 12 TOUCHED:** **NO**
* **BOOTLOADER TOUCHED:** **NO**
* **PHONE FLASHED:** **NO**
