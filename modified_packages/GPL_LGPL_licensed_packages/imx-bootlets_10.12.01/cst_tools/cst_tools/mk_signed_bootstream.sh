#!/bin/bash
# expects objcopy, elftosb and sbtool in the path
export PATH=$PATH:../linux:~/build_trunk/ltib/otp_tools:/opt/freescale/usr/local/gcc-4.4.4-glibc-2.11.1-multilib-1.0/arm-fsl-linux-gnueabi/arm-fsl-linux-gnueabi/bin
BOOTLETS_PATH=~/build_trunk/ltib/rpm/BUILD/imx-bootlets-src-10.12.01
OTP_KEY=~/build_trunk/ltib/otp_tools/otp.key_Bunker2
BD_FILE=~/build_trunk/ltib/cst_tools/build/linux_ivt_signed.bd
BOOTSTREAM_FILE=~/build_trunk/ltib/cst_tools/build/imx28_ivt_linux_signed.sb
ZIMAGE_FILE=~/build_trunk/ltib/rootfs/boot/zImage

# build bootlets
spath=`pwd`
cd ..
./mk_bootstream
cd $spath

# Doh! We HAVE to do it from sibling-dir, as required by cst tool
cd ./build

# convert bootlets to binary
cp -f $ZIMAGE_FILE zImage
cp -f $BOOTLETS_PATH/chooser_prep/chooser_prep chooser_prep
cp -f $BOOTLETS_PATH/power_prep/power_prep power_prep
cp -f $BOOTLETS_PATH/boot_prep/boot_prep boot_prep
cp -f $BOOTLETS_PATH/linux_prep/output-target/linux_prep linux_prep
objcopy -I elf32-little -O binary --gap-fill 0xFF $BOOTLETS_PATH/chooser_prep/chooser_prep chooser_prep.bin
objcopy -I elf32-little -O binary --gap-fill 0xFF $BOOTLETS_PATH/power_prep/power_prep power_prep.bin
objcopy -I elf32-little -O binary --gap-fill 0xFF $BOOTLETS_PATH/boot_prep/boot_prep boot_prep.bin
objcopy -I elf32-little -O binary --gap-fill 0xFF $BOOTLETS_PATH/linux_prep/output-target/linux_prep linux_prep.bin

# linux_prep is modified in-place and called twice, so we need to generate 2 signatures,
# modify entry_count 0->1 (hardcoded, offset 0x24 can move!)
./patch_linux_prep.py linux_prep.bin linux_kernel.bin

# generate HAB data
./fill_csf_tmpl.py chooser_prep.csf.tmpl
./fill_csf_tmpl.py boot_prep.csf.tmpl
./fill_csf_tmpl.py power_prep.csf.tmpl
./fill_csf_tmpl.py linux_prep.csf.tmpl
./fill_csf_tmpl.py linux_kernel.csf.tmpl
cst -o chooser_prep_hab_data < chooser_prep.csf
cst -o boot_prep_hab_data < boot_prep.csf
cst -o power_prep_hab_data < power_prep.csf
cst -o linux_prep_hab_data < linux_prep.csf
cst -o linux_kernel_hab_data < linux_kernel.csf

# generate signed bootstream with HAB data (SRK table) and encrypted with OTP key (AES-128)
# (-k, so only 1 key is possible)
elftosb -V -f imx28 -k $OTP_KEY -c $BD_FILE -o $BOOTSTREAM_FILE

# verify bootstream with OTP key only (no zero key)
sbtool -k $OTP_KEY $BOOTSTREAM_FILE

cd ..
