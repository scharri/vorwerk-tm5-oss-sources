DESCRIPTION = "Linux kernel"
SUMMARY = "kernel"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit kernel vorwerk_extsrc
DEPENDS += "lzop-native"
SRC_URI_append = "\
            file://add_gcc5_header.patch \
            file://fix_uninitialized_macro_req.patch \
            file://fix_inotify_gcc5.patch \
            file://defconfig-std \
            file://defconfig-rdv \
            file://defconfig \
            file://dcp.c \
            file://perl_deprecated_defined.patch \
            file://ptrace_coredump_linux2.6.35.patch \
            file://exec_tid_core_pattern_linux2.6.35.patch \
            file://freebox.patch \
            file://ubi_upd_flash_after_prep_for_update.patch \
          "

SRCREV = "${AUTOREV}"

S = "${WORKDIR}/kernel"

# Set an arbitrary kernel priority to avoid issue when we tweak PV variable...
KERNEL_PRIORITY = "1"

# Note : At the moment we always use the fsl gcc v4.4.4 to build the kernel may
# we use this gcc version to build user space utils or not
# Indeed, building kernel with gcc5.2 does not boot for some reason...
python __anonymous () {
    toolchain_type = d.getVar('TCMODE', True)
    if toolchain_type == "external-fsl" :
        # If we use fsl toolchain :
        # set KERNEL_CC to avoid default fuse-ld=bfd option
        d.setVar("KERNEL_CC", "${CCACHE}${HOST_PREFIX}gcc ${HOST_CC_KERNEL_ARCH}")
        # set KERNEL_LD to avoid default ld.bfd name
        d.setVar("KERNEL_LD", "${CCACHE}${HOST_PREFIX}ld ${HOST_LD_KERNEL_ARCH}")
    else:
        d.setVar("KERNEL_CC", "${EXTERNAL_TOOLCHAIN}/bin/arm-fsl-linux-gnueabi-gcc ${HOST_CC_KERNEL_ARCH}")
        d.setVar("KERNEL_LD", "${EXTERNAL_TOOLCHAIN}/bin/arm-fsl-linux-gnueabi-ld ${HOST_LD_KERNEL_ARCH}")
}

addtask do_handle_keys after do_unpack before do_patch do_force_patch
do_handle_keys() {
    # Replace dcp driver and inject keys
    cp -f ${WORKDIR}/dcp.c ${S}/drivers/crypto/dcp.c
    # act_key (1. Remove old key, 2. Place in the place of the old key new key from file)
    sed -i '/static const u8 act_key/{$!{N;s/static const u8 act_key.*\n.*/static const u8 act_key\[AES_KEYSIZE_128\] = {\n};/}}' ${S}/drivers/crypto/dcp.c
    sed -i '/static const u8 act_key/r ${KEYS_PATH}/rc/dcp.c_act_key' ${S}/drivers/crypto/dcp.c
    # cmd_key  (1. Remove old key, 2. Place in the place of the old key new key from file)
    sed -i '/static const u8 cmp_key/{$!{N;s/static const u8 cmp_key.*\n.*/static const u8 cmp_key\[AES_KEYSIZE_128\] = {\n};/}}' ${S}/drivers/crypto/dcp.c
    sed -i '/static const u8 cmp_key/r ${KEYS_PATH}/rc/dcp.c_cmp_key' ${S}/drivers/crypto/dcp.c
    cp -f ${KEYS_PATH}/cloud/*.h ${S}/drivers/crypto/key_store/
}

remove_touchscreen_option() {
    sed -i "s@CONFIG_TOUCHSCREEN_NT11003=y@# CONFIG_TOUCHSCREEN_NT11003 is not set@" ${WORKDIR}/defconfig
}

enable_fcr_keys_option() {
    sed -i "s@^# CONFIG_CRYPTO_VORWERK_USE_FCR_CLOUD_KEYS is not set@CONFIG_CRYPTO_VORWERK_USE_FCR_CLOUD_KEYS=y@" ${WORKDIR}/defconfig
}

do_patch_append() {
    # Configuration could be handled by fragments rather
    # For now only handle that way :
    import shutil
    workdir = d.getVar('WORKDIR', True)
    if bb.utils.contains("IMAGE_FEATURES", "rdv", True, False, d):
        shutil.copyfile(workdir+"/defconfig-rdv", workdir+"/defconfig")
    else:
        shutil.copyfile(workdir+"/defconfig-std", workdir+"/defconfig")
    if not bb.utils.contains("MACHINE_FEATURES", "touchscreen", True, False, d):
        bb.build.exec_func("remove_touchscreen_option", d)
    # Enable FCR_KEYS option if it is a FCR build
    keys_path = d.getVar("KEYS_PATH", True)
    if keys_path and keys_path.find("sw-PRODUCT") != -1 :
        bb.build.exec_func("enable_fcr_keys_option", d)
}

# Copy again the original defconfig as oldconfig has altered it
do_configure_append(){
     cp "${WORKDIR}/defconfig" "${B}/.config"
}

COMPATIBLE_MACHINE = "(imx28-bt10)"
