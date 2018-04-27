DESCRIPTION = "i.MXS boot streams"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://power_prep/power_prep.c;beginline=5;endline=12;md5=49e09642208485988b5c22074c896653"

# Note : This recipe is widely rewritten instead of simply extending the one
#        from the meta-fsl-arm meta layer because source package is different
#        and specific vorwerk signature process is added

# Kernel is required to generate bootstream
DEPENDS = "virtual/kernel"
SRC_URI_append =  " file://makefile.patch "

SRCREV = "${AUTOREV}"
SRCREV_FORMAT = "bootlets.cst.otp"

inherit deploy vorwerk_base

# Disable parallel building or it may fail to build.
PARALLEL_MAKE = ""

CMDLINE1_DEBUG = "console=ttyAM0,115200 ro gpmi ubi.mtd=1 rootfstype=squashfs root=254:0 lpj=1130496 vt.global_cursor_default=0"
CMDLINE1_RELEASE = "console= ro gpmi ubi.mtd=1 rootfstype=squashfs root=254:0 lpj=1130496 vt.global_cursor_default=0 quiet"
CMDLINE = "${@bb.utils.contains("IMAGE_FEATURES", "debug-tweaks", "${CMDLINE1_DEBUG}", "${CMDLINE1_RELEASE}", d)}"

EXTRA_OEMAKE = "CROSS_COMPILE=${TARGET_PREFIX} CMDLINE1='${CMDLINE}' \
                CMDLINE2='${CMDLINE1_RELEASE}' CMDLINE3='${CMDLINE1_RELEASE}' \
                CMDLINE4='${CMDLINE1_RELEASE}'"
do_set_debug() {
    for file in "linux_prep/core" "chooser_prep" "boot_prep" "power_prep"
    do
        sed -i "s@^//#define TM41_DEBUG@#define TM41_DEBUG@" ${S}/${file}/debug.h
    done
}

do_unset_debug() {
    for file in "linux_prep/core" "chooser_prep" "boot_prep" "power_prep"
    do
        sed -i "s@^#define TM41_DEBUG@//#define TM41_DEBUG@" /${S}/${file}/debug.h
    done
}

do_patch_append() {
    # Switch debug according to the image feature
    if bb.utils.contains("IMAGE_FEATURES", "debug-tweaks", True, False, d):
        bb.build.exec_func("do_set_debug", d)
    else :
        bb.build.exec_func("do_unset_debug", d)
}

do_compile () {
    oe_runmake BOARD=${IMXBOOTLETS_MACHINE} linux_prep \
                                            boot_prep \
                                            power_prep \
                                            chooser_prep \
               'CC=${TARGET_PREFIX}gcc --sysroot="${STAGING_DIR_TARGET}"' \
               'LD=${TARGET_PREFIX}ld --sysroot="${STAGING_DIR_TARGET}"'
}

do_install () {
}

do_deploy () {
    # Generate non signed bootstreams in deploy folder
    cd ${S}
    sed -i 's,[^ *]zImage.*;,\tzImage="${DEPLOY_DIR_IMAGE}/zImage";,' ./linux.bd
    sed -i 's,[^ *]zImage.*;,\tzImage="${DEPLOY_DIR_IMAGE}/zImage";,' ./linux_ivt.bd
    # FIXME elfosb should be built instead of fetched as prebuilt binary
    ${S}/../otp_tools/elftosb -z -c ./linux.bd -o ${DEPLOY_DIR_IMAGE}/imx28_linux.sb
    ${S}/../otp_tools/elftosb -z -f imx28 -c ./linux_ivt.bd -o ${DEPLOY_DIR_IMAGE}/imx28_ivt_linux.sb

    # Generate signed bootstream in deploy folder
    SIGNED_BOOTLETS_PATH=${WORKDIR}/bootstream
    mkdir -p ${SIGNED_BOOTLETS_PATH}

    #distribute keys to proper folders
    cp -fr ${KEYS_PATH}/otp/*.key ${S}/../otp_tools/
    cp -fr ${KEYS_PATH}/srk/* ${S}/../cst_tools/

    objcopy -I elf32-little -O binary --gap-fill 0xFF ${S}/chooser_prep/chooser_prep ${SIGNED_BOOTLETS_PATH}/chooser_prep.bin
    objcopy -I elf32-little -O binary --gap-fill 0xFF ${S}/power_prep/power_prep ${SIGNED_BOOTLETS_PATH}/power_prep.bin
    objcopy -I elf32-little -O binary --gap-fill 0xFF ${S}/boot_prep/boot_prep ${SIGNED_BOOTLETS_PATH}/boot_prep.bin
    objcopy -I elf32-little -O binary --gap-fill 0xFF ${S}/linux_prep/output-target/linux_prep ${SIGNED_BOOTLETS_PATH}/linux_prep.bin
    ### linux_prep is modified in-place and called twice, so we need to generate 2 signatures,
    ### modify entry_count 0->1 (hardcoded, offset 0x24 can move!)
    cd ${S}/../cst_tools/build && ./patch_linux_prep.py ${SIGNED_BOOTLETS_PATH}/linux_prep.bin ${SIGNED_BOOTLETS_PATH}/linux_kernel.bin

    ### generate HAB data
    sed -i "s#\"chooser_prep.bin\"#\"${SIGNED_BOOTLETS_PATH}/chooser_prep.bin\"#" ${S}/../cst_tools/build/chooser_prep.csf.tmpl
    ./fill_csf_tmpl.py chooser_prep.csf.tmpl
    sed -i "s#\"boot_prep.bin\"#\"${SIGNED_BOOTLETS_PATH}/boot_prep.bin\"#" ${S}/../cst_tools/build/boot_prep.csf.tmpl
    ./fill_csf_tmpl.py boot_prep.csf.tmpl
    sed -i "s#\"power_prep.bin\"#\"${SIGNED_BOOTLETS_PATH}/power_prep.bin\"#" ${S}/../cst_tools/build/power_prep.csf.tmpl
    ./fill_csf_tmpl.py power_prep.csf.tmpl
    sed -i "s#\"linux_prep.bin\"#\"${SIGNED_BOOTLETS_PATH}/linux_prep.bin\"#" ${S}/../cst_tools/build/linux_prep.csf.tmpl
    ./fill_csf_tmpl.py linux_prep.csf.tmpl
    sed -i "s#\"linux_kernel.bin\"#\"${SIGNED_BOOTLETS_PATH}/linux_kernel.bin\"#" ${S}/../cst_tools/build/linux_kernel.csf.tmpl
    sed -i "s#\"zImage\"#\"${DEPLOY_DIR_IMAGE}/zImage\"#" ${S}/../cst_tools/build/linux_kernel.csf.tmpl
    ./fill_csf_tmpl.py linux_kernel.csf.tmpl

    ../linux/cst -o ${S}/../cst_tools/build/chooser_prep_hab_data < ${S}/../cst_tools/build/chooser_prep.csf
    ../linux/cst -o ${S}/../cst_tools/build/boot_prep_hab_data < ${S}/../cst_tools/build/boot_prep.csf
    ../linux/cst -o ${S}/../cst_tools/build/power_prep_hab_data < ${S}/../cst_tools/build/power_prep.csf
    ../linux/cst -o ${S}/../cst_tools/build/linux_prep_hab_data < ${S}/../cst_tools/build/linux_prep.csf
    ../linux/cst -o ${S}/../cst_tools/build/linux_kernel_hab_data < ${S}/../cst_tools/build/linux_kernel.csf

    ## generate signed bootstream with HAB data (SRK table) and encrypted with OTP key (AES-128)
    ## (-k, so only 1 key is possible)
    ## Setting some old variables
    BD_FILE="${S}/../cst_tools/build/linux_ivt_signed.bd"
    BOOTSTREAM_FILE="imx28_ivt_linux_signed.sb"
    OTP_KEY="${S}/../otp_tools/otp.key"
    ## sedding bdfile
    cp -f ${BD_FILE} ${BD_FILE}_copy
    sed -i "s#zImage = \"./zImage\"#zImage = \"${DEPLOY_DIR_IMAGE}/zImage\"#" ${BD_FILE}
    sed -i "s#chooser_prep = \"./#chooser_prep = \"${S}/chooser_prep/#" ${BD_FILE}
    sed -i "s#power_prep = \"./#power_prep = \"${S}/power_prep/#" ${BD_FILE}
    sed -i "s#boot_prep = \"./#boot_prep = \"${S}/boot_prep/#" ${BD_FILE}
    sed -i "s#linux_prep = \"./#linux_prep = \"${S}/linux_prep/output-target/#" ${BD_FILE}

    sed -i "s#prep_bin = \"./#prep_bin = \"${SIGNED_BOOTLETS_PATH}/#" ${BD_FILE}
    sed -i "s#prep_hab_data = \"./#prep_hab_data = \"${S}/../cst_tools/build/#" ${BD_FILE}
    sed -i "s#kernel_hab_data = \"./#kernel_hab_data = \"${S}/../cst_tools/build/#" ${BD_FILE}

    ## finally generating signed bootstream file
    ${S}/../otp_tools/elftosb -V -f imx28 -k ${OTP_KEY} -c ${BD_FILE} -o ${DEPLOY_DIR_IMAGE}/${BOOTSTREAM_FILE}

    ### verify bootstream with OTP key only (no zero key)
    ${S}/../otp_tools/sbtool -k ${OTP_KEY} ${DEPLOY_DIR_IMAGE}/${BOOTSTREAM_FILE}

    # Check size of signed bootstream
    IMG_SIZE=$(stat -c %s  ${DEPLOY_DIR_IMAGE}/${BOOTSTREAM_FILE})
    if [ $IMG_SIZE -gt 4194304 ] ; then
        bbfatal "Kernel bootstream image ($IMG_SIZE bytes) is too big. Aborting..."
    fi
}

addtask deploy before do_build after do_compile
PACKAGE_ARCH = "${MACHINE_ARCH}"
COMPATIBLE_MACHINE = "(mxs)"
