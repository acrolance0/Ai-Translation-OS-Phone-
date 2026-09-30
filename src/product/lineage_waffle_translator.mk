# Product configuration overlay for the offline speech-translation appliance

# Include the translator native service
PRODUCT_PACKAGES += \
    translator_service

# Include custom SELinux policies
BOARD_VENDOR_SEPOLICY_DIRS += \
    device/oneplus/waffle/oneplus12-offline-translator/src/sepolicy/vendor
