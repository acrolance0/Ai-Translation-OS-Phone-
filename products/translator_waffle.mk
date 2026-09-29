#
# Custom Translator Product Configuration for OnePlus 12 (waffle)
# Derived from official LineageOS 23.2 waffle baseline product configuration
#

# Inherit directly from official LineageOS waffle target to preserve 100% hardware compatibility
$(call inherit-product, device/oneplus/waffle/lineage_waffle.mk)

# Product Branding & Identification
PRODUCT_NAME := translator_waffle
PRODUCT_DEVICE := waffle
PRODUCT_BRAND := OnePlus
PRODUCT_MODEL := OnePlus 12 Translator Appliance
PRODUCT_MANUFACTURER := OnePlus

# Custom Firmware Identification
PRODUCT_SYSTEM_DEFAULT_PROPERTIES += \
    ro.translator.version=1.0.0-baseline \
    ro.translator.target=waffle \
    ro.translator.type=appliance
