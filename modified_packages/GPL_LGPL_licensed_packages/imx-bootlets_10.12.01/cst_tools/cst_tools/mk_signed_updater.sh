#!/bin/bash
# expects objcopy, elftosb and sbtool in the path
export PATH=$PATH:../linux:~/build_trunk/ltib/otp_tools:/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/arm-fsl-linux-gnueabi/bin
BOOTLETS_PATH=~/build_updater/ltib/rpm/BUILD/imx-bootlets-src-10.12.01
OTP_KEY=~/build_trunk/ltib/otp_tools/otp.key_Bunker2
BD_FILE=~/build_trunk/ltib/cst_tools/build/updater_ivt_signed.bd
BOOTSTREAM_FILE=~/build_trunk/ltib/cst_tools/build/updater_ivt_signed.sb
ZIMAGE_FILE=~/build_updater/ltib/rootfs/boot/zImage
INITRAMFS_FILE=~/build_updater/ltib/initramfs.cpio.gz

# build bootlets
spath=`pwd`
cd ~/ltib/ltib_updater
./ltib
./ltib -p boot_stream.spec -f
cd $spath


# Doh! We HAVE to do it from sibling-dir, as required by cst tool
cd ./build

# convert bootlets to binary
cp -f $ZIMAGE_FILE ./updater/zImage
cp -f $INITRAMFS_FILE ./updater/initramfs.cpio.gz
cp -f $BOOTLETS_PATH/power_prep/power_prep ./updater/power_prep
cp -f $BOOTLETS_PATH/boot_prep/boot_prep ./updater/boot_prep
cp -f $BOOTLETS_PATH/linux_prep/output-target/linux_prep ./updater/linux_prep
objcopy -I elf32-little -O binary --gap-fill 0xFF ./updater/power_prep ./updater/power_prep.bin
objcopy -I elf32-little -O binary --gap-fill 0xFF ./updater/boot_prep ./updater/boot_prep.bin
objcopy -I elf32-little -O binary --gap-fill 0xFF ./updater/linux_prep ./updater/linux_prep.bin

# linux_prep is modified in-place and called twice, so we need to generate 2 signatures,
# modify entry_count 0->1 (hardcoded, offset 0x24 can move!)
./patch_linux_prep.py ./updater/linux_prep.bin ./updater/linux_kernel.bin

# generate HAB data
./fill_csf_tmpl.py ./updater/boot_prep.csf.tmpl
./fill_csf_tmpl.py ./updater/power_prep.csf.tmpl
./fill_csf_tmpl.py ./updater/linux_prep.csf.tmpl
./fill_csf_tmpl.py ./updater/linux_kernel.csf.tmpl
cst -o ./updater/boot_prep_hab_data < ./updater/boot_prep.csf
cst -o ./updater/power_prep_hab_data < ./updater/power_prep.csf
cst -o ./updater/linux_prep_hab_data < ./updater/linux_prep.csf
cst -o ./updater/linux_kernel_hab_data < ./updater/linux_kernel.csf

# generate signed updater with HAB data (SRK table) and encrypted with OTP key (AES-128)
# (-z -k, so zero key is also included)
elftosb -V -f imx28 -z -k $OTP_KEY -c $BD_FILE -o $BOOTSTREAM_FILE

# verify bootstream with OTP key
sbtool -k $OTP_KEY $BOOTSTREAM_FILE
sbtool -z $BOOTSTREAM_FILE

cd ..
