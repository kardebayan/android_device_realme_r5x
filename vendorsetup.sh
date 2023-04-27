echo 'Starting to clone stuffs needed for your device'

echo 'Cloning Vendor tree'
# Vendor Tree
git clone --depth=1 https://github.com/Amritorock/vendor_realme_r5x -b Trinket vendor/realme/r5x

echo 'Cloning Kernel tree'
# Kernel Tree
git clone --depth=1 https://github.com/Amritorock/kernel_realme_r5x -b OSS kernel/realme/r5x

echo 'Cloning NFC Packages'
# NFC Packages
git clone --depth=1 https://github.com/ArrowOS/android_vendor_nxp_opensource_packages_apps_Nfc vendor/nxp/opensource/commonsys/packages/apps/Nfc arrow-13.0

echo 'Completed cloning your trees, proceed with lunch Command'
