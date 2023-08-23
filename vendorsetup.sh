# Vendor Tree
git clone --depth=1 https://github.com/Amritorock/vendor_realme_r5x -b libperf vendor/realme/r5x

# Kernel Tree
git clone --depth=1 https://github.com/Amritorock/kernel_realme_r5x -b OSS kernel/realme/r5x

# NFC Packages
rm -rf vendor/nxp/opensource/commonsys/packages/apps/Nfc
git clone --depth=1 https://github.com/ArrowOS/android_vendor_nxp_opensource_packages_apps_Nfc vendor/nxp/opensource/commonsys/packages/apps/Nfc -b arrow-13.0

# Viper Fx
git clone --depth=1 https://github.com/Amritorock/packages_apps_ViPER4AndroidFX packages/apps/ViPER4AndroidFX
