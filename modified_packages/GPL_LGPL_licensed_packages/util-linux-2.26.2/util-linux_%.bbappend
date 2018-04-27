FILESEXTRAPATHS_prepend := "${THISDIR}/files:"
# Apply loop-aes patch for the arm cross compiled version only (not the native one)
SRC_URI_arm = "${KERNELORG_MIRROR}/linux/utils/util-linux/v${MAJOR_VERSION}/util-linux-${PV}.tar.xz \
               file://util-linux-2.26-20150310.diff"
