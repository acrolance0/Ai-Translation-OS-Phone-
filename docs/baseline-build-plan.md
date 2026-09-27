# Baseline Firmware Build Plan for OnePlus 12 (`waffle`)

## Goal
Produce an untouched, official-style LineageOS 23.2 baseline image for the OnePlus 12 (`waffle`). This baseline serves as the "known-good" reference build before any smartphone component removal or custom translator service integration takes place.

---

## Phase 1: Environment & Workspace Verification
1. **Verify LineageOS Source Tree (`~/android/lineage`):**
   * Ensure `device/oneplus/waffle` and `device/oneplus/sm8650-common` are on branch `lineage-23.2`.
2. **Verify Build Tools:**
   * Confirm `repo`, `git-lfs`, `ccache`, `ninja`, `flex`, `bison`, `bc`, `lz4`, and `openjdk-21` are accessible in shell PATH.
3. **Verify ccache Limit:**
   * Confirm `ccache` is enabled (`USE_CCACHE=1`) with a conservative size limit (15 GB).

---

## Phase 2: Proprietary Blobs & Firmware Acquisition
1. **Source Blobs:**
   * Extract or assemble required proprietary blobs (`vendor/oneplus/waffle` and `vendor/oneplus/sm8650-common`) using `extract-files.py` or official LineageOS extractable zip scripts.
2. **Firmware Manifest:**
   * Ensure `proprietary-files.txt` and `proprietary-firmware.txt` dependencies match expected OnePlus 12 target build version (`CPH2573_16.0.9.400`).

---

## Phase 3: Compilation Sequence
1. **Navigate to Source Root:**
   ```bash
   cd ~/android/lineage
   ```
2. **Initialize Environment Setup:**
   ```bash
   source build/envsetup.sh
   ```
3. **Select Target Device Product:**
   ```bash
   breakfast waffle
   ```
4. **Execute Build Command:**
   ```bash
   brunch waffle
   ```

---

## Phase 4: Output Validation & Baseline Freeze
1. **Verify Generated Artifacts (`$OUT`):**
   * Confirm `recovery.img` (LineageOS Recovery) is generated.
   * Confirm `lineage-23.2-*-UNOFFICIAL-waffle.zip` installer package is generated.
2. **Baseline Freeze:**
   * Record exact git commit hashes of all repositories in `.repo/manifest.xml`.
   * Save build metrics and output logs as baseline benchmarks before starting Stage 5 customization.
